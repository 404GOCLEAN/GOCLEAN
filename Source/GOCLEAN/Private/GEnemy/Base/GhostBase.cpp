#include "GEnemy/Base/GhostBase.h"
#include "GEnemy/CommonBehaviors/Components/PlayCommonSound.h"
#include "GEnemy/CommonBehaviors/Components/Manifest.h"
#include "GEnemy/CommonBehaviors/Components/CloseDoor.h"
#include "GEnemy/CommonBehaviors/Components/FlashlightBreakdown.h"
#include "GEnemy/CommonBehaviors/Components/PlayFootstepSound.h"
#include "GEnemy/GhostAIController.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "ServerModule/GameSession/PlayerSessionState.h"


AGhostBase::AGhostBase()
{
	PrimaryActorTick.bCanEverTick = true;

	StatsComp = CreateDefaultSubobject<UGhostStatsComponent>(TEXT("GhostStats"));

	// Default variables
	BehaviorEventCycleDelay = 15.0f;
	bCanSetBehaviourEventCycleTimer = true;
	CurrentPatrolIndex = 0;

	// Sound setup
	RageLoopAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RageAudio"));
	RageLoopAudio->SetupAttachment(GetRootComponent());
	RageLoopAudio->bAutoActivate = false;

	ChaseLoopAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ChaseAudio"));
	ChaseLoopAudio->SetupAttachment(GetRootComponent());
	ChaseLoopAudio->bAutoActivate = false;
}


// Overrided //
void AGhostBase::BeginPlay()
{
	Super::BeginPlay();

	GetMesh()->SetHiddenInGame(true);

	// AI Controller
	GhostAIController = Cast<AGhostAIController>(GetController());

	// Add Common behaviors
	CommonBehaviors.Add(NewObject<UPlayCommonSound>(this));
	CommonBehaviors.Add(NewObject<UManifest>(this));
	CommonBehaviors.Add(NewObject<UCloseDoor>(this));
	CommonBehaviors.Add(NewObject<UFlashlightBreakdown>(this));
	CommonBehaviors.Add(NewObject<UPlayFootstepSound>(this));

	// Init default value
	GetCharacterMovement()->MaxWalkSpeed = StatsComp->GetBaseMovementSpeed();
	// StatsComp->InitActivityLevel();

	// Sound
	if (RageLoopAudio && RageCue)
	{
		RageLoopAudio->SetSound(RageCue);
	}

	if (ChaseLoopAudio && ChaseCue)
	{
		ChaseLoopAudio->SetSound(ChaseCue);
	}

	if (!HasAuthority()) return;

	// Init BehaviorEventCycle
	GetWorldTimerManager().SetTimer(GhostBehaviorEventCycleHandle, this, &AGhostBase::EvaluateBehaviorEventCondition, BehaviorEventCycleDelay, true);

	// Specific event
	GetWorldTimerManager().SetTimer(CheckPlayerSanityHalfReachedHandle, this, &AGhostBase::CheckPlayerSanityHalfReached, 1.0f, true);
}

int32 AGhostBase::GetRageModifier()
{
	return StatsComp->GetRageModifier();
}

float AGhostBase::GetRageCooldown()
{
	return StatsComp->GetRageCooldown();
}

void AGhostBase::Multicast_PlayCommonEventSound_Implementation()
{
	if (CommonSound == nullptr || CommonEventAttenuation == nullptr) return;

	UGameplayStatics::PlaySoundAtLocation(this, CommonSound, GetActorLocation(), 1.0f, 1.0f, 0.0f, CommonEventAttenuation);
}

void AGhostBase::Multicast_PlayFootstepSound_Implementation()
{
	if (FootstepSound == nullptr || CommonEventAttenuation == nullptr) return;

	UGameplayStatics::PlaySoundAtLocation(this, FootstepSound, GetActorLocation(), 1.0f, 1.0f, 0.0f, CommonEventAttenuation);
}

void AGhostBase::PlayRageSound()
{
	UE_LOG(LogTemp, Warning, TEXT("Rage Play"));

	RageLoopAudio->Play();
}

void AGhostBase::StopRageSound()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop"));

	RageLoopAudio->FadeOut(0.5f, 0.f);
}
void AGhostBase::PlayChaseSound()
{
	UE_LOG(LogTemp, Warning, TEXT("Rage Play"));

	ChaseLoopAudio->Play();
}

void AGhostBase::StopChaseSound()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop"));
	ChaseLoopAudio->FadeOut(0.5f, 0.f);
}

void AGhostBase::Multicast_PlayRageSound_Implementation()
{
	if (RageLoopAudio && !RageLoopAudio->IsPlaying())
	{
		UE_LOG(LogTemp, Warning, TEXT("Multicast Rage Play"));
		RageLoopAudio->Play();
	}
}

void AGhostBase::Multicast_StopRageSound_Implementation()
{
	if (RageLoopAudio && RageLoopAudio->IsPlaying())
	{
		UE_LOG(LogTemp, Warning, TEXT("Multicast Rage Stop"));
		RageLoopAudio->FadeOut(2.0f, 0.f);
	}
}

void AGhostBase::Multicast_PlayChaseSound_Implementation()
{
	if (ChaseLoopAudio && !ChaseLoopAudio->IsPlaying())
	{
		UE_LOG(LogTemp, Warning, TEXT("Multicast Chase Play"));
		ChaseLoopAudio->Play();
	}
}

void AGhostBase::Multicast_StopChaseSound_Implementation()
{
	if (ChaseLoopAudio && ChaseLoopAudio->IsPlaying())
	{
		UE_LOG(LogTemp, Warning, TEXT("Multicast Chase Stop"));
		ChaseLoopAudio->FadeOut(2.0f, 0.f);
	}
}

void AGhostBase::Multicast_PlayOnHuntedSound_Implementation()
{
	UGameplayStatics::PlaySoundAtLocation(
		this,
		OnHuntedCue,
		GetActorLocation()
	);
}

void AGhostBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


// Behavior event //
void AGhostBase::EvaluateBehaviorEventCondition()
{
	if (GhostAIController == nullptr) return;

	if (GhostAIController->CheckBehaviorEventCondition())
		PerformBehaviorEvent();
	else return;
}

void AGhostBase::PerformBehaviorEvent()
{
	int32 RandEventNum;
	int32 RandEvidenceBehaviorNum;
	int32 RandCommonBehaviorNum;
	RandEventNum = FMath::RandRange(0, 8);

	UE_LOG(LogTemp, Warning, TEXT("---------------------------------------------------------------"));

	UE_LOG(LogTemp, Warning, TEXT("RandEventNumResult: %d"), RandEventNum);

	if (0 <= RandEventNum && RandEventNum <= 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Perform Evidence Behavior]"));
		
		RandEvidenceBehaviorNum = FMath::RandRange(0, EvidenceBehaviors.Num() - 1);
		if (EvidenceBehaviors[RandEvidenceBehaviorNum] == nullptr) return;
		EvidenceBehaviors[RandEvidenceBehaviorNum]->ExecuteBehavior(this);
	}
	else if (4 <= RandEventNum && RandEventNum <= 5)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Perform Common Behavior]"));
		
		RandCommonBehaviorNum = FMath::RandRange(0, CommonBehaviors.Num() - 1);
		if (CommonBehaviors[RandCommonBehaviorNum] == nullptr) return;
		CommonBehaviors[RandCommonBehaviorNum]->ExecuteBehavior(this);
	}
	else if (6 <= RandEventNum && RandEventNum <= 7)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Sustain Patrol State]"));
	}
	else if (RandEventNum == 8)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Stop]"));
	}
}

void AGhostBase::CheckPlayerSanityHalfReached()
{
	if (GhostAIController == nullptr) return;
	
	GhostAIController->UpdateAlivePlayerList();

	for (AGOCLEANCharacter* PlayerCharacter : GhostAIController->AlivePlayers)
	{
		if (PlayerCharacter == nullptr) continue;

		APlayerSessionState* PSS = PlayerCharacter->GetPlayerState<APlayerSessionState>();
		if (PSS == nullptr) continue;

		if (SanityHalfTriggeredPlayers.Contains(PSS)) continue;

		if (PlayerCharacter->GetPlayerCurrentSanity() > 50.0f) continue;

		SanityHalfTriggeredPlayers.Add(PSS);
		OnPlayerSanityHalfReached(PSS);
	}
}


// Server //
void AGhostBase::Server_RequestSetVisible_Implementation(bool IsVisible)
{
	Multicast_SetVisible(IsVisible);
}
void AGhostBase::Multicast_SetVisible_Implementation(bool IsVisible)
{
	GetMesh()->SetHiddenInGame(!IsVisible);
}

void AGhostBase::NotifyUnendingRageStarted()
{
	if (!HasAuthority()) return;

	OnUnendingRageStarted();
}
