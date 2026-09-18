#include "ServerModule/GameSession/LobbyGameState.h"
#include "ServerModule/GameSession/PlayerSessionState.h"

#include "Net/UnrealNetwork.h"


ALobbyGameState::ALobbyGameState()
{
    bReplicates = true;
}


// ============================================================
// Replication
// ============================================================

void ALobbyGameState::NotifyLobbyPlayersChanged()
{
    if (!HasAuthority())
        return;

    ++LobbyRosterRevision;

    // Listen Server Host는 RepNotify가 자동 호출되지 않으므로 직접 처리
    OnRep_LobbyRosterRevision();
}

bool ALobbyGameState::AreAllPlayersReady() const
{
    int32 ValidPlayerCount = 0;

    for (APlayerState* PlayerState : PlayerArray)
    {
        const APlayerSessionState* PSS =
            Cast<APlayerSessionState>(PlayerState);

        if (!IsValid(PSS))
        {
            continue;
        }

        ++ValidPlayerCount;

        // 방장을 포함하여 한 명이라도 Ready가 아니면 시작 불가
        if (!PSS->IsReady())
        {
            return false;
        }
    }

    // 최소 한 명은 존재해야 함
    // 방장 혼자 있어도 방장이 Ready라면 true
    return ValidPlayerCount > 0;
}



void ALobbyGameState::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        LobbyStartServerTime = GetServerWorldTimeSeconds();

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Lobby started: %f"),
            LobbyStartServerTime
        );
    }
}


void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ALobbyGameState, SelectedContractId);
    DOREPLIFETIME(ALobbyGameState, PurchasedVendingItems);
    DOREPLIFETIME(ALobbyGameState, LobbyRosterRevision);
    DOREPLIFETIME(ALobbyGameState, LobbyStartServerTime);
}


// ============================================================
// Contract
// ============================================================

void ALobbyGameState::SetSelectedContractId(int32 NewContractId)
{
    if (!HasAuthority())
        return;

    if (SelectedContractId == NewContractId)
        return;

    SelectedContractId = NewContractId;

    OnRep_SelectedContractId();
}


// ============================================================
// Query
// ============================================================

bool ALobbyGameState::IsVendingItemPurchased(int32 ItemId) const
{
    return PurchasedVendingItems.ContainsByPredicate(
        [ItemId](const FVendingPurchaseState& Purchase)
        {
            return Purchase.ItemId == ItemId;
        }
    );
}


int32 ALobbyGameState::GetVendingBuyerSeatIndex(int32 ItemId) const
{
    const FVendingPurchaseState* Found =
        PurchasedVendingItems.FindByPredicate(
            [ItemId](const FVendingPurchaseState& Purchase)
            {
                return Purchase.ItemId == ItemId;
            }
        );

    return Found ? Found->BuyerSeatIndex : INDEX_NONE;
}


bool ALobbyGameState::DidSeatPurchaseItem(int32 BuyerSeatIndex, int32 ItemId) const
{
    return PurchasedVendingItems.ContainsByPredicate(
        [BuyerSeatIndex, ItemId](
            const FVendingPurchaseState& Purchase
            )
        {
            return
                Purchase.ItemId == ItemId && Purchase.BuyerSeatIndex == BuyerSeatIndex;
        }
    );
}


// ============================================================
// Purchase
// ============================================================

bool ALobbyGameState::AddVendingPurchase(int32 ItemId, int32 BuyerSeatIndex)
{
    if (!HasAuthority())
        return false;

    if (ItemId < 0 || BuyerSeatIndex < 0)
        return false;

    // 같은 종류 중복 구매 금지
    if (IsVendingItemPurchased(ItemId))
        return false;


    FVendingPurchaseState NewPurchase;

    NewPurchase.ItemId = ItemId;

    NewPurchase.BuyerSeatIndex = BuyerSeatIndex;


    PurchasedVendingItems.Add(NewPurchase);

    OnRep_PurchasedVendingItems();

    return true;
}


// ============================================================
// Cancel
// ============================================================

bool ALobbyGameState::RemoveVendingPurchase(int32 ItemId, int32 BuyerSeatIndex)
{
    if (!HasAuthority())
        return false;


    const int32 Removed = PurchasedVendingItems.RemoveAll(
            [ItemId, BuyerSeatIndex]( const FVendingPurchaseState& Purchase )
            {
                return
                    Purchase.ItemId == ItemId &&
                    Purchase.BuyerSeatIndex == BuyerSeatIndex;
            }
        );


    if (Removed <= 0)
        return false;


    OnRep_PurchasedVendingItems();

    return true;
}


// ============================================================
// Remove All
// ============================================================

int32 ALobbyGameState::RemoveAllVendingPurchasesBySeat(int32 BuyerSeatIndex)
{
    if (!HasAuthority())
        return 0;


    const int32 Removed = PurchasedVendingItems.RemoveAll(
            [BuyerSeatIndex]( const FVendingPurchaseState& Purchase)
            {
                return Purchase.BuyerSeatIndex == BuyerSeatIndex;
            }
        );


    if (Removed > 0)
    {
        OnRep_PurchasedVendingItems();
    }


    return Removed;
}


// ============================================================
// Clear
// ============================================================

void ALobbyGameState::ClearVendingPurchases()
{
    if (!HasAuthority())
        return;


    if (PurchasedVendingItems.IsEmpty())
        return;


    PurchasedVendingItems.Reset();

    OnRep_PurchasedVendingItems();
}


// ============================================================
// RepNotify
// ============================================================

void ALobbyGameState::OnRep_SelectedContractId()
{
    BP_OnSelectedContractChanged(SelectedContractId);
}


void ALobbyGameState::OnRep_PurchasedVendingItems()
{
    OnVendingItemsChanged.Broadcast();

    BP_OnVendingItemsChanged();
}


void ALobbyGameState::OnRep_LobbyRosterRevision()
{
    OnLobbyPlayersChanged.Broadcast();
}


// ============================================================
// Lobby Time
// ============================================================

float ALobbyGameState::GetLobbyElapsedSeconds() const
{
    // -1일 때만 아직 시작 시간이 설정되지 않은 상태
    if (LobbyStartServerTime < 0.f)
        return 0.f;

    return FMath::Max(
        0.f,
        GetServerWorldTimeSeconds() - LobbyStartServerTime
    );
}


FString ALobbyGameState::GetLobbyElapsedTimeText() const
{
    const int32 TotalSeconds =
        FMath::FloorToInt(GetLobbyElapsedSeconds());

    const int32 Minutes = TotalSeconds / 60;
    const int32 Seconds = TotalSeconds % 60;

    return FString::Printf(
        TEXT("%02d:%02d"),
        Minutes,
        Seconds
    );
}