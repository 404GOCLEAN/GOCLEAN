// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"

#include <GObjectSystem/Server/GObjectManager.h>
#include <GPlayerSystem/Server/GPlayerManager.h>

#include "GameSessionState.generated.h"

class APlayerSessionState;

/**
 * 
 */
UCLASS()
class GOCLEAN_API AGameSessionState : public AGameState
{
	GENERATED_BODY()
	
public:
	AGameSessionState();

public:

    UGObjectManager* GetObjectManager() const { return ObjectManager; }
    UGPlayerManager* GetPlayerManager() const { return PlayerManager; }

protected:
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

public:

    // SeatIndex -> PlayerState
    UFUNCTION(BlueprintCallable, Category = "Session")
    APlayerSessionState* GetPlayerSessionStateBySeat(int32 SeatIndex) const;

    // SeatIndex -> Pawn
    UFUNCTION(BlueprintCallable, Category = "Session")
    APawn* GetPawnBySeat(int32 SeatIndex) const;

    // Pawn -> SeatIndex
    UFUNCTION(BlueprintCallable, Category = "Session")
    int32 GetSeatIndexOfPawn(const APawn* Pawn) const;

    // PlayerState -> SeatIndex
    UFUNCTION(BlueprintCallable, Category = "Session")
    int32 GetSeatIndexOfPlayerState(const APlayerState* PlayerState) const;

    // 로컬 플레이어 SeatIndex (플레이어가 자기 자신 인덱스 찾기)
    UFUNCTION(BlueprintCallable, Category = "Session")
    int32 GetLocalSeatIndex() const;

    // SeatIndex -> PlayerController
    UFUNCTION(BlueprintCallable, Category = "Session")
    APlayerController* GetPlayerControllerBySeat(int32 SeatIndex) const;

private:
    UPROPERTY()
    UGObjectManager* ObjectManager;

    UPROPERTY()
    UGPlayerManager* PlayerManager = nullptr;


    // Getters (Client/Server)
    float GetSpiritualGauge() const { return SpiritualGauge; }
    float GetRestGauge() const { return RestGauge; }
    float GetExorcismProgress() const { return ExorcismProgress; }
    float GetPostExorcismTimeRemaining() const { return PostExorcismTimeRemaining; }
    int32 GetAliveSurvivorCount() const { return AliveSurvivorCount; }
    int32 GetFinalRewardMoney() const { return FinalRewardMoney; }


public:
    // GameMode에서 호출 (Server-only mutators)
    // 게이지 증감 (클램프 포함)
    void AddSpiritualGauge(float Delta, float Min = 0.f, float Max = 100.f);
    void AddRestGauge(float Delta, float Min = 0.f, float Max = 100.f);

    // 진행도 설정/증가
    void SetExorcismProgress(float NewProgress, float Min = 0.f, float Max = 100.f);
    void AddExorcismProgress(float Delta, float Min = 0.f, float Max = 100.f);

    // 퇴마 성공 후 남은 시간
    void StartPostExorcismTimer(float DurationSeconds);   // Duration으로 시작
    void SetPostExorcismTimeRemaining(float NewRemainingSeconds); // 강제 설정
    void AddPostExorcismTimeRemaining(float DeltaSeconds); // 증감

    // 생존자 수
    void SetAliveSurvivorCount(int32 NewCount);
    void AddAliveSurvivorCount(int32 Delta);

    // 보상 (최종)
    void SetFinalRewardMoney(int32 NewMoney);


protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Setters (Server-only internal)
    void SetSpiritualGauge_Internal(float NewValue);
    void SetRestGauge_Internal(float NewValue);
    void SetExorcismProgress_Internal(float NewValue);
    void SetPostExorcismTimeRemaining_Internal(float NewValue);
    void SetAliveSurvivorCount_Internal(int32 NewValue);
    void SetFinalRewardMoney_Internal(int32 NewValue);
    


    // OnRep 
    UFUNCTION() 
    void OnRep_SpiritualGauge();

    UFUNCTION() 
    void OnRep_RestGauge();

    UFUNCTION() 
    void OnRep_ExorcismProgress();

    UFUNCTION() 
    void OnRep_PostExorcismTimeRemaining();

    UFUNCTION() 
    void OnRep_AliveSurvivorCount();

    UFUNCTION() 
    void OnRep_FinalRewardMoney();


protected:
    // 영적 게이지
    UPROPERTY(ReplicatedUsing = OnRep_SpiritualGauge)
    float SpiritualGauge = 0.f;

    // 안식 게이지
    UPROPERTY(ReplicatedUsing = OnRep_RestGauge)
    float RestGauge = 0.f;

    // 퇴마 진행도
    UPROPERTY(ReplicatedUsing = OnRep_ExorcismProgress)
    float ExorcismProgress = 0.f;

    // 퇴마 성공 후 남은 시간
    UPROPERTY(ReplicatedUsing = OnRep_PostExorcismTimeRemaining)
    float PostExorcismTimeRemaining = 0.f;

    // 생존자 수
    UPROPERTY(ReplicatedUsing = OnRep_AliveSurvivorCount)
    int32 AliveSurvivorCount = 0;

    // 추가금 (최종 보상)
    UPROPERTY(ReplicatedUsing = OnRep_FinalRewardMoney)
    int32 FinalRewardMoney = 0;
	
};
