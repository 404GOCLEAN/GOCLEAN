// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ServerModule/GameSession/GameSessionState.h"
#include "InGameGameState.generated.h"

class UGObjectManager;
class UGPlayerManager;
class UGMapManager;
class UDataTable;

// 게임 전체 Phase
UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    None,
    Cleaning,
    Exorcism,
    Extraction,
    End
};


// 귀신 상태
UENUM(BlueprintType)
enum class EGhostState : uint8
{
    Normal,
    // 플레이어를 사냥하는 격노 상태
    Hunting,
    // 게임을 종료시키지 않는 격노 이벤트
    RageEvent
};


// 퇴마 상태
UENUM(BlueprintType)
enum class EExorcismState : uint8
{
    None,
    Waiting,
    InProgress,
    Success,
    Failed
};

USTRUCT(BlueprintType)
struct FInGameVendingStock
{
    GENERATED_BODY()

    // DT_VendingItem의 VendingItemId
    UPROPERTY(BlueprintReadOnly, Category = "InGame|Vending")
    int32 ItemId = INDEX_NONE;

    // -1 = 무제한
    //  0 = 품절
    //  1 이상 = 남은 수량
    UPROPERTY(BlueprintReadOnly, Category = "InGame|Vending")
    int32 RemainingCount = 0;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInGameVendingStockChanged);


/**
 * 인게임의 공유 게임 상태
 *
 * 값을 저장하고 Replication하는 역할만 담당
 *
 * 게임 규칙 및 조건 판단은 InGameGameMode에서 처리
 */

UCLASS()
class GOCLEAN_API AInGameGameState : public AGameSessionState
{
	GENERATED_BODY()
	
public:

    AInGameGameState();

    UGObjectManager* GetObjectManager() const
    {
        return ObjectManager;
    }

    UGPlayerManager* GetPlayerManager() const
    {
        return PlayerManager;
    }

    UGMapManager* GetMapManager() const
    {
        return MapManager;
    }


    // =============
    // Contract
    // =============

    UFUNCTION(BlueprintPure, Category = "InGame|Contract")
    int32 GetSelectedContractId() const
    {
        return SelectedContractId;
    }

    void SetSelectedContractId(int32 NewContractId);


    // ==============
    // Phase
    // ============

    UFUNCTION(BlueprintPure, Category = "InGame|Phase")
    EGamePhase GetGamePhase() const
    {
        return GamePhase;
    }

    void SetGamePhase(EGamePhase NewPhase);


    UFUNCTION(BlueprintPure, Category = "InGame|Ghost")
    EGhostState GetGhostState() const
    {
        return GhostState;
    }

    void SetGhostState(EGhostState NewState);


    UFUNCTION(BlueprintPure, Category = "InGame|Exorcism")
    EExorcismState GetExorcismState() const
    {
        return ExorcismState;
    }

    void SetExorcismState(EExorcismState NewState);


    // ============
    // Gauge
    // ===========

    UFUNCTION(BlueprintPure, Category = "InGame|Gauge")
    float GetSpiritualGauge() const
    {
        return SpiritualGauge;
    }

    UFUNCTION(BlueprintPure, Category = "InGame|Gauge")
    float GetRestGauge() const
    {
        return RestGauge;
    }


    void SetSpiritualGauge(float NewValue);

    void AddSpiritualGauge(float Delta);

    void SetRestGauge(float NewValue);

    void AddRestGauge(float Delta);


    // ==============
    // Exorcism
    // ==============

    UFUNCTION(BlueprintPure, Category = "InGame|Exorcism")
    float GetExorcismProgress() const
    {
        return ExorcismProgress;
    }

    void SetExorcismProgress(float NewProgress);

    void AddExorcismProgress(float Delta);


    // =================
    // Extraction
    // =================

    UFUNCTION(BlueprintPure, Category = "InGame|Extraction")
    float GetExtractionTimeRemaining() const
    {
        return ExtractionTimeRemaining;
    }

    void SetExtractionTimeRemaining(float NewTime);

    void AddExtractionTimeRemaining(float Delta);


    // =================
    // Player Count
    // ================

    UFUNCTION(BlueprintPure, Category = "InGame|Player")
    int32 GetAliveSurvivorCount() const
    {
        return AliveSurvivorCount;
    }

    void SetAliveSurvivorCount(int32 NewCount);


    // =============
    // Reward
    // =============

    UFUNCTION(BlueprintPure, Category = "InGame|Reward")
    int32 GetFinalRewardMoney() const
    {
        return FinalRewardMoney;
    }

    void SetFinalRewardMoney( int32 NewMoney );


    // =================
    // Vending
    // =================

/**
 * 로비에서 선택된 아이템 ID와 DT_VendingItem을 이용하여
 * 인게임 공용 재고를 초기화한다.
 *
 * 서버에서만 호출.
 */
    void InitializeVendingStock(const TArray<int32>& SelectedItemIds, UDataTable* VendingDataTable);


    // 로비에서 선택되어 현재 밴딩머신에서 사용할 수 있는 종류인가?
    UFUNCTION(BlueprintPure, Category = "InGame|Vending")
    bool IsVendingItemAvailable(int32 ItemId) const;


    // 현재 남은 재고.
    // -1 = 무제한
    //  0 = 품절 또는 선택되지 않은 아이템
    UFUNCTION(BlueprintPure, Category = "InGame|Vending")
    int32 GetVendingRemainingCount(int32 ItemId) const;


    // 현재 실제로 꺼낼 수 있는가?
    UFUNCTION(BlueprintPure, Category = "InGame|Vending")
    bool CanTakeVendingItem(int32 ItemId) const;


    // 실제 재고 하나 사용.
    // 서버에서 호출해야 함.
    bool TryTakeVendingItem(int32 ItemId);


    // 현재 선택된 전체 아이템/재고
    UFUNCTION(BlueprintPure, Category = "InGame|Vending")
    const TArray<FInGameVendingStock>& GetVendingStocks() const
    {
        return VendingStocks;
    }


    // 재고 변경 시 Widget 등이 받을 이벤트
    UPROPERTY(BlueprintAssignable, Category = "InGame|Vending")
    FOnInGameVendingStockChanged OnInGameVendingStockChanged;

protected:

    virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

    virtual void BeginPlay() override;


    // ==============
    // OnRep
    // ==============

    UFUNCTION()
    void OnRep_SelectedContractId();

    UFUNCTION()
    void OnRep_GamePhase();

    UFUNCTION()
    void OnRep_GhostState();

    UFUNCTION()
    void OnRep_ExorcismState();

    UFUNCTION()
    void OnRep_SpiritualGauge();

    UFUNCTION()
    void OnRep_RestGauge();

    UFUNCTION()
    void OnRep_ExorcismProgress();

    UFUNCTION()
    void OnRep_ExtractionTimeRemaining();

    UFUNCTION()
    void OnRep_AliveSurvivorCount();

    UFUNCTION()
    void OnRep_FinalRewardMoney();

    UFUNCTION()
    void OnRep_VendingStocks();


    // ============
    // BP Event
    // ===========

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnGamePhaseChanged(EGamePhase NewPhase);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnGhostStateChanged(EGhostState NewState);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnExorcismStateChanged(EExorcismState NewState);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnSpiritualGaugeChanged(float NewValue);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnRestGaugeChanged(float NewValue);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnExorcismProgressChanged(float NewValue);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnExtractionTimeChanged(float NewValue);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnAliveSurvivorCountChanged(int32 NewCount);

    UFUNCTION(BlueprintImplementableEvent)
    void BP_OnFinalRewardChanged(int32 NewMoney);


private:

    UPROPERTY()
    UGObjectManager* ObjectManager = nullptr;

    UPROPERTY()
    UGPlayerManager* PlayerManager = nullptr;

    UPROPERTY()
    UGMapManager* MapManager = nullptr;



    UPROPERTY(ReplicatedUsing = OnRep_SelectedContractId)
    int32 SelectedContractId = 0;


    UPROPERTY(ReplicatedUsing = OnRep_GamePhase)
    EGamePhase GamePhase = EGamePhase::None;


    UPROPERTY(ReplicatedUsing = OnRep_GhostState)
    EGhostState GhostState = EGhostState::Normal;


    UPROPERTY(ReplicatedUsing = OnRep_ExorcismState)
    EExorcismState ExorcismState = EExorcismState::None;


    UPROPERTY(ReplicatedUsing = OnRep_SpiritualGauge)
    float SpiritualGauge = 100.f;


    UPROPERTY(ReplicatedUsing = OnRep_RestGauge)
    float RestGauge = 0.f;


    UPROPERTY(ReplicatedUsing = OnRep_ExorcismProgress)
    float ExorcismProgress = 0.f;


    UPROPERTY(ReplicatedUsing = OnRep_ExtractionTimeRemaining)
    float ExtractionTimeRemaining = 0.f;


    UPROPERTY(ReplicatedUsing = OnRep_AliveSurvivorCount)
    int32 AliveSurvivorCount = 0;


    UPROPERTY(ReplicatedUsing = OnRep_FinalRewardMoney)
    int32 FinalRewardMoney = 0;


    // =================
    // Vending
    // =================

    UPROPERTY(ReplicatedUsing = OnRep_VendingStocks, BlueprintReadOnly, Category = "InGame|Vending", meta = (AllowPrivateAccess = "true"))
    TArray<FInGameVendingStock> VendingStocks;
};
