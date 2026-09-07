// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ServerModule/GameSession/GameSessionState.h"
#include "LobbyGameState.generated.h"


// 벤딩 아이템 상태
USTRUCT(BlueprintType)
struct FVendingItemState
{
    GENERATED_BODY()


    // 아이템 ID
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending" )
    int32 ItemId = INDEX_NONE;


    // 남은 구매 가능 개수
    //
    // -1 = 무제한
    //  0 = 품절
    //  1 이상 = 남은 재고
    UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "Vending" )
    int32 RemainingCount = 0;


    // 해당 아이템을 구매한 플레이어 Seat
    //
    // 예: [0, 2] -> 1P / 3P가 구매
    UPROPERTY( BlueprintReadOnly, Category = "Vending" )
    TArray<int32> BuyerSeatIndices;
};


/**
 * 로비에서 모든 플레이어가 공유하는 상태
 *
 * - 선택된 의뢰
 * - 벤딩 아이템
 * - 벤딩 재고
 * - 구매자
 */

UCLASS()
class GOCLEAN_API ALobbyGameState : public AGameSessionState
{
	GENERATED_BODY()
	
public:

    ALobbyGameState();


    // ============
    // Contract
    // ============

    UFUNCTION(BlueprintPure, Category = "Lobby|Contract")
    int32 GetSelectedContractId() const
    {
        return SelectedContractId;
    }


    // GameMode 서버에서 호출
    void SetSelectedContractId( int32 NewContractId );


    // =========
    // Vending
    // =========

    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    const TArray<FVendingItemState>& GetVendingItems() const
    {
        return VendingItems;
    }


    // 특정 아이템 조회
    UFUNCTION(BlueprintCallable, Category = "Lobby|Vending")
    bool GetVendingItemState(int32 ItemId, FVendingItemState& OutState) const;


    // 현재 전체 구매 개수
    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    int32 GetTotalPurchasedCount() const;


    // 해당 아이템 구매 가능 여부
    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    bool CanPurchaseItem(int32 ItemId) const;


    // 서버에서 구매 반영
    bool AddVendingPurchase(int32 ItemId, int32 BuyerSeatIndex);


    // 구매 취소
    bool RemoveVendingPurchase(int32 ItemId, int32 BuyerSeatIndex);


    // 특정 플레이어가 구매한 모든 아이템 제거
    // Logout 시 사용
    int32 RemoveAllVendingPurchasesBySeat(int32 BuyerSeatIndex);


    // 초기 벤딩 아이템 설정
    void InitializeVendingItems( const TArray<FVendingItemState>& InitialItems );


    // 전체 초기화
    void ClearVendingItems();


protected:

    virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;


    // =============
    // Rep Notify
    // =============

    UFUNCTION()
    void OnRep_SelectedContractId();

    UFUNCTION()
    void OnRep_VendingItems();


    // =================
    // Blueprint Event
    // =================

    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Contract")
    void BP_OnSelectedContractChanged(int32 NewContractId);


    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Vending")
    void BP_OnVendingItemsChanged();


private:

    FVendingItemState* FindVendingItemMutable(int32 ItemId );

    const FVendingItemState* FindVendingItem(int32 ItemId ) const;


private:

    // ==============
    // Contract
    // ===============

    UPROPERTY(ReplicatedUsing = OnRep_SelectedContractId, BlueprintReadOnly, Category = "Lobby|Contract", meta = (AllowPrivateAccess = "true"))
    int32 SelectedContractId = 0;


    // ==========
    // Vending
    // ==========

    UPROPERTY( ReplicatedUsing = OnRep_VendingItems, BlueprintReadOnly, Category = "Lobby|Vending", meta = (AllowPrivateAccess = "true"))
    TArray<FVendingItemState> VendingItems;
};
