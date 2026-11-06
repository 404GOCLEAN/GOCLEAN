// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GObjectSystem/GAdditionalObjFuncComponent.h"

#include "GExocismComponent.generated.h"


class USphereComponent;
class USphereCollision;
class UDecalComponent;


DECLARE_MULTICAST_DELEGATE_OneParam(FOnExocismCircleInteracted, AGOCLEANCharacter*);


UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class GOCLEAN_API UGExocismComponent : public UGAdditionalObjFuncComponent
{
	GENERATED_BODY()


public:
	UGExocismComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitializeAdditionalData(const FGNonfixedObjData& Data) override;

	UFUNCTION(BlueprintCallable)
	void DoOnInteractionTriggered(AGOCLEANCharacter* Target)
	{
		OnInteractionTriggered(Target);
	}


protected:
	virtual void BeginPlay() override;

	virtual void OnInteractionTriggered(AGOCLEANCharacter* Target) override;

	virtual void OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState) override {};
	

	// Exocism
public:
	UFUNCTION()
	void ActivateExocismCircle();

	UFUNCTION()
	void DeactivateExocismCircle();

	UFUNCTION()
	void OnSuccessedInExocism();

	UFUNCTION()
	void OnFailedInExocism();

	UFUNCTION()
	void OnFinishExocism();


protected:
	// condition
	FOnExocismCircleInteracted StartCondition;

	FOnExocismCircleInteracted OnExocismCircleInteracted;

	UPROPERTY()
	FName NextTargetTID;

	UPROPERTY()
	FTimerHandle NextStepTimerHandler;


	// decal
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USphereComponent> InteractionVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UDecalComponent> Decal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> FailDecal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UMaterialInterface> SuccessDecal;


private:
	// 수살귀
	// step0. 초기 세팅
	UFUNCTION()
	void ExocismStep0_DrownGhost(AGOCLEANCharacter* Target);

	// step1. 백자 달항아리 배치
	UFUNCTION()
	void ExocismStep1_DrownGhost(AGOCLEANCharacter* Target);

	// step2. 부적 상호작용
	UFUNCTION()
	void ExocismStep2_DrownGhost(AGOCLEANCharacter* Target);


	// 손각시
	// step1. 흰 소복 배치
	UFUNCTION()
	void ExocismStep0_MaidenGhost(AGOCLEANCharacter* Target);

	// step1. 흰 소복 배치
	UFUNCTION()
	void ExocismStep1_MaidenGhost(AGOCLEANCharacter* Target);

	// step2. 쇠 말뚝 상호작용
	UFUNCTION()
	void ExocismStep2_MaidenGhost(AGOCLEANCharacter* Target);


	// 태자귀
	// step1. 
	UFUNCTION()
	void ExocismStep0_LittleGhost(AGOCLEANCharacter* Target);

	// step1. 
	UFUNCTION()
	void ExocismStep1_LittleGhost(AGOCLEANCharacter* Target);

	// step2. 
	UFUNCTION()
	void ExocismStep2_LittleGhost(AGOCLEANCharacter* Target);



	UPROPERTY()
	TObjectPtr<AGFixedObject> OwnerActor;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_bIsFailed, Category = "Exocism")
	bool bIsFailed;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_bIsActiveExcosim, Category = "Exocism")
	bool bIsActiveExcosim;

	UPROPERTY()
	float SuccessProgress;


	// replication function
	UFUNCTION()
	void OnRep_bIsFailed();

	UFUNCTION()
	void OnRep_bIsActiveExcosim();


public:
	void ResetFailState()
	{
		bIsFailed = false;
	}

};
