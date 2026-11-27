// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/Sound/AmbientAudioManager.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "GMapSystem/Server/GMapManager.h"
#include "GTypes/GMapTypes.h"
#include "ServerModule/GameSession/InGameGameState.h"
#include "ServerModule/GameSession/PlayerSessionState.h"


void UAmbientAudioManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UAmbientAudioManager::Deinitialize()
{
    StopAmbient();
    Super::Deinitialize();
}

UWorld* UAmbientAudioManager::GetWorldSafe() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return World;
}

UGMapManager* UAmbientAudioManager::GetMapManager() const
{
    UWorld* World = GetWorldSafe();
    if (!World) return nullptr;

    return World->GetSubsystem<UGMapManager>();
}

APawn* UAmbientAudioManager::GetLocalPawn(UWorld* World) const
{
    const ULocalPlayer* Player = GetLocalPlayer();
    APlayerController* Controller = Player && World ? Player->GetPlayerController(World) : nullptr;
    return Controller ? Controller->GetPawn() : nullptr;
}

void UAmbientAudioManager::StopAmbient()
{
    bRunning = false;
    if (UWorld* World = AmbientWorld.Get())
    {
        World->GetTimerManager().ClearTimer(ContextPollTimer);
        World->GetTimerManager().ClearTimer(EventSfxTimer);
    }
    if (IsValid(BaseLoopComp))
    {
        BaseLoopComp->Stop();
        BaseLoopComp->DestroyComponent();
    }
    BaseLoopComp = nullptr;
    AmbientWorld.Reset();
    ContextPawn.Reset();
    bIsOutdoor = false;
    bBaseLoopInitialized = false;
    CurrentZoneId = NAME_None;
}

void UAmbientAudioManager::StartAmbient(USoundBase* InIndoorLoop, USoundBase* InOutdoorLoop, UDataTable* InZoneTable)
{
    UWorld* World = GetWorldSafe();
    if (!World) return;

    if (bRunning && AmbientWorld.Get() == World &&
        IndoorBaseLoop == InIndoorLoop && OutdoorBaseLoop == InOutdoorLoop && ZoneAudioTable == InZoneTable)
    {
        return;
    }

    StopAmbient();
    IndoorBaseLoop = InIndoorLoop;
    OutdoorBaseLoop = InOutdoorLoop;
    ZoneAudioTable = InZoneTable;
    AmbientWorld = World;
    bRunning = true;

    World->GetTimerManager().SetTimer(ContextPollTimer, this,
        &UAmbientAudioManager::TickContext, FMath::Max(0.05f, ContextPollInterval), true);
    TickContext();
}

void UAmbientAudioManager::TickContext()
{
    if (!bRunning) return;
    UWorld* World = AmbientWorld.Get();
    if (!World || World != GetWorldSafe()) return;

    // Replicated GameState can arrive after StartAmbient on a joining client.
    AGameStateBase* GameState = World->GetGameState();
    if (!GameState || !GameState->IsA<AInGameGameState>())
    {
        World->GetTimerManager().ClearTimer(EventSfxTimer);
        if (IsValid(BaseLoopComp))
        {
            BaseLoopComp->Stop();
            BaseLoopComp->DestroyComponent();
        }
        BaseLoopComp = nullptr;
        ContextPawn.Reset();
        CurrentZoneId = NAME_None;
        bBaseLoopInitialized = false;
        if (GameState) StopAmbient();
        return;
    }

    APawn* Pawn = GetLocalPawn(World);
    UGMapManager* MapMgr = GetMapManager();
    if (!Pawn || !MapMgr) return;

    const APlayerSessionState* PS = Pawn->GetPlayerState<APlayerSessionState>();
    // The host reads the authoritative map; clients read their owner-only snapshot.
    TArray<FName> ZoneIds = Pawn->HasAuthority()
        ? MapMgr->GetActorZoneIds(Pawn)
        : (PS ? PS->GetAmbientZoneIds() : TArray<FName>());
    if (ZoneIds.IsEmpty() && !InitialZoneId.IsNone())
    {
        if (!MapMgr->GetZoneData(InitialZoneId)) return;
        // Audio-only startup fallback: do not write into gameplay zone membership.
        ZoneIds.Add(InitialZoneId);
    }
    if (ContextPawn.Get() != Pawn)
    {
        ContextPawn = Pawn;
        bBaseLoopInitialized = false;
        CurrentZoneId = NAME_None;
        World->GetTimerManager().ClearTimer(EventSfxTimer);
    }

    const FName NewZoneId = ChooseDominantZoneId(ZoneIds, MapMgr);
    const FGZoneData* Zone = MapMgr->GetZoneData(NewZoneId);
    const bool bNowOutdoor = Zone && Zone->Type == EGZoneType::E_Outdoor;
    if (!bBaseLoopInitialized || bNowOutdoor != bIsOutdoor)
    {
        bIsOutdoor = bNowOutdoor;
        UpdateBaseLoop(bIsOutdoor);
        bBaseLoopInitialized = true;
        if (bDebugLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[Ambient] Base loop initialized: %s Zone=%s"),
                bIsOutdoor ? TEXT("Outdoor") : TEXT("Indoor"), *NewZoneId.ToString());
        }
    }

    if (NewZoneId != CurrentZoneId)
    {
        CurrentZoneId = NewZoneId;
        if (bDebugLog)
        {
            UE_LOG(LogTemp, Log, TEXT("[Ambient] Zone changed: %s"), *CurrentZoneId.ToString());
        }
        RescheduleEventSfx();
    }
}

void UAmbientAudioManager::UpdateBaseLoop(bool bNowOutdoor)
{
    UWorld* World = GetWorldSafe();
    if (!World) return;

    USoundBase* Target = bNowOutdoor ? OutdoorBaseLoop : IndoorBaseLoop;
    if (!Target)
    {
        // 타겟 루프가 없으면 끔
        if (IsValid(BaseLoopComp))
        {
            BaseLoopComp->Stop();
            BaseLoopComp->DestroyComponent();
            BaseLoopComp = nullptr;
        }
        return;
    }

    // 이미 같은 사운드가 재생 중이면 유지
    if (IsValid(BaseLoopComp) && BaseLoopComp->Sound == Target && BaseLoopComp->IsPlaying())
    {
        return;
    }

    // 기존 루프 정지
    if (IsValid(BaseLoopComp))
    {
        BaseLoopComp->Stop();
        BaseLoopComp->DestroyComponent();
        BaseLoopComp = nullptr;
    }

    // 새 루프 시작
    BaseLoopComp = UGameplayStatics::SpawnSound2D(World, Target, FMath::Max(0.f, BaseLoopVolume),
        1.f, 0.f, nullptr, false, false);
    if (IsValid(BaseLoopComp))
    {
        BaseLoopComp->bAutoDestroy = false;
    }
}

FName UAmbientAudioManager::ChooseDominantZoneId(const TArray<FName>& ZoneIds, UGMapManager* MapMgr) const
{
    if (!MapMgr) return NAME_None;
    if (ZoneIds.Num() == 0) return NAME_None;

    // 우선순위 룰: Room(3) > Basecamp(2) > Corridor(1) > Outdoor(0)
    int32 BestScore = INT_MIN;
    FName BestId = NAME_None;

    for (const FName& Id : ZoneIds)
    {
        const FGZoneData* Z = MapMgr->GetZoneData(Id);
        if (!Z) continue;

        int32 Score = 0;
        switch (Z->Type)
        {
        case EGZoneType::E_Room:     Score = 3; break;
        case EGZoneType::E_Basecamp: Score = 2; break;
        case EGZoneType::E_Corridor: Score = 1; break;
        case EGZoneType::E_Outdoor:  Score = 0; break;
        default:                     Score = 0; break;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestId = Id;
        }
    }

    return BestId;
}

const FAmbientZoneAudioRow* UAmbientAudioManager::FindZoneAudioRow(FName ZoneId) const
{
    if (!ZoneAudioTable) return nullptr;
    if (ZoneId.IsNone()) return nullptr;

    // RowName을 ZoneId로 맞춰 쓰는 방식
    return ZoneAudioTable->FindRow<FAmbientZoneAudioRow>(ZoneId, TEXT("AmbientZoneAudioRow"), false);
}

USoundBase* UAmbientAudioManager::PickRandom(const TArray<TObjectPtr<USoundBase>>& List)
{
    if (List.Num() <= 0) return nullptr;

    const int32 Idx = FMath::RandRange(0, List.Num() - 1);
    return List[Idx].Get();
}

void UAmbientAudioManager::RescheduleEventSfx()
{
    UWorld* World = GetWorldSafe();
    if (!World) return;

    World->GetTimerManager().ClearTimer(EventSfxTimer);

    if (!bEnableEventSfx) return;
    if (CurrentZoneId.IsNone()) return;

    const FAmbientZoneAudioRow* Row = FindZoneAudioRow(CurrentZoneId);
    if (!Row) return;
    if (Row->RandomSfxList.Num() == 0) return;

    const float MinT = FMath::Max(0.1f, Row->MinInterval);
    const float MaxT = FMath::Max(MinT, Row->MaxInterval);
    const float Delay = FMath::FRandRange(MinT, MaxT);

    World->GetTimerManager().SetTimer(
        EventSfxTimer,
        this,
        &UAmbientAudioManager::TryPlayEventSfx,
        Delay,
        false
    );
}

void UAmbientAudioManager::TryPlayEventSfx()
{
    UWorld* World = GetWorldSafe();
    if (!World) return;
    if (!bRunning || World != AmbientWorld.Get() || !World->GetGameState<AInGameGameState>()) return;

    if (!bEnableEventSfx)
    {
        RescheduleEventSfx();
        return;
    }

    APawn* Pawn = GetLocalPawn(World);
    if (!Pawn)
    {
        RescheduleEventSfx();
        return;
    }

    const FAmbientZoneAudioRow* Row = FindZoneAudioRow(CurrentZoneId);
    if (!Row)
    {
        RescheduleEventSfx();
        return;
    }

    const float Chance = FMath::Clamp(Row->PlayChance, 0.f, 1.f);
    if (FMath::FRand() > Chance)
    {
        RescheduleEventSfx();
        return;
    }

    USoundBase* Sfx = PickRandom(Row->RandomSfxList);
    if (!Sfx)
    {
        RescheduleEventSfx();
        return;
    }

    FVector Loc = Pawn->GetActorLocation();

    if (Row->bPlayNearPlayer)
    {
        const float R = FMath::Max(0.f, Row->NearPlayerRadius);
        if (R > 0.f)
        {
            Loc += FMath::VRand() * FMath::FRandRange(0.f, R);
        }
    }

    // 3D 이벤트 사운드 재생
    UGameplayStatics::PlaySoundAtLocation(World, Sfx, Loc, FMath::Max(0.f, EventSfxVolume));

    if (bDebugLog)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Ambient] EventSfx Played: Zone=%s"), *CurrentZoneId.ToString());
    }

    // 다음 예약
    RescheduleEventSfx();
}