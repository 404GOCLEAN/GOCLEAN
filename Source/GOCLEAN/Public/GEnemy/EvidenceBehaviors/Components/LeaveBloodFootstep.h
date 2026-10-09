/**
 * @class ULeaveBloodFootstep
 * @brief 단서 행동: 핏발자국
 *
 * **[Core functions]**
 * - ExecuteBehavior(AActor* GhostActor): 행동 실행
 */

#pragma once

#include "CoreMinimal.h"
#include "GEnemy/EvidenceBehaviors/Base/UEvidenceBehavior.h"
#include "LeaveBloodFootstep.generated.h"

UCLASS()
class GOCLEAN_API ULeaveBloodFootstep : public UEvidenceBehavior
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	void SpawnFootprint();
	

public:
	virtual void ExecuteBehavior(AActor* GhostActor) override;


private:
	// Timer handler
	UPROPERTY()
	FTimerHandle SpawnTimerHandle;

	// Target ghost
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AActor> TargetGhost;

	// Spawn parameters
	FName FootprintID;
	int32 SpawnRemainingCount = 0;
};