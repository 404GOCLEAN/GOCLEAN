#include "GEnemy/EvidenceBehaviors/Components/RestoreWaste.h"
#include "GObjectSystem/Server/GObjectManager.h"
#include "GObjectSystem/GNonfixedObject.h"

void URestoreWaste::ExecuteBehavior(AActor* GhostActor)
{
	if (GhostActor == nullptr) return;

	UE_LOG(LogTemp, Warning, TEXT("RestoreWaste Implemented"));


	// get object manager
	auto ObjManager = GetWorld()->GetSubsystem<UGObjectManager>();
	if (!IsValid(ObjManager)) return;

	TArray<int32> WasteList = ObjManager->GetDestroyedBigWasteIndices();
	if (WasteList.IsEmpty()) return;



	// find the closest waste
	int32 ClosestWasteIndex = -1;

	float MinDistSquared = TNumericLimits<float>::Max();
	FVector GhostLocation = GhostActor->GetActorLocation();

	for (auto WasteIndex : WasteList)
	{
		auto Waste = ObjManager->GetNonfixedObject(WasteIndex);
		if (IsValid(Waste))
		{
			float CurrentDistSquared = FVector::DistSquared(GhostLocation, Waste->GetActorLocation());
			if (CurrentDistSquared < MinDistSquared)
			{
				MinDistSquared = CurrentDistSquared;
				ClosestWasteIndex = WasteIndex;
			}
		}
	}


	// restore waste
	if (ClosestWasteIndex != -1)
	{
		ObjManager->RestoreBigWasteObject(ClosestWasteIndex);
	}
}