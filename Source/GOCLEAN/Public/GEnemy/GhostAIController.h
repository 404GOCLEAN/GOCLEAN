/**
 * @class AGhostAIController
 * @brief Ghost actor의 상태 제어 및, 플레이어 정신력에 따른 행동 변화를 관리하는 AI Controller
 * 
 * [Core functions]
 * - CheckPlayerSanityCorruptionRate(): 플레이어들의 정신력을 기반으로, 정신 오염도 계산
 * - MoveToPatrolPoint(): Patrol point로의 이동
 * - CheckArrivalCurrentPatrolPoint(): 현재 대상 Patrol point에 도착했는지를 체크
 * - CheckEnrageEventCondition(): 격노 이벤트 조건 확인
 * - StartChase(): 격노 상태로의 전환
 * - ChasePlayer(): 격노 이벤트 로직 수행
 * - PlayerHunt(): 플레이어 Hunt 처리
 */

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GEnemy/Base/GhostBase.h"

#include "ServerModule/GameSession/GameSessionState.h"
#include "ServerModule/GameSession/PlayerSessionState.h"

#include "GhostAIController.generated.h"

class AGOCLEANCharacter;

UCLASS()
class GOCLEAN_API AGhostAIController : public AAIController
{
	GENERATED_BODY()
	
public:

	// Getter //
	//float GetPlayerSanityCorruptionRate() const;

	UFUNCTION(BlueprintPure)
	bool IsPatrolling() const { return bIsPatrolling; }

	UFUNCTION(BlueprintPure)
	bool IsChasing() const { return bIsChasing; }

	// Server //
	UPROPERTY()
	TArray<TObjectPtr<AGOCLEANCharacter>> AlivePlayers;

	UPROPERTY()
	TObjectPtr<AGOCLEANCharacter> TargetPlayer;

	void UpdateAlivePlayerList();
	float CalculateAverageSanityCorruptionRate();
	bool CheckBehaviorEventCondition();


	// Server //
	UFUNCTION(Server, Reliable)
	void Server_RequestPlayerHunt();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayerHunt();

	UFUNCTION(Server, Reliable)
	void Server_RequestSetVisible(bool IsVisible);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetVisible(bool IsVisible);

private:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	// Overrieded //
	void BeginPlay() override; // JSH TMP
	void Tick(float DeltaTime) override;
	void OnPossess(APawn* InPawn) override;


	// Behavior event //
	//void CheckPlayerSanityCorruptionRate();
	void CalculateAveragePlayerSanity();
	void InitActivityLevel();


	// State //

	// Patrol
	void MoveToPatrolPoint();
	void CheckArrivalCurrentPatrolPoint();

	// Chase
	void CheckRageEventCondition();
	void StartUnendingRageEvent();
	void StartChase();
	void ChasePlayer(AActor* TargetPlayerCharacter);

	// Rage
	void EnableRageTrigger() { bCanRageEvent = true; }
	void EndRageEvent();

	// Hunt
	void PlayerHunt();
	void EndlessPlayerHunt();

	void FindTarget();

public:
	// Patrol
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsPatrolling;
	FTimerHandle CheckArrivalToCurrentPointHandle;

	// Chase
	UPROPERTY(ReplicatedUsing=OnRep_IsChasing, BlueprintReadOnly)
	bool bIsChasing;
	UPROPERTY(Replicated=OnRep_IsRageEvent, BlueprintReadOnly)
	bool bIsRageEvent;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsUnendingRageEvent;
	FTimerHandle ChasingPlayerHandle;

	UFUNCTION()
	void OnRep_IsRageEvent();

	UFUNCTION()
	void OnRep_IsChasing();

private:
	// Check player sanity //
	//float PlayersSanityCorruptionRate;
	//FTimerHandle CheckPlayerSanityCorruptionHandle;
	float PlayerSaninty;
	//FTimerHandle CheckPlayerSanityHandle;

	// Rage
	bool bCanRageEvent;
	FTimerHandle CheckRageEventConditionHandle;
	FTimerHandle EnableRageEventTriggerHandle;

	// Hunt
	float ManifestRadius;
	float HuntRadius;

	// Behavior event
	int32 AveragePlayerSanity;
	int32 ActivityLevel;
	int32 MaxEventValueRange;
	int32 ActivityLevelModifier;
};