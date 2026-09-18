// Fill out your copyright notice in the Description page of Project Settings.


#include "GObjectSystem/GExocismComponent.h"

#include "Kismet/GameplayStatics.h"
#include "GEnemy/Components/DrownedGhost.h"
#include "GEnemy/Components/LittleGhost.h"
#include "GEnemy/Components/MaidenGhost.h"

#include "GObjectSystem/GFixedObject.h"
#include "GObjectSystem/Server/GObjectManager.h"

#include "Components/SphereComponent.h"

#include "GCharacter/GOCLEANCharacter.h"
#include "GPlayerSystem/GEquipment/GEquipmentComponent.h"


UGExocismComponent::UGExocismComponent()
{

}

void UGExocismComponent::InitializeAdditionalData(const FGNonfixedObjData& Data)
{
    // 0. Owner Actor 지정
    {
        OwnerActor = Cast<AGFixedObject>(GetOwner());

        if (!IsValid(OwnerActor))
        {
            UE_LOG(LogTemp, Warning, TEXT("Cannot found Owner exocism circle!"));
        }
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

        OnExocismCircleInteracted.Clear();

        // Case 1. 수살귀
        if (ADrownedGhost* Drowned = Cast<ADrownedGhost>(FoundGhost))
        {
            UE_LOG(LogTemp, Log, TEXT("Ghost type - DrownedGhost"));

            OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep1_DrownGhost);
            NextTargetTID = "Cursed_MoonJar";
        }
        // Case 2. 손각시
        else if (AMaidenGhost* Maiden = Cast<AMaidenGhost>(FoundGhost))
        {
            UE_LOG(LogTemp, Log, TEXT("Ghost type - MaidenGhost"));

            OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep1_MaidenGhost);
            NextTargetTID = "Cursed_WhiteDress";
        }
        // Case 3. 태자귀
        else if (ALittleGhost* Little = Cast<ALittleGhost>(FoundGhost))
        {
            UE_LOG(LogTemp, Log, TEXT("Ghost type - LittleGhost"));

            OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep1_LittleGhost);
            NextTargetTID = "Cursed_BambooTube";
        }
    }


    // 2. 컴포넌트 등록
    {
        if (IsValid(OwnerActor))
        {
            InteractionVolume = OwnerActor->GetComponentByClass<USphereComponent>();

            InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            InteractionVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
            InteractionVolume->SetHiddenInGame(true);
            InteractionVolume->SetSimulatePhysics(false);
        }
    }


    // 3. deactive 상태로 초기화
    DeactivateExocismCircle();
}



// Exocism circle default logic
void UGExocismComponent::ActivateExocismCircle()
{
    // 바운딩박스 활성화
    if (IsValid(InteractionVolume))
    {
        InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Block);
    }

    // 데칼 변경
}

void UGExocismComponent::DeactivateExocismCircle()
{
    // 바운딩박스 비활성화
    if (IsValid(InteractionVolume))
    {
        InteractionVolume->SetCollisionResponseToChannel(ECC_GInteractable, ECR_Ignore);
    }

    // 데칼 변경
}

void UGExocismComponent::OnSuccessedInExocism()
{
    // GameState에서 퇴마 프로세스 종료 처리
}

void UGExocismComponent::OnFailedInExocism()
{
    auto* ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
    if (!IsValid(ObjectManager)) return;

    ObjectManager->ActivateExocismCircle(OwnerActor);
    DeactivateExocismCircle();
}



// 수살귀
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
        // 연출 - 달 항아리 위치 옮기기


        // 로직
        TargetEquipID = "Obj_Extool_Amulet";

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
        // 연출 - 달 항아리 부적 붙음 메시로 변경


        // 로직
        OnSuccessedInExocism();
    }
}



// 손각시
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
        // 연출 - 흰 소복 위치 옮기기


        // 로직
        TargetEquipID = "Obj_Extool_IronStake";

        OnExocismCircleInteracted.Clear();
        OnExocismCircleInteracted.AddUObject(this, &UGExocismComponent::ExocismStep2_DrownGhost);
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
        // 연출 - 지정된 위치에 쇠말뚝 생성


        // 로직
        OnSuccessedInExocism();
    }
}



// 태자귀
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
        // 연출 - 


        // 로직
        OnSuccessedInExocism();
    }
}
