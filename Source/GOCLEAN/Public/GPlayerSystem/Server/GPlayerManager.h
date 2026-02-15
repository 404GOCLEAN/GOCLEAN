// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Net/RpcTypes.h" 
#include "GPlayerManager.generated.h"

/**
 * 
 */


class APlayerController;
class AGOCLEANCharacter;
class UCharacterStatsComponent;

UCLASS()
class GOCLEAN_API UGPlayerManager : public UObject
{
	GENERATED_BODY()

public:
    void HandlePlayerEvent_C2S(APlayerController* PC, EPlayerEvent_C2S EventType, const FPlayerPayload_C2S& Payload);

    // UI °»½Å(Owner Only RepNotify)
    UCharacterStatsComponent* GetStatsComp(APlayerController* PC) const;
    AGOCLEANCharacter* GetCharacter(APlayerController* PC) const;

private:
    void Handle_RequestSetAnimState(APlayerController* PC, const FPlayerPayload_C2S& Payload);
    void Handle_RequestSetHeldItem(APlayerController* PC, const FPlayerPayload_C2S& Payload);
	
};
