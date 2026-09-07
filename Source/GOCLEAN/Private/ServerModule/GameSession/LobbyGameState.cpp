// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/LobbyGameState.h"

#include "Net/UnrealNetwork.h"

ALobbyGameState::ALobbyGameState()
{
    bReplicates = true;
}


// ===============
// Replication
// ==============

void ALobbyGameState::GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const
{
    Super::GetLifetimeReplicatedProps( OutLifetimeProps );

    DOREPLIFETIME( ALobbyGameState, SelectedContractId );

    DOREPLIFETIME( ALobbyGameState, VendingItems );
}


// =================
// Contract
// =================

void ALobbyGameState::SetSelectedContractId( int32 NewContractId )
{
    if (!HasAuthority())
        return;

    if (SelectedContractId == NewContractId)
        return;

    SelectedContractId = NewContractId;

    OnRep_SelectedContractId();
}


// =================
// Find Vending
// =================

FVendingItemState*
ALobbyGameState::FindVendingItemMutable( int32 ItemId )
{
    return VendingItems.FindByPredicate(
        [ItemId](const FVendingItemState& Item)
        {
            return Item.ItemId == ItemId;
        }
    );
}


const FVendingItemState*
ALobbyGameState::FindVendingItem( int32 ItemId ) const
{
    return VendingItems.FindByPredicate( [ItemId](const FVendingItemState& Item)
        {
            return Item.ItemId == ItemId;
        }
    );
}


// =======================
// Vending State 조회
// ========================

bool ALobbyGameState::GetVendingItemState( int32 ItemId, FVendingItemState& OutState ) const
{
    const FVendingItemState* Item =
        FindVendingItem(ItemId);

    if (!Item)
        return false;

    OutState = *Item;

    return true;
}


// ====================
// 전체 구매 개수
// ====================

int32 ALobbyGameState::GetTotalPurchasedCount() const
{
    int32 TotalCount = 0;

    for (const FVendingItemState& Item : VendingItems)
    {
        TotalCount += Item.BuyerSeatIndices.Num();
    }

    return TotalCount;
}


// ==================
// 구매 가능 여부
// ==================

bool ALobbyGameState::CanPurchaseItem(int32 ItemId) const
{
    const FVendingItemState* Item = FindVendingItem(ItemId);

    if (!Item)
        return false;


    // -1 = 무제한
    if (Item->RemainingCount < 0)
    {
        return true;
    }


    return Item->RemainingCount > 0;
}


// ============
// 구매
// ===========

bool ALobbyGameState::AddVendingPurchase(int32 ItemId, int32 BuyerSeatIndex)
{
    if (!HasAuthority())
        return false;

    if (BuyerSeatIndex < 0)
        return false;


    FVendingItemState* Item = FindVendingItemMutable(ItemId);

    if (!Item)
        return false;


    // 재고 없음
    if (Item->RemainingCount == 0)
        return false;


    // 같은 플레이어의 같은 아이템 중복 구매 방지
    if (Item->BuyerSeatIndices.Contains(BuyerSeatIndex))
    {
        return false;
    }


    // 구매자 추가
    Item->BuyerSeatIndices.Add(BuyerSeatIndex);


    // -1은 무제한이므로 감소하지 않음
    if (Item->RemainingCount > 0)
    {
        Item->RemainingCount--;
    }


    OnRep_VendingItems();

    return true;
}


// ==================
// 구매 취소
// ==================

bool ALobbyGameState::RemoveVendingPurchase( int32 ItemId, int32 BuyerSeatIndex )
{
    if (!HasAuthority())
        return false;


    FVendingItemState* Item = FindVendingItemMutable(ItemId);

    if (!Item)
        return false;


    const int32 RemovedCount = Item->BuyerSeatIndices.Remove(
            BuyerSeatIndex
        );


    if (RemovedCount <= 0)
        return false;


    // 무제한 아이템은 재고 복원 필요 없음
    if (Item->RemainingCount >= 0)
    {
        Item->RemainingCount += RemovedCount;
    }


    OnRep_VendingItems();

    return true;
}


// ===================================
// 한 플레이어의 모든 구매 제거
// ===================================

int32 ALobbyGameState::RemoveAllVendingPurchasesBySeat( int32 BuyerSeatIndex )
{
    if (!HasAuthority())
        return 0;


    int32 TotalRemoved = 0;


    for (FVendingItemState& Item : VendingItems)
    {
        const int32 RemovedCount = Item.BuyerSeatIndices.Remove( BuyerSeatIndex );


        if (RemovedCount <= 0)
            continue;


        TotalRemoved += RemovedCount;


        // 유한 재고만 복구
        if (Item.RemainingCount >= 0)
        {
            Item.RemainingCount += RemovedCount;
        }
    }


    if (TotalRemoved > 0)
    {
        OnRep_VendingItems();
    }


    return TotalRemoved;
}


// ====================
// 벤딩 초기화
// ====================

void ALobbyGameState::InitializeVendingItems( const TArray<FVendingItemState>& InitialItems )
{
    if (!HasAuthority())
        return;


    VendingItems = InitialItems;

    OnRep_VendingItems();
}


void ALobbyGameState::ClearVendingItems()
{
    if (!HasAuthority())
        return;


    if (VendingItems.IsEmpty())
        return;


    VendingItems.Reset();

    OnRep_VendingItems();
}


// ========
// OnRep
// ========

void ALobbyGameState::OnRep_SelectedContractId()
{
    BP_OnSelectedContractChanged( SelectedContractId );
}


void ALobbyGameState::OnRep_VendingItems()
{
    BP_OnVendingItemsChanged();
}