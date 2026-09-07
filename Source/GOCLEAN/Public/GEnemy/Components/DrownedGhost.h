/**
 * @class ADrownedGhost
 * @brief ¼ö»ì±Í
 */

#pragma once

#include "CoreMinimal.h"
#include "GEnemy/Base/GhostBase.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "DrownedGhost.generated.h"

class APlayerSessionState;

UCLASS()
class GOCLEAN_API ADrownedGhost : public AGhostBase
{
	GENERATED_BODY()


public:
	ADrownedGhost();

	virtual void BeginPlay() override;

protected:
	void OnPlayerSanityHalfReached(APlayerSessionState* InPlayerState) override;

private:
	UPROPERTY()
	TSet<TObjectPtr<APlayerSessionState>> SanityHalfReadyPlayers;

	UPROPERTY()
	TSet<TObjectPtr<APlayerSessionState>> WaterRoomPlayersLastCheck;

	void CheckPlayersInWaterRoom();
	bool IsPlayerInWaterRoom(const AGOCLEANCharacter* PlayerCharacter) const;

	FTimerHandle CheckWaterRoomHandle;
};