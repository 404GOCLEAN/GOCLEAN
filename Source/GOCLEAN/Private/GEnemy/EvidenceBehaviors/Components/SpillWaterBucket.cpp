#include "GEnemy/EvidenceBehaviors/Components/SpillWaterBucket.h"
#include "GObjectSystem/Server/GObjectManager.h"
#include "GObjectSystem/GNonfixedObject.h"

void USpillWaterBucket::ExecuteBehavior(AActor* GhostActor)
{
	if (GhostActor == nullptr) return;

	UE_LOG(LogTemp, Warning, TEXT("SpillWaterBucket Implemented"));


	// get object manager
	auto ObjManager = GetWorld()->GetSubsystem<UGObjectManager>();
	if (!IsValid(ObjManager)) return;

	TArray<int32> BucketList = ObjManager->GetBucketIndices();
	if (BucketList.IsEmpty()) return;


	// find the closest bucket
	AGNonfixedObject* ClosestBucket = nullptr;

	float MinDistSquared = TNumericLimits<float>::Max();
	FVector GhostLocation = GhostActor->GetActorLocation();

	for (auto BucketIndex : BucketList)
	{
		auto Bucket = ObjManager->GetNonfixedObject(BucketIndex);
		if (IsValid(Bucket))
		{
			float CurrentDistSquared = FVector::DistSquared(GhostLocation, Bucket->GetActorLocation());
			if (CurrentDistSquared < MinDistSquared)
			{
				MinDistSquared = CurrentDistSquared;
				ClosestBucket = Bucket;
			}
		}
	}


	// apply force to the closest bucket
	if (ClosestBucket)
    {
        UPrimitiveComponent* PrimitiveComp = Cast<UPrimitiveComponent>(ClosestBucket->GetRootComponent());

        if (PrimitiveComp && PrimitiveComp->IsSimulatingPhysics())
        {
            FVector PushDirection = (ClosestBucket->GetActorLocation() - GhostLocation).GetSafeNormal();

            // 테스트 후 수치 조절할 것
            PushDirection.Z += 0.3f;
            PushDirection.Normalize();

			// 테스트 후 수치 조절
            float PushStrength = 800.0f;

            // add force
            PrimitiveComp->AddImpulse(PushDirection * PushStrength, NAME_None, true);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Bucket's state is not simulate physics!"));
        }
    }
}