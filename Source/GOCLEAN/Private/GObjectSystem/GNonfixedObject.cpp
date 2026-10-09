// Fill out your copyright notice in the Description page of Project Settings.

#include "GObjectSystem/GNonfixedObject.h"

#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

#include "GTypes/DataTableRow/GObjectDataRow.h"
#include "GDataManagerSubsystem.h"
#include "GObjectSystem/GNonfixedObjCoreComponent.h"
#include "GObjectSystem/GAdditionalObjFuncComponent.h"
#include "GObjectSystem/Server/GObjectManager.h"
#include "GCharacter/GOCLEANCharacter.h"

#include "../../GOCLEAN.h"

#include "ServerModule/GameSession/GameSessionState.h"



AGNonfixedObject::AGNonfixedObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	bReplicates = true;
	SetReplicateMovement(true);


	RootPrimitive = CreateDefaultSubobject<UStaticMeshComponent>("RootPrimitive");
	SetRootComponent(RootPrimitive);


	InteractionVolume = CreateDefaultSubobject<UBoxComponent>("InteractionVolume");
	InteractionVolume->SetupAttachment(RootPrimitive);

	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionVolume->SetHiddenInGame(true);

	InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Block);
	InteractionVolume->SetSimulatePhysics(false);


	CoreComp = CreateDefaultSubobject<UGNonfixedObjCoreComponent>("CoreComponent");

}

void AGNonfixedObject::UpdateInteractionBounds()
{
	if (!RootPrimitive) return;

	FBoxSphereBounds Bounds = RootPrimitive->Bounds;

	InteractionVolume->SetBoxExtent(Bounds.BoxExtent);
	InteractionVolume->SetRelativeLocation(FVector(0, 0, Bounds.BoxExtent.Z));
}


void AGNonfixedObject::BeginPlay()
{
	Super::BeginPlay();

	UpdateInteractionBounds();
}


void AGNonfixedObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}



void AGNonfixedObject::UpdateVisualByState()
{
	if (!CoreComp) return;
	if (!RootPrimitive) return;

	switch (CoreComp->GetNonfixedObjState())
	{
	case ENonfixedObjState::E_Static:

		SetActorHiddenInGame(false);

		break;


	case ENonfixedObjState::E_Picked:

		SetActorHiddenInGame(false);

		break;

	case ENonfixedObjState::E_Kinematic:

		SetActorHiddenInGame(false);

		break;

	case ENonfixedObjState::E_Fixed:
		
		SetActorHiddenInGame(false);

		break;

	case ENonfixedObjState::E_Invisible:

		SetActorHiddenInGame(true);

		break;

	case ENonfixedObjState::E_Disintegrating:

		SetActorHiddenInGame(false);	
		
		FVector NewScale = GetActorScale3D() * 0.45f;
		SetActorScale3D(NewScale);

		break;

	case ENonfixedObjState::E_Destroyed:

		SetActorHiddenInGame(true);

		break;

	case ENonfixedObjState::E_Temporary:

		SetActorHiddenInGame(true);

		break;

	default:

		SetActorHiddenInGame(true);

		break;
	}
}
void AGNonfixedObject::UpdatePhysicsByState()
{
	if (!CoreComp) return;
	if (!RootPrimitive) return;

	UGDataManagerSubsystem* DataManager;
	const FGObjectDataRow* Data;
	AGameSessionState* GameState;
	UGObjectManager* ObjectManager;

	switch (CoreComp->GetNonfixedObjState())
	{
	case ENonfixedObjState::E_Static:
		RootPrimitive->SetSimulatePhysics(true);
		RootPrimitive->SetEnableGravity(true);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RootPrimitive->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Ignore);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Block);
		}

		break;


	case ENonfixedObjState::E_Picked:
		RootPrimitive->SetSimulatePhysics(false);
		RootPrimitive->SetEnableGravity(false);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		break;

	case ENonfixedObjState::E_Kinematic:
		RootPrimitive->SetSimulatePhysics(true);
		RootPrimitive->SetEnableGravity(true);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RootPrimitive->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Ignore);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Block);
		}

		break;

	case ENonfixedObjState::E_Fixed:
		RootPrimitive->SetSimulatePhysics(false);
		RootPrimitive->SetEnableGravity(false);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RootPrimitive->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Ignore);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Block);
		}

		break;

	case ENonfixedObjState::E_Invisible:
		RootPrimitive->SetSimulatePhysics(false);
		RootPrimitive->SetEnableGravity(false);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		break;

	case ENonfixedObjState::E_Disintegrating:
		RootPrimitive->SetSimulatePhysics(true);
		RootPrimitive->SetEnableGravity(true);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RootPrimitive->SetCollisionObjectType(EEC_GIncinerator);
		RootPrimitive->SetCollisionResponseToAllChannels(ECR_Ignore);
		RootPrimitive->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		RootPrimitive->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		RootPrimitive->SetCollisionResponseToChannel(EEC_GIncinerator, ECR_Block);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		break;

	case ENonfixedObjState::E_Destroyed:
		RootPrimitive->SetSimulatePhysics(false);
		RootPrimitive->SetEnableGravity(false);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}


		// reduce spirit guage
		DataManager = UGDataManagerSubsystem::Get(GetWorld());
		Data = DataManager ? DataManager->GetObjectData(CoreComp->TID) : nullptr;
		if (!Data) break;

		GameState = Cast<AGameSessionState>(GetWorld()->GetGameState());
		if (!GameState) break;

		//GameState->ApplySpiritualOrRestGauge(Data->Pollution);


		// 대형 폐기물의 경우, Destroyed Queue에 삽입하여 귀신 단서 행동에서 사용할 수 있도록 함.
		ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
		if (ObjectManager)
		{
			ObjectManager->OnDestoyed(this, Data);
		}


		break;

	case ENonfixedObjState::E_Temporary:
		RootPrimitive->SetSimulatePhysics(false);
		RootPrimitive->SetEnableGravity(false);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		break;

	default:
		RootPrimitive->SetSimulatePhysics(false);
		RootPrimitive->SetEnableGravity(false);

		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (InteractionVolume)
		{
			InteractionVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		break;
	}
}



void AGNonfixedObject::UpdateObjectData(int32 IID)
{
	if (!CoreComp) return;

	CoreComp->IID = IID;
	UpdateInteractionBounds();

	FGNonfixedObjData Temp;

	TArray<UGAdditionalObjFuncComponent*> FuncComps;
	GetComponents<UGAdditionalObjFuncComponent>(FuncComps);
	for (auto FuncComp : FuncComps)
	{
		FuncComp->InitializeAdditionalData(Temp);
	}
}

void AGNonfixedObject::Multicast_OnPickedUp_Implementation(AGOCLEANCharacter* TargetCharacter)
{
	if (!TargetCharacter) return;

	GetStaticMeshComp()->SetSimulatePhysics(false);
	GetStaticMeshComp()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AttachToComponent(TargetCharacter->GetHandMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Charater001_Bip001-R-HandSocket"));

	TargetCharacter->SetHeldObjectRelativeTransform(this);
}




void AGNonfixedObject::SetObjectData(FGNonfixedObjData& InitData)
{
	auto* DataManager = GetWorld()->GetGameInstance()->GetSubsystem<UGDataManagerSubsystem>();
	auto* Data = DataManager ? DataManager->GetObjectData(InitData.TID) : nullptr;
	if (!Data)
	{
		UE_LOG(LogGObject, Warning, TEXT("[NonfixedObject] Can not find matched object data!: %s"), *InitData.TID.ToString());
		return;
	}


	// set mesh
	if (Data->SM_MeshAssets.Num() != 0)
	{
		UStaticMesh* LoadedMesh = Data->SM_MeshAssets[FMath::RandRange(0, Data->SM_MeshAssets.Num() - 1)].LoadSynchronous();

		if (LoadedMesh)
		{
			RootPrimitive->SetStaticMesh(LoadedMesh);
		}
		else
		{
			UE_LOG(LogGObject, Error, TEXT("[NonfixedObject] Mesh load failed!"));
		}
	}


	if (!HasAuthority()) return;

	UpdateInteractionBounds();

	// set init data
	CoreComp->IID = InitData.IID;
	CoreComp->TID = InitData.TID;

	// attach component with category
	switch (Data->Category)
	{
	case EGObjectCategory::E_Trash_S:
	{

		auto* PickComp = NewObject<UGPickComponent>(this, UGPickComponent::StaticClass(), TEXT("PickComponent"));
		if (PickComp)
		{
			PickComp->RegisterComponent();
			PickComp->InitializeAdditionalData(InitData);
		}

		auto* BurningComp = NewObject<UGBurningCompopnent>(this, UGBurningCompopnent::StaticClass(), TEXT("BurningComponent"));
		if (BurningComp)
		{
			BurningComp->RegisterComponent();
			BurningComp->InitializeAdditionalData(InitData);
		}

		auto* SoundComp = NewObject<UGInteractSoundCompopnent>(this, UGInteractSoundCompopnent::StaticClass(), TEXT("SoundComponent"));
		if (SoundComp)
		{
			SoundComp->RegisterComponent();
			SoundComp->InitializeAdditionalData(InitData);
		}
	}

		break;

	case EGObjectCategory::E_Equip:
	{

		auto* PickComp = NewObject<UGPickComponent>(this, UGPickComponent::StaticClass(), TEXT("PickComponent"));
		if (PickComp)
		{
			PickComp->RegisterComponent();
			PickComp->InitializeAdditionalData(InitData);
		}
	}

		break;

	case EGObjectCategory::E_Item_F:
		break;

	case EGObjectCategory::E_Item_P:
	{
		auto* PickComp = NewObject<UGPickComponent>(this, UGPickComponent::StaticClass(), TEXT("PickComponent"));
		if (PickComp)
		{
			PickComp->RegisterComponent();
			PickComp->InitializeAdditionalData(InitData);
		}
	}

		break;

	case EGObjectCategory::E_Fetish_N:
	{
		auto* PickComp = NewObject<UGPickComponent>(this, UGPickComponent::StaticClass(), TEXT("PickComponent"));
		if (PickComp)
		{
			PickComp->RegisterComponent();
			PickComp->InitializeAdditionalData(InitData);
		}

		auto* SoundComp = NewObject<UGInteractSoundCompopnent>(this, UGInteractSoundCompopnent::StaticClass(), TEXT("SoundComponent"));
		if (SoundComp)
		{
			SoundComp->RegisterComponent();
			SoundComp->InitializeAdditionalData(InitData);
		}
	}

		break;

	default:
		break;
	}


	CoreComp->ChangeState(InitData.SpawnState);
}


