/**
 * @class UPlayCommonSound
 * @brief 공통 행동
 *
 * **[Core functions]**
 * - ExecuteBehavior(AActor* GhostActor): 행동 실행
 */

#pragma once

#include "CoreMinimal.h"
#include "GEnemy/CommonBehaviors/Base/CommonBehavior.h"
#include "GEnemy/Base/GhostBase.h"
#include "PlayCommonSound.generated.h"

UCLASS()
class GOCLEAN_API UPlayCommonSound : public UCommonBehavior
{
	GENERATED_BODY()
	
public:
	virtual void ExecuteBehavior(AActor* GhostActor) override;
};
