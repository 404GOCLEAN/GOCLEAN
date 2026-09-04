// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * @class AGOCLEANCharacter
 * @brief Character stats component를 기반으로 하여, 유저의 입력에 기반한 플레이어 이동 처리
 *
 * **[Core functions]**
 * - Move(const FInputActionValue& Value): 키 입력에 따른 플레이어 이동 처리
 * - Look(const FInputActionValue& Value): 마우스 이동에 따른 플레이어 카메라 이동
 * - Crouch(): 플레이어 웅크리기 상태 전환
 * - Sprint(): 플레이어 달리기
 * - SprintRelease(): 플레이어 달리기 상태 릴리즈
 * - StartStaminaRecovery(): Stamina 재생 시작
 * - RecoverStamina(): Stamina 재생
 * - ToggleFlashlight(): 플레이어 손전등 토글 기능
 * 
 * **[Getter & Setter functions]**
 * - IsCrouching() const: 웅크리기 상태인지에 대한 판단값 반환
 * - IsSprinting() const: 달리기 상태인지에 대한 판단값 반환
 * - GetPlayerCurrentSanity() const: 플레이어의 현재 정신력 반환
 * - SetPlayerCurrentSanity(float NewPlayerCurrentSanity): 플레이어의 현재 정신력 설정
 * - OnHunted(): Ghost에게 Hunt 당했을 시 호출
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "InputAction.h"

#include "GCharacter/StatsComponent/CharacterStatsComponent.h"

#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "DefaultLevelSequenceInstanceData.h"

#include "Animation/AnimMontage.h"
#include "Engine/DataTable.h"
#include "GCharacter/DataTable/FAnimationData.h"

#include "Net/RpcTypes.h"

#include "GOCLEANCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpotLightComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

class UGEquipmentComponent;
class UInteractionComponent;
class UEnhancedInputComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class GOCLEAN_API AGOCLEANCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGOCLEANCharacter();


	// Getter, Setter //
	UFUNCTION(BlueprintPure)
	bool IsSprinting() const { return bIsSprinting; }

	int32 GetPlayerCurrentLife() const;
	void DecreaseLife(int32 Amount);
	float GetPlayerCurrentSanity() const;
	void SetPlayerCurrentSanity(float NewPlayerCurrentSanity);
	void SetCanToggleFlashlight(bool bCanToggle) { bCanToggleFlashlight = bCanToggle; }

	UPROPERTY(Replicated)
	EPlayerAnimState AnimState;
	
	void SetAnimState(EPlayerAnimState NewAnimState) { AnimState = NewAnimState; }

	UFUNCTION(BlueprintPure)
	EPlayerAnimState GetAnimState() { return AnimState; }

	void SetSanityDrainMultiplier(float NewSanityDrainMultiplier)
	{
		if (StatsComp == nullptr) return;

		StatsComp->SetSanityDrainMultiplier(NewSanityDrainMultiplier);
	}

	float GetDefaultSpeed() const { return StatsComp->GetWalkSpeed(); };
	void SetDefaultSpeed(float NewDefaultSpeed) { StatsComp->SetDefaultSpeed(NewDefaultSpeed); };

	// Server //

	void TryCrouch();
	void TrySprint();
	void TrySprintRelease();
	void TryToggleFlashlight();
	void TryPlayerInteractionAnim();

	UFUNCTION(Server, Reliable)
	void Server_RequestToggleFlashlight();
	UFUNCTION(Server, Reliable)
	void Server_RequestPlayerInteractionAnim();
	UFUNCTION(Server, Reliable)
	void Server_RequestCrouch();
	UFUNCTION(Server, Reliable)
	void Server_RequestSprint();
	UFUNCTION(Server, Reliable)
	void Server_RequestSprintRelease();
	UFUNCTION(Server, Reliable)
	void Server_RequestOnHunted();
	UFUNCTION(Server, Reliable)
	void Server_RequestSetVisible(bool IsVisible);
	UFUNCTION(Server, Reliable)
	void Server_RequestPlayHuntCameraSequence();
	UFUNCTION(Server, Reliable)
	void Server_RequestRespawn();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetDefaultSpeed(float NewDefaultSpeed);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Crouch();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Sprint();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SprintRelease();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ToggleFlashlight();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnHunted();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayerInteractionAnim();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetVisible(bool IsVisible);
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayHuntCameraSequence();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Respawn();

	UFUNCTION(Client, Reliable)
	void Client_PlayHuntCameraSequence();
	UFUNCTION(Client, Reliable)
	void Client_Respawn();
	UFUNCTION(Client, Reliable)
	void Client_DisableInput(bool bIsDisable);

	// OnHunted //
	void OnHunted();

	void PlayHuntCameraSequence();

	void SpawnDummyCharacter();

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Animation|Aim")
	float AimPitch = 0.f;

	UFUNCTION(Server, Unreliable)
	void Server_SetAimPitch(float NewPitch);

	UFUNCTION(BlueprintPure, Category = "Animation|Aim")
	float GetAimPitch() const;

private:
	// Components //
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> FirstPersonMeshComp;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> ThirdPersonMeshComp;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> CameraComp;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpotLightComponent> FlashlightComp;

	UPROPERTY(VisibleAnywhere, Replicated)
	TObjectPtr<UCharacterStatsComponent> StatsComp;


	// Dummy //
	UPROPERTY(EditAnywhere, Category = "DummyActor")
	TSubclassOf<AActor> DummyGhost;

	UPROPERTY(EditAnywhere, Category = "DummyActor")
	TSubclassOf<AActor> DummyCharacter;


	// Respawn //
	UPROPERTY(EditAnywhere, Category = "RespawnPoint")
	FTransform RespawnTransform;


	// Input Actions //
	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> FlashlightAction;


	// Level sequence //
	UPROPERTY(EditAnywhere, Category="Camera sequence")
	TObjectPtr<ULevelSequence> HuntCameraSequence;

	UPROPERTY(Replicated)
	bool bCanToggleFlashlight = true;


	// Respawn //
	UFUNCTION()
	void Respawn();



	// Overrided //
	void Tick(float DeltaTime) override;
	void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	void Jump() override;


	// Actions //
	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void Crouch();

	void Sprint();
	void SprintRelease();

	void StartStaminaRecovery();
	void RecoverStamina();

	void ToggleFlashlight();


	// Animation //
	UPROPERTY(EditDefaultsOnly, Category="Animation|Gender")
	bool Gender;
	UPROPERTY(EditDefaultsOnly, Category = "Animation|DataTable")
	TObjectPtr<UDataTable> FirstPersonAnimDataTable;
	UPROPERTY(EditDefaultsOnly, Category="Animation|DataTable")
	TObjectPtr<UDataTable> ThirdPersonManAnimDataTable;
	UPROPERTY(EditDefaultsOnly, Category = "Animation|DataTable")
	TObjectPtr<UDataTable> ThirdPersonWomanAnimDataTable;

	void PlayerInteractionAnim();


	// Handles //
	FTimerHandle StaminaRecoveryHandle;


	// States //
	bool bIsRecoveringStamina;
	bool bIsSprinting;



	// Anim ID //
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = "OnRep_AnimID")
	int32 AnimID;

	UFUNCTION()
	void OnRep_AnimID();

	UFUNCTION(BlueprintCallable)
	void SetAnimID(int32 NewID) { AnimID = NewID; }

	UFUNCTION(BlueprintPure)
	int32 GetAnimID() { return AnimID; }


	// Equipment // 
	UPROPERTY(VisibleAnywhere, Replicated)
	TObjectPtr<UGEquipmentComponent> EquipComp;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> ChangeSlotAction_Slot1;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> ChangeSlotAction_Slot2;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> ChangeSlotAction_Slot3;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> ChangeSlotAction_Slot4;


	// Interaction // 
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInteractionComponent> InteractionComp;

	UPROPERTY(EditDefaultsOnly, Category = "Input Actions")
	TObjectPtr<UInputAction> InteractAction;

public:
	UGEquipmentComponent* GetEquipComp() const { return EquipComp; }
	UInteractionComponent* GetInteractionComp() const { return InteractionComp; }

	void DoInteraction();
	void TryChangeCurrentEquipmentSlot0();
	void TryChangeCurrentEquipmentSlot1();
	void TryChangeCurrentEquipmentSlot2();
	void TryChangeCurrentEquipmentSlot3();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHeldObject(class AGNonfixedObject* NewObj);
	void DropHeldObject(int32 Index);
	USceneComponent* GetHandMesh() const { return FirstPersonMeshComp; }

	void SetHeldObjectRelativeTransform(class AGNonfixedObject* NewObj);
	

private:
	float LastSentAimPitch = 0.f;
};
