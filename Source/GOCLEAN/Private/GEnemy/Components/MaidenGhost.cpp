#include "GEnemy/Components/MaidenGhost.h"
#include "GEnemy/EvidenceBehaviors/Components/LeaveBloodFootstep.h"
#include "GEnemy/EvidenceBehaviors/Components/LeaveFrost.h"
#include "GEnemy/EvidenceBehaviors/Components/SpillWaterBucket.h"
#include "GEnemy/EvidenceBehaviors/Components/RestoreWaste.h"
#include "GEnemy/GhostAIController.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "ServerModule/GameSession/PlayerSessionState.h"
#include "GObjectSystem/Server/GObjectManager.h"

AMaidenGhost::AMaidenGhost()
{
	StatsComp->InitGhostStats(3.0f, 4.5f, 10.0f, 110.0f, 60.0f, 0.0f, 7.0f);
}

void AMaidenGhost::BeginPlay()
{
	Super::BeginPlay();

	EvidenceBehaviors.Add(NewObject<ULeaveFrost>(this));
	EvidenceBehaviors.Add(NewObject<ULeaveBloodFootstep>(this));
	EvidenceBehaviors.Add(NewObject<USpillWaterBucket>(this));
	EvidenceBehaviors.Add(NewObject<URestoreWaste>(this));

	if (!HasAuthority()) return;
	InitializeObsessionTarget();
	if (ObsessionTarget == nullptr) return;

	AGOCLEANCharacter* ObsessionCharacter = Cast<AGOCLEANCharacter>(ObsessionTarget->GetPawn());
	if (ObsessionCharacter == nullptr) return;

	ObsessionCharacter->SetSanityDrainMultiplier(1.2f);

	GetWorldTimerManager().SetTimer(CheckApplyFrostEventConditionHandle, this, &AMaidenGhost::CheckApplyFrostEventCondition, 10.0f, true);
}

void AMaidenGhost::InitializeObsessionTarget()
{
	AGhostAIController* AIController = Cast<AGhostAIController>(GetController());
	if (AIController == nullptr) return;

	AIController->UpdateAlivePlayerList();
	if (AIController->AlivePlayers.IsEmpty()) return;

	TArray<APlayerSessionState*> FemalePlayers;
	TArray<APlayerSessionState*> AllPlayers;

	for (AGOCLEANCharacter* PlayerCharacter : AIController->AlivePlayers)
	{
		if (PlayerCharacter == nullptr) continue;

		APlayerSessionState* PSS = PlayerCharacter->GetPlayerState<APlayerSessionState>();
		if (PSS == nullptr) continue;

		AllPlayers.Add(PSS);

		if (PSS->GetGender() == EPlayerGender::Female)
		{
			FemalePlayers.Add(PSS);
		}
	}

	const TArray<APlayerSessionState*>& TargetPlayers = FemalePlayers.IsEmpty() ? AllPlayers : FemalePlayers;
	
	if (TargetPlayers.IsEmpty()) return;

	const int32 RandomIndex = FMath::RandRange(0, TargetPlayers.Num()-1);

	ObsessionTarget = TargetPlayers[RandomIndex];
	UE_LOG(LogTemp, Warning, TEXT("Maiden ghost obssesion target: %s"), *ObsessionTarget->GetName());
}

void AMaidenGhost::CheckApplyFrostEventCondition()
{
	const int32 Result = FMath::RandRange(0, 2);
	if (Result == 2)
		ApplyFrostEvent();
	else 
		return;
}

void AMaidenGhost::ApplyFrostEvent()
{
	if (ObsessionTarget == nullptr) return;

	AGOCLEANCharacter* ObsessionCharacter = Cast<AGOCLEANCharacter>(ObsessionTarget->GetPawn());
	if (ObsessionCharacter == nullptr) return;

	UGObjectManager* ObjManager = GetWorld()->GetSubsystem<UGObjectManager>();
	if (ObjManager == nullptr) return;

	const TArray<AActor*> WindowList = ObjManager->GetWindowList();

	const FVector PlayerLocation = ObsessionCharacter->GetActorLocation();

	constexpr float EffectRadius = 300.0f;
	constexpr float EffectRadiusSquared = EffectRadius * EffectRadius;

	for (AActor* Window : WindowList)
	{
		if (Window == nullptr) continue;

		const float DistanceSquared = FVector::DistSquared(PlayerLocation, Window->GetActorLocation());

		if (DistanceSquared > EffectRadiusSquared) continue;

		FVector Origin;
		FVector BoxExtent;
		Window->GetActorBounds(false, Origin, BoxExtent);

		const FBox WindowBox(Origin - BoxExtent, Origin + BoxExtent);

		const FVector SpawnLocation = FMath::RandPointInBox(WindowBox);

		FRotator SpawnRotation = Window->GetActorRotation();

		SpawnRotation += FRotator(90.0f, 90.0f, 0.0f);

		ObjManager->SpawnNonfixedObject("Obj_Frost", ENonfixedObjState::E_Static, SpawnLocation, SpawnRotation);
	}
	UE_LOG(LogTemp, Warning, TEXT("Maiden ghost frost event implemented"));
}

void AMaidenGhost::OnUnendingRageStarted()
{
}

void AMaidenGhost::OnPlayerSanityHalfReached(APlayerSessionState* InPlayerState)
{
	if (HallucinationActorClass == nullptr) return;
	if (InPlayerState == nullptr) return;

	AGOCLEANCharacter* PlayerCharacter = Cast<AGOCLEANCharacter>(InPlayerState->GetPawn());
	if (PlayerCharacter == nullptr) return;

	APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController());
	if (PlayerController == nullptr) return;

	FVector PlayerViewLocation;
	FRotator PlayerViewRotation;

	PlayerController->GetPlayerViewPoint(PlayerViewLocation, PlayerViewRotation);

	const FVector PlayerViewForward = PlayerViewRotation.Vector();
	const FVector DesiredLocation = PlayerViewLocation + PlayerViewForward * 500.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PlayerCharacter);

	FHitResult ForwardHitResult;
	FVector GroundCheckLocation = DesiredLocation;

	if (GetWorld()->LineTraceSingleByChannel(ForwardHitResult, PlayerViewLocation, DesiredLocation, ECC_Visibility, Params))
	{
		GroundCheckLocation = ForwardHitResult.ImpactPoint - (PlayerViewForward * 50.0f);
	}

	const FVector TraceStart = GroundCheckLocation + FVector(0.0f, 0.0f, 100.0f);

	const FVector TraceEnd = GroundCheckLocation - FVector(0.0f, 0.0f, 1000.0f);

	FHitResult GroundHit;

	if (!GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Params)) return;

	const FVector SpawnLocation = GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 100.0f);

	FRotator SpawnRotation = (PlayerCharacter->GetActorLocation() - SpawnLocation).Rotation();

	SpawnRotation.Pitch = 0.0f;
	SpawnRotation.Roll = 0.0f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PlayerController;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Hallucination = GetWorld()->SpawnActor<AActor>(HallucinationActorClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Hallucination == nullptr) return;
	Hallucination->SetLifeSpan(3.0f);
}
