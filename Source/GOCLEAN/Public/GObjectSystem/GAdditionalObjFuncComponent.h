// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GTypes/GObjectTypes.h"
#include "GTypes/DataTableRow/GObjectDataRow.h"

#include "GAdditionalObjFuncComponent.generated.h"


class AGOCLEANCharacter;
class AGNonfixedObject;
class AGFixedObject;


UCLASS(Abstract, Blueprintable)
class GOCLEAN_API UGAdditionalObjFuncComponent : public UActorComponent
{
	GENERATED_BODY()


public:	
	UGAdditionalObjFuncComponent();

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) {}

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) PURE_VIRTUAL(UGAdditionalObjFuncComponent::OnInteractionTriggered, );

	UFUNCTION()
	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) 
		PURE_VIRTUAL(UGAdditionalObjFuncComponent::OnStateChangeTriggered, );
		
};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGPickComponent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()


public:
	UGPickComponent();

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) override;

	UFUNCTION()
	void DropObject();


protected:
	virtual void BeginPlay() override;

	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) override;

	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) override;


	// custom functions
	UFUNCTION()
	void PickUpObject(AGOCLEANCharacter* Target);


private:
	UPROPERTY(VisibleAnywhere)
	bool bIsPickedUp = false;

	UPROPERTY(VisibleAnywhere)
	AGOCLEANCharacter* OwnerPlayer;

};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGRemovingComponent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()


public:
	UGRemovingComponent();

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	virtual void BeginPlay() override;

	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) override;

	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) override {};


	// custom functions
	UFUNCTION(NetMulticast, Reliable)
	void SetVisualByInteractionCnt(AGNonfixedObject* Owner, class UGEquipmentComponent* EquipComp, const FGObjectDataRow& ObjData);

	UFUNCTION()
	void SetDestroyThisObject(AGNonfixedObject* Owner);


protected:
	UPROPERTY(VisibleAnywhere)
	int32 InteractionMaxCnt;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<UDecalComponent>> Decals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UStaticMesh> BrokenMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> DestroyedActor;


	UPROPERTY(ReplicatedUsing="OnRep_CleaningRatio")
	float CleaningRatio;

	UFUNCTION()
	void OnRep_CleaningRatio();

};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGMultiInteractionComponent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()


public:
	UGMultiInteractionComponent();

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) override;


protected:
	virtual void BeginPlay() override;

	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) override 
	{ 
		OnInteractionEvent(); 
	}

	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) override 
	{
		OnStateChangeEvent();
	}


protected:
	UFUNCTION(BlueprintImplementableEvent)
	void OnInteractionEvent();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStateChangeEvent();

};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGBurningCompopnent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()


public:
	UGBurningCompopnent();

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) override;


protected:
	virtual void BeginPlay() override;

	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) override {};

	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) override;


	// custom functions
	UFUNCTION()
	void StartBurning(AGNonfixedObject* Owner);


private:
	FTimerHandle BurnTimerHandle;

	AGFixedObject* Incinerator;

	void OnBurnTimerFinished();

	UPROPERTY(EditAnywhere, Category = "Burning")
	float BurningInterval = 5.0f;

};



UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGSpawnerCompopnent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()


public:
	UGSpawnerCompopnent();

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) override;


protected:
	virtual void BeginPlay() override;

	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) override {};

	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) override;


	// custom functions
	UFUNCTION()
	void SpawnDerivedObject(AGNonfixedObject* Owner);

};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGInteractSoundCompopnent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()


public:
	UGInteractSoundCompopnent();

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) override;


protected:
	virtual void BeginPlay() override;

	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) override;

	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) override {};


	// custom functions
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<USoundBase> CachedInteractSound;

};


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGBucketComponent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()

public:
	UGBucketComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnInteractionTriggered(class AGOCLEANCharacter* Target) override;


public:
	void FillBucket(bool bAbPenomena = false);
	void EmptyBucket();

private:
	// water
	bool bHasWater = false;


	// spill
	void CheckSpill();
	void SpillFilth();

	UPROPERTY(EditAnywhere, Category = "Bucket")
	float SpillThreshold = 0.7f; // UpVector.Z가 이보다 낮으면 엎어진 것으로 판단

	bool bIsSpilled = false;


	// pollution
	void AddPollution(bool bAbPenomena = false);

	UPROPERTY(EditAnywhere, Category = "Bucket")
	float Pollution;

	UPROPERTY(EditAnywhere, Category = "Bucket")
	float InteractionPollution = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Bucket")
	float PollutionLV1 = 20.0f;

	UPROPERTY(EditAnywhere, Category = "Bucket")
	float PollutionLV2 = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Bucket")
	float PollutionLV3 = 60.0f;


	// derived filth
	UPROPERTY(EditAnywhere, Category = "Bucket")
	TMap<int32, FName> FilthTID = {
		{1, "Obj_Blood1"},
		{2, "Obj_Blood2"},
		{3, "Obj_Blood3"}
	};

};
