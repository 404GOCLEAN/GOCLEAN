/**
 * @class ULeaveFrost
 * @brief 단서 행동: 성에
 *
 * **[Core functions]**
 * - ExecuteBehavior(AActor* GhostActor): 행동 실행
 */

#pragma once

#include "CoreMinimal.h"
#include "GEnemy/EvidenceBehaviors/Base/UEvidenceBehavior.h"
#include "LeaveFrost.generated.h"

UCLASS()
class GOCLEAN_API ULeaveFrost : public UEvidenceBehavior
{
	GENERATED_BODY()

public:
	virtual void ExecuteBehavior(AActor* GhostActor) override;


private:
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<AActor> FrostObjectClass;
};
