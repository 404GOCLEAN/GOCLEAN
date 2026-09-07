#include "GEnemy/EvidenceBehaviors/Components/LeaveBloodFootstep.h"
#include "GObjectSystem/Server/GObjectManager.h"

void ULeaveBloodFootstep::ExecuteBehavior(AActor* GhostActor)
{
	if (GhostActor == nullptr || GetWorld()->GetTimerManager().IsTimerActive(SpawnTimerHandle))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("LeaveBloodFootstep Implemented"));


	// set start parameters
	TargetGhost = GhostActor;
	FootprintID = "Obj_FootprintL";
	SpawnRemainingCount = 3;


	// start timer
	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimerHandle,
		this,
		&ULeaveBloodFootstep::SpawnFootprint,
		1.0f,
		true
	);
}

void ULeaveBloodFootstep::SpawnFootprint()
{
	auto ObjManager = GetWorld()->GetSubsystem<UGObjectManager>();
	FVector SpawnLocation = TargetGhost->GetActorLocation();
	FRotator SpawnRotation = TargetGhost->GetActorRotation();

	auto SpawnedActor = ObjManager->SpawnNonfixedObject(
		"Obj_DerivedBlood",
		ENonfixedObjState::E_Static,
		SpawnLocation,
		SpawnRotation
	);


	FootprintID = (FootprintID == "Obj_FootprintL") ? "Obj_FootprintR" : "Obj_FootprintL";
	SpawnRemainingCount--;


	if (SpawnRemainingCount <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}
}