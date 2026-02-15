#include "GCharacter/StatsComponent/CharacterStatsComponent.h"

UCharacterStatsComponent::UCharacterStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCharacterStatsComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Character base stats initalize
	InitializeStats(2, 100.0f, 10.0f, 1.0f, 1.0f, 1.0f, 400.0f);

	SetIsReplicatedByDefault(true);

	// 시작 시 UI 초기 갱신(클라에서)
	if (!GetOwner() || GetOwner()->HasAuthority()) return;

	OnLifeChanged.Broadcast(Life);
	OnSanityChanged.Broadcast(CurrentSanity);
	OnMoveSpeedChanged.Broadcast(DefaultSpeed);
}

void UCharacterStatsComponent::ResetStats()
{
	SetCurrentSanity(100.0f);
	SetCurrentStamina(10.0f);
	SetDefaultSpeed(400.0f);
	SetSanityDrainRate(10.0f);
}

void UCharacterStatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UCharacterStatsComponent, Life, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCharacterStatsComponent, CurrentSanity, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UCharacterStatsComponent, DefaultSpeed, COND_OwnerOnly);
}

void UCharacterStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


// Getter, Setter //
int UCharacterStatsComponent::GetCurrentLife() const { return Life; };
void UCharacterStatsComponent::DecreaseLife(int32 Amount) { Life -= Amount; }

float UCharacterStatsComponent::GetCurrentSanity() const { return CurrentSanity; }
void UCharacterStatsComponent::SetCurrentSanity(float NewCurrentSanity) { CurrentSanity = NewCurrentSanity; }
void UCharacterStatsComponent::IncreaseCurrentSanity(float Amount) { CurrentSanity += Amount; }
void UCharacterStatsComponent::DecreaseCurrentSanity(float Amount) { CurrentSanity -= Amount; }
void UCharacterStatsComponent::SetSanityDrainRate(float NewSanityDrainRate) { SanityDrainRate = NewSanityDrainRate; }
float UCharacterStatsComponent::GetSanityDrainRate() { return SanityDrainRate; }

float UCharacterStatsComponent::GetCurrentStamina() const { return CurrentStamina; }
float UCharacterStatsComponent::GetStaminaDrainRate() const { return StaminaDrainRate; }
float UCharacterStatsComponent::GetStaminaRecoveryRate() const { return StaminaRecoveryRate; }
float UCharacterStatsComponent::GetStaminaRecoveryDelay() const { return StaminaRecoveryDelay; }
float UCharacterStatsComponent::GetMaxStamina() const { return MaxStamina;  }
void UCharacterStatsComponent::SetCurrentStamina(float NewCurrentStamina) { CurrentStamina = NewCurrentStamina; }
void UCharacterStatsComponent::IncreaseCurrentStamina(float Amount) { CurrentStamina += Amount; }
void UCharacterStatsComponent::DecreaseCurrentStamina(float Amount) { CurrentStamina -= Amount; }

float UCharacterStatsComponent::GetWalkSpeed() const { return WalkSpeed; }
float UCharacterStatsComponent::GetCrouchSpeed() const { return CrouchSpeed; }
float UCharacterStatsComponent::GetSprintSpeed() const { return SprintSpeed; }
void UCharacterStatsComponent::IncreaseDefaultSpeed(float Amount) { DefaultSpeed += Amount; }
void UCharacterStatsComponent::DecreaseDefaultSpeed(float Amount) { DefaultSpeed -= Amount; }
void UCharacterStatsComponent::SetDefaultSpeed(float NewDefaultSpeed) {
	DefaultSpeed = NewDefaultSpeed;
	WalkSpeed = NewDefaultSpeed;
	CrouchSpeed = NewDefaultSpeed - 100;
	SprintSpeed = NewDefaultSpeed + 300;
}


void UCharacterStatsComponent::OnRep_Life()
{
	OnLifeChanged.Broadcast(Life);
}

void UCharacterStatsComponent::OnRep_CurrentSanity()
{
	OnSanityChanged.Broadcast(CurrentSanity);
}

void UCharacterStatsComponent::OnRep_DefaultSpeed()
{
	OnMoveSpeedChanged.Broadcast(DefaultSpeed);
}
