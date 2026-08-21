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
	// 매 프레임 기울기를 체크하기 위해 Tick을 켭니다.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 상호작용 트리거 (걸레 세척용)
	virtual void OnInteractionTriggered(class AGOCLEANCharacter* Target) override;

private:
	// 엎어짐 체크 및 처리
	void CheckSpill();
	void SpillFilth();

	UPROPERTY(EditAnywhere, Category = "Bucket")
	float SpillThreshold = 0.7f; // UpVector.Z가 이보다 낮으면 엎어진 것으로 판단

	UPROPERTY(EditAnywhere, Category = "Bucket")
	FName FilthTID = "Obj_DerivedBlood"; // 엎어졌을 때 스폰할 Filth의 TID

	bool bIsSpilled = false; // 이미 엎어졌는지 확인 (중복 스폰 방지)
};
