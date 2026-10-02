// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GEquipmentComponent.generated.h"


class AGOCLEANCharacter;
class AGNonfixedObject;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GOCLEAN_API UGEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()


	// Variables
protected:
	// replicated
	//		slot datas
	UPROPERTY(ReplicatedUsing="OnRep_CurrentSlotIndex")
	int32 CurrentSlotIndex;

	UFUNCTION()
	void OnRep_CurrentSlotIndex();


	UPROPERTY(Replicated)
	TArray<FName> EquipmentSlots; // ¿Â∫Ò ΩΩ∑‘(size 4), element: equip ID


	//		picked object
	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<TObjectPtr<AGNonfixedObject>> HeldObjects;


	//		pollution
	UPROPERTY(ReplicatedUsing = "OnRep_MopPollusion")
	float MopPollution;

	UFUNCTION()
	void OnRep_MopPollusion();
	
	UPROPERTY(Replicated)
	float AutoMopPollution;

	

	//		owner character
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AGOCLEANCharacter> Owner;


	// functions - custom
public:
	// in-game
	int32 GetCurrentSlotIndex() const { return CurrentSlotIndex; }

	FName GetCurrentEquipmentID() const;


	bool ChangeEuquipmentInCurrSlot(FName ChangedEquipID);

	bool ChangeEquipment(int32 SlotIndex, FName EquipID);


	bool ChangeCurrentSlot(int32 ChangedSlotIndex);


	AGNonfixedObject* GetCurrentHeldObject() const;

	AGNonfixedObject* GetHeldObject(int32 SlotIndex) const;

	FName GetCurrentHeldObjectName() const;


	bool SetCurrentHeldObject(AGNonfixedObject* NFixedObject);

	bool ChangeHeldObject(int32 SlotIndex, AGNonfixedObject* Obj);



	UFUNCTION(BlueprintCallable)
	void AddMopPollution(float Value);

	UFUNCTION(BlueprintCallable)
	void CleanMopPollution();

	UFUNCTION(BlueprintCallable)
	float GetMopPollution() { return MopPollution; }

	UFUNCTION(BlueprintCallable)
	float GetAutoMopPollution() { return AutoMopPollution; }



protected:
	// in-game
	FName GetEquipmentID(int32 SlotIndex) const;
	bool ChangeCurrentSlot_Interval(int32 From, int32 To);

	// in-beginPlay
	bool InitiateEquipmentSlots();
	


	// functions - default
public:	
	UGEquipmentComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
		
};
