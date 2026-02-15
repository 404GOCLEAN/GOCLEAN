/**
 * @class UCharacterStatsComponent
 * @brief Character stats 관련 component
 *
 * **[Core functions]**
 * - InitializeStats(int32 BaseLife, int32 BaseMaxSanity, float BaseMaxStamina, float BaseStaminaDrainRate, float BaseStaminaRecoveryRate, float BaseStaminaRecoveryDelay, float BaseDefaultSpeed): 입력된 값에 따른 기본 스탯 초기화
 *
 * **[Getter & Setter functions]**
 * - GetCurrentSanity() const: 현재 정신력 반환
 * - GetCurrentLife() const: 현재 잔여 목숨 반환
 * - DecreaseLife(int32 Amount): 잔여 목숨 감소
 * - SetCurrentSanity(float Amount): 현재 정신력 설정
 * - IncreaseCurrentSanity(float Amount): 현재 정신력 증가
 * - DecreaseCurrentSanity(float Amount): 현재 정신력 감소
 * - GetCurrentStamina() const: 현재 Stamina 반환
 * - GetStaminaDrainRate() const: Stamina 감소비율 반환
 * - GetStaminaRecoveryRate() const: Stamina 재생비율 반환
 * - GetStaminaRecoveryDelay() const: Stamina 재생 간격 반환
 * - GetMaxStamina() const: Stamina 최대치 반환
 * - SetCurrentStamina(float NewCurrentStamina): 현재 Stamina 설정
 * - IncreaseCurrentStamina(float Amount): 현재 Stamina 증가
 * - DecreaseCurrentStamina(float Amount): 현재 Stamina 감소
 * - GetWalkSpeed() const: WalkSpeed 반환
 * - GetCrouchSpeed() const: 웅크리기 시 속도 반환
 * - GetSprintSpeed() const: 달리기 시 속도 반환
 * - SetDefaultSpeed(float NewDefaultSpeed): 기준 속도 설정 == WalkSpeed == CrouchSpeed+300 == SprintSpeed-300
 * - IncreaseDefaultSpeed(float Amount): 기준 속도 증가
 * - DecreaseDefaultSpeed(float Amount): 기준 속도 감소
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "CharacterStatsComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLifeChanged, int32, NewLife);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSanityChanged, float, NewSanity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoveSpeedChanged, float, NewDefaultSpeed);


UCLASS( meta=(BlueprintSpawnableComponent) )
class GOCLEAN_API UCharacterStatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCharacterStatsComponent();

public:
	// UI가 바인딩해서 쓰는 레퍼런스용 콜백
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnLifeChanged OnLifeChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnSanityChanged OnSanityChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnMoveSpeedChanged OnMoveSpeedChanged;

public:


	// Getter, Setter //
	float GetCurrentSanity() const;

	int GetCurrentLife() const;
	void DecreaseLife(int32 Amount);

	void SetCurrentSanity(float Amount);
	void IncreaseCurrentSanity(float Amount);
	void DecreaseCurrentSanity(float Amount);
	void SetSanityDrainRate(float NewCurrentSanityDrainRate);
	float GetSanityDrainRate();

	float GetCurrentStamina() const;
	float GetStaminaDrainRate() const;
	float GetStaminaRecoveryRate() const;
	float GetStaminaRecoveryDelay() const;
	float GetMaxStamina() const;
	void SetCurrentStamina(float NewCurrentStamina);
	void IncreaseCurrentStamina(float Amount);
	void DecreaseCurrentStamina(float Amount);

	float GetWalkSpeed() const;
	float GetCrouchSpeed() const;
	float GetSprintSpeed() const;
	void SetDefaultSpeed(float NewDefaultSpeed);
	void IncreaseDefaultSpeed(float Amount);
	void DecreaseDefaultSpeed(float Amount);



public:
	// Respawn //
	void ResetStats();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// Overidded //
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	// Initialize //
	void InitializeStats(
		int32 BaseLife, 
		int32 BaseMaxSanity, 
		float BaseMaxStamina, float BaseStaminaDrainRate, float BaseStaminaRecoveryRate, float BaseStaminaRecoveryDelay,
		float BaseDefaultSpeed)
	{
		Life = BaseLife;

		MaxSanity = BaseMaxSanity;
		CurrentSanity = MaxSanity;
		SanityDrainRate = 10.0f;

		MaxStamina = BaseMaxStamina;
		CurrentStamina = BaseMaxStamina;
		StaminaDrainRate = BaseStaminaDrainRate;
		StaminaRecoveryRate = BaseStaminaRecoveryRate;
		StaminaRecoveryDelay = BaseStaminaRecoveryDelay;

		DefaultSpeed = BaseDefaultSpeed;
		WalkSpeed = DefaultSpeed;
		CrouchSpeed = DefaultSpeed - 100.0f;
		SprintSpeed = DefaultSpeed + 300.0f;
	}


	// Character stats //
	int32 Life;

	float MaxSanity;
	float CurrentSanity;
	float SanityDrainRate;

	float MaxStamina;
	float CurrentStamina;
	float StaminaDrainRate;
	float StaminaRecoveryRate;
	float StaminaRecoveryDelay;

	float DefaultSpeed;
	float WalkSpeed;
	float CrouchSpeed;
	float SprintSpeed;


private:
	UFUNCTION()
	void OnRep_Life();

	UFUNCTION()
	void OnRep_CurrentSanity();

	UFUNCTION()
	void OnRep_DefaultSpeed();
};
