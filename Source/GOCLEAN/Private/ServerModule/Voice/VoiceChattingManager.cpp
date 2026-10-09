// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/Voice/VoiceChattingManager.h"

#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

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

void UVoiceChattingManager::Initialize(
    FSubsystemCollectionBase& Collection
)
{
    Super::Initialize(Collection);

    InitVivox();


    if (bDebugHudEnabled)
    {
        UGameInstance* GI =
            GetGameInstance();

        UWorld* World =
            GI ? GI->GetWorld() : nullptr;


        if (World)
        {
            World->GetTimerManager().SetTimer(
                DebugHudTimerHandle,
                this,
                &UVoiceChattingManager::Vivox_TickDebugHud,
                0.2f,
                true
            );
        }
    }
}


// ============================================================
// Deinitialize
// ============================================================

void UVoiceChattingManager::Deinitialize()
{
    Vivox_LeaveVoice();


    UGameInstance* GI =
        GetGameInstance();

    UWorld* World =
        GI ? GI->GetWorld() : nullptr;


    if (World)
    {
        World->GetTimerManager().ClearTimer(
            DebugHudTimerHandle
        );

        World->GetTimerManager().ClearTimer(
            VoiceCacheTimerHandle
        );

        World->GetTimerManager().ClearTimer(
            Vivox3DTimer
        );

        World->GetTimerManager().ClearTimer(
            PawnRetryTimerHandle
        );

        World->GetTimerManager().ClearTimer(
            ChannelSwitchTimerHandle
        );
    }


    if (VivoxVoiceClient &&
        bVivoxInitialized)
    {
        VivoxVoiceClient->Uninitialize();

        bVivoxInitialized = false;
    }


    Super::Deinitialize();
}


// ============================================================
// Init Vivox
// ============================================================

void UVoiceChattingManager::InitVivox()
{
    if (bVivoxInitialized)
        return;


    auto* VivoxModule =
        static_cast<FVivoxCoreModule*>(
            &FModuleManager::Get()
            .LoadModuleChecked(TEXT("VivoxCore"))
            );


    VivoxVoiceClient =
        &VivoxModule->VoiceClient();


    VivoxVoiceClient->Initialize();

    bVivoxInitialized = true;


    Vivox_ScreenLog(
        TEXT("[Vivox] Initialized"),
        FColor::Green
    );
}


// ============================================================
// Login Internal
// ============================================================

void UVoiceChattingManager::Vivox_BeginLoginInternal(
    const FString& PlayerIdOverride
)
{
    FString PlayerId =
        PlayerIdOverride;


    if (PlayerId.IsEmpty())
    {
        PlayerId = FString::Printf(
            TEXT("Player_%lld"),
            FDateTime::UtcNow().GetTicks()
        );
    }


    LoggedInUserId =
        AccountId(
            VIVOX_VOICE_ISSUER,
            PlayerId,
            VIVOX_VOICE_DOMAIN
        );


    ILoginSession& LoginSession =
        VivoxVoiceClient->GetLoginSession(
            LoggedInUserId
        );


    const FTimespan TokenExpiration =
        FTimespan::FromHours(1);


    const FString LoginToken =
        LoginSession.GetLoginToken(
            VIVOX_VOICE_KEY,
            TokenExpiration
        );


    ILoginSession::FOnBeginLoginCompletedDelegate
        OnLoginDone;


    OnLoginDone.BindLambda(
        [this](VivoxCoreError Error)
        {
            bLoginInProgress = false;


            const bool bOk =
                (Error == 0);


            bIsLoggedIn = bOk;


            Vivox_ScreenLog(
                FString::Printf(
                    TEXT(
                        "[Vivox] Login %s (Error=%d)"
                    ),
                    bOk
                    ? TEXT("SUCCESS")
                    : TEXT("FAIL"),
                    static_cast<int32>(Error)
                ),
                bOk
                ? FColor::Green
                : FColor::Red
            );


            if (!bOk)
                return;


            Vivox_EnableEnergyMeter();


            // 로그인 완료 후 대기 중 Channel Join
            if (!PendingChannelName.IsEmpty() &&
                !bHasJoinedChannel)
            {
                Vivox_ScreenLog(
                    TEXT(
                        "[Vivox] Login OK -> Join pending channel"
                    ),
                    FColor::Cyan
                );


                JoinPendingChannel();
            }
        }
    );


    Vivox_ScreenLog(
        FString::Printf(
            TEXT("[Vivox] BeginLogin as '%s'"),
            *PlayerId
        ),
        FColor::Cyan
    );


    LoginSession.BeginLogin(
        VIVOX_VOICE_SERVER,
        LoginToken,
        OnLoginDone
    );
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

void UVoiceChattingManager::Vivox_StartVoice(
    const FString& PlayerIdOverride,
    const FString& ChannelName,
    bool bPositional
)
{
    if (!VivoxVoiceClient)
    {
        InitVivox();

        if (!VivoxVoiceClient)
            return;
    }


    // 요청할 Channel 정보 저장
    PendingPlayerId =
        PlayerIdOverride;

    PendingChannelName =
        ChannelName;

    bPendingPositional =
        bPositional;


    // ========================================================
    // 이미 Channel에 들어가 있음
    // ========================================================

    if (bHasJoinedChannel)
    {
        const bool bSameChannel =
            CurrentVoiceChannelName ==
            ChannelName;


        const bool bSameType =
            bCurrentChannelPositional ==
            bPositional;


        if (bSameChannel && bSameType)
        {
            Vivox_ScreenLog(
                TEXT(
                    "[Vivox] Already in requested Voice Channel"
                ),
                FColor::Yellow
            );


            // 같은 InGame 채널이어도
            // Pawn만 새로 생성되었을 가능성 대응
            if (bPositional &&
                CurrentVoiceMode ==
                EVoiceChannelMode::InGame3D)
            {
                Vivox_TryAutoStart3DUpdate();
            }


            return;
        }


        // 다른 Channel / 다른 Type
        Vivox_ScreenLog(
            TEXT(
                "[Vivox] Switching Voice Channel..."
            ),
            FColor::Cyan
        );


        DisconnectCurrentChannel(false);


        SchedulePendingChannelJoin();

        return;
    }


    // ========================================================
    // Login 진행 중
    // ========================================================

    if (bLoginInProgress)
    {
        Vivox_ScreenLog(
            TEXT(
                "[Vivox] Voice request queued: Login in progress"
            ),
            FColor::Yellow
        );

        return;
    }


    // ========================================================
    // 이미 Login 완료
    // ========================================================

    if (bIsLoggedIn)
    {
        JoinPendingChannel();

        return;
    }


    // ========================================================
    // Login 시작
    // ========================================================

    bLoginInProgress = true;


    Vivox_ScreenLog(
        TEXT(
            "[Vivox] StartVoice -> Begin Login"
        ),
        FColor::Cyan
    );


    Vivox_BeginLoginInternal(
        PendingPlayerId
    );
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
    if (PendingChannelName.IsEmpty())
        return;


    if (!bIsLoggedIn)
    {
        if (!bLoginInProgress)
        {
            bLoginInProgress = true;

            Vivox_BeginLoginInternal(
                PendingPlayerId
            );
        }

        return;
    }


    Vivox_JoinVoice(
        PendingChannelName,
        bPendingPositional
    );
}


// ============================================================
// Join Voice Channel
// ============================================================

void UVoiceChattingManager::Vivox_JoinVoice(
    const FString& ChannelName,
    bool bPositional
)
{
    if (!VivoxVoiceClient)
        return;


    if (!bIsLoggedIn)
    {
        Vivox_ScreenLog(
            TEXT(
                "[Vivox] JoinVoice ignored: Not logged in"
            ),
            FColor::Red
        );

        return;
    }


    ILoginSession& LoginSession =
        VivoxVoiceClient->GetLoginSession(
            LoggedInUserId
        );


    const ChannelType Type =
        bPositional
        ? ChannelType::Positional
        : ChannelType::NonPositional;


    const ChannelId Channel(
        VIVOX_VOICE_ISSUER,
        ChannelName,
        VIVOX_VOICE_DOMAIN,
        Type
    );


    IChannelSession& ChannelSession =
        LoginSession.GetChannelSession(
            Channel
        );


    const FTimespan TokenExpiration =
        FTimespan::FromHours(1);


    const FString JoinToken =
        ChannelSession.GetConnectToken(
            VIVOX_VOICE_KEY,
            TokenExpiration
        );


    IChannelSession::FOnBeginConnectCompletedDelegate
        OnConnectDone;


    OnConnectDone.BindLambda(
        [
            this,
            ChannelName,
            bPositional,
            Channel
        ]
    (VivoxCoreError Error)
        {
            const bool bOk =
                (Error == 0);


            Vivox_ScreenLog(
                FString::Printf(
                    TEXT(
                        "[Vivox] Join '%s' (%s) %s Error=%d"
                    ),
                    *ChannelName,
                    bPositional
                    ? TEXT("Positional")
                    : TEXT("NonPositional"),
                    bOk
                    ? TEXT("SUCCESS")
                    : TEXT("FAIL"),
                    static_cast<int32>(Error)
                ),
                bOk
                ? FColor::Green
                : FColor::Red
            );


            if (!bOk)
                return;


            // 참가 성공 후에만 현재 Channel 확정
            JoinedChannel =
                Channel;


            bHasJoinedChannel =
                true;


            CurrentVoiceChannelName =
                ChannelName;


            bCurrentChannelPositional =
                bPositional;


            StartVoiceCacheTimer();


            // Lobby는 3D Update 금지
            if (!bPositional)
            {
                Vivox_Stop3DUpdate();

                return;
            }


            // InGame Positional
            if (bAutoStart3DUpdate &&
                CurrentVoiceMode ==
                EVoiceChannelMode::InGame3D)
            {
                Vivox_TryAutoStart3DUpdate();
            }
        }
        );


    Vivox_ScreenLog(
        FString::Printf(
            TEXT(
                "[Vivox] BeginConnect '%s' (%s)"
            ),
            *ChannelName,
            bPositional
            ? TEXT("Positional")
            : TEXT("NonPositional")
        ),
        FColor::Cyan
    );


    ChannelSession.BeginConnect(
        true,
        false,
        true,
        JoinToken,
        OnConnectDone
    );
}


// ============================================================
// Disconnect Internal
// ============================================================

void UVoiceChattingManager::DisconnectCurrentChannel(
    bool bClearMode
)
{
    // 먼저 3D / Cache 정리
    StopVoiceCacheTimer();

    Vivox_Stop3DUpdate();


    if (VivoxVoiceClient &&
        bHasJoinedChannel)
    {
        ILoginSession& LoginSession =
            VivoxVoiceClient->GetLoginSession(
                LoggedInUserId
            );


        IChannelSession& ChannelSession =
            LoginSession.GetChannelSession(
                JoinedChannel
            );


        ChannelSession.Disconnect(true);


        Vivox_ScreenLog(
            FString::Printf(
                TEXT(
                    "[Vivox] Disconnect Channel: %s"
                ),
                *CurrentVoiceChannelName
            ),
            FColor::Yellow
        );
    }


    bHasJoinedChannel = false;

    CurrentVoiceChannelName.Empty();

    bCurrentChannelPositional = false;


    if (bClearMode)
    {
        CurrentVoiceMode =
            EVoiceChannelMode::None;

        PendingChannelName.Empty();

        bPendingPositional = false;
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

void UVoiceChattingManager::Vivox_LoginSimple(
    const FString& PlayerIdOverride
)
{
    if (!VivoxVoiceClient)
    {
        InitVivox();
    }


    if (!VivoxVoiceClient)
        return;


    FString PlayerId =
        PlayerIdOverride;


    if (PlayerId.IsEmpty())
    {
        PlayerId =
            FString::Printf(
                TEXT("Player_%lld"),
                FDateTime::UtcNow().GetTicks()
            );
    }


    LoggedInUserId =
        AccountId(
            VIVOX_VOICE_ISSUER,
            PlayerId,
            VIVOX_VOICE_DOMAIN
        );


    ILoginSession& LoginSession =
        VivoxVoiceClient->GetLoginSession(
            LoggedInUserId
        );


    const FTimespan TokenExpiration =
        FTimespan::FromHours(1);


    const FString LoginToken =
        LoginSession.GetLoginToken(
            VIVOX_VOICE_KEY,
            TokenExpiration
        );


    ILoginSession::FOnBeginLoginCompletedDelegate
        OnLoginDone;


    OnLoginDone.BindLambda(
        [this](VivoxCoreError Error)
        {
            const bool bOk =
                (Error == 0);


            bIsLoggedIn =
                bOk;


            Vivox_ScreenLog(
                FString::Printf(
                    TEXT(
                        "[Vivox] Login %s (Error=%d)"
                    ),
                    bOk
                    ? TEXT("SUCCESS")
                    : TEXT("FAIL"),
                    static_cast<int32>(Error)
                ),
                bOk
                ? FColor::Green
                : FColor::Red
            );


            if (bOk)
            {
                Vivox_EnableEnergyMeter();
            }
        }
    );


    LoginSession.BeginLogin(
        VIVOX_VOICE_SERVER,
        LoginToken,
        OnLoginDone
    );
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