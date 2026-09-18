// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ServerModule/GameSession/GameSessionState.h"
#include "LobbyGameState.generated.h"


// 벤딩 아이템 상태
USTRUCT(BlueprintType)
struct FVendingPurchaseState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Vending")
    int32 ItemId = INDEX_NONE;

    // 0~3
    // 0 = 1P, 1 = 2P ...
    UPROPERTY(BlueprintReadOnly, Category = "Vending")
    int32 BuyerSeatIndex = INDEX_NONE;
};

// 벤딩 구매 정보가 변경되었을 때
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVendingItemsChanged);

/**
 * 로비에서 모든 플레이어가 공유하는 상태
 *
 * - 선택된 의뢰
 * - 벤딩 아이템
 * - 벤딩 재고
 * - 구매자
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLobbyPlayersChanged);
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


    // ============================================================
    // Vending - Blueprint Getter
    // ============================================================

    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    const TArray<FVendingPurchaseState>& GetPurchasedVendingItems() const
    {
        return PurchasedVendingItems;
    }


    // 현재 구매된 종류 수 (최대 5)
    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    int32 GetPurchasedVendingCount() const
    {
        return PurchasedVendingItems.Num();
    }


    // 해당 아이템이 이미 구매되었는지
    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    bool IsVendingItemPurchased(int32 ItemId) const;


    // 해당 아이템을 구매한 SeatIndex
    // 구매되지 않았다면 INDEX_NONE
    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    int32 GetVendingBuyerSeatIndex(int32 ItemId) const;


    // 특정 Seat가 구매한 아이템인지
    UFUNCTION(BlueprintPure, Category = "Lobby|Vending")
    bool DidSeatPurchaseItem(
        int32 BuyerSeatIndex,
        int32 ItemId
    ) const;


    // ============================================================
    // Server Only
    // ============================================================

    bool AddVendingPurchase(
        int32 ItemId,
        int32 BuyerSeatIndex
    );


    bool RemoveVendingPurchase(
        int32 ItemId,
        int32 BuyerSeatIndex
    );


    // Logout 시 사용
    int32 RemoveAllVendingPurchasesBySeat(
        int32 BuyerSeatIndex
    );


    void ClearVendingPurchases();


    // ============================================================
    // Vending Event
    // ============================================================

    // PurchasedVendingItems가 변경되면 호출.
    // Lobby Widget에서 Bind해서 UI를 갱신한다.
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Vending")
    FOnVendingItemsChanged OnVendingItemsChanged;


    // ============================================================
    // Lobby Player
    // ============================================================

    UPROPERTY(BlueprintAssignable, Category = "Lobby|Player")
    FOnLobbyPlayersChanged OnLobbyPlayersChanged;

    // GameMode에서 플레이어 입장/퇴장 처리가 모두 끝난 뒤 호출
    void NotifyLobbyPlayersChanged();

    // 현재 로비에 있는 모든 플레이어가 Ready인지 검사
    UFUNCTION(BlueprintPure, Category = "Lobby|Player")
    bool AreAllPlayersReady() const;

    // ============================================================
    // Lobby Time
    // ============================================================

    UFUNCTION(BlueprintPure, Category = "Lobby|Time")
    float GetLobbyElapsedSeconds() const;

    UFUNCTION(BlueprintPure, Category = "Lobby|Time")
    FString GetLobbyElapsedTimeText() const;

protected:

    virtual void BeginPlay() override;

    virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;


    // =============
    // Rep Notify
    // =============

    UFUNCTION()
    void OnRep_SelectedContractId();

    UFUNCTION()
    void OnRep_PurchasedVendingItems();


    UFUNCTION()
    void OnRep_LobbyRosterRevision();


    // =================
    // Blueprint Event
    // =================

    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Contract")
    void BP_OnSelectedContractChanged(int32 NewContractId);


    UFUNCTION(BlueprintImplementableEvent, Category = "Lobby|Vending")
    void BP_OnVendingItemsChanged();


private:

    // ==============
    // Contract
    // ===============

    UPROPERTY(ReplicatedUsing = OnRep_SelectedContractId, BlueprintReadOnly, Category = "Lobby|Contract", meta = (AllowPrivateAccess = "true"))
    int32 SelectedContractId = 0;


    // ==========
    // Vending
    // ==========

    UPROPERTY(ReplicatedUsing = OnRep_PurchasedVendingItems, BlueprintReadOnly, Category = "Lobby|Vending",
        meta = (AllowPrivateAccess = "true"))
    TArray<FVendingPurchaseState> PurchasedVendingItems;



    // 플레이어 목록 변경을 클라이언트에게 알리기 위한 Revision
    UPROPERTY(ReplicatedUsing = OnRep_LobbyRosterRevision)
    int32 LobbyRosterRevision = 0;


    // 서버 기준 로비 시작 시간
    UPROPERTY(Replicated)
    float LobbyStartServerTime = -1.f;
};
