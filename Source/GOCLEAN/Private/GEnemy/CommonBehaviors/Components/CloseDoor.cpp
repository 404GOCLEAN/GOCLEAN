#include "GEnemy/CommonBehaviors/Components/CloseDoor.h"
#include "GEnemy/Base/GhostBase.h"
#include "GEnemy/GhostAIController.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "GMapSystem/GDoorway.h"
#include "EngineUtils.h"

void UCloseDoor::ExecuteBehavior(AActor* GhostActor)
{
	if (GhostActor == nullptr) return;

	AGhostBase* GhostCharacter = Cast<AGhostBase>(GhostActor);
	if (GhostCharacter == nullptr) return;

	if (!GhostCharacter->HasAuthority()) return;

	AGhostAIController* AIController = Cast<AGhostAIController>(GhostCharacter->GetController());
	if (AIController == nullptr) return;
	
	AIController->UpdateAlivePlayerList();

	if (AIController->AlivePlayers.IsEmpty()) return;

	AGOCLEANCharacter* ClosestPlayerCharacter = nullptr;
	float ClosestPlayerDistanceSquared = TNumericLimits<float>::Max();

	for (AGOCLEANCharacter* PlayerCharacter : AIController->AlivePlayers)
	{
		if (PlayerCharacter == nullptr) continue;

		const float DistanceSquared = FVector::DistSquared(GhostCharacter->GetActorLocation(), PlayerCharacter->GetActorLocation());

		if (DistanceSquared < ClosestPlayerDistanceSquared)
		{
			ClosestPlayerDistanceSquared = DistanceSquared;
			ClosestPlayerCharacter = PlayerCharacter;
		}
	}

	if (ClosestPlayerCharacter == nullptr) return;

	AGDoorway* ClosestDoor = nullptr;
	float ClosestDoorDistanceSquared = TNumericLimits<float>::Max();
	for (AGDoorway* Door : TActorRange<AGDoorway>(GhostCharacter->GetWorld()))
	{
		if (Door->IsClosed() || !Door->CanClose()) continue;

		const float DistanceSquared = FVector::DistSquared(ClosestPlayerCharacter->GetActorLocation(), Door->GetActorLocation());

		if (DistanceSquared < ClosestDoorDistanceSquared)
		{
			ClosestDoorDistanceSquared = DistanceSquared;
			ClosestDoor = Door;
		}
	}

	if (ClosestDoor == nullptr) return;

	ClosestDoor->CloseDoor();

	UE_LOG(LogTemp, Warning, TEXT("CloseDoor Implemented"));
}