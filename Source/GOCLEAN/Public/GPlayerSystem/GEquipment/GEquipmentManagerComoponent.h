// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GEquipmentManagerComoponent.generated.h"


class AGOCLEANCharacter;
class UGBaseEquipmentComponent;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GOCLEAN_API UGEquipmentManagerComoponent : public UActorComponent
{
	GENERATED_BODY()
	


	//////////////////////////////////
	//	life cycle
	//////////////////////////////////
public:	
	UGEquipmentManagerComoponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;



	//////////////////////////////////
	//	variables - equipment componet
	//////////////////////////////////
protected:
	UPROPERTY(VisibleAnywhere)
	int32 CurrentSlotIndex;

	UPROPERTY()
	TArray<TObjectPtr<UGBaseEquipmentComponent>> EquipmentSlots;

	UPROPERTY(ReplicatedUsing = "OnRep_CurrentEquipID")
	FName CurrentEquipID;



	//////////////////////////////////
	//	functions - equipment componet
	//////////////////////////////////
public:
	// getter
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	FName GetCurrentEquipmentID() const { return CurrentEquipID; }

	UGBaseEquipmentComponent* GetCurrentEquipementComp() const;

	// change
	void ChangeCurrentSlotIndex(int32 NewIndex);

	void ChangeCurrentSlotIndex(bool direction);

	void ChangeEquipment(FName NewEquip);

protected:
	// setter
	void SetCurrentSlotIndex(int32 NewValue);

	void SetCurrentEquipmentID(FName NewValue);

	void SetEquipmentComp(int32 Index, FName NewEquip);

	// initialize
	bool InitiateEquipmentSlots();



	//////////////////////////////////
	//	variables - owner character
	//////////////////////////////////
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AGOCLEANCharacter> OnwerCharacter;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> EquipmentMeshComp;



	//////////////////////////////////
	//	functions - owner character
	//////////////////////////////////
protected:
	bool RegisterOwnerCharacter();



	//////////////////////////////////
	//	net - replication
	//////////////////////////////////
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_CurrentEquipID();
		
};
