// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/Voice/VoiceChattingManager.h"

#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"

static int32 ActiveVivoxManagers = 0;

#include <ServerModule/GameSession/GameSessionInstance.h>
#include <ServerModule/GameSession/PlayerSessionState.h>
#include <ServerModule/GameSession/GameSessionState.h>

#define VIVOX_VOICE_SERVER TEXT("https://unity.vivox.com/appconfig/14569-test-91617")
#define VIVOX_VOICE_DOMAIN TEXT("mtu1xp.vivox.com")
#define VIVOX_VOICE_ISSUER TEXT("14569-test-91617")
#define VIVOX_VOICE_KEY TEXT("KkLAV53qnqWCFd6xNEcHVnZnFMT53Txx")


// ============================================================
// Initialize
// ============================================================

void UVoiceChattingManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    InitVivox();
    HandleWorldChanged(GetWorld());
}


// ============================================================
// Deinitialize
// ============================================================

void UVoiceChattingManager::Deinitialize()
{
    bShuttingDown = true;
    ClearChannelCallbacks();
    DisconnectCurrentChannel(true);
    if (UWorld* World = VoiceTimerWorld.Get())
    {
        World->GetTimerManager().ClearTimer(DebugHudTimerHandle);
        World->GetTimerManager().ClearTimer(VoiceCacheTimerHandle);
        World->GetTimerManager().ClearTimer(Vivox3DTimer);
        World->GetTimerManager().ClearTimer(PawnRetryTimerHandle);
        World->GetTimerManager().ClearTimer(ChannelSwitchTimerHandle);
    }
    if (VivoxVoiceClient && bVivoxInitialized)
    {
        if (bIsLoggedIn || bLoginInProgress)
            VivoxVoiceClient->GetLoginSession(LoggedInUserId).Logout();
        if (--ActiveVivoxManagers == 0)
            VivoxVoiceClient->Uninitialize();
    }
    bVivoxInitialized = false;
    VivoxVoiceClient = nullptr;
    Super::Deinitialize();
}


// ============================================================
// Init Vivox
// ============================================================

void UVoiceChattingManager::InitVivox()
{
    if (bVivoxInitialized || bShuttingDown) return;
    auto& Module = FModuleManager::LoadModuleChecked<FVivoxCoreModule>(TEXT("VivoxCore"));
    VivoxVoiceClient = &Module.VoiceClient();
    if (ActiveVivoxManagers == 0)
    {
        const VivoxCoreError Error = VivoxVoiceClient->Initialize();
        if (Error != 0 && Error != VxErrorAlreadyInitialized)
        {
            Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Initialize FAIL Error=%d"), int32(Error)), FColor::Red);
            VivoxVoiceClient = nullptr;
            return;
        }
    }
    ++ActiveVivoxManagers;
    bVivoxInitialized = true;
    Vivox_ScreenLog(TEXT("[Vivox] Initialized"));
}


// ============================================================
// Login Internal
// ============================================================

void UVoiceChattingManager::Vivox_BeginLoginInternal(const FString& PlayerIdOverride)
{
    if (!VivoxVoiceClient || bShuttingDown) return;
    const FString PlayerId = GetLocalVoicePlayerId();
    LoggedInUserId = AccountId(VIVOX_VOICE_ISSUER, PlayerId, VIVOX_VOICE_DOMAIN);
    ILoginSession& Login = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
    bLoginInProgress = true;
    ILoginSession::FOnBeginLoginCompletedDelegate Done;
    const TWeakObjectPtr<UVoiceChattingManager> WeakThis(this);
    Done.BindLambda([WeakThis](VivoxCoreError Error)
    {
        UVoiceChattingManager* Self = WeakThis.Get();
        if (!Self || Self->bShuttingDown) return;
        Self->bLoginInProgress = false;
        Self->bIsLoggedIn = Error == 0;
        Self->Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Login %s Error=%d"),
            Error == 0 ? TEXT("SUCCESS") : TEXT("FAIL"), int32(Error)),
            Error == 0 ? FColor::Green : FColor::Red);
        if (Error == 0)
        {
            Self->Vivox_EnableEnergyMeter();
            Self->JoinPendingChannel();
        }
    });
    Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] BeginLogin as '%s'"), *PlayerId), FColor::Cyan);
    const VivoxCoreError Error = Login.BeginLogin(VIVOX_VOICE_SERVER,
        Login.GetLoginToken(VIVOX_VOICE_KEY, FTimespan::FromHours(1)), Done);
    if (Error != 0)
    {
        bLoginInProgress = false;
        bIsLoggedIn = false;
        Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] BeginLogin rejected Error=%d"), int32(Error)), FColor::Red);
    }
}


// ============================================================
// Lobby Voice
// ============================================================

void UVoiceChattingManager::Vivox_EnterLobbyVoice(
    const FString& PlayerId,
    const FString& SessionVoiceId
)
{
    if (SessionVoiceId.IsEmpty())
    {
        Vivox_ScreenLog(
            TEXT(
                "[Vivox] EnterLobbyVoice FAIL: Empty SessionVoiceId"
            ),
            FColor::Red
        );

        return;
    }


    // Lobby에서는 3D 업데이트가 필요 없음
    Vivox_Stop3DUpdate();


    CurrentVoiceMode =
        EVoiceChannelMode::Lobby2D;


    const FString ChannelName =
        SessionVoiceId + TEXT("_Lobby");


    Vivox_ScreenLog(
        FString::Printf(
            TEXT(
                "[Vivox] Request Lobby2D Voice: %s"
            ),
            *ChannelName
        ),
        FColor::Cyan
    );


    Vivox_StartVoice(
        PlayerId,
        ChannelName,
        false
    );
}


// ============================================================
// InGame Voice
// ============================================================

void UVoiceChattingManager::Vivox_EnterInGameVoice(
    const FString& PlayerId,
    const FString& SessionVoiceId
)
{
    if (SessionVoiceId.IsEmpty())
    {
        Vivox_ScreenLog(
            TEXT(
                "[Vivox] EnterInGameVoice FAIL: Empty SessionVoiceId"
            ),
            FColor::Red
        );

        return;
    }


    // Travel 이전 Pawn 참조 제거
    Vivox_Stop3DUpdate();


    CurrentVoiceMode =
        EVoiceChannelMode::InGame3D;


    const FString ChannelName =
        SessionVoiceId + TEXT("_Game");


    Vivox_ScreenLog(
        FString::Printf(
            TEXT(
                "[Vivox] Request InGame3D Voice: %s"
            ),
            *ChannelName
        ),
        FColor::Cyan
    );


    Vivox_StartVoice(
        PlayerId,
        ChannelName,
        true
    );
}


// ============================================================
// Start Voice
// ============================================================

void UVoiceChattingManager::Vivox_StartVoice(const FString& PlayerIdOverride,
    const FString& ChannelName, bool bPositional)
{
    UWorld* World = GetWorld();
    if (bShuttingDown || !World || World->GetNetMode() == NM_DedicatedServer) return;
    FString RequestedChannel = ChannelName;
    bool bRequestedPositional = bPositional;
    const FString Level = UGameplayStatics::GetCurrentLevelName(World, true);
    // Legacy Blueprint calls must not override the room's canonical channel.
    if (Level == TEXT("test_Title"))
    {
        Vivox_LeaveVoice();
        return;
    }
    if (Level == TEXT("test_Lobby") || Level == TEXT("FirstPersonMap"))
    {
        UGameSessionInstance* GI = Cast<UGameSessionInstance>(GetGameInstance());
        if (!GI || !GI->RefreshJoinCodeFromSession())
        {
            Vivox_ScreenLog(TEXT("[Vivox] Voice skipped: no valid room join code"), FColor::Red);
            return;
        }
        bRequestedPositional = Level == TEXT("FirstPersonMap");
        RequestedChannel = GI->GetPendingJoinCode() +
            (bRequestedPositional ? TEXT("_Game") : TEXT("_Lobby"));
    }
    if (RequestedChannel.IsEmpty()) return;
    if (!VivoxVoiceClient) InitVivox();
    if (!VivoxVoiceClient) return;
    PendingPlayerId = GetLocalVoicePlayerId();
    PendingChannelName = RequestedChannel;
    bPendingPositional = bRequestedPositional;
    CurrentVoiceMode = bRequestedPositional ? EVoiceChannelMode::InGame3D : EVoiceChannelMode::Lobby2D;
    JoinPendingChannel();
}


// ============================================================
// Schedule Switch
// ============================================================

void UVoiceChattingManager::SchedulePendingChannelJoin(
    float DelaySeconds
)
{
    UWorld* World =
        GetWorld();

    if (!World)
        return;


    World->GetTimerManager().ClearTimer(
        ChannelSwitchTimerHandle
    );


    World->GetTimerManager().SetTimer(
        ChannelSwitchTimerHandle,
        this,
        &UVoiceChattingManager::JoinPendingChannel,
        DelaySeconds,
        false
    );
}


// ============================================================
// Pending Channel Join
// ============================================================

void UVoiceChattingManager::JoinPendingChannel()
{
    if (bShuttingDown || !VivoxVoiceClient) return;
    if (bHasChannelSession)
    {
        ILoginSession& Login = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
        IChannelSession& Session = Login.GetChannelSession(JoinedChannel);
        if (Session.ChannelState() == ConnectionState::Disconnecting || bChannelDisconnectInProgress) return;
        if (Session.ChannelState() != ConnectionState::Disconnected)
        {
            if (CurrentVoiceChannelName != PendingChannelName ||
                bCurrentChannelPositional != bPendingPositional || PendingChannelName.IsEmpty())
            {
                DisconnectCurrentChannel(false);
            }
            else if (Session.AudioState() == ConnectionState::Connected)
            {
                MarkChannelAudioConnected();
            }
            return;
        }
        ClearChannelCallbacks();
        Login.DeleteChannelSession(JoinedChannel);
        bHasChannelSession = false;
        bHasJoinedChannel = false;
        bChannelConnectInProgress = false;
        CurrentVoiceChannelName.Empty();
    }
    if (PendingChannelName.IsEmpty()) return;
    if (!bIsLoggedIn)
    {
        if (!bLoginInProgress) Vivox_BeginLoginInternal(PendingPlayerId);
        return;
    }
    Vivox_JoinVoice(PendingChannelName, bPendingPositional);
}


// ============================================================
// Join Voice Channel
// ============================================================

void UVoiceChattingManager::Vivox_JoinVoice(const FString& ChannelName, bool bPositional)
{
    if (bShuttingDown || !VivoxVoiceClient || !bIsLoggedIn || ChannelName.IsEmpty()) return;
    // Public legacy calls are routed through the same serialized switch path.
    if (bHasChannelSession)
    {
        Vivox_StartVoice(GetLocalVoicePlayerId(), ChannelName, bPositional);
        return;
    }
    ILoginSession& Login = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
    const int32 AudibleDistance = FMath::Max(1, InGameAudibleDistance);
    const Channel3DProperties Properties(AudibleDistance,
        FMath::Clamp(InGameConversationalDistance, 1, AudibleDistance),
        FMath::Max(0.001f, InGameAudioFadeIntensity), InGameAudioFadeModel);
    JoinedChannel = ChannelId(VIVOX_VOICE_ISSUER, ChannelName, VIVOX_VOICE_DOMAIN,
        bPositional ? ChannelType::Positional : ChannelType::NonPositional, Properties);
    CurrentVoiceChannelName = ChannelName;
    bCurrentChannelPositional = bPositional;
    bHasChannelSession = true;
    bHasJoinedChannel = false;
    bChannelConnectInProgress = true;
    IChannelSession& Session = Login.GetChannelSession(JoinedChannel);
    ChannelStateChangedHandle = Session.EventChannelStateChanged.AddUObject(this,
        &UVoiceChattingManager::HandleChannelStateChanged);
    AudioStateChangedHandle = Session.EventAudioStateChanged.AddUObject(this,
        &UVoiceChattingManager::HandleAudioStateChanged);
    ParticipantAddedHandle = Session.EventAfterParticipantAdded.AddUObject(this,
        &UVoiceChattingManager::HandleParticipantAdded);
    IChannelSession::FOnBeginConnectCompletedDelegate Done;
    const TWeakObjectPtr<UVoiceChattingManager> WeakThis(this);
    const ChannelId RequestedChannel = JoinedChannel;
    Done.BindLambda([WeakThis, RequestedChannel, ChannelName](VivoxCoreError Error)
    {
        UVoiceChattingManager* Self = WeakThis.Get();
        if (!Self || Self->bShuttingDown || !Self->bHasChannelSession ||
            !(Self->JoinedChannel == RequestedChannel)) return;
        Self->Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Connect request '%s' Error=%d"),
            *ChannelName, int32(Error)), Error == 0 ? FColor::Cyan : FColor::Red);
        if (Error != 0)
        {
            if (Self->PendingChannelName == ChannelName) Self->PendingChannelName.Empty();
            Self->bChannelConnectInProgress = false;
            Self->DisconnectCurrentChannel(false);
        }
        // Only AudioState::Connected marks a channel as joined.
    });
    Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] BeginConnect '%s' (%s)"), *ChannelName,
        bPositional ? TEXT("Positional") : TEXT("NonPositional")), FColor::Cyan);
    const VivoxCoreError Error = Session.BeginConnect(true, false, true,
        Session.GetConnectToken(VIVOX_VOICE_KEY, FTimespan::FromHours(1)), Done);
    if (Error != 0)
    {
        Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] BeginConnect rejected Error=%d"), int32(Error)), FColor::Red);
        if (PendingChannelName == ChannelName) PendingChannelName.Empty();
        bChannelConnectInProgress = false;
        DisconnectCurrentChannel(false);
    }
}


// ============================================================
// Disconnect Internal
// ============================================================

void UVoiceChattingManager::DisconnectCurrentChannel(bool bClearMode)
{
    StopVoiceCacheTimer();
    Vivox_Stop3DUpdate();
    if (UWorld* World = VoiceTimerWorld.Get())
        World->GetTimerManager().ClearTimer(ChannelSwitchTimerHandle);
    if (bClearMode)
    {
        CurrentVoiceMode = EVoiceChannelMode::None;
        PendingChannelName.Empty();
        bPendingPositional = false;
    }
    bHasJoinedChannel = false;
    LastSelfAudioEnergy = 0.0;
    LastStimuliMap.Reset();
    PushVoiceCacheToGameInstance();
    if (VivoxVoiceClient && bHasChannelSession)
    {
        IChannelSession& Session = VivoxVoiceClient->GetLoginSession(LoggedInUserId).GetChannelSession(JoinedChannel);
        if (Session.ChannelState() == ConnectionState::Disconnected)
        {
            bChannelDisconnectInProgress = false;
            if (!bShuttingDown) SchedulePendingChannelJoin();
        }
        else if (!bChannelDisconnectInProgress)
        {
            bChannelDisconnectInProgress = true;
            Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Disconnecting '%s'"), *CurrentVoiceChannelName), FColor::Yellow);
            Session.Disconnect(bShuttingDown);
        }
    }
}


// ============================================================
// Public Leave
// ============================================================

void UVoiceChattingManager::Vivox_LeaveVoice()
{
    DisconnectCurrentChannel(true);
}


// ============================================================
// Auto 3D Start
// ============================================================

void UVoiceChattingManager::Vivox_TryAutoStart3DUpdate()
{
    // 3D Mode가 아니면 실행하지 않음
    if (CurrentVoiceMode !=
        EVoiceChannelMode::InGame3D)
    {
        return;
    }


    if (!bHasJoinedChannel ||
        !bCurrentChannelPositional)
    {
        return;
    }


    UWorld* World =
        GetWorld();

    if (!World)
        return;


    APlayerController* PC =
        UGameplayStatics::GetPlayerController(
            World,
            0
        );


    if (!PC ||
        !PC->IsLocalController())
    {
        Vivox_ScreenLog(
            TEXT(
                "[Vivox] Auto3D: Waiting Local PlayerController"
            ),
            FColor::Yellow,
            0.5f
        );


        World->GetTimerManager().ClearTimer(
            PawnRetryTimerHandle
        );


        World->GetTimerManager().SetTimer(
            PawnRetryTimerHandle,
            this,
            &UVoiceChattingManager::Vivox_TryAutoStart3DUpdate,
            0.2f,
            false
        );


        return;
    }


    APawn* Pawn =
        PC->GetPawn();


    // ServerTravel 이후 Pawn이 아직 Spawn되지 않은 경우
    if (!Pawn)
    {
        Vivox_ScreenLog(
            TEXT(
                "[Vivox] Auto3D: Waiting for new Pawn..."
            ),
            FColor::Yellow,
            0.5f
        );


        World->GetTimerManager().ClearTimer(
            PawnRetryTimerHandle
        );


        World->GetTimerManager().SetTimer(
            PawnRetryTimerHandle,
            this,
            &UVoiceChattingManager::Vivox_TryAutoStart3DUpdate,
            0.2f,
            false
        );


        return;
    }


    World->GetTimerManager().ClearTimer(
        PawnRetryTimerHandle
    );


    CachedSpeaker =
        Pawn;

    CachedListener =
        Pawn;


    Vivox_ScreenLog(
        FString::Printf(
            TEXT(
                "[Vivox] Auto3D Pawn Bound: %s Pos=%s"
            ),
            *GetNameSafe(Pawn),
            *Pawn->GetActorLocation().ToString()
        ),
        FColor::Green
    );


    Vivox_Start3DUpdate(
        Pawn,
        Pawn,
        Auto3DUpdateInterval
    );
}


// ============================================================
// Update 3D Internal
// ============================================================

void UVoiceChattingManager::Vivox_Update3D_Internal()
{
    if (!VivoxVoiceClient ||
        !bHasJoinedChannel ||
        !bCurrentChannelPositional)
    {
        return;
    }


    AActor* SpeakerActor =
        CachedSpeaker.Get();

    AActor* ListenerActor =
        CachedListener.Get();


    if (!SpeakerActor ||
        !ListenerActor)
    {
        Vivox_Stop3DUpdate();

        Vivox_TryAutoStart3DUpdate();

        return;
    }


    Vivox_Update3DFromActors(
        SpeakerActor,
        ListenerActor
    );
}


// ============================================================
// Manual 3D Update
// ============================================================

void UVoiceChattingManager::Vivox_Update3DFromActors(
    AActor* SpeakerActor,
    AActor* ListenerActor
)
{
    if (!VivoxVoiceClient ||
        !bHasJoinedChannel ||
        !bCurrentChannelPositional)
    {
        return;
    }


    if (!SpeakerActor ||
        !ListenerActor)
    {
        return;
    }


    ILoginSession& LoginSession =
        VivoxVoiceClient->GetLoginSession(
            LoggedInUserId
        );


    IChannelSession& ChannelSession =
        LoginSession.GetChannelSession(
            JoinedChannel
        );


    const FVector SpeakerPos =
        SpeakerActor->GetActorLocation();


    const FVector ListenerPos =
        ListenerActor->GetActorLocation();


    const FVector ListenerForward =
        ListenerActor->GetActorForwardVector();


    const FVector ListenerUp =
        ListenerActor->GetActorUpVector();


    const VivoxCoreError Err =
        ChannelSession.Set3DPosition(
            SpeakerPos,
            ListenerPos,
            ListenerForward,
            ListenerUp
        );


    if (Err != 0)
    {
        Vivox_ScreenLog(
            FString::Printf(
                TEXT(
                    "[Vivox] Set3DPosition FAIL Err=%d"
                ),
                static_cast<int32>(Err)
            ),
            FColor::Red,
            0.5f
        );
    }
}


// ============================================================
// Start 3D Timer
// ============================================================

void UVoiceChattingManager::Vivox_Start3DUpdate(
    AActor* SpeakerActor,
    AActor* ListenerActor,
    float Interval
)
{
    UWorld* World =
        GetWorld();

    if (!World ||
        !SpeakerActor ||
        !ListenerActor)
    {
        return;
    }


    CachedSpeaker =
        SpeakerActor;

    CachedListener =
        ListenerActor;


    Interval =
        FMath::Clamp(
            Interval,
            0.05f,
            0.5f
        );


    // 기존 Timer 중복 방지
    World->GetTimerManager().ClearTimer(
        Vivox3DTimer
    );


    World->GetTimerManager().SetTimer(
        Vivox3DTimer,
        this,
        &UVoiceChattingManager::Vivox_Update3D_Internal,
        Interval,
        true
    );


    Vivox_ScreenLog(
        TEXT("[Vivox] 3D Update Started"),
        FColor::Cyan
    );


    // 첫 프레임 위치 즉시 전송
    Vivox_Update3D_Internal();
}


// ============================================================
// Stop 3D
// ============================================================

void UVoiceChattingManager::Vivox_Stop3DUpdate()
{
    UWorld* World =
        GetWorld();


    if (World)
    {
        World->GetTimerManager().ClearTimer(
            Vivox3DTimer
        );

        World->GetTimerManager().ClearTimer(
            PawnRetryTimerHandle
        );
    }


    CachedSpeaker.Reset();

    CachedListener.Reset();
}


// ============================================================
// Login Simple
// ============================================================

void UVoiceChattingManager::Vivox_LoginSimple(const FString& PlayerIdOverride)
{
    if (bShuttingDown || bIsLoggedIn || bLoginInProgress) return;
    if (!VivoxVoiceClient) InitVivox();
    if (VivoxVoiceClient) Vivox_BeginLoginInternal(GetLocalVoicePlayerId());
}


// ============================================================
// Mic
// ============================================================

void UVoiceChattingManager::Vivox_SetMicActive(
    bool bMuted
)
{
    if (!VivoxVoiceClient)
        return;


    VivoxVoiceClient
        ->AudioInputDevices()
        .SetMuted(bMuted);


    SetMicMuted(bMuted);


    Vivox_ScreenLog(
        bMuted
        ? TEXT("[Vivox] Mic MUTED")
        : TEXT("[Vivox] Mic UNMUTED"),
        bMuted
        ? FColor::Yellow
        : FColor::Green
    );
}


// ============================================================
// Debug Log
// ============================================================

void UVoiceChattingManager::Vivox_ScreenLog(
    const FString& Msg,
    const FColor& Color,
    float Time
)
{
    UE_LOG(
        LogTemp,
        Log,
        TEXT("%s"),
        *Msg
    );


    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            Time,
            Color,
            Msg
        );
    }
}


// ============================================================
// Energy Meter
// ============================================================

void UVoiceChattingManager::Vivox_EnableEnergyMeter()
{
    if (!VivoxVoiceClient ||
        !bIsLoggedIn)
    {
        return;
    }


    ILoginSession& LoginSession =
        VivoxVoiceClient->GetLoginSession(
            LoggedInUserId
        );


    const VivoxCoreError Err =
        LoginSession
        .SetParticipantSpeakingUpdateRate(
            ParticipantSpeakingUpdateRate::Update10Hz
        );


    Vivox_ScreenLog(
        FString::Printf(
            TEXT(
                "[Vivox] SetParticipantSpeakingUpdateRate Err=%d"
            ),
            static_cast<int32>(Err)
        ),
        Err == 0
        ? FColor::Green
        : FColor::Red
    );
}


// ============================================================
// Debug HUD
// ============================================================

void UVoiceChattingManager::Vivox_TickDebugHud()
{
    if (!bDebugHudEnabled ||
        !VivoxVoiceClient)
    {
        return;
    }


    const FString ModeString =
        CurrentVoiceMode ==
        EVoiceChannelMode::Lobby2D
        ? TEXT("Lobby2D")
        : CurrentVoiceMode ==
        EVoiceChannelMode::InGame3D
        ? TEXT("InGame3D")
        : TEXT("None");


    FString Base =
        FString::Printf(
            TEXT(
                "[Vivox] Init=%d Joined=%d Mode=%s Channel=%s"
            ),
            bVivoxInitialized ? 1 : 0,
            bHasJoinedChannel ? 1 : 0,
            *ModeString,
            *CurrentVoiceChannelName
        );


    if (!bHasJoinedChannel)
    {
        Vivox_ScreenLog(
            Base,
            FColor::Silver,
            0.25f
        );

        return;
    }


    ILoginSession& LoginSession =
        VivoxVoiceClient->GetLoginSession(
            LoggedInUserId
        );


    IChannelSession& ChannelSession =
        LoginSession.GetChannelSession(
            JoinedChannel
        );


    const auto& Participants =
        ChannelSession.Participants();


    IParticipant* Self =
        nullptr;


    for (const auto& Kvp : Participants)
    {
        if (Kvp.Value &&
            Kvp.Value->IsSelf())
        {
            Self =
                Kvp.Value;

            break;
        }
    }


    if (!Self)
    {
        Vivox_ScreenLog(
            Base + TEXT(" | Self=NONE"),
            FColor::Silver,
            0.25f
        );

        return;
    }


    LastSelfAudioEnergy =
        Self->AudioEnergy();


    bLastSelfSpeechDetected =
        Self->SpeechDetected();


    const FString Meter =
        FString::Printf(
            TEXT(
                "%s | Energy=%.3f Speech=%d InAudio=%d Tx=%d"
            ),
            *Base,
            LastSelfAudioEnergy,
            bLastSelfSpeechDetected ? 1 : 0,
            Self->InAudio() ? 1 : 0,
            ChannelSession.IsTransmitting() ? 1 : 0
        );


    Vivox_ScreenLog(
        Meter,
        bLastSelfSpeechDetected
        ? FColor::Green
        : FColor::Silver,
        0.25f
    );
}


// ============================================================
// Debug HUD Toggle
// ============================================================

void UVoiceChattingManager::Vivox_SetDebugHudEnabled(
    bool bEnabled
)
{
    bDebugHudEnabled =
        bEnabled;


    UWorld* World =
        GetWorld();

    if (!World)
        return;


    if (bDebugHudEnabled)
    {
        if (!World
            ->GetTimerManager()
            .IsTimerActive(DebugHudTimerHandle))
        {
            World
                ->GetTimerManager()
                .SetTimer(
                    DebugHudTimerHandle,
                    this,
                    &UVoiceChattingManager::Vivox_TickDebugHud,
                    0.2f,
                    true
                );
        }


        Vivox_ScreenLog(
            TEXT(
                "[Vivox] Debug HUD Enabled"
            ),
            FColor::Yellow
        );
    }
    else
    {
        World
            ->GetTimerManager()
            .ClearTimer(
                DebugHudTimerHandle
            );
    }
}


// ============================================================
// Player Location
// ============================================================

bool UVoiceChattingManager::TryGetPlayerLocationByIndex(
    int32 SeatIndex,
    FVector& OutLocation
) const
{
    UWorld* World =
        GetWorld();

    if (!World)
        return false;


    AGameSessionState* GS =
        World->GetGameState<AGameSessionState>();


    if (!GS)
        return false;


    APawn* Pawn =
        GS->GetPawnBySeat(
            SeatIndex
        );


    if (!Pawn)
        return false;


    OutLocation =
        Pawn->GetActorLocation();


    return true;
}


// ============================================================
// Voice Cache
// ============================================================

void UVoiceChattingManager::StartVoiceCacheTimer()
{
    UGameInstance* GI =
        GetGameInstance();

    UWorld* World =
        GI ? GI->GetWorld() : nullptr;


    if (!World)
        return;


    if (World
        ->GetTimerManager()
        .IsTimerActive(
            VoiceCacheTimerHandle
        ))
    {
        return;
    }


    World->GetTimerManager().SetTimer(
        VoiceCacheTimerHandle,
        this,
        &UVoiceChattingManager::TickVoiceCache,
        VoiceCacheUpdateInterval,
        true
    );
}


void UVoiceChattingManager::StopVoiceCacheTimer()
{
    UGameInstance* GI =
        GetGameInstance();

    UWorld* World =
        GI ? GI->GetWorld() : nullptr;


    if (!World)
        return;


    World->GetTimerManager().ClearTimer(
        VoiceCacheTimerHandle
    );
}


void UVoiceChattingManager::TickVoiceCache()
{
    if (VivoxVoiceClient && bHasJoinedChannel)
    {
        const auto& Participants = VivoxVoiceClient->GetLoginSession(LoggedInUserId)
            .GetChannelSession(JoinedChannel).Participants();
        LastSelfAudioEnergy = 0.0;
        for (const auto& Item : Participants)
        {
            if (Item.Value && Item.Value->IsSelf())
            {
                LastSelfAudioEnergy = Item.Value->AudioEnergy();
                break;
            }
        }
    }

    const float Corrected01 =
        ApplyVoiceLevelCorrection(
            static_cast<float>(
                LastSelfAudioEnergy
                )
        );


    LastCorrectedVoiceLevel =
        Corrected01;


    LastStimuliMap.Reset();


    for (
        int32 SeatIndex = 0;
        SeatIndex < 4;
        ++SeatIndex
        )
    {
        FVector Location;


        if (TryGetPlayerLocationByIndex(
            SeatIndex,
            Location
        ))
        {
            LastStimuliMap.Add(
                SeatIndex,
                TPair<FVector, float>(
                    Location,
                    Corrected01
                )
            );
        }
    }


    PushVoiceCacheToGameInstance();
}


// ============================================================
// Voice Level Correction
// ============================================================

float UVoiceChattingManager::ApplyVoiceLevelCorrection(
    float Raw01
) const
{
    // 추후 실제 보정 로직 적용

    return FMath::Clamp(
        Raw01,
        0.f,
        1.f
    );
}


// ============================================================
// Push Voice Cache
// ============================================================

void UVoiceChattingManager::PushVoiceCacheToGameInstance()
{
    UGameSessionInstance* GSI =
        Cast<UGameSessionInstance>(
            GetGameInstance()
        );


    if (!GSI)
        return;


    GSI->Voice_UpdateCache(
        bHasJoinedChannel,
        static_cast<float>(
            LastSelfAudioEnergy
            ),
        ApplyVoiceLevelCorrection(
            static_cast<float>(
                LastSelfAudioEnergy
                )
        ),
        bMicMuted,
        LastStimuliMap
    );
}

FString UVoiceChattingManager::GetLocalVoicePlayerId()
{
    if (!LocalVoicePlayerId.IsEmpty()) return LocalVoicePlayerId;
    UWorld* World = GetWorld();
    // PIE players may share one Steam account, so each instance gets its own ID.
    if (World && World->WorldType != EWorldType::PIE)
    {
        if (IOnlineSubsystem* OSS = Online::GetSubsystem(World))
        {
            if (OSS->GetSubsystemName() == FName(TEXT("STEAM")))
            {
                const IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
                const FUniqueNetIdPtr UserId = Identity.IsValid() ? Identity->GetUniquePlayerId(0) : nullptr;
                if (UserId.IsValid()) LocalVoicePlayerId = TEXT("steam_") + UserId->ToString();
            }
        }
    }
    if (LocalVoicePlayerId.IsEmpty())
        LocalVoicePlayerId = TEXT("guest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    return LocalVoicePlayerId;
}

void UVoiceChattingManager::HandleWorldChanged(UWorld* World)
{
    if (!World || bShuttingDown) return;
    if (UWorld* OldWorld = VoiceTimerWorld.Get())
    {
        OldWorld->GetTimerManager().ClearTimer(DebugHudTimerHandle);
        OldWorld->GetTimerManager().ClearTimer(VoiceCacheTimerHandle);
        OldWorld->GetTimerManager().ClearTimer(Vivox3DTimer);
        OldWorld->GetTimerManager().ClearTimer(PawnRetryTimerHandle);
        OldWorld->GetTimerManager().ClearTimer(ChannelSwitchTimerHandle);
    }
    CachedSpeaker.Reset();
    CachedListener.Reset();
    VoiceTimerWorld = World;
    if (bDebugHudEnabled)
        World->GetTimerManager().SetTimer(DebugHudTimerHandle, this,
            &UVoiceChattingManager::Vivox_TickDebugHud, 0.2f, true);
    if (bHasJoinedChannel) StartVoiceCacheTimer();
    if (bHasChannelSession && !bChannelConnectInProgress && !bChannelDisconnectInProgress)
        SchedulePendingChannelJoin();
}

void UVoiceChattingManager::ClearChannelCallbacks()
{
    if (!VivoxVoiceClient || !bHasChannelSession) return;
    IChannelSession& Session = VivoxVoiceClient->GetLoginSession(LoggedInUserId).GetChannelSession(JoinedChannel);
    Session.EventChannelStateChanged.Remove(ChannelStateChangedHandle);
    Session.EventAudioStateChanged.Remove(AudioStateChangedHandle);
    Session.EventAfterParticipantAdded.Remove(ParticipantAddedHandle);
    ChannelStateChangedHandle.Reset();
    AudioStateChangedHandle.Reset();
    ParticipantAddedHandle.Reset();
}

void UVoiceChattingManager::HandleChannelStateChanged(const IChannelConnectionState& State)
{
    if (bShuttingDown || !bHasChannelSession || !(State.ChannelSession().Channel() == JoinedChannel)) return;
    if (State.State() == ConnectionState::Disconnected)
    {
        bHasJoinedChannel = false;
        bChannelConnectInProgress = false;
        bChannelDisconnectInProgress = false;
        StopVoiceCacheTimer();
        Vivox_Stop3DUpdate();
        Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Disconnected '%s'"), *CurrentVoiceChannelName), FColor::Yellow);
        // Defer deletion and the next join until after the SDK event finishes.
        SchedulePendingChannelJoin();
    }
}

void UVoiceChattingManager::HandleAudioStateChanged(const IChannelConnectionState& State)
{
    if (bShuttingDown || !bHasChannelSession || !(State.ChannelSession().Channel() == JoinedChannel)) return;
    if (State.State() == ConnectionState::Connected)
    {
        if (bChannelDisconnectInProgress || PendingChannelName.IsEmpty() ||
            PendingChannelName != CurrentVoiceChannelName || bPendingPositional != bCurrentChannelPositional)
            DisconnectCurrentChannel(false);
        else
            MarkChannelAudioConnected();
    }
    else if (State.State() == ConnectionState::Disconnected)
    {
        bHasJoinedChannel = false;
        StopVoiceCacheTimer();
        Vivox_Stop3DUpdate();
        LastSelfAudioEnergy = 0.0;
        LastStimuliMap.Reset();
        PushVoiceCacheToGameInstance();
    }
}

void UVoiceChattingManager::MarkChannelAudioConnected()
{
    const bool bFirstConnection = !bHasJoinedChannel;
    bHasJoinedChannel = true;
    if (bFirstConnection) ApplyInGameVoiceGain();
    bChannelConnectInProgress = false;
    StartVoiceCacheTimer();
    PushVoiceCacheToGameInstance();
    if (bFirstConnection)
        Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Audio CONNECTED '%s' (%s)"), *CurrentVoiceChannelName,
            bCurrentChannelPositional ? TEXT("3D") : TEXT("2D")), FColor::Green);
    if (bCurrentChannelPositional && bAutoStart3DUpdate)
        Vivox_TryAutoStart3DUpdate();
    else
        Vivox_Stop3DUpdate();
}


void UVoiceChattingManager::Vivox_SetInGameVoiceGain(int32 Gain)
{
    InGameVoiceGain = FMath::Clamp(Gain, -50, 50);
    ApplyInGameVoiceGain();
}

void UVoiceChattingManager::HandleParticipantAdded(const IParticipant& Participant)
{
    if (!Participant.IsSelf()) ApplyInGameVoiceGain();
}

void UVoiceChattingManager::ApplyInGameVoiceGain()
{
    if (bShuttingDown || !VivoxVoiceClient || !bHasJoinedChannel || !bCurrentChannelPositional) return;
    IChannelSession& Session = VivoxVoiceClient->GetLoginSession(LoggedInUserId).GetChannelSession(JoinedChannel);
    const int32 Gain = FMath::Clamp(InGameVoiceGain, -50, 50);
    for (const auto& Entry : Session.Participants())
    {
        IParticipant* Participant = Entry.Value;
        if (!Participant || Participant->IsSelf() || Participant->LocalVolumeAdjustment() == Gain) continue;
        IParticipantProperties::FOnBeginSetLocalVolumeAdjustmentCompletedDelegate Done;
        const TWeakObjectPtr<UVoiceChattingManager> WeakThis(this);
        Done.BindLambda([WeakThis](VivoxCoreError Error)
        {
            if (UVoiceChattingManager* Self = WeakThis.Get())
            {
                if (!Self->bShuttingDown && Error != 0)
                    Self->Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Voice gain failed Error=%d"), int32(Error)), FColor::Red);
            }
        });
        const VivoxCoreError Error = Participant->BeginSetLocalVolumeAdjustment(Gain, Done);
        if (Error != 0)
            Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Voice gain rejected Error=%d"), int32(Error)), FColor::Red);
    }
}
