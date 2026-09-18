// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerSystem/GEquipment/GEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "GOCLEANSettings.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "GObjectSystem/Server/GObjectManager.h"
#include "../../../GOCLEAN.h"
#include "GObjectSystem/GNonfixedObject.h"
#include "GObjectSystem/GNonfixedObjCoreComponent.h"


// Sets default values for this component's properties
UGEquipmentComponent::UGEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);

	InitiateEquipmentSlots();
}

void UGEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 속한 데이터 중 일부를 '나'에게만 복제하고 싶을 때
	// DOREPLIFETIME_CONDITION(UGEquipmentComponent, CurrentSlotIndex, COND_OwnerOnly);
	DOREPLIFETIME(UGEquipmentComponent, EquipmentSlots);
	DOREPLIFETIME(UGEquipmentComponent, CurrentSlotIndex);
	DOREPLIFETIME(UGEquipmentComponent, MopPollution);
	DOREPLIFETIME(UGEquipmentComponent, AutoMopPollution);
	DOREPLIFETIME(UGEquipmentComponent, HeldObjects);
}


// Called when the game starts
void UGEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = Cast<AGOCLEANCharacter>(GetOwner());
}


// Called every frame
void UGEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}



// equip ID - public
FName UGEquipmentComponent::GetCurrentEquipmentID() const
{
	return GetEquipmentID(CurrentSlotIndex);
}

bool UGEquipmentComponent::ChangeEuquipmentInCurrSlot(FName ChangedEquipID)
{
	return ChangeEquipment(CurrentSlotIndex, ChangedEquipID);
}

bool UGEquipmentComponent::ChangeCurrentSlot(int32 ChangedSlotIndex)
{
	if (!Owner || !Owner->HasAuthority()) return false;

	return ChangeCurrentSlot_Interval(CurrentSlotIndex, ChangedSlotIndex);
}


// equip ID - private
FName UGEquipmentComponent::GetEquipmentID(int32 SlotIndex) const
{
	if (EquipmentSlots.Num() <= 0 || EquipmentSlots.Num() <= SlotIndex || SlotIndex < 0)
	{
		return "Error";
	}

	return EquipmentSlots[SlotIndex];
}

bool UGEquipmentComponent::ChangeEquipment(int32 SlotIndex, FName EquipID)
{
	if (EquipmentSlots.Num() <= 0 || EquipmentSlots.Num() <= SlotIndex || SlotIndex < 0)
	{
		return false;
	}

	EquipmentSlots[SlotIndex] = EquipID;
	return true;
}

bool UGEquipmentComponent::ChangeCurrentSlot_Interval(int32 From, int32 To)
{
	auto* PlayerChar = Cast<AGOCLEANCharacter>(GetOwner());
	if (!PlayerChar) return false;

	if (EquipmentSlots.Num() <= 0 || EquipmentSlots.Num() <= To || To < 0)
	{
		return false;
	}

	auto* ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
	if (!ObjectManager)
	{
		UE_LOG(LogGObject, Warning, TEXT("[ObjectManager] There's no object manager"));
		return false;
	}


	if (From == 0 && GetEquipmentID(From) == "Eq_OVariable")
	{
		AGNonfixedObject* HeldObj = HeldObjects[From];

		PlayerChar->DropHeldObject(From);
		HeldObj->GetNonfixedObjCoreComp()->ChangeState(ENonfixedObjState::E_Static);
	}
	if (To == 2 && GetEquipmentID(To) != "Eq_Hand")
	{
		AGNonfixedObject* HeldObj = HeldObjects[To];

		PlayerChar->DropHeldObject(To);
		HeldObj->GetNonfixedObjCoreComp()->ChangeState(ENonfixedObjState::E_Static);
	}

	CurrentSlotIndex = To;
	OnRep_CurrentSlotIndex();


	// change anim - swap


	return true;
}


// held Object
AGNonfixedObject* UGEquipmentComponent::GetCurrentHeldObject() const
{
	return GetHeldObject(CurrentSlotIndex);
}

bool UGEquipmentComponent::SetCurrentHeldObject(AGNonfixedObject* NFixedObject)
{
	return ChangeHeldObject(CurrentSlotIndex, NFixedObject);
}

FName UGEquipmentComponent::GetCurrentHeldObjectName() const
{
	FName Name = "";

	auto Target = GetHeldObject(CurrentSlotIndex);
	if (IsValid(Target))
	{
		Name = Target->GetNonfixedObjCoreComp()->TID;
	}

	return Name;
}


AGNonfixedObject* UGEquipmentComponent::GetHeldObject(int32 SlotIndex) const
{
	if (HeldObjects.Num() <= 0 || HeldObjects.Num() <= SlotIndex || SlotIndex < 0)
	{
		return nullptr;
	}

	return HeldObjects[SlotIndex];
}

bool UGEquipmentComponent::ChangeHeldObject(int32 SlotIndex, AGNonfixedObject* Obj)
{
	if (HeldObjects.Num() <= 0 || HeldObjects.Num() <= SlotIndex || SlotIndex < 0)
	{
		return false;
	}
	else if (SlotIndex == 1)
	{
		UE_LOG(LogGObject, Warning, TEXT("[Equipment] Slot 2 can hold only mop!"));
		return false;
	}
	else if (Obj && HeldObjects[SlotIndex])
	{
		UE_LOG(LogGObject, Warning, TEXT("[Equipment] This slot already has other object!: SlotID - %d, BlockedObjName - %s"), SlotIndex, *Obj->GetName());
		return false;
	}

	HeldObjects[SlotIndex] = Obj;
	return true;
}



bool UGEquipmentComponent::InitiateEquipmentSlots()
{
	EquipmentSlots.Empty();
	HeldObjects.Empty();

	// slot 1
	EquipmentSlots.Add("Eq_Hand");
	HeldObjects.Add(nullptr);

	// slot 2
	EquipmentSlots.Add("Eq_Mop");
	HeldObjects.Add(nullptr);

	// slot 3
	EquipmentSlots.Add("Eq_Hand");
	HeldObjects.Add(nullptr);

	// slot 4
	EquipmentSlots.Add("Eq_Hand");
	HeldObjects.Add(nullptr);

	return true;
}



// mop pollution
void UGEquipmentComponent::AddMopPollution(float Value)
{
	FName CurrEquipID = GetCurrentEquipmentID();
	if (CurrEquipID == "Eq_Mop")
	{
		MopPollution += Value;
		UE_LOG(LogGObject, Log, TEXT("[Equipment] Add Mop's pollution! : %f"), MopPollution);

		OnRep_MopPollusion();
	}
	else if (CurrEquipID == "Eq_AutoMop")
	{
		AutoMopPollution += Value;
		UE_LOG(LogGObject, Log, TEXT("[Equipment] Add AutoMop's pollution! : %f"), AutoMopPollution);
	}
	else
	{
		UE_LOG(LogGObject, Warning, TEXT("[Equipment] this equipment can not add pollution! : %s"), *CurrEquipID.ToString());
	}
}

void UGEquipmentComponent::CleanMopPollution()
{
	FName CurrEquipID = GetCurrentEquipmentID();
	if (CurrEquipID == "Eq_Mop")
	{
		MopPollution = 0;
		UE_LOG(LogGObject, Log, TEXT("[Equipment] Clean Mop's pollution! : %f"), MopPollution);

		OnRep_MopPollusion();
	}
	else if (CurrEquipID == "Eq_AutoMop")
	{
		AutoMopPollution = 0;
		UE_LOG(LogGObject, Log, TEXT("[Equipment] Clean AutoMop's pollution! : %f"), AutoMopPollution);
	}
	else
	{
		UE_LOG(LogGObject, Warning, TEXT("[Equipment] this equipment can not clean pollution! : %s"), *CurrEquipID.ToString());
	}
}



// OnRep
void UGEquipmentComponent::OnRep_CurrentSlotIndex()
{
	AGNonfixedObject* HeldObj;

	// update visual
	for (int i = 0; i<HeldObjects.Num(); ++i)
	{
		HeldObj = HeldObjects[i];
		if (!HeldObj) continue;

		if (i == CurrentSlotIndex)
		{
			// show visual
			HeldObj->SetActorHiddenInGame(false);

			if (Owner)
			{
				// change character anim

				// set actor's relative location
			}
		}
		else
		{
			// hide visual
			HeldObj->SetActorHiddenInGame(true);
		}
	}
}

void UGEquipmentComponent::OnRep_MopPollusion()
{
	// update mop's material
}
