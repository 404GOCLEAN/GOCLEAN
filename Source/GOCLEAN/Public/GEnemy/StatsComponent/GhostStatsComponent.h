/**
 * @class UGhostStatsComponent
 * @brief Ghost stats 관련 component
 * 
 * **[Core functions]**
 * - InitGhostStats(): 입력된 값에 따른 기본 스탯 초기화
 * 
 * **[Getter functions]**
 * - GetMoveSpeed() const: 이동 속도 반환
 * - GetPlayerDetectionRadius() const: 플레이어 감지 범위 반환
 * - GetSoundDetectionRadius() const: 플레이어 소음 감지 범위 반환
 * - GetBehaviorFrequency() const: 행동 빈도 반환
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GhostStatsComponent.generated.h"

UCLASS( meta=(BlueprintSpawnableComponent) )
class GOCLEAN_API UGhostStatsComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class AGhostBase;

public:
	UGhostStatsComponent();



	// Init
	void InitGhostStats(float InBaseMovementSpeed,
		float InRageMovementSpeed,
		float InPlayerDetectionRange,
		float InPlayerDetectionSightAngle,
		float InRageCooldown,
		int32 InRageModifier,
		float InSoundDetectionRadius);

	// Getter
	float GetBaseMovementSpeed()		 const;
	float GetRageMovementSpeed()         const;
	float GetPlayerDetectionRange()      const;
	float GetPlayerDetectionSightAngle() const;
	float GetRageCooldown()				 const;
	int32 GetRageModifier()				 const;
	float GetSoundDetectionRadius()		 const;

private:
	UPROPERTY(EditAnywhere, Category = "Stats")
	float BaseMovementSpeed;
	UPROPERTY(EditAnywhere, Category = "Stats")
	float RageMovementSpeed;
	UPROPERTY(EditAnywhere, Category = "Stats")
	float PlayerDetectionRange;
	UPROPERTY(EditAnywhere, Category = "Stats")
	float PlayerDetectionSightAngle;
	UPROPERTY(EditAnywhere, Category = "Stats")
	float RageCooldown;
	UPROPERTY(EditAnywhere, Category = "Stats")
	int32 RageModifier;
	UPROPERTY(EditAnywhere, Category = "Stats")
	float SoundDetectionRadius;
};