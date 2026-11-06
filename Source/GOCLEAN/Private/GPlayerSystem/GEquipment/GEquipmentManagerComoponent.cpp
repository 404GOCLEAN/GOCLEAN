// Fill out your copyright notice in the Description page of Project Settings.


#include "GPlayerSystem/GEquipment/GEquipmentManagerComoponent.h"

#include "GCharacter/GOCLEANCharacter.h"
#include "../../../GOCLEAN.h"

#include "Net/UnrealNetwork.h"


//////////////////////////////////
//	life cycle
//////////////////////////////////

UGEquipmentManagerComoponent::UGEquipmentManagerComoponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

void UGEquipmentManagerComoponent::BeginPlay()
{
	Super::BeginPlay();

	// owner character
	RegisterOwnerCharacter();
	
}

void UGEquipmentManagerComoponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}



//////////////////////////////////
//	functions - equipment componet
//////////////////////////////////

// getter
UGBaseEquipmentComponent* UGEquipmentManagerComoponent::GetCurrentEquipementComp() const
{
	UGBaseEquipmentComponent* CurrentEquip = nullptr;

	if (EquipmentSlots.IsValidIndex(CurrentSlotIndex))
	{
		CurrentEquip = EquipmentSlots[CurrentSlotIndex];
	}
	else
	{
		UE_LOG(LogGObject, Warning, TEXT("[EquipmentManager] Invalid euipment index"));
	}

	return CurrentEquip;
}

// setter
void UGEquipmentManagerComoponent::SetCurrentSlotIndex(int32 NewValue)
{

}

void UGEquipmentManagerComoponent::SetCurrentEquipmentID(FName NewValue)
{

}

void UGEquipmentManagerComoponent::SetEquipmentComp(int32 Index, FName NewEquip)
{

}

// change
void UGEquipmentManagerComoponent::ChangeCurrentSlotIndex(int32 NewIndex)
{

}

void UGEquipmentManagerComoponent::ChangeCurrentSlotIndex(bool direction)
{

}

void UGEquipmentManagerComoponent::ChangeEquipment(FName NewEquip)
{

}



//////////////////////////////////
//	functions - owner character
//////////////////////////////////

bool UGEquipmentManagerComoponent::RegisterOwnerCharacter()
{
	AGOCLEANCharacter* Target = Cast<AGOCLEANCharacter>(GetOwner());
	if (!IsValid(Target))
	{
		OnwerCharacter = nullptr;
		EquipmentMeshComp = nullptr;

		UE_LOG(LogGObject, Warning, TEXT("[EquipmentManager] Can not found owner character!"));

		return false;
	}


}



//////////////////////////////////
//	net - replication
//////////////////////////////////

void UGEquipmentManagerComoponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGEquipmentManagerComoponent, CurrentEquipID);
}

// on replicated
void UGEquipmentManagerComoponent::OnRep_CurrentEquipID()
{
	// equip static mesh 변경

	// equip location 변경

	// equip rotation 변경
}

