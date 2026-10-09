#include "GEnemy/CommonBehaviors/Components/FlashlightBreakdown.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "GEnemy/Base/GhostBase.h"
#include "GEnemy/GhostAIController.h"

void UFlashlightBreakdown::ExecuteBehavior(AActor* GhostActor)
{
	if (GhostActor == nullptr) return;

	AGhostBase* GhostCharacter = Cast<AGhostBase>(GhostActor);
	if (GhostCharacter == nullptr) return;

	if (!GhostCharacter->HasAuthority()) return;

	AGhostAIController* AIController = Cast<AGhostAIController>(GhostCharacter->GetController());
	if (AIController == nullptr) return;

	AIController->UpdateAlivePlayerList();

	if (AIController->AlivePlayers.IsEmpty()) return;

	TargetPlayers.Empty();

	const float EffectRadius = 1000.0f;
	const float EffectRadiusSquared = EffectRadius * EffectRadius;

	for(AGOCLEANCharacter * PlayerCharacter : AIController->AlivePlayers)
	{
		if (PlayerCharacter == nullptr) continue;

		const float DistanceSquared = FVector::DistSquared(GhostCharacter->GetActorLocation(), PlayerCharacter->GetActorLocation());

		if (DistanceSquared > EffectRadiusSquared) continue;

		TargetPlayers.Add(PlayerCharacter);
	}

	if (TargetPlayers.IsEmpty()) return;

	ToggleCount = 0;

	for (AGOCLEANCharacter* TargetPlayer : TargetPlayers)
	{
		if (TargetPlayer == nullptr) continue;

		TargetPlayer->SetCanToggleFlashlight(false);
	}

	GetWorld()->GetTimerManager().SetTimer(ToggleFlashlightHandle, this, &UFlashlightBreakdown::ToggleTargetFlashlights, 0.5f, true);
	GetWorld()->GetTimerManager().SetTimer(UnlockFlashlightHandle, this, &UFlashlightBreakdown::UnlockTargetFlashlights, 5.5f, false);

	UE_LOG(LogTemp, Warning, TEXT("FlashlightBreakdown Implemented"));
}

void UFlashlightBreakdown::ToggleTargetFlashlights()
{
	for (AGOCLEANCharacter* PlayerCharacter : TargetPlayers)
	{
		if (PlayerCharacter == nullptr) continue;

		PlayerCharacter->Multicast_ToggleFlashlight();
	}

	++ToggleCount;

	if (ToggleCount >= 5)
	{
		GetWorld()->GetTimerManager().ClearTimer(ToggleFlashlightHandle);
	}
}

void UFlashlightBreakdown::UnlockTargetFlashlights()
{
	for (AGOCLEANCharacter* PlayerCharacter : TargetPlayers)
	{
		if (PlayerCharacter == nullptr) continue;

		PlayerCharacter->SetCanToggleFlashlight(true);
	}

	TargetPlayers.Empty();
	ToggleCount = 0;
}
