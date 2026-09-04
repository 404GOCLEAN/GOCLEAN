#include "GEnemy/CommonBehaviors/Components/PlayFootstepSound.h"

void UPlayFootstepSound::ExecuteBehavior(AActor* GhostActor)
{
	AGhostBase* GhostCharacter = Cast<AGhostBase>(GhostActor);
	if (GhostCharacter == nullptr) return;

	if (!GhostCharacter->HasAuthority()) return;

	GhostCharacter->Multicast_PlayFootstepSound();

	UE_LOG(LogTemp, Warning, TEXT("PlayFootstepSound Implemented"));
}