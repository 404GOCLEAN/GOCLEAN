#include "GEnemy/StatsComponent/GhostStatsComponent.h"

constexpr float MeterToCentimeter = 100.0f;

UGhostStatsComponent::UGhostStatsComponent()
{
	;
}

void UGhostStatsComponent::InitGhostStats(
	float InBaseMovementSpeed,
	float InRageMovementSpeed,
	float InPlayerDetectionRange,
	float InPlayerDetectionSightAngle,
	float InRageCooldown,
	int32 InRageModifier,
	float InSoundDetectionRadius)
{
	BaseMovementSpeed = InBaseMovementSpeed * MeterToCentimeter;
	RageMovementSpeed = InRageMovementSpeed * MeterToCentimeter;
	PlayerDetectionRange = InPlayerDetectionRange * MeterToCentimeter;
	PlayerDetectionSightAngle = InPlayerDetectionSightAngle;
	RageCooldown = InRageCooldown;
	RageModifier = InRageModifier;
	SoundDetectionRadius = InSoundDetectionRadius * MeterToCentimeter;
}

// Getter, Setter //
float UGhostStatsComponent::GetBaseMovementSpeed()		   const { return BaseMovementSpeed; }
float UGhostStatsComponent::GetRageMovementSpeed()		   const { return RageMovementSpeed; }
float UGhostStatsComponent::GetPlayerDetectionRange()      const { return PlayerDetectionRange; }
float UGhostStatsComponent::GetPlayerDetectionSightAngle() const { return PlayerDetectionSightAngle; }
float UGhostStatsComponent::GetRageCooldown()		       const { return RageCooldown; }
int32 UGhostStatsComponent::GetRageModifier()			   const { return RageModifier; }
float UGhostStatsComponent::GetSoundDetectionRadius()      const { return SoundDetectionRadius; }