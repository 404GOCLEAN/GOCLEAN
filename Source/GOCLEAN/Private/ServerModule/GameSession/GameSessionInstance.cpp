// Fill out your copyright notice in the Description page of Project Settings.

#include "ServerModule/GameSession/GameSessionInstance.h"
#include "ServerModule/Voice/VoiceChattingManager.h"

#include "Kismet/GameplayStatics.h"
#include "GCharacter/GOCLEANPlayerController.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

static float Clamp01(float v) { return FMath::Clamp(v, 0.f, 1.f); }

void UGameSessionInstance::Init()
{
    Super::Init();

    UE_LOG(LogTemp, Warning, TEXT("[GI] Init called"));

    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &UGameSessionInstance::HandlePostLoadMap
    );
}

void UGameSessionInstance::Shutdown()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

    Super::Shutdown();
}

void UGameSessionInstance::OnStart()
{
    Super::OnStart();
    UE_LOG(LogTemp, Warning, TEXT("[GI] OnStart called"));

    PendingUIWorld = GetWorld();
    UITryCount = 0;

    TryShowLevelUI();

    ScheduleTryShowUI(PendingUIWorld.Get(), 0.1f);
}

void UGameSessionInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!LoadedWorld || LoadedWorld->GetGameInstance() != this ||
        LoadedWorld->GetNetMode() == NM_DedicatedServer) return;

    const FString LevelName = UGameplayStatics::GetCurrentLevelName(LoadedWorld, true);
    UE_LOG(LogTemp, Warning, TEXT("[GI] Loaded Level: %s"), *LevelName);

    PendingUIWorld = LoadedWorld;
    UITryCount = 0;

    ScheduleTryShowUI(LoadedWorld, 0.1f);


    // ==========================
    // Voice
    // ==========================

    UVoiceChattingManager* VoiceManager =
        GetSubsystem<UVoiceChattingManager>();

    if (!VoiceManager)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[GI] VoiceManager NULL")
        );

        return;
    }


    VoiceManager->HandleWorldChanged(LoadedWorld);
    RefreshJoinCodeFromSession();
    const FString PlayerVoiceId = VoiceManager->GetLocalVoicePlayerId();

    // Lobby
    if (LevelName.Equals(TEXT("test_Lobby")))
    {
        if (!PendingJoinCode.IsEmpty())
        {
            VoiceManager->Vivox_EnterLobbyVoice(PlayerVoiceId, PendingJoinCode);
        }
    }

    // InGame
    else if (LevelName.Equals(TEXT("FirstPersonMap")))
    {
        if (!PendingJoinCode.IsEmpty())
        {
            VoiceManager->Vivox_EnterInGameVoice(PlayerVoiceId, PendingJoinCode);
        }
    }

    // Title
    else if (LevelName.Equals(TEXT("test_Title")))
    {
        VoiceManager->Vivox_LeaveVoice();
    }
}

void UGameSessionInstance::TryShowLevelUI()
{
    UE_LOG(LogTemp, Warning, TEXT("[GI] TryShowLevelUI ENTER"));

    UWorld* World = PendingUIWorld.Get();
    if (!World) World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GI] World NULL"));
        return;
    }

    const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    UE_LOG(LogTemp, Warning, TEXT("[GI] TryShowLevelUI: PC=%s"),
        PC ? *PC->GetClass()->GetName() : TEXT("NULL"));

    if (!PC)
    {
        if (++UITryCount > MaxUITryCount)
        {
            UE_LOG(LogTemp, Warning, TEXT("[GI] TryShowLevelUI timeout (no PC)"));
            return;
        }

        ScheduleTryShowUI(World, 0.1f);
        return;
    }

    AGOCLEANPlayerController* MyPC = Cast<AGOCLEANPlayerController>(PC);
    UE_LOG(LogTemp, Warning, TEXT("[GI] CastToGOCLEANPC=%s"), MyPC ? TEXT("OK") : TEXT("FAIL"));
    if (!MyPC || !MyPC->IsLocalController())
    {
        if (++UITryCount <= MaxUITryCount) ScheduleTryShowUI(World, 0.1f);
        return;
    }

    if (LevelName.Equals(TEXT("test_Title")))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GI] Calling ShowTitleUI"));
        MyPC->ShowTitleUI();
    }
    else if (LevelName.Equals(TEXT("test_Lobby")))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GI] Calling ShowLobbyUI"));
        MyPC->ShowLobbyUI();
    }
}

void UGameSessionInstance::ScheduleTryShowUI(UWorld* InWorld, float DelaySeconds)
{
    if (!InWorld) return;

    InWorld->GetTimerManager().ClearTimer(UITryTimerHandle);
    InWorld->GetTimerManager().SetTimer(
        UITryTimerHandle, this, &UGameSessionInstance::TryShowLevelUI,
        DelaySeconds, false
    );
}

void UGameSessionInstance::SetPendingJoinCode(const FString& NewCode)
{
    const FString Code = NewCode.TrimStartAndEnd();
    if (!IsValidJoinCode(Code))
    {
        UE_LOG(LogTemp, Warning, TEXT("[Voice] Ignoring invalid join code: '%s'"), *Code);
        return;
    }
    PendingJoinCode = Code;
}

bool UGameSessionInstance::RefreshJoinCodeFromSession()
{
    const IOnlineSessionPtr Sessions = Online::GetSessionInterface(GetWorld());
    if (Sessions.IsValid())
    {
        const FNamedOnlineSession* Session = Sessions->GetNamedSession(NAME_GameSession);
        FString SessionCode;
        if (Session && Session->SessionSettings.Get(FName(TEXT("JoinCode")), SessionCode))
        {
            SetPendingJoinCode(SessionCode);
        }
    }
    return IsValidJoinCode(PendingJoinCode);
}

const FString& UGameSessionInstance::GetPendingJoinCode() const
{
    return PendingJoinCode;
}


void UGameSessionInstance::Voice_UpdateCache(bool bJoined, float EnergyRaw01, float LevelCorrected01, bool bMicMuted, const TMap<int32, TPair<FVector, float>>& InStimuliMap)
{
    bVoiceJoined = bJoined;
    VoiceEnergyRaw01 = EnergyRaw01;
    VoiceLevelCorrected01 = LevelCorrected01;
    bVoiceMicMuted = bMicMuted;
    VoiceStimuliMap = InStimuliMap;
}

FString UGameSessionInstance::GenerateJoinCode(int32 DigitCount)
{
    DigitCount =  FMath::Clamp(DigitCount, 1, 9);


    FString Result;

    Result.Reserve(DigitCount);


    for (int32 i = 0; i < DigitCount; ++i)
    {
        const int32 Digit = FMath::RandRange(0, 9);

        Result.AppendChar(TCHAR('0' + Digit));
    }


    return Result;
}


bool UGameSessionInstance::IsValidJoinCode(const FString& JoinCode, int32 ExpectedDigitCount) const
{
    if (ExpectedDigitCount <= 0)
        return false;


    if (JoinCode.Len() != ExpectedDigitCount)
        return false;


    for (TCHAR Character : JoinCode)
    {
        if (!FChar::IsDigit(Character))
        {
            return false;
        }
    }


    return true;
}


void UGameSessionInstance::ClearPendingJoinCode()
{
    PendingJoinCode.Empty();
}

void UGameSessionInstance::ReturnToTitle()
{
    if (bReturningToTitle)
        return;

    bReturningToTitle = true;

    UE_LOG(LogTemp, Warning, TEXT("[Session] Returning to Title"));

    // 1. 음성 채널 퇴장
    UVoiceChattingManager* VoiceManager = GetSubsystem<UVoiceChattingManager>();

    if (VoiceManager)
    {
        VoiceManager->Vivox_LeaveVoice();
    }

    // 2. 세션 정보 정리
    ClearPendingJoinCode();
    ClearPendingVendingItemIds();
    SetPendingContractId(0);

    // 3. Steam 세션 종료
    SessionInterfaceForCleanup = Online::GetSessionInterface(GetWorld());

    if (!SessionInterfaceForCleanup.IsValid() || !SessionInterfaceForCleanup->GetNamedSession(NAME_GameSession))
    {
        OpenTitleMap();
        return;
    }

    DestroySessionDelegateHandle =
        SessionInterfaceForCleanup->
        AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(
                this,
                &UGameSessionInstance::HandleDestroySessionComplete
            )
        );

    if (!SessionInterfaceForCleanup->DestroySession(NAME_GameSession))
    {
        SessionInterfaceForCleanup->
            ClearOnDestroySessionCompleteDelegate_Handle(
                DestroySessionDelegateHandle
            );

        UE_LOG(LogTemp, Error, TEXT("[Session] Failed to start DestroySession"));

        bReturningToTitle = false;
    }
}

void UGameSessionInstance::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (SessionInterfaceForCleanup.IsValid())
    {
        SessionInterfaceForCleanup->
            ClearOnDestroySessionCompleteDelegate_Handle(
                DestroySessionDelegateHandle
            );
    }

    SessionInterfaceForCleanup.Reset();

    UE_LOG(LogTemp, Warning,
        TEXT("[Session] Destroy completed: %s"),
        bWasSuccessful ? TEXT("Success") : TEXT("Failed"));

    if (!bWasSuccessful)
    {
        bReturningToTitle = false;
        return;
    }

    OpenTitleMap();
}

void UGameSessionInstance::OpenTitleMap()
{
    UWorld* World = GetWorld();

    if (!World)
    {
        bReturningToTitle = false;
        return;
    }

    // 타이틀 맵으로 로컬 이동
    UGameplayStatics::OpenLevel(
        World,
        FName(TEXT("test_Title")),
        true
    );

    bReturningToTitle = false;
}
