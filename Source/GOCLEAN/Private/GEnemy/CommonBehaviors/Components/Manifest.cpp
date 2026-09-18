#include "GEnemy/CommonBehaviors/Components/Manifest.h"
#include "GEnemy/Base/GhostBase.h"

void UManifest::ExecuteBehavior(AActor* GhostActor)
{
	if (GhostActor == nullptr) return;
	AGhostBase* GhostCharacter = Cast<AGhostBase>(GhostActor);
	if (GhostCharacter == nullptr) return;

	if (!GhostCharacter->HasAuthority()) return;

	TSubclassOf<AActor> ManifestActorClass = GhostCharacter->GetManifestActorClass();
	if (ManifestActorClass == nullptr) return;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* ManifestActor = GhostCharacter->GetWorld()->SpawnActor<AActor>(ManifestActorClass, GhostCharacter->GetActorTransform(), SpawnParameters);
	if (ManifestActor == nullptr) return;

	ManifestActor->SetLifeSpan(3.0f);

	UE_LOG(LogTemp, Warning, TEXT("Manifest Implemented"));
}