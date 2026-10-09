// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/InGameGameState.h"

#include "GObjectSystem/Server/GObjectManager.h"
#include "GPlayerSystem/Server/GPlayerManager.h"
#include "GMapSystem/Server/GMapManager.h"

#include "Engine/DataTable.h"
#include "ItemModule/Vending/VendingItemData.h"

#include "Net/UnrealNetwork.h"

void AInGameGameState::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
        return;

    ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();

    PlayerManager = NewObject<UGPlayerManager>(this);

    MapManager = GetWorld()->GetSubsystem<UGMapManager>();
}

AInGameGameState::AInGameGameState()
{
    bReplicates = true;
}


// =================
// Replication
// =================

void AInGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AInGameGameState, SelectedContractId);

    DOREPLIFETIME(AInGameGameState, GamePhase);
    DOREPLIFETIME(AInGameGameState, GhostState);
    DOREPLIFETIME(AInGameGameState, ExorcismState);

    DOREPLIFETIME(AInGameGameState, SpiritualGauge);
    DOREPLIFETIME(AInGameGameState, RestGauge);

    DOREPLIFETIME(AInGameGameState, ExorcismProgress);
    DOREPLIFETIME(AInGameGameState, ExtractionTimeRemaining);

    DOREPLIFETIME(AInGameGameState, AliveSurvivorCount);

    DOREPLIFETIME(AInGameGameState, FinalRewardMoney);

    DOREPLIFETIME(AInGameGameState, VendingStocks);
}


// ==============
// Contract
// ==============

void AInGameGameState::SetSelectedContractId(int32 NewContractId)
{
    if (!HasAuthority())
        return;

    if (SelectedContractId == NewContractId)
        return;

    SelectedContractId = NewContractId;

    OnRep_SelectedContractId();
}


// =============
// Phase
// ============

void AInGameGameState::SetGamePhase(EGamePhase NewPhase)
{
    if (!HasAuthority())
        return;

    if (GamePhase == NewPhase)
        return;

    GamePhase = NewPhase;

    OnRep_GamePhase();
}


void AInGameGameState::SetGhostState(EGhostState NewState)
{
    if (!HasAuthority())
        return;

    if (GhostState == NewState)
        return;

    GhostState = NewState;

    OnRep_GhostState();
}


void AInGameGameState::SetExorcismState(EExorcismState NewState)
{
    if (!HasAuthority())
        return;

    if (ExorcismState == NewState)
        return;

    ExorcismState = NewState;

    OnRep_ExorcismState();
}


// ===========
// Gauge
// ===========

void AInGameGameState::SetSpiritualGauge(float NewValue)
{
    if (!HasAuthority())
        return;

    const float Clamped = FMath::Clamp(NewValue, 0.f, 100.f);

    if (FMath::IsNearlyEqual(SpiritualGauge, Clamped))
    {
        return;
    }

    SpiritualGauge = Clamped;

    OnRep_SpiritualGauge();
}


void AInGameGameState::AddSpiritualGauge(float Delta)
{
    SetSpiritualGauge(SpiritualGauge + Delta);
}


void AInGameGameState::SetRestGauge(float NewValue)
{
    if (!HasAuthority())
        return;

    const float Clamped = FMath::Clamp(NewValue, 0.f, 100.f);

    if (FMath::IsNearlyEqual(RestGauge, Clamped))
    {
        return;
    }

    RestGauge = Clamped;

    OnRep_RestGauge();
}


void AInGameGameState::AddRestGauge(float Delta)
{
    SetRestGauge(RestGauge + Delta);
}


// ============
// Exorcism
// ============

void AInGameGameState::SetExorcismProgress(float NewProgress)
{
    if (!HasAuthority())
        return;

    const float Clamped = FMath::Clamp(NewProgress, 0.f, 100.f);

    if (FMath::IsNearlyEqual(ExorcismProgress, Clamped))
    {
        return;
    }

    ExorcismProgress = Clamped;

    OnRep_ExorcismProgress();
}


void AInGameGameState::AddExorcismProgress(float Delta)
{
    SetExorcismProgress(ExorcismProgress + Delta);
}


// ================
// Extraction
// ===============

void AInGameGameState::SetExtractionTimeRemaining(float NewTime)
{
    if (!HasAuthority())
        return;

    const float NewValue = FMath::Max(0.f, NewTime);

    if (FMath::IsNearlyEqual(ExtractionTimeRemaining, NewValue))
    {
        return;
    }

    ExtractionTimeRemaining = NewValue;

    OnRep_ExtractionTimeRemaining();
}


void AInGameGameState::AddExtractionTimeRemaining(float Delta)
{
    SetExtractionTimeRemaining( ExtractionTimeRemaining + Delta );
}


// =================
// Alive Count
// =================

void AInGameGameState::SetAliveSurvivorCount(int32 NewCount)
{
    if (!HasAuthority())
        return;

    const int32 NewValue = FMath::Max(0, NewCount);

    if (AliveSurvivorCount == NewValue)
        return;

    AliveSurvivorCount = NewValue;

    OnRep_AliveSurvivorCount();
}


// =================
// Vending
// =================

void AInGameGameState::InitializeVendingStock(const TArray<int32>& SelectedItemIds, UDataTable* VendingDataTable)
{
    if (!HasAuthority())
        return;

    VendingStocks.Reset();

    if (!VendingDataTable)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Vending] VendingItemDataTable is null.")
        );

        OnRep_VendingStocks();
        return;
    }


    // 최대 5종
    const int32 ItemCount = FMath::Min(SelectedItemIds.Num(), 5);


    const TArray<FName> RowNames = VendingDataTable->GetRowNames();


    for (int32 i = 0; i < ItemCount; ++i)
    {
        const int32 ItemId = SelectedItemIds[i];


        const FVendingItemData* FoundData = nullptr;


        for (const FName& RowName : RowNames)
        {
            const FVendingItemData* Row = VendingDataTable->FindRow<FVendingItemData>(
                    RowName,
                    TEXT("InitializeVendingStock")
                );


            if (Row && Row->VendingItemId == ItemId)
            {
                FoundData = Row;
                break;
            }
        }


        if (!FoundData)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[Vending] ItemId %d not found in DT_VendingItem."),
                ItemId
            );

            continue;
        }


        FInGameVendingStock NewStock;

        NewStock.ItemId = ItemId;

        NewStock.RemainingCount = FoundData->GrantedQuantity;


        VendingStocks.Add(NewStock);


        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[Vending] Initialized ItemId=%d, Count=%d"),
            NewStock.ItemId,
            NewStock.RemainingCount
        );
    }


    // Listen Server에서도 즉시 이벤트 발생
    OnRep_VendingStocks();
}

bool AInGameGameState::IsVendingItemAvailable(int32 ItemId) const
{
    return VendingStocks.ContainsByPredicate(
        [ItemId](const FInGameVendingStock& Stock)
        {
            return Stock.ItemId == ItemId;
        }
    );
}

int32 AInGameGameState::GetVendingRemainingCount(int32 ItemId) const
{
    const FInGameVendingStock* Stock =
        VendingStocks.FindByPredicate(
            [ItemId](const FInGameVendingStock& Entry)
            {
                return Entry.ItemId == ItemId;
            }
        );


    if (!Stock)
        return 0;


    return Stock->RemainingCount;
}


bool AInGameGameState::CanTakeVendingItem(int32 ItemId) const
{
    const FInGameVendingStock* Stock =
        VendingStocks.FindByPredicate(
            [ItemId](const FInGameVendingStock& Entry)
            {
                return Entry.ItemId == ItemId;
            }
        );


    // 로비에서 선택되지 않은 아이템
    if (!Stock)
        return false;


    // -1 = 무제한
    if (Stock->RemainingCount < 0)
        return true;


    return Stock->RemainingCount > 0;
}


bool AInGameGameState::TryTakeVendingItem(int32 ItemId)
{
    if (!HasAuthority())
        return false;


    FInGameVendingStock* Stock =
        VendingStocks.FindByPredicate(
            [ItemId](const FInGameVendingStock& Entry)
            {
                return Entry.ItemId == ItemId;
            }
        );


    if (!Stock)
        return false;


    // 무제한 아이템
    if (Stock->RemainingCount < 0)
    {
        return true;
    }


    // 품절
    if (Stock->RemainingCount <= 0)
    {
        return false;
    }


    --Stock->RemainingCount;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Vending] ItemId=%d, RemainingCount=%d"),
        ItemId,
        Stock->RemainingCount
    );


    // Listen Server 즉시 갱신
    OnRep_VendingStocks();


    return true;
}

// ===========
// Reward
// ==========

void AInGameGameState::SetFinalRewardMoney(int32 NewMoney)
{
    if (!HasAuthority())
        return;

    const int32 NewValue = FMath::Max(0, NewMoney);

    if (FinalRewardMoney == NewValue)
        return;

    FinalRewardMoney = NewValue;

    OnRep_FinalRewardMoney();
}


// ============
// OnRep
// ===========

void AInGameGameState::OnRep_SelectedContractId()
{
}


void AInGameGameState::OnRep_GamePhase()
{
    BP_OnGamePhaseChanged(GamePhase);
}


void AInGameGameState::OnRep_GhostState()
{
    BP_OnGhostStateChanged(GhostState);
}


void AInGameGameState::OnRep_ExorcismState()
{
    BP_OnExorcismStateChanged(ExorcismState);
}


void AInGameGameState::OnRep_SpiritualGauge()
{
    BP_OnSpiritualGaugeChanged(SpiritualGauge);
}


void AInGameGameState::OnRep_RestGauge()
{
    BP_OnRestGaugeChanged(RestGauge);
}


void AInGameGameState::OnRep_ExorcismProgress()
{
    BP_OnExorcismProgressChanged(ExorcismProgress);
}


void AInGameGameState::OnRep_ExtractionTimeRemaining()
{
    BP_OnExtractionTimeChanged(ExtractionTimeRemaining);
}


void AInGameGameState::OnRep_AliveSurvivorCount()
{
    BP_OnAliveSurvivorCountChanged(AliveSurvivorCount);
}


void AInGameGameState::OnRep_FinalRewardMoney()
{
    BP_OnFinalRewardChanged(FinalRewardMoney);
}


void AInGameGameState::OnRep_VendingStocks()
{
    OnInGameVendingStockChanged.Broadcast();
}