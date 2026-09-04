/**
 * @class UFlashlightBreakdown
 * @brief 공통 행동
 *
 * **[Core functions]**
 * - ExecuteBehavior(AActor* GhostActor): 행동 실행
 */

#pragma once

#include "CoreMinimal.h"
#include "GEnemy/CommonBehaviors/Base/CommonBehavior.h"
#include "FlashlightBreakdown.generated.h"

class AGOCLEANCharacter;
class AGhostBase;

UCLASS()
class GOCLEAN_API UFlashlightBreakdown : public UCommonBehavior
{
	GENERATED_BODY()
	
public:
	virtual void ExecuteBehavior(AActor* GhostActor) override;

private:
	UPROPERTY()
	TArray<TObjectPtr<AGOCLEANCharacter>> TargetPlayers;

	void ToggleTargetFlashlights();
	void UnlockTargetFlashlights();

	FTimerHandle ToggleFlashlightHandle;
	FTimerHandle UnlockFlashlightHandle;

	int32 ToggleCount = 0;
};
