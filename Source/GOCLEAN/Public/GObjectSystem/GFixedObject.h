// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GTypes/GObjectTypes.h"
#include "GTypes/IGInteractable.h"

#include "GFixedObject.generated.h"



UCLASS()
class GOCLEAN_API AGFixedObject : public AActor, public IGInteractable
{
	GENERATED_BODY()
	

public:	
	// Sets default values for this actor's properties
	AGFixedObject();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	UFUNCTION(BlueprintCallable)
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	int32 GetInstanceID() { return IID; }
	FName GetTypeID() { return TID; }
	FName GetLocatedZoneID() { return LocatedZoneID; }
	EGFixedObjState GetState() { return State; }

	UFUNCTION(BlueprintCallable)
	void ChangeState(EGFixedObjState ChangedState);

	UFUNCTION(BlueprintImplementableEvent)
	void OnCustomEvent_Bool(bool Param);

	// interact
	virtual bool CanInteract(FName EquipID, AGOCLEANCharacter* Target) const {
		return true;
	}

	virtual void ExecuteInteraction(FName EquipID, AGOCLEANCharacter* Target) override;


	UFUNCTION(BlueprintImplementableEvent)
	void ExecuteInteraction_Internal();



protected:
	// instance ID - ObjectPool에서 재활용 시, 생성된 obj cnt가 instance id로 사용됨
	UPROPERTY(EditAnywhere, Category = "ID")
	int32 IID;

	// type ID - 현재 오브젝트가 어떤 오브젝트인지 식별하는 ID
	UPROPERTY(EditAnywhere, Category = "ID")
	FName TID;

	UPROPERTY(EditAnywhere, Category = "Zone")
	FName LocatedZoneID;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_State, Category = "State")
	EGFixedObjState State;


	// state
	UFUNCTION(BlueprintImplementableEvent)
	void OnStateExit(EGFixedObjState ExitedState);

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateEnter(EGFixedObjState EnteredState);

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateUpdate();


	// state - visual
	UFUNCTION()
	void OnRep_State();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateChanged_Visual();

};



UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGFixedObjInteractionComponent : public UActorComponent, public IGInteractable
{
	GENERATED_BODY()


public:
	// Sets default values for this component's properties
	UGFixedObjInteractionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bChangeStateAfterExecuteInteraction = false;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// interact
	virtual bool CanInteract(FName EquipID, AGOCLEANCharacter* Target) const override;

	virtual void ExecuteInteraction(FName EquipID, AGOCLEANCharacter* Target) override;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UStaticMeshComponent>> Meshs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SpawnObjTID;


	UFUNCTION(BlueprintCallable)
	FName GetSpawnObjTID() const { return SpawnObjTID; }

	UFUNCTION(BlueprintCallable)
	void SetSpawnObjTID(FName TID);


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UBoxComponent> InteractionVolume;

};
