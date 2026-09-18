/**
 * @class AMaidenGhost
 * @brief ¼Õ°¢½Ã
 */

#pragma once

#include "CoreMinimal.h"
#include "GEnemy/Base/GhostBase.h"
#include "MaidenGhost.generated.h"

class APlayerSessionState;

UCLASS()
class GOCLEAN_API AMaidenGhost : public AGhostBase
{
	GENERATED_BODY()

public:
	AMaidenGhost();
	
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<APlayerSessionState> ObsessionTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Specific event")
	TSubclassOf<AActor> HallucinationActorClass;

	FTimerHandle CheckApplyFrostEventConditionHandle;

	void InitializeObsessionTarget();
	void CheckApplyFrostEventCondition();
	void ApplyFrostEvent();

	// Specific event //
	void OnUnendingRageStarted() override;
	void OnPlayerSanityHalfReached(APlayerSessionState* InPlayerState) override;
};
