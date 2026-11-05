// Fill out your copyright notice in the Description page of Project Settings.


#include "GObjectSystem/GExocismComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GEnemy/Components/DrownedGhost.h"
#include "GEnemy/Components/LittleGhost.h"
#include "GEnemy/Components/MaidenGhost.h"

#include "GObjectSystem/GFixedObject.h"
#include "GObjectSystem/GNonfixedObject.h"
#include "GObjectSystem/Server/GObjectManager.h"

#include "Components/SphereComponent.h"
#include "Components/DecalComponent.h"

#include "GCharacter/GOCLEANCharacter.h"
#include "GPlayerSystem/GEquipment/GEquipmentComponent.h"

#include "ServerModule/GameSession/InGameGameState.h"
#include "ServerModule/GameSession/InGameGameMode.h"

#include "Net/UnrealNetwork.h"


UGExocismComponent::UGExocismComponent()
{
    SetIsReplicatedByDefault(true);
}

void UGExocismComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UGExocismComponent, bIsFailed);
    DOREPLIFETIME(UGExocismComponent, bIsActiveExcosim);
}

void UGExocismComponent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
    // 0. Owner Actor 지정
    OwnerActor = Cast<AGFixedObject>(GetOwner());

    if (!IsValid(OwnerActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot found Owner exocism circle!"));
        return;
    }

    if (OwnerActor->HasAuthority())
    {
        return;
    }
}

void UGExocismComponent::BeginPlay()
{
    auto* _Owner = GetOwner();

    Decal = _Owner->GetComponentByClass<UDecalComponent>();


    InteractionVolume = _Owner->GetComponentByClass<USphereComponent>();
    if (IsValid(InteractionVolume))
    {
        InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        InteractionVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
        InteractionVolume->SetHiddenInGame(true);
        InteractionVolume->SetSimulatePhysics(false);
    }



    // 1. 귀신 종류에 따라 조건 함수 설정
    {
        AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), AGhostBase::StaticClass());

        AGhostBase* FoundGhost = Cast<AGhostBase>(FoundActor);
        if (!IsValid(FoundGhost))
        {
            UE_LOG(LogTemp, Warning, TEXT("Cannot found Ghost!"));
            return;
        }

        StartCondition.Clear();

        // Case 1. 수살귀
        if (ADrownedGhost* Drowned = Cast<ADrownedGhost>(FoundGhost))
        {
            UE_LOG(LogTemp, Log, TEXT("Ghost type - DrownedGhost"));

            StartCondition.AddUObject(this, &UGExocismComponent::ExocismStep0_DrownGhost);
        }
        // Case 2. 손각시
        else if (AMaidenGhost* Maiden = Cast<AMaidenGhost>(FoundGhost))
        {
            UE_LOG(LogTemp, Log, TEXT("Ghost type - MaidenGhost"));

            StartCondition.AddUObject(this, &UGExocismComponent::ExocismStep0_MaidenGhost);
        }
        // Case 3. 태자귀
        else if (ALittleGhost* Little = Cast<ALittleGhost>(FoundGhost))
        {
            UE_LOG(LogTemp, Log, TEXT("Ghost type - LittleGhost"));

            StartCondition.AddUObject(this, &UGExocismComponent::ExocismStep0_LittleGhost);
        }
    }


    // 2. deactive 상태로 초기화
    DeactivateExocismCircle();
}

void UGExocismComponent::OnInteractionTriggered(AGOCLEANCharacter* Target)
{
    if (!IsValid(Target) || !Target->GetEquipComp()) return;

    AGNonfixedObject* HeldObj = Target->GetEquipComp()->GetCurrentHeldObject();
    if (!HeldObj) return;


    UGEXToolComponent* EXToolComp = HeldObj->GetComponentByClass< UGEXToolComponent>();
    if (EXToolComp)
    {
        OnExocismCircleInteracted.Broadcast(Target);
    }
}



// Exocism circle default logic
void UGExocismComponent::ActivateExocismCircle()
{
    bIsFailed = false;

    bIsActiveExcosim = true;
    OnRep_bIsActiveExcosim();

    // 바운딩박스 활성화
    if (IsValid(InteractionVolume))
    {
        InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Block);
    }


    StartCondition.Broadcast(nullptr);


    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,                 // 메시지 키
            5.0f,               // 표시 시간
            FColor::Red,      // 글자 색상
            FString::Printf(TEXT("[Exocism] circle active!"))
        );
    }
}

void UGExocismComponent::DeactivateExocismCircle()
{
    bIsActiveExcosim = false;
    OnRep_bIsActiveExcosim();

    // 바운딩박스 비활성화
    if (IsValid(InteractionVolume))
    {
        InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Ignore);
    }
}

void UGExocismComponent::OnSuccessedInExocism()
{
    // GameState에서 퇴마 프로세스 종료 처리
    AInGameGameMode* GameMode = Cast<AInGameGameMode>(GetWorld()->GetAuthGameMode());
    if (IsValid(GameMode))
    {
        GameMode->ChangeExorcismProgress(SuccessProgress);
    }
}

void UGExocismComponent::OnFailedInExocism()
{
    auto* ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
    if (!IsValid(ObjectManager)) return;

    ObjectManager->ActivateExocismCircle(OwnerActor);

    bIsFailed = true;
    OnRep_bIsFailed();

    DeactivateExocismCircle();
}

void UGExocismComponent::OnFinishExocism()
{
    // 바운딩박스 비활성화
    if (IsValid(InteractionVolume))
    {
        InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Ignore);
    }
}



// 수살귀
void UGExocismComponent::ExocismStep0_DrownGhost(AGOCLEANCharacter* Target)
{
    OnExocismCircleInteracted.Clear();
    OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep1_DrownGhost);

    NextTargetTID = "Obj_FPot";

    SuccessProgress = 50.0f;
}

// step1. 백자 달항아리 배치
void UGExocismComponent::ExocismStep1_DrownGhost(AGOCLEANCharacter* Target)
{
    if (!IsValid(Target))
    {
        return;
    }

    FName TargetEquipID = Target->GetEquipComp()->GetCurrentHeldObjectName();

    if (TargetEquipID != NextTargetTID)
    {
        OnFailedInExocism();
    }
    else
    {
        OnSuccessedInExocism();


        // 연출 - 달 항아리 위치 옮기기


        // 로직
        NextTargetTID = "Obj_TAmulet";

        OnExocismCircleInteracted.Clear();
        OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep2_DrownGhost);
    }
}

// step2. 부적 상호작용
void UGExocismComponent::ExocismStep2_DrownGhost(AGOCLEANCharacter* Target)
{
    if (!IsValid(Target))
    {
        return;
    }

    FName TargetEquipID = Target->GetEquipComp()->GetCurrentHeldObjectName();

    if (TargetEquipID != NextTargetTID)
    {
        OnFailedInExocism();
    }
    else
    {
        OnSuccessedInExocism();


        // 연출 - 달 항아리 부적 붙음 메시로 변경


        OnExocismCircleInteracted.Clear();
    }
}



// 손각시
void UGExocismComponent::ExocismStep0_MaidenGhost(AGOCLEANCharacter* Target)
{
    OnExocismCircleInteracted.Clear();
    OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep1_MaidenGhost);

    NextTargetTID = "Obj_FDress";

    SuccessProgress = 50.0f;
}

// step1. 흰 소복 배치
void UGExocismComponent::ExocismStep1_MaidenGhost(AGOCLEANCharacter* Target)
{
    if (!IsValid(Target))
    {
        return;
    }

    FName TargetEquipID = Target->GetEquipComp()->GetCurrentHeldObjectName();

    if (TargetEquipID != NextTargetTID)
    {
        OnFailedInExocism();
    }
    else
    {
        OnSuccessedInExocism();


        // 연출 - 흰 소복 위치 옮기기


        // 로직
        NextTargetTID = "Obj_TPile";

        OnExocismCircleInteracted.Clear();
        OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep2_MaidenGhost);
    }
}

// step2. 쇠 말뚝 상호작용
void UGExocismComponent::ExocismStep2_MaidenGhost(AGOCLEANCharacter* Target)
{
    if (!IsValid(Target))
    {
        return;
    }

    FName TargetEquipID = Target->GetEquipComp()->GetCurrentHeldObjectName();

    if (TargetEquipID != NextTargetTID)
    {
        OnFailedInExocism();
    }
    else
    {
        OnSuccessedInExocism();


        // 연출 - 지정된 위치에 쇠말뚝 생성


        // 로직
        OnExocismCircleInteracted.Clear();
    }
}



// 태자귀
void UGExocismComponent::ExocismStep0_LittleGhost(AGOCLEANCharacter* Target)
{
    OnExocismCircleInteracted.Clear();
    OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep1_LittleGhost);

    NextTargetTID = "Cursed_BambooTube";

    SuccessProgress = 50.0f;
}

// step1.
void UGExocismComponent::ExocismStep1_LittleGhost(AGOCLEANCharacter* Target)
{
    if (!IsValid(Target))
    {
        return;
    }

    FName TargetEquipID = Target->GetEquipComp()->GetCurrentHeldObjectName();

    if (TargetEquipID != NextTargetTID)
    {
        OnFailedInExocism();
    }
    else
    {
        OnSuccessedInExocism();


        // 연출 - 대나무 통 위치 옮기기


        // 로직
        TargetEquipID = "Obj_Extool_ChickenBloodBowl";

        OnExocismCircleInteracted.Clear();
        OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep2_DrownGhost);
    }
}

// step2.
void UGExocismComponent::ExocismStep2_LittleGhost(AGOCLEANCharacter* Target)
{
    if (!IsValid(Target))
    {
        return;
    }

    FName TargetEquipID = Target->GetEquipComp()->GetCurrentHeldObjectName();

    if (TargetEquipID != NextTargetTID)
    {
        OnFailedInExocism();
    }
    else
    {
        OnSuccessedInExocism();


        // 연출 - 


        // 로직
        OnExocismCircleInteracted.Clear();
    }
}



// replication func
void UGExocismComponent::OnRep_bIsFailed()
{
    if (IsValid(Decal) && bIsFailed)
    {
        Decal->SetMaterial(0, FailDecal);
        Decal->SetVisibility(true);
    }
}

void UGExocismComponent::OnRep_bIsActiveExcosim()
{
    if (IsValid(Decal))
    {
        if (bIsActiveExcosim)
        {
            Decal->SetMaterial(0, SuccessDecal);
            Decal->SetVisibility(true);
        }
        else if (bIsFailed)
        {
            Decal->SetMaterial(0, FailDecal);
            Decal->SetVisibility(true);
        }
        else
        {
            Decal->SetVisibility(false);
        }
    }
}
