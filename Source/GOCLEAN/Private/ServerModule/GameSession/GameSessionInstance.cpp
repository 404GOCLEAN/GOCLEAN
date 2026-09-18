// Fill out your copyright notice in the Description page of Project Settings.

#include "ServerModule/GameSession/GameSessionInstance.h"
#include "ServerModule/Voice/VoiceChattingManager.h"

#include "Kismet/GameplayStatics.h"
#include "GCharacter/GOCLEANPlayerController.h"

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
    if (!LoadedWorld) return;

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


    // TODO:
    // 실제 Steam ID 등으로 교체
    const FString PlayerVoiceId = TEXT("TODO_PlayerId");


    // Lobby
    if (LevelName.Equals(TEXT("test_Lobby")))
    {
        if (!PendingJoinCode.IsEmpty())
        {
            VoiceManager->Vivox_EnterLobbyVoice(PlayerVoiceId, PendingJoinCode);
        }
    }

    // InGame
    else if (LevelName.Equals(TEXT("YourInGameMapName")))
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
    if (!MyPC) return;

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
    PendingJoinCode = NewCode;
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