#include "GEnemy/Components/DrownedGhost.h"
#include "AIController.h"
#include "GEnemy/EvidenceBehaviors/Components/LeaveFrost.h"
#include "GEnemy/EvidenceBehaviors/Components/Poltergeist.h"
#include "GEnemy/EvidenceBehaviors/Components/SpillWaterBucket.h"
#include "GEnemy/EvidenceBehaviors/Components/TurnOffLight.h"
#include "GEnemy/GhostAIController.h"
#include "GMapSystem/Server/GMapManager.h"
#include "Kismet/GameplayStatics.h"
#include "ServerModule/GameSession/PlayerSessionState.h"
#include "GameFramework/CharacterMovementComponent.h"

ADrownedGhost::ADrownedGhost()
{
	StatsComp->InitGhostStats(2.0f, 4.0f, 10.0f, 100.0f, 60.0f, 2.0f, 5.0f);
}

void ADrownedGhost::BeginPlay()
{
	Super::BeginPlay();

	EvidenceBehaviors.Add(NewObject<ULeaveFrost>(this));
	EvidenceBehaviors.Add(NewObject<UPoltergeist>(this));
	EvidenceBehaviors.Add(NewObject<USpillWaterBucket>(this));
	EvidenceBehaviors.Add(NewObject<UTurnOffLight>(this));

	if (!HasAuthority()) return;
	GetWorldTimerManager().SetTimer(CheckWaterRoomHandle, this, &ADrownedGhost::CheckPlayersInWaterRoom, 1.0f, true);
}

void ADrownedGhost::OnPlayerSanityHalfReached(APlayerSessionState* InPlayerState)
{
	if (InPlayerState == nullptr) return;

	AGOCLEANCharacter* PlayerCharacter = Cast<AGOCLEANCharacter>(InPlayerState->GetPawn());
	if (PlayerCharacter == nullptr) return;
	
	SanityHalfReadyPlayers.Add(InPlayerState);

	if (IsPlayerInWaterRoom(PlayerCharacter))
	{
		WaterRoomPlayersLastCheck.Add(InPlayerState);
	}
	else
	{
		WaterRoomPlayersLastCheck.Remove(InPlayerState);
	}
}

void ADrownedGhost::CheckPlayersInWaterRoom()
{
	AGhostAIController* AIController = Cast<AGhostAIController>(GetController());
	if (AIController == nullptr) return;

	AIController->UpdateAlivePlayerList();

	for (AGOCLEANCharacter* PlayerCharacter : AIController->AlivePlayers)
	{
		if (PlayerCharacter == nullptr) continue;

		APlayerSessionState* PSS = PlayerCharacter->GetPlayerState<APlayerSessionState>();
		if (PSS == nullptr) continue;

		const bool bIsInWaterRoom = IsPlayerInWaterRoom(PlayerCharacter);

		const bool bWasInWaterRoom = WaterRoomPlayersLastCheck.Contains(PSS);

		PlayerCharacter->SetSanityDrainMultiplier(bIsInWaterRoom ? 1.2f : 1.0f);

		if (SanityHalfReadyPlayers.Contains(PSS) && bIsInWaterRoom && !bWasInWaterRoom)
		{
			SanityHalfReadyPlayers.Remove(PSS);

			const float CurrentSanity = PlayerCharacter->GetPlayerCurrentSanity();

			const float NewSanity = CurrentSanity * 0.95f;

			PlayerCharacter->SetPlayerCurrentSanity(NewSanity);

			const float CachedDefaultSpeed = PlayerCharacter->GetDefaultSpeed();

			PlayerCharacter->Multicast_SetDefaultSpeed(CachedDefaultSpeed * 0.95f);

			FTimerHandle RestoreSpeedHandle;
			TWeakObjectPtr<AGOCLEANCharacter> WeakPlayer = PlayerCharacter;

			GetWorldTimerManager().SetTimer(RestoreSpeedHandle, FTimerDelegate::CreateLambda(
				[WeakPlayer, CachedDefaultSpeed]()
				{
					if (!WeakPlayer.IsValid()) return;

					WeakPlayer->Multicast_SetDefaultSpeed(CachedDefaultSpeed);
				}), 3.0f, false);
		}

		if (bIsInWaterRoom)
		{
			WaterRoomPlayersLastCheck.Add(PSS);
		}
		else
		{
			WaterRoomPlayersLastCheck.Remove(PSS);
		}
	}
}

bool ADrownedGhost::IsPlayerInWaterRoom(const AGOCLEANCharacter* PlayerCharacter) const
{
	if (PlayerCharacter == nullptr) return false;

	UGMapManager* MapManager = GetWorld()->GetSubsystem<UGMapManager>();
	if (MapManager == nullptr) return false;

	const TArray<FName> ZoneIds = MapManager->GetActorZoneIds(PlayerCharacter);

	for (const FName& ZoneId : ZoneIds)
	{
		const FGZoneData* ZoneData = MapManager->GetZoneData(ZoneId);
		if (ZoneData == nullptr) continue;

		if (ZoneData->Attribute == EGZoneAttribute::E_Water)
			return true;
	}

	return false;
}
