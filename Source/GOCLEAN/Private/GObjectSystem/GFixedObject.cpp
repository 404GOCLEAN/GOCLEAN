// Fill out your copyright notice in the Description page of Project Settings.


#include "GObjectSystem/GFixedObject.h"
#include "GObjectSystem/GNonfixedObject.h"
#include "GObjectSystem/Server/GObjectManager.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

#include "GPlayerSystem/GEquipment/GEquipmentComponent.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "GDataManagerSubsystem.h"

#include "GTypes/DataTableRow/GEquipmentDataRow.h"
#include "GTypes/DataTableRow/GObjectDataRow.h"

#include "Net/UnrealNetwork.h"


// Sets default values
AGFixedObject::AGFixedObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AGFixedObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGFixedObject, State);
}

// Called when the game starts or when spawned
void AGFixedObject::BeginPlay()
{
	Super::BeginPlay();
	
    if (auto* ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>())
    {
        ObjectManager->RegisterFixedObject(TID, this);
    }
}

// Called every frame
void AGFixedObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGFixedObject::ExecuteInteraction(FName EquipID, AGOCLEANCharacter* Target)
{
	ExecuteInteraction_Internal();

	auto SpawnerComp = GetComponentByClass<UGFixedObjInteractionComponent>();
	if (SpawnerComp && SpawnerComp->CanInteract(EquipID, Target))
	{
		SpawnerComp->ExecuteInteraction(EquipID, Target);
	}
}



void AGFixedObject::ChangeState(EGFixedObjState ChangedState)
{
	auto PrevState = State;
	if (PrevState == ChangedState) return;

	OnStateExit(PrevState);
	OnStateEnter(ChangedState);

	State = ChangedState;
	OnRep_State();
}

void AGFixedObject::OnRep_State()
{
	OnStateChanged_Visual();
}



// Sets default values for this component's properties
UGFixedObjInteractionComponent::UGFixedObjInteractionComponent() {} 

void UGFixedObjInteractionComponent::BeginPlay()
{
	MaxCount = Meshs.Num();
	if (MaxCount == 0) MaxCount = -1;
}

bool UGFixedObjInteractionComponent::CanInteract(FName EquipID, AGOCLEANCharacter* Target) const
{
	if (!Target) return false;
	if (EquipID != "Eq_Hand") return false;
	if (MaxCount == 0) return false;

	return true;
}

void UGFixedObjInteractionComponent::SetSpawnObjTID(FName TID)
{
	SpawnObjTID = TID;

	AGFixedObject* Owner = Cast<AGFixedObject>(GetOwner());
	if (Owner) Owner->ChangeState(EGFixedObjState::E_Static);
}

void UGFixedObjInteractionComponent::ExecuteInteraction(FName EquipID, AGOCLEANCharacter* Target)
{
	UGEquipmentComponent* EquipComp = Target->GetEquipComp();
	if (!EquipComp) return;

	auto* ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
	if (!ObjectManager) return;

	auto* DataManager = GetWorld()->GetGameInstance()->GetSubsystem<UGDataManagerSubsystem>();
	auto* ObjData = DataManager ? DataManager->GetObjectData(SpawnObjTID) : nullptr;
	if (!ObjData)
	{	
		return;
	}

	auto* EquipData = DataManager->GetEquipmentData(ObjData->PickedEquipID);
	if (!EquipData) return;


	auto* NewObj = ObjectManager->SpawnNonfixedObject(
		SpawnObjTID,
		ENonfixedObjState::E_Static,
		FVector(0.0f, 0.0f, 0.0f),
		FRotator(0.0f, 0.0f, 0.0f));

	if (!NewObj) return;

	EquipComp->ChangeEquipment(EquipData->MatchedSlotIndex, ObjData->PickedEquipID);
	Target->SetHeldObject(NewObj);
	NewObj->GetNonfixedObjCoreComp()->ChangeState(ENonfixedObjState::E_Picked);
	NewObj->Multicast_OnPickedUp(Target);


	MaxCount--;

	if (MaxCount >= 0) Meshs[MaxCount]->SetVisibility(false);
	if (MaxCount == 0) InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);


	if (bChangeStateAfterExecuteInteraction)
	{
		AGFixedObject* Owner = Cast<AGFixedObject>(GetOwner());
		if (Owner) Owner->ChangeState(EGFixedObjState::E_Invisible);
	}
}
