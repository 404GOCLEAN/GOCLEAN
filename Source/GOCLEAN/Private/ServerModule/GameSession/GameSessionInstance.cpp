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
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SessionCleanupRetryTimer);
    }
    ClearSessionCleanupDelegate();

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

    // Network disconnects can load Title without calling ReturnToTitle.
    // Clean only this GameInstance's world before exposing Create/Join UI.
    if (LevelName.Equals(TEXT("test_Title")))
    {
        if (UVoiceChattingManager* VoiceManager = GetSubsystem<UVoiceChattingManager>())
        {
            VoiceManager->HandleWorldChanged(LoadedWorld);
        }
        BeginSessionCleanup(false);
        return;
    }

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

    if (LevelName.Equals(TEXT("test_Title")) && bReturningToTitle)
    {
        ScheduleTryShowUI(World, 0.1f);
        return;
    }

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
    BeginSessionCleanup(true);
}

void UGameSessionInstance::BeginSessionCleanup(bool bOpenTitleAfterCleanup)
{
    if (bReturningToTitle)
    {
        // If travel finished during an asynchronous destroy, stay on Title.
        if (!bOpenTitleAfterCleanup) bOpenTitleAfterSessionCleanup = false;
        return;
    }

    bReturningToTitle = true;
    bOpenTitleAfterSessionCleanup = bOpenTitleAfterCleanup;
    UE_LOG(LogTemp, Log, TEXT("[Session] Cleanup begin: GI=%s Reason=%s"),
        *GetName(), bOpenTitleAfterCleanup ? TEXT("ReturnToTitle") : TEXT("TitleLoaded"));

    if (UVoiceChattingManager* VoiceManager = GetSubsystem<UVoiceChattingManager>())
    {
        VoiceManager->Vivox_LeaveVoice();
    }
    ClearPendingJoinCode();
    ClearPendingVendingItemIds();
    SetPendingContractId(0);
    TryDestroySession();
}

void UGameSessionInstance::ClearSessionCleanupDelegate()
{
    if (SessionInterfaceForCleanup.IsValid() && DestroySessionDelegateHandle.IsValid())
    {
        SessionInterfaceForCleanup->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
    }
    DestroySessionDelegateHandle.Reset();
    SessionInterfaceForCleanup.Reset();
}

void UGameSessionInstance::TryDestroySession()
{
    if (!bReturningToTitle) return;
    if (!SessionInterfaceForCleanup.IsValid())
    {
        SessionInterfaceForCleanup = Online::GetSessionInterface(GetWorld());
    }
    if (!SessionInterfaceForCleanup.IsValid() ||
        !SessionInterfaceForCleanup->GetNamedSession(NAME_GameSession))
    {
        FinishSessionCleanup();
        return;
    }

    if (!DestroySessionDelegateHandle.IsValid())
    {
        DestroySessionDelegateHandle = SessionInterfaceForCleanup->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &UGameSessionInstance::HandleDestroySessionComplete));
    }
    // Another caller may already be destroying it. Observe its completion.
    if (SessionInterfaceForCleanup->GetSessionState(NAME_GameSession) == EOnlineSessionState::Destroying)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[Session] Destroy requested: GI=%s State=%d"),
        *GetName(), static_cast<int32>(SessionInterfaceForCleanup->GetSessionState(NAME_GameSession)));
    // Null/LAN providers can complete synchronously and reset the member pointer.
    const IOnlineSessionPtr Sessions = SessionInterfaceForCleanup;
    if (!Sessions->DestroySession(NAME_GameSession) && bReturningToTitle)
    {
        ClearSessionCleanupDelegate();
        RetrySessionCleanup();
    }
}

void UGameSessionInstance::RetrySessionCleanup()
{
    UE_LOG(LogTemp, Warning, TEXT("[Session] Cleanup incomplete: GI=%s; retrying before Create/Join"), *GetName());
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SessionCleanupRetryTimer,
            this, &UGameSessionInstance::TryDestroySession, 1.0f, false);
    }
}

void UGameSessionInstance::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (SessionName != NAME_GameSession || !bReturningToTitle) return;
    ClearSessionCleanupDelegate();
    UE_LOG(LogTemp, Log, TEXT("[Session] Destroy completed: GI=%s Result=%s"),
        *GetName(), bWasSuccessful ? TEXT("Success") : TEXT("Failed"));
    if (!bWasSuccessful)
    {
        RetrySessionCleanup();
        return;
    }
    // Verify the local named record is gone before allowing the title menu.
    TryDestroySession();
}

void UGameSessionInstance::FinishSessionCleanup()
{
    ClearSessionCleanupDelegate();
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SessionCleanupRetryTimer);
    }
    const bool bShouldOpenTitle = bOpenTitleAfterSessionCleanup;
    bOpenTitleAfterSessionCleanup = false;
    bReturningToTitle = false;
    UE_LOG(LogTemp, Log, TEXT("[Session] Cleanup complete: GI=%s NoSession"), *GetName());
    if (bShouldOpenTitle)
    {
        OpenTitleMap();
    }
    else
    {
        ScheduleTryShowUI(GetWorld(), 0.1f);
    }
}

void UGameSessionInstance::OpenTitleMap()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (UGameplayStatics::GetCurrentLevelName(World, true).Equals(TEXT("test_Title")))
    {
        ScheduleTryShowUI(World, 0.1f);
        return;
    }
    UGameplayStatics::OpenLevel(World, FName(TEXT("test_Title")), true);
}
