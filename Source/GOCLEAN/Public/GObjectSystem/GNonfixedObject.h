// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "GTypes/GObjectTypes.h"
#include "GObjectSystem/GNonfixedObjCoreComponent.h"

#include "GNonfixedObject.generated.h"


class UGAdditionalObjFuncComponent;
class UBoxComponent;


UCLASS()
class GOCLEAN_API AGNonfixedObject : public AActor
{
	GENERATED_BODY()


	// defaults
public:
	AGNonfixedObject();

	// virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// life cycle
protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UGNonfixedObjCoreComponent* GetNonfixedObjCoreComp() const { return CoreComp; }

	FName GetTID() { return CoreComp ? CoreComp->TID : ""; }
	ENonfixedObjState GetState() { return CoreComp ? CoreComp->GetState() : ENonfixedObjState::E_None; }



	// init
	void ResetForPool() {};

	// TID를 새로 할당해야 하는 경우
	void SetObjectData(FGNonfixedObjData& InitData);

	// TID는 유지하되, 그 외의 설정을 해야하는 경우 -> UGObjectManager::FindAllNonfixedObjects
	void UpdateObjectData(int32 IID);

	void UpdateVisualByState();
	void UpdatePhysicsByState();

	UStaticMeshComponent* GetStaticMeshComp() { return RootPrimitive; }



	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnPickedUp(class AGOCLEANCharacter* TargetCharacter);


	// variables
protected:
	// root
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> RootPrimitive;

	void UpdateInteractionBounds();

	// interaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UBoxComponent> InteractionVolume; // Collision Profile -> Interactable


	// root
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UGNonfixedObjCoreComponent> CoreComp;

};
