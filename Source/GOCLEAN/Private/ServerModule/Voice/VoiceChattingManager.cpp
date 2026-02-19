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


void UVoiceChattingManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    InitVivox();

    // 디버그 HUD 기본 ON이면 타이머 시작
    if (bDebugHudEnabled)
    {
        UGameInstance* GI = GetGameInstance();
        UWorld* World = GI ? GI->GetWorld() : nullptr;
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

void UVoiceChattingManager::Deinitialize()
{
    Vivox_LeaveVoice();

    UGameInstance* GI = GetGameInstance();
    UWorld* World = GI ? GI->GetWorld() : nullptr;
    if (World)
    {
        World->GetTimerManager().ClearTimer(DebugHudTimerHandle);
        World->GetTimerManager().ClearTimer(VoiceCacheTimerHandle);
    }

    if (VivoxVoiceClient && bVivoxInitialized)
    {
        VivoxVoiceClient->Uninitialize();
        bVivoxInitialized = false;
    }

    Super::Deinitialize();
}


void UVoiceChattingManager::InitVivox()
{
    if (bVivoxInitialized) return;

    auto* VivoxModule = static_cast<FVivoxCoreModule*>(
        &FModuleManager::Get().LoadModuleChecked(TEXT("VivoxCore"))
        );

    VivoxVoiceClient = &VivoxModule->VoiceClient();
    VivoxVoiceClient->Initialize();
    bVivoxInitialized = true;

    Vivox_ScreenLog(TEXT("[Vivox] Initialized"), FColor::Green);
}

void UVoiceChattingManager::Vivox_BeginLoginInternal(const FString& PlayerIdOverride)
{
    FString PlayerId = PlayerIdOverride;
    if (PlayerId.IsEmpty())
    {
        PlayerId = FString::Printf(TEXT("Player_%lld"), FDateTime::UtcNow().GetTicks());
    }

    LoggedInUserId = AccountId(VIVOX_VOICE_ISSUER, PlayerId, VIVOX_VOICE_DOMAIN);
    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);

    const FTimespan TokenExpiration = FTimespan::FromHours(1);
    const FString LoginToken = LoginSession.GetLoginToken(VIVOX_VOICE_KEY, TokenExpiration);

    ILoginSession::FOnBeginLoginCompletedDelegate OnLoginDone;
    OnLoginDone.BindLambda([this](VivoxCoreError Error)
        {
            bLoginInProgress = false;

            const bool bOk = (Error == 0);
            bIsLoggedIn = bOk;

            Vivox_ScreenLog(
                FString::Printf(TEXT("[Vivox] Login %s (Error=%d)"),
                    bOk ? TEXT("SUCCESS") : TEXT("FAIL"),
                    (int)Error),
                bOk ? FColor::Green : FColor::Red);

            if (!bOk)
                return;

            // 에너지 미터 설정
            Vivox_EnableEnergyMeter();

            // 로그인 성공 후에만 Join을 호출 (NotLoggedIn 5025 방지 핵심)
            if (!PendingChannelName.IsEmpty() && !bHasJoinedChannel)
            {
                Vivox_ScreenLog(TEXT("[Vivox] Login OK -> joining pending channel..."), FColor::Cyan);
                Vivox_JoinVoice(PendingChannelName, bPendingPositional);
            }
        });

    Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] BeginLogin as '%s'"), *PlayerId), FColor::Cyan);
    LoginSession.BeginLogin(VIVOX_VOICE_SERVER, LoginToken, OnLoginDone);
}

void UVoiceChattingManager::Vivox_TryAutoStart3DUpdate()
{
    if (!GetWorld()) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->IsLocalController()) return;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn)
    {
        Vivox_ScreenLog(TEXT("[Vivox] Auto3DUpdate: No Pawn yet."), FColor::Yellow);
        return;
    }

    CachedSpeaker = Pawn;
    CachedListener = Pawn;

    Vivox_Start3DUpdate(CachedSpeaker.Get(), CachedListener.Get(), Auto3DUpdateInterval);
}

void UVoiceChattingManager::Vivox_Update3D_Internal()
{
    if (!VivoxVoiceClient || !bHasJoinedChannel) return;

    AActor* SpeakerActor = CachedSpeaker.Get();
    AActor* ListenerActor = CachedListener.Get();
    if (!SpeakerActor || !ListenerActor) return;

    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
    IChannelSession& ChannelSession = LoginSession.GetChannelSession(JoinedChannel);

    const FVector SpeakerPos = SpeakerActor->GetActorLocation();

    const FVector ListenerPos = ListenerActor->GetActorLocation();
    const FVector ListenerForward = ListenerActor->GetActorForwardVector();
    const FVector ListenerUp = ListenerActor->GetActorUpVector();

    VivoxCoreError Err = ChannelSession.Set3DPosition(SpeakerPos, ListenerPos, ListenerForward, ListenerUp);

    // 실패 시
    if (Err != 0)
    {
        Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Set3DPosition FAIL Err=%d"), (int)Err), FColor::Red, 0.5f);
    }
}

// 디버그용. 나중에 삭제.
void UVoiceChattingManager::Vivox_ScreenLog(const FString& Msg, const FColor& Color, float Time)
{
    UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, Time, Color, Msg);
    }
}

void UVoiceChattingManager::Vivox_EnableEnergyMeter()
{
    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);

    // Update10Hz / 5Hz / 1Hz 중 선택
    const VivoxCoreError Err = LoginSession.SetParticipantSpeakingUpdateRate(ParticipantSpeakingUpdateRate::Update10Hz);
    Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] SetParticipantSpeakingUpdateRate Err=%d"), (int)Err), (Err == 0) ? FColor::Green : FColor::Red);
}

void UVoiceChattingManager::Vivox_TickDebugHud()
{
    if (!bDebugHudEnabled || !VivoxVoiceClient) return;

    // 로그인/채널 기본 상태 출력
    FString Base = FString::Printf(TEXT("[Vivox] Init=%d Joined=%d"), bVivoxInitialized ? 1 : 0, bHasJoinedChannel ? 1 : 0);

    if (!bHasJoinedChannel)
    {
//        Vivox_ScreenLog(Base, FColor::Silver, 0.25f);
        return;
    }

    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
    IChannelSession& ChannelSession = LoginSession.GetChannelSession(JoinedChannel);

    const auto& Participants = ChannelSession.Participants();

    IParticipant* Self = nullptr;
    for (const auto& Kvp : Participants)
    {
        if (Kvp.Value && Kvp.Value->IsSelf())
        {
            Self = Kvp.Value;
            break;
        }
    }

    if (!Self)
    {
        Vivox_ScreenLog(Base + TEXT(" | Self=NONE"), FColor::Silver, 0.25f);
        return;
    }

    LastSelfAudioEnergy = Self->AudioEnergy();
    bLastSelfSpeechDetected = Self->SpeechDetected();

    const FString Meter = FString::Printf(TEXT("%s | SelfEnergy=%.3f Speech=%d InAudio=%d Tx=%d"),
        *Base,
        LastSelfAudioEnergy,
        bLastSelfSpeechDetected ? 1 : 0,
        Self->InAudio() ? 1 : 0,
        ChannelSession.IsTransmitting() ? 1 : 0);

    Vivox_ScreenLog(Meter, bLastSelfSpeechDetected ? FColor::Green : FColor::Silver, 0.25f);
}

bool UVoiceChattingManager::TryGetPlayerLocationByIndex(int32 SeatIndex, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    if (!World) return false;

    AGameStateBase* GSBase = World->GetGameState();
    AGameSessionState* GS = Cast<AGameSessionState>(GSBase);
    if (!GS) return false;

    for (APlayerState* PS : GS->PlayerArray)
    {
        if (!PS) continue;

        APlayerSessionState* PSS = Cast<APlayerSessionState>(PS);
        if (!PSS) continue;

        if (PSS->GetSeatIndex() != SeatIndex)
            continue;

        APlayerController* PC = Cast<APlayerController>(PSS->GetOwner());
        if (!PC) return false;

        APawn* Pawn = PC->GetPawn();
        if (!Pawn) return false;

        OutLocation = Pawn->GetActorLocation();
        return true;
    }

    return false;
}


void UVoiceChattingManager::Vivox_LoginSimple(const FString& PlayerIdOverride)
{
    if (!VivoxVoiceClient) InitVivox();

    FString PlayerId = PlayerIdOverride;
    if (PlayerId.IsEmpty())
    {
        PlayerId = FString::Printf(TEXT("Player_%lld"), FDateTime::UtcNow().GetTicks());
    }

    LoggedInUserId = AccountId(VIVOX_VOICE_ISSUER, PlayerId, VIVOX_VOICE_DOMAIN);
    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);

    // 토큰 유효 시간 : 테스트용 (1시간)
    FTimespan TokenExpiration = FTimespan::FromHours(1);
    FString LoginToken = LoginSession.GetLoginToken(VIVOX_VOICE_KEY, TokenExpiration);

    ILoginSession::FOnBeginLoginCompletedDelegate OnLoginDone;
    OnLoginDone.BindLambda([this](VivoxCoreError Error)
        {
            const bool bOk = (Error == 0);
            Vivox_ScreenLog(
                FString::Printf(TEXT("[Vivox] Login %s (Error=%d)"),
                    bOk ? TEXT("SUCCESS") : TEXT("FAIL"), (int)Error),
                bOk ? FColor::Green : FColor::Red);

            if (bOk)
            {
                Vivox_EnableEnergyMeter();
            }
        });

    Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] BeginLogin as '%s'"), *PlayerId), FColor::Cyan);
    LoginSession.BeginLogin(VIVOX_VOICE_SERVER, LoginToken, OnLoginDone);
}

void UVoiceChattingManager::Vivox_JoinVoice(const FString& ChannelName, bool bPositional)
{
    if (!VivoxVoiceClient) return;

    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);

    // 채널 타입 결정
    ChannelType Type = bPositional ? ChannelType::Positional : ChannelType::NonPositional;

    ChannelId Channel(VIVOX_VOICE_ISSUER, ChannelName, VIVOX_VOICE_DOMAIN, Type);
    IChannelSession& ChannelSession = LoginSession.GetChannelSession(Channel);

    FTimespan TokenExpiration = FTimespan::FromHours(1);
    FString JoinToken = ChannelSession.GetConnectToken(VIVOX_VOICE_KEY, TokenExpiration);

    // Audio=true, Text=false, bSwitchTransmission=true(보통 true)
    IChannelSession::FOnBeginConnectCompletedDelegate OnConnectDone;
    OnConnectDone.BindLambda([this, ChannelName, bPositional](VivoxCoreError Error)
        {
            const bool bOk = (Error == 0);

            Vivox_ScreenLog(
                FString::Printf(TEXT("[Vivox] Join '%s' (%s) %s (Error=%d)"),
                    *ChannelName,
                    bPositional ? TEXT("Positional") : TEXT("NonPositional"),
                    bOk ? TEXT("SUCCESS") : TEXT("FAIL"),
                    (int)Error),
                bOk ? FColor::Green : FColor::Red);

            // 성공일 때만 상태 확정
            if (bOk)
            {
                bHasJoinedChannel = true;

                StartVoiceCacheTimer();

                if (bAutoStart3DUpdate && bPositional)
                {
                    Vivox_TryAutoStart3DUpdate();
                }
            }
        });

    Vivox_ScreenLog(
        FString::Printf(TEXT("[Vivox] BeginConnect '%s' (%s)"),
            *ChannelName, bPositional ? TEXT("Positional") : TEXT("NonPositional")),
        FColor::Cyan);

    ChannelSession.BeginConnect(true, false, true, JoinToken, OnConnectDone);

    // 채널 추적은 일단 저장(실패할 수도 있으니 bHasJoinedChannel은 콜백에서만 true)
    JoinedChannel = Channel;
}

void UVoiceChattingManager::Vivox_LeaveVoice()
{
    if (!VivoxVoiceClient || !bHasJoinedChannel) return;

    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
    IChannelSession& ChannelSession = LoginSession.GetChannelSession(JoinedChannel);

    StopVoiceCacheTimer();
    Vivox_Stop3DUpdate();

    ChannelSession.Disconnect(true);

    bHasJoinedChannel = false;

    Vivox_ScreenLog(TEXT("[Vivox] Leave channel requested."), FColor::Yellow);
}

void UVoiceChattingManager::Vivox_StartVoice(const FString& PlayerIdOverride, const FString& ChannelName, bool bPositional)
{
    if (!VivoxVoiceClient)
    {
        InitVivox();
        if (!VivoxVoiceClient) return;
    }

    // 요청값 저장(트래블/위젯 중복 호출이 와도 마지막 요청으로 덮어씀)
    PendingPlayerId = PlayerIdOverride;
    PendingChannelName = ChannelName;
    bPendingPositional = bPositional;

    // 이미 채널에 들어가 있으면 중복 조인 방지
    if (bHasJoinedChannel)
    {
        Vivox_ScreenLog(TEXT("[Vivox] StartVoice ignored: already joined."), FColor::Yellow);
        return;
    }

    // 로그인 진행 중이면 (로그인 완료 콜백에서 Join 해줄 거라) 여기서 리턴
    if (bLoginInProgress)
    {
        Vivox_ScreenLog(TEXT("[Vivox] StartVoice queued: login in progress."), FColor::Yellow);
        return;
    }

    // 이미 로그인 상태면 바로 Join
    if (bIsLoggedIn)
    {
        Vivox_ScreenLog(TEXT("[Vivox] StartVoice: already logged in -> joining..."), FColor::Cyan);
        Vivox_JoinVoice(PendingChannelName, bPendingPositional);
        return;
    }

    // 로그인 시작
    bLoginInProgress = true;
    Vivox_ScreenLog(TEXT("[Vivox] StartVoice: begin login..."), FColor::Cyan);

    Vivox_BeginLoginInternal(PendingPlayerId);
}

void UVoiceChattingManager::Vivox_SetMicActive(bool bMuted)
{
    if (!VivoxVoiceClient) return;

    // 채널에 들어가 있든 아니든 입력 장치 자체를 mute/unmute
    VivoxVoiceClient->AudioInputDevices().SetMuted(bMuted);
    SetMicMuted(bMuted);

    Vivox_ScreenLog(
        bMuted ? TEXT("[Vivox] Mic MUTED") : TEXT("[Vivox] Mic UNMUTED"),
        bMuted ? FColor::Yellow : FColor::Green
    );
}

void UVoiceChattingManager::Vivox_Update3DFromActors(AActor* SpeakerActor, AActor* ListenerActor)
{

    if (!VivoxVoiceClient || !bHasJoinedChannel) return;
    if (!SpeakerActor || !ListenerActor) return;

    ILoginSession& LoginSession = VivoxVoiceClient->GetLoginSession(LoggedInUserId);
    IChannelSession& ChannelSession = LoginSession.GetChannelSession(JoinedChannel);

    // 리스너 기준(보통 카메라/로컬 플레이어)
    const FVector ListenerPos = ListenerActor->GetActorLocation() * 0.01f;
    const FVector ListenerForward = ListenerActor->GetActorForwardVector();
    const FVector ListenerUp = ListenerActor->GetActorUpVector();

    // 스피커(내 목소리 위치)
    const FVector SpeakerPos = SpeakerActor->GetActorLocation() * 0.01f;

    VivoxCoreError Err = ChannelSession.Set3DPosition(SpeakerPos, ListenerPos, ListenerForward, ListenerUp);

    // 디버그용
    if (Err != 0)
    {
        Vivox_ScreenLog(FString::Printf(TEXT("[Vivox] Set3DPosition FAIL Err=%d"), (int)Err), FColor::Red, 0.5f);
    }
}

void UVoiceChattingManager::Vivox_Start3DUpdate(AActor* SpeakerActor, AActor* ListenerActor, float Interval)
{
    if (!GetWorld()) return;

    CachedSpeaker = SpeakerActor;
    CachedListener = ListenerActor;

    // 위치 업데이트 주기 (0.1~0.2 로 나중에 수정)
    Interval = FMath::Clamp(Interval, 0.05f, 0.5f);

    GetWorld()->GetTimerManager().SetTimer(
        Vivox3DTimer,
        [this]()
        {
            AActor* Spk = CachedSpeaker.Get();
            AActor* Lis = CachedListener.Get();
            Vivox_Update3DFromActors(Spk, Lis);
        },
        Interval,
        true
    );

    Vivox_ScreenLog(TEXT("[Vivox] 3D Update Started"), FColor::Cyan);
}

void UVoiceChattingManager::Vivox_Stop3DUpdate()
{
    if (!GetWorld()) return;
    GetWorld()->GetTimerManager().ClearTimer(Vivox3DTimer);
    CachedSpeaker.Reset();
    CachedListener.Reset();

    Vivox_ScreenLog(TEXT("[Vivox] 3D Update Stopped"), FColor::Cyan);
}

void UVoiceChattingManager::Vivox_SetDebugHudEnabled(bool bEnabled)
{
    bDebugHudEnabled = bEnabled;

    if (!GetWorld()) return;

    if (bDebugHudEnabled)
    {
        if (!GetWorld()->GetTimerManager().IsTimerActive(DebugHudTimerHandle))
        {
            GetWorld()->GetTimerManager().SetTimer(
                DebugHudTimerHandle, this, &UVoiceChattingManager::Vivox_TickDebugHud, 0.2f, true);
        }
        Vivox_ScreenLog(TEXT("[Vivox] Debug HUD Enabled"), FColor::Yellow);
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(DebugHudTimerHandle);
        Vivox_ScreenLog(TEXT("[Vivox] Debug HUD Disabled"), FColor::Yellow);
    }
}

void UVoiceChattingManager::StartVoiceCacheTimer()
{
    UGameInstance* GI = GetGameInstance();
    UWorld* World = GI ? GI->GetWorld() : nullptr;
    if (!World) return;

    if (World->GetTimerManager().IsTimerActive(VoiceCacheTimerHandle))
        return;

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
    UGameInstance* GI = GetGameInstance();
    UWorld* World = GI ? GI->GetWorld() : nullptr;
    if (!World) return;

    World->GetTimerManager().ClearTimer(VoiceCacheTimerHandle);
}

void UVoiceChattingManager::TickVoiceCache()
{

    const float Corrected01 = ApplyVoiceLevelCorrection(LastSelfAudioEnergy);

    LastStimuliMap.Reset();

    for (int32 SeatIndex = 0; SeatIndex < 4; ++SeatIndex)
    {
        FVector Loc;
        if (TryGetPlayerLocationByIndex(SeatIndex, Loc))
        {
            LastStimuliMap.Add(
                SeatIndex,
                TPair<FVector, float>(Loc, Corrected01)
            );
        }
    }

    PushVoiceCacheToGameInstance();
}

float UVoiceChattingManager::ApplyVoiceLevelCorrection(float Raw01) const
{
    // 보정 로직 따로 추가

    return Raw01;
}

void UVoiceChattingManager::PushVoiceCacheToGameInstance()
{
    UGameInstance* GI = GetGameInstance();
    UGameSessionInstance* GSI = Cast<UGameSessionInstance>(GI);
    if (!GSI) return;

    GSI->Voice_UpdateCache(
        bHasJoinedChannel,
        LastSelfAudioEnergy,
        ApplyVoiceLevelCorrection(LastSelfAudioEnergy),
        bMicMuted,
        LastStimuliMap
    );
}
