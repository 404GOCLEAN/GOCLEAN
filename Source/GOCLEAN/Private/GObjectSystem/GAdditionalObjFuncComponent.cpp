// Fill out your copyright notice in the Description page of Project Settings.


#include "GObjectSystem/GAdditionalObjFuncComponent.h"

#include "Components/DecalComponent.h"

#include "GTypes/IGInteractable.h"
#include "GTypes/DataTableRow/GEquipmentDataRow.h"
#include "GObjectSystem/GNonfixedObjCoreComponent.h"
#include "GObjectSystem/GNonfixedObject.h"
#include "GObjectSystem/GFixedObject.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "GPlayerSystem/GEquipment/GEquipmentComponent.h"
#include "GDataManagerSubsystem.h"
#include "GObjectSystem/Server/GObjectManager.h"
#include "../../GOCLEAN.h"

#include "Kismet/GameplayStatics.h"



UGAdditionalObjFuncComponent::UGAdditionalObjFuncComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UGAdditionalObjFuncComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogGObject, Log, TEXT("[AdditionalFuncComp] Begin Play %s: Add delegate funcs"), *GetOwner()->GetName());

	TArray<UActorComponent*> InteractComps =
		GetOwner()->GetComponentsByInterface(UGInteractable::StaticClass());
	if (InteractComps.Num() > 0)
	{
		if (UGNonfixedObjCoreComponent* CoreComp = Cast<UGNonfixedObjCoreComponent>(InteractComps[0]))
		{
			CoreComp->GetOnInteractionDelegate().AddUObject(this, &UGAdditionalObjFuncComponent::OnInteractionTriggered);
			CoreComp->GetOnStateChangedDelegate().AddUObject(this, &UGAdditionalObjFuncComponent::OnStateChangeTriggered);
		}
	}
}



UGPickComponent::UGPickComponent()
{

}

void UGPickComponent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
	Super::InitializeAdditionalData(Data);

	bIsPickedUp = false;
	OwnerPlayer = nullptr;
}

void UGPickComponent::BeginPlay() 
{
	Super::BeginPlay();
}

void UGPickComponent::OnInteractionTriggered(AGOCLEANCharacter* Target)
{
	if (Target->GetEquipComp()->GetCurrentEquipmentID() != "Eq_Hand") return;

	PickUpObject(Target);
}

void UGPickComponent::PickUpObject(AGOCLEANCharacter* Target)
{
	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());
	if (!Owner) return;

	UGNonfixedObjCoreComponent* CoreComp = Owner->GetNonfixedObjCoreComp();
	if (!CoreComp) return;

	UGEquipmentComponent* EquipComp = Target->GetEquipComp();
	if (!EquipComp) return;

	UGDataManagerSubsystem* DataManager = UGDataManagerSubsystem::Get(GetWorld());
	const FGObjectDataRow* ObjData =
		DataManager ? DataManager->GetObjectData(CoreComp->TID) : nullptr;
	if (!ObjData)
	{
		UE_LOG(LogGObject, Warning, TEXT("[NonfixedObject] Can not found matched object data!: TID - %s"), *CoreComp->TID.ToString());
		return;
	}

	FName PickedEquipID = ObjData->PickedEquipID;
	const FGEquipmentDataRow* EquipData = DataManager->GetEquipmentData(PickedEquipID);
	if (!EquipData)
	{
		UE_LOG(LogGObject, Warning, TEXT("[NonfixedObject] Can not found picked equipment data!: TID - %s"), *CoreComp->TID.ToString());
		return;
	}


	// set target
	int32 MatchedSlotIndex = EquipData->MatchedSlotIndex;
	if (MatchedSlotIndex != EquipComp->GetCurrentSlotIndex())
	{
		if (!EquipComp->ChangeCurrentSlot(MatchedSlotIndex))
		{
			UE_LOG(LogGObject, Warning, TEXT("[NonfixedObject] Error in change current slot"));
			return;
		}
	}

	Target->SetHeldObject(Owner);
	EquipComp->ChangeEuquipmentInCurrSlot(PickedEquipID);


	// set owner
	Owner->GetNonfixedObjCoreComp()->ChangeState(ENonfixedObjState::E_Picked);


	// set my variables
	bIsPickedUp = true;
	OwnerPlayer = Target;


	// sync force server-client
	Owner->Multicast_OnPickedUp(Target);
}

void UGPickComponent::OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState)
{
	if (PrevState == ENonfixedObjState::E_Picked && ChangedState == ENonfixedObjState::E_Static)
	{
		DropObject();
	}
}

void UGPickComponent::DropObject()
{
	if (!OwnerPlayer) return;

	AGNonfixedObject* OwnerActor = Cast<AGNonfixedObject>(GetOwner());
	if (!OwnerActor) return;


	// get spawn data from owner player
	float DropDistance = 100.0f;
	FVector DropLocation = OwnerPlayer->GetActorLocation() + (OwnerPlayer->GetActorForwardVector() * DropDistance);

	DropLocation.Z = OwnerPlayer->GetActorLocation().Z;


	// set owner actor's
	OwnerActor->SetActorLocation(DropLocation);
	OwnerActor->SetActorRotation(OwnerPlayer->GetActorRotation());


	bIsPickedUp = false;
	OwnerPlayer = nullptr;
}



UGRemovingComponent::UGRemovingComponent()
{
	SetIsReplicatedByDefault(true);
}

void UGRemovingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 속한 데이터 중 일부를 '나'에게만 복제하고 싶을 때
	// DOREPLIFETIME_CONDITION(UGEquipmentComponent, CurrentSlotIndex, COND_OwnerOnly);
	DOREPLIFETIME(UGRemovingComponent, Decals);
	DOREPLIFETIME(UGRemovingComponent, CleaningRatio);
}

void UGRemovingComponent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
	Super::InitializeAdditionalData(Data);

	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());
	if (Owner && Owner->GetNonfixedObjCoreComp() && GetWorld())
	{
		UGDataManagerSubsystem* DataManager = UGDataManagerSubsystem::Get(GetWorld());
		const FGObjectDataRow* Data =
			DataManager ? DataManager->GetObjectData(Owner->GetNonfixedObjCoreComp()->TID) : nullptr;

		if (Data)
		{
			InteractionMaxCnt = Data->InteractionFinCnt;
		}
	}
}

void UGRemovingComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGRemovingComponent::OnInteractionTriggered(AGOCLEANCharacter* Target)
{
	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());

	// equip check
	UGEquipmentComponent* EquipComp = Target->GetEquipComp();
	UGDataManagerSubsystem* DataManager = UGDataManagerSubsystem::Get(GetWorld());
	const FGObjectDataRow* Data =
		DataManager ? DataManager->GetObjectData(Owner->GetNonfixedObjCoreComp()->TID) : nullptr;
	if (!EquipComp || !Data) return;

	FName EquipID = EquipComp->GetCurrentEquipmentID();
	if (EquipID == "Eq_Hand" || !Data->MatchedEquipID.Contains(EquipID))
	{
		UE_LOG(LogGObject, Log, TEXT("[GObject] equipment is not matched with object type!: %s, %s"), *EquipID.ToString(), *Data->TID.ToString());
		return;
	}

	if (Owner && Owner->GetNonfixedObjCoreComp())
	{
		if (Owner->GetNonfixedObjCoreComp()->InteractionCnt < InteractionMaxCnt)
		{
			SetVisualByInteractionCnt(Owner, EquipComp, *Data);

			// 물걸레의 오염도를 증가
			// EquipComp->AddMopPollution(20.0f);
		}
		else
		{
			SetDestroyThisObject(Owner);
		}
	}
}

void UGRemovingComponent::SetVisualByInteractionCnt_Implementation(AGNonfixedObject* Owner, UGEquipmentComponent* EquipComp, const FGObjectDataRow& ObjData)
{
	if (!Owner || !EquipComp) return;

	if (ObjData.Category == EGObjectCategory::E_Filth)
	{
		// 데칼타입의 경우, 메시의 알파값을 감소
		if (InteractionMaxCnt <= 0) return;

		float _CleaningRatio = 1.0f - (Owner->GetNonfixedObjCoreComp()->InteractionCnt / (float)InteractionMaxCnt);
		CleaningRatio = FMath::Clamp(_CleaningRatio, 0.0f, 1.0f);
		OnRep_CleaningRatio();
	}
	else if (ObjData.Category == EGObjectCategory::E_Trash_B && BrokenMesh)
	{
		// 큰 쓰레기의 경우, 1/2 달성 시 broken 메시 활성화
		if (Owner->GetNonfixedObjCoreComp()->InteractionCnt * 2 >= InteractionMaxCnt)
		{
			Owner->GetStaticMeshComp()->SetStaticMesh(BrokenMesh);
		}
	}
}

void UGRemovingComponent::OnRep_CleaningRatio() {

	for (auto Decal : Decals)
	{
		if (!Decal) continue;

		UMaterialInstanceDynamic* DynMat = Decal->CreateDynamicMaterialInstance();
		if (DynMat)
		{
			DynMat->SetScalarParameterValue(TEXT("Opacity"), CleaningRatio);
			DynMat->SetScalarParameterValue(TEXT("Opacity Intensity"), CleaningRatio);
		}
	}
}

void UGRemovingComponent::SetDestroyThisObject(AGNonfixedObject* Owner)
{
	FVector SpawnLocation = Owner->GetActorLocation();
	FRotator SpawnRotation = Owner->GetActorRotation();

	Owner->GetNonfixedObjCoreComp()->ChangeState(ENonfixedObjState::E_Destroyed);

	if (DestroyedActor)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		auto* TempActor = GetWorld()->SpawnActor<AActor>(
			DestroyedActor,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);
	}
}




UGMultiInteractionComponent::UGMultiInteractionComponent()
{

}

void UGMultiInteractionComponent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
	Super::InitializeAdditionalData(Data);
}

void UGMultiInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
}




UGBurningCompopnent::UGBurningCompopnent()
{

}

void UGBurningCompopnent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
	Super::InitializeAdditionalData(Data);
}

void UGBurningCompopnent::BeginPlay()
{
	Super::BeginPlay();
}

void UGBurningCompopnent::OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState)
{
	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());
	if (!Owner) return;

	if (PrevState == ENonfixedObjState::E_Picked && ChangedState == ENonfixedObjState::E_Disintegrating)
	{
		StartBurning(Owner);
	}

	else if (PrevState == ENonfixedObjState::E_Disintegrating && ChangedState != ENonfixedObjState::E_Destroyed)
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(BurnTimerHandle))
		{
			GetWorld()->GetTimerManager().ClearTimer(BurnTimerHandle);
		}
	}
}

void UGBurningCompopnent::StartBurning(AGNonfixedObject* Owner)
{
	if (!Owner || !GetWorld()) return;


	// set object's transform
	auto ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
	if (ObjectManager && ObjectManager->GetIncineratorActor())
	{
		AGFixedObject* Incinerator = ObjectManager->GetIncineratorActor();


		FBox Bounds = Incinerator->GetComponentsBoundingBox();

		FVector DropLocation = Incinerator->GetActorLocation();
		DropLocation.Z = Bounds.Max.Z + 10.f;

		
		Owner->SetActorLocation(DropLocation);
		Owner->SetActorRotation(FRotator::ZeroRotator);


		Incinerator->OnCustomEvent_Bool(true);
	}


	// start timer
	GetWorld()->GetTimerManager().SetTimer(
		BurnTimerHandle,
		this,
		&UGBurningCompopnent::OnBurnTimerFinished,
		BurningInterval,
		false
	);
}

void UGBurningCompopnent::OnBurnTimerFinished()
{
	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());
	if (Owner && Owner->GetNonfixedObjCoreComp())
	{
		Owner->GetNonfixedObjCoreComp()->ChangeState(ENonfixedObjState::E_Destroyed);
	}
}



UGSpawnerCompopnent::UGSpawnerCompopnent()
{

}

void UGSpawnerCompopnent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
	Super::InitializeAdditionalData(Data);
}

void UGSpawnerCompopnent::BeginPlay()
{
	Super::BeginPlay();
}

void UGSpawnerCompopnent::OnStateChangeTriggered(ENonfixedObjState PrevState, ENonfixedObjState ChangedState)
{
	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());
	if (!Owner) return;

	if (ChangedState == ENonfixedObjState::E_Destroyed)
	{
		SpawnDerivedObject(Owner);
	}
}

void UGSpawnerCompopnent::SpawnDerivedObject(AGNonfixedObject* Owner)
{
	UWorld* World = GetWorld();
	if (!World) return;

	auto* DataManager = UGDataManagerSubsystem::Get(World);
	auto* Data = DataManager ? DataManager->GetObjectData(Owner->GetNonfixedObjCoreComp()->TID) : nullptr;
	if (!Data) return;

	auto* ObjectManager = World->GetSubsystem<UGObjectManager>();
	if (ObjectManager)
	{
		FVector BaseSpawnLocation = Owner->GetActorLocation();
		FRotator SpawnRotation = Owner->GetActorRotation();

		float RandomXYRadius = 50.0f;
		float ZOffsetStep = 5.0f;

		for (int32 i = 0; i < Data->DerivedObjCnt; ++i)
		{
			float RandX = (i == 0) ? 0.0f : FMath::RandRange(-RandomXYRadius, RandomXYRadius);
			float RandY = (i == 0) ? 0.0f : FMath::RandRange(-RandomXYRadius, RandomXYRadius);
			float OffsetZ = ZOffsetStep * i;

			FVector CurrentSpawnLocation = BaseSpawnLocation + FVector(RandX, RandY, OffsetZ);

			auto* SpawnedObj = ObjectManager->SpawnNonfixedObject(
				Data->DerivedObjID,
				ENonfixedObjState::E_Static,
				CurrentSpawnLocation,
				SpawnRotation);

			if (i == 0 && SpawnedObj)
			{
				FVector Origin, BoxExtent;
				SpawnedObj->GetActorBounds(true, Origin, BoxExtent);

				RandomXYRadius = FMath::Max(BoxExtent.X, BoxExtent.Y) * 1.5f;
			}
		}
	}
}




UGInteractSoundCompopnent::UGInteractSoundCompopnent()
{
	CachedInteractSound = nullptr;
}

void UGInteractSoundCompopnent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
	Super::InitializeAdditionalData(Data);

	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());
	if (Owner && Owner->GetNonfixedObjCoreComp() && GetWorld())
	{
		// 2. 데이터 매니저에서 TID 기반 데이터 가져오기
		UGDataManagerSubsystem* DataManager = UGDataManagerSubsystem::Get(GetWorld());
		const FGObjectDataRow* DataRow = DataManager ? DataManager->GetObjectData(Owner->GetNonfixedObjCoreComp()->TID) : nullptr;

		// 3. 사운드 에셋 캐싱
		if (DataRow && DataRow->InteractSoundAsset)
		{
			CachedInteractSound = DataRow->InteractSoundAsset;
		}
	}
}

void UGInteractSoundCompopnent::BeginPlay()
{
	Super::BeginPlay();
}

void UGInteractSoundCompopnent::OnInteractionTriggered(AGOCLEANCharacter* Target)
{
	if (CachedInteractSound && GetOwner())
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			CachedInteractSound,
			GetOwner()->GetActorLocation()
		);
	}
}




UGBucketComponent::UGBucketComponent()
{
	PrimaryComponentTick.bCanEverTick = true; // 기울기 체크를 위해 활성화
}

void UGBucketComponent::BeginPlay()
{
	Super::BeginPlay();

	auto ObjManager = GetWorld()->GetSubsystem<UGObjectManager>();
	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());

	if (Owner && Owner->GetNonfixedObjCoreComp() && ObjManager)
	{
		ObjManager->RegisterBucketIndex(Owner->GetNonfixedObjCoreComp()->IID);
	}
}

void UGBucketComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 손에 들려있는 상태가 아닐 때만 엎어짐 체크 (들고 있을 땐 손 각도에 따라다니므로 제외 가능)
	AGNonfixedObject* Owner = Cast<AGNonfixedObject>(GetOwner());
	if (Owner && Owner->GetNonfixedObjCoreComp()->GetState() != ENonfixedObjState::E_Picked)
	{
		CheckSpill();
	}
}

void UGBucketComponent::OnInteractionTriggered(AGOCLEANCharacter* Target)
{
	if (!Target) return;

	FName EquipID = Target->GetEquipComp()->GetCurrentEquipmentID();

	if (EquipID == "Eq_Mop")
	{
		UE_LOG(LogGObject, Log, TEXT("[Bucket] Cleaning the mop..."));
		Target->GetEquipComp()->AddMopPollution(-100.0f);

		// 여기에 물 출렁이는 사운드나 파티클을 추가하면 금상첨화!
	}
}

void UGBucketComponent::CheckSpill()
{
	if (bIsSpilled) return;

	// 액터의 UpVector.Z가 1.0이면 정방향, 0.0이면 90도 누운 상태입니다.
	float UpZ = GetOwner()->GetActorUpVector().Z;

	if (UpZ < SpillThreshold)
	{
		SpillFilth();
	}
}

void UGBucketComponent::SpillFilth()
{
	bIsSpilled = true;
	UE_LOG(LogGObject, Warning, TEXT("[Bucket] OOPS! Spilled!"));

	UWorld* World = GetWorld();
	if (!World) return;

	auto* ObjectManager = World->GetSubsystem<UGObjectManager>();
	if (ObjectManager)
	{
		// 양동이 위치 바닥에 Filth 스폰
		FVector SpawnLocation = GetOwner()->GetActorLocation();
		FRotator SpawnRotation = FRotator::ZeroRotator;

		ObjectManager->SpawnNonfixedObject(
			FilthTID,
			ENonfixedObjState::E_Static,
			SpawnLocation,
			SpawnRotation);
	}
}


