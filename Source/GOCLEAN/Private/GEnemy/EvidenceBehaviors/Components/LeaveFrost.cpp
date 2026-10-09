#include "GEnemy/EvidenceBehaviors/Components/LeaveFrost.h"
#include "GObjectSystem/Server/GObjectManager.h"

void ULeaveFrost::ExecuteBehavior(AActor* GhostActor)
{
	if (GhostActor == nullptr || FrostObjectClass == nullptr)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("LeaveFrost Implemented"));


	// get object manager
	auto ObjManager = GetWorld()->GetSubsystem<UGObjectManager>();
	if (!IsValid(ObjManager)) return;

	// get window list
	const TArray<AActor*>& WindowList = ObjManager->GetWindowList();
	if (WindowList.IsEmpty()) return;


	// find the closest window
	AActor* ClosestWindow = nullptr;

	float MinDistSquared = TNumericLimits<float>::Max();
	FVector GhostLocation = GhostActor->GetActorLocation();

	for (auto Window : WindowList)
	{
		if (IsValid(Window))
		{
			float CurrentDistSquared = FVector::DistSquared(GhostLocation, Window->GetActorLocation());

			if (CurrentDistSquared < MinDistSquared)
			{
				MinDistSquared = CurrentDistSquared;
				ClosestWindow = Window;
			}
		}
	}


	// spawn frost
	if (ClosestWindow)
	{
		// set random location in window's bounds
		FVector Origin;
		FVector BoxExtent;
		ClosestWindow->GetActorBounds(false, Origin, BoxExtent);

		FBox WindowBox = FBox(Origin - BoxExtent, Origin + BoxExtent);

		FVector SpawnLocation = FMath::RandPointInBox(WindowBox);


		// set frost's rotation
		FRotator SpawnRotation = ClosestWindow->GetActorRotation();
		SpawnRotation += FRotator(90.0f, 90.0f, 0.0f);


		// spawn frost
		auto SpawnedActor = ObjManager->SpawnNonfixedObject(
			"Obj_Frost",
			ENonfixedObjState::E_Static,
			SpawnLocation,
			SpawnRotation
		);
	}
}
