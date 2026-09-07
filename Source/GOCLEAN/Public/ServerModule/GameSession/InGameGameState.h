// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ServerModule/GameSession/GameSessionState.h"
#include "InGameGameState.generated.h"

class UGObjectManager;
class UGPlayerManager;
class UGMapManager;

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
};
