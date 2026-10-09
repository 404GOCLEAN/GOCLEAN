#include "GEnemy/CommonBehaviors/Components/PlayCommonSound.h"

void UPlayCommonSound::ExecuteBehavior(AActor* GhostActor)
{
	AGhostBase* GhostCharacter = Cast<AGhostBase>(GhostActor);
	if (GhostCharacter == nullptr) return;

	if (!GhostCharacter->HasAuthority()) return;

	GhostCharacter->Multicast_PlayCommonEventSound();

	UE_LOG(LogTemp, Warning, TEXT("PlayCommonSound Implemented"));
}