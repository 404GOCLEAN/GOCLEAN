// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerSystem/Server/GPlayerManager.h"
#include "GameFramework/PlayerController.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "GCharacter/StatsComponent/CharacterStatsComponent.h"


void UGPlayerManager::HandlePlayerEvent_C2S(APlayerController* PC, EPlayerEvent_C2S EventType, const FPlayerPayload_C2S& Payload)
{
    if (!PC) return;

    UWorld* World = PC->GetWorld();
    if (!World || !World->GetAuthGameMode()) return;

    switch (EventType)
    {
    case EPlayerEvent_C2S::RequestSetAnimState:
        Handle_RequestSetAnimState(PC, Payload);
        break;

    case EPlayerEvent_C2S::RequestSetHeldItem:
        Handle_RequestSetHeldItem(PC, Payload);
        break;

    default:
        break;
    }
}

UCharacterStatsComponent* UGPlayerManager::GetStatsComp(APlayerController* PC) const
{
    AGOCLEANCharacter* Ch = GetCharacter(PC);
    if (!Ch) return nullptr;

    return Ch->FindComponentByClass<UCharacterStatsComponent>();
}

AGOCLEANCharacter* UGPlayerManager::GetCharacter(APlayerController* PC) const
{
	if (!PC) return nullptr;
	return Cast<AGOCLEANCharacter>(PC->GetPawn());
}

void UGPlayerManager::Handle_RequestSetAnimState(APlayerController* PC, const FPlayerPayload_C2S& Payload)
{
    AGOCLEANCharacter* Ch = GetCharacter(PC);
    if (!Ch) return;

    // 캐릭터에서 AnimState를 가지고 관리할 수 있도록.
    // Payload의 AnimState 사용
    //Ch->ServerSetAnimState(Payload.AnimState);
}

void UGPlayerManager::Handle_RequestSetHeldItem(APlayerController* PC, const FPlayerPayload_C2S& Payload)
{
    AGOCLEANCharacter* Ch = GetCharacter(PC);
    if (!Ch) return;

    // 캐릭터에서 HeldItem를 가지고 관리할 수 있도록.
    // Payload의 HeldItem 사용
    // Ch->ServerSetHeldItem(Payload.HeldItem);
}
