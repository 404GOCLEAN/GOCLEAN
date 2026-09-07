// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "ServerModule/GameSession/RPCRouterComponent.h"
#include "ServerModule/GameSession/InGameGameState.h"
#include "ServerModule/GameSession/PlayerSessionState.h"

#include "GObjectSystem/Server/GObjectManager.h"
#include "GPlayerSystem/Server/GPlayerManager.h"

#include "GTypes/GObjectTypes.h"

#include "GObjectSystem/GNonfixedObjCoreComponent.h"
#include "GObjectSystem/GNonfixedObject.h"

#include "GCharacter/GOCLEANCharacter.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"


// =================
// Constructor
// =================

URPCRouterComponent::URPCRouterComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}


// ==============
// BeginPlay
// ==============

void URPCRouterComponent::BeginPlay()
{
    Super::BeginPlay();
}


// ============
// Tick
// ===========

void URPCRouterComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction
    );
}


// ============
// Helper
// ===========

AInGameGameState* URPCRouterComponent::GetInGameState() const
{
    UWorld* World = GetWorld();

    if (!World)
        return nullptr;

    return World->GetGameState<AInGameGameState>();
}


// =======================
// Object Event
// Client -> Server
// =====================

void URPCRouterComponent::Server_ObjectEvent_Implementation(
    EObjectEvent_C2S EventType, const FObjectPayload_C2S& Payload)
{
    // 서버 권한 확인
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }


    // RouterComponent의 Owner는 PlayerController
    APlayerController* PC = Cast<APlayerController>(GetOwner());

    if (!PC)
        return;


    // InGame State
    AInGameGameState* GS = GetInGameState();

    if (!GS)
        return;


    // Object Manager
    UGObjectManager* OM = GS->GetObjectManager();

    if (!OM)
        return;


    switch (EventType)
    {
        // ------------------------
        // 기본 오브젝트 상호작용
        // ------------------------

    case EObjectEvent_C2S::Object_TryInteract:

        OM->HandleTryInteract(PC, Payload.TargetInstanceId);

        break;


        // -----------
        // 소각기
        // ---------

    case EObjectEvent_C2S::Incinerator_ThrowTrash:

        OM->HandleIncineratorThrowTrash(PC, Payload.TargetInstanceIds);

        break;


        // ----------
        // 캐비넷
        // -----------

    case EObjectEvent_C2S::Cabinet_Enter:

        OM->HandleCabinetEnter(PC, Payload.TargetInstanceId);

        break;


    case EObjectEvent_C2S::Cabinet_Exit:

        OM->HandleCabinetExit(PC, Payload.TargetInstanceId);

        break;


        // ---------
        // 물탱크
        // ---------

    case EObjectEvent_C2S::WaterTank_StartFill:

        OM->HandleWaterTankStartFill(PC, Payload.TargetInstanceId);

        break;


        // -----------------
        // 벤딩 머신
        // -----------------

    case EObjectEvent_C2S::Vending_SelectItem:

        OM->HandleVendingSelectItem(PC, Payload.ItemTypeId);

        break;


        // -----------
        // 양동이
        // ----------

    case EObjectEvent_C2S::Bucket_PourWater:

        OM->HandleBucketPourWater(PC, Payload.TargetInstanceId);

        break;


    case EObjectEvent_C2S::Bucket_EmptyWater:

        OM->HandleBucketEmptyWater(PC, Payload.TargetInstanceId);

        break;


    case EObjectEvent_C2S::Bucket_IncreaseContamination:

        OM->HandleBucketIncreaseContamination(PC, Payload.TargetInstanceId, Payload.ParamInt);

        break;


        // ---------------
        // 바구니
        // -------------

    case EObjectEvent_C2S::Basket_PutTrash:

        OM->HandleBasketPutTrash(PC, Payload.TargetInstanceId, Payload.ParamInt);

        break;


    case EObjectEvent_C2S::Basket_EmptyTrash:

        OM->HandleBasketEmptyTrash(PC, Payload.TargetInstanceId);

        break;


        // -------------------
        // Object Spawn
        // ------------------

    case EObjectEvent_C2S::Object_ActorSpawnReady:

        OM->HandleObjectActorSpawnReady(PC, Payload.TargetInstanceId);

        break;


    default:
        break;
    }
}


// ====================
// Object Event
// Server -> Client
// ===================

void URPCRouterComponent::Client_ObjectEvent_Implementation(
    EObjectEvent_S2C EventType, const FObjectPayload_S2C& Payload)
{
    AInGameGameState* GS = GetInGameState();

    if (!GS)
        return;


    UGObjectManager* OM = GS->GetObjectManager();

    if (!OM)
        return;


    switch (EventType)
    {
        // ------------
        // 소각기
        // ------------

    case EObjectEvent_S2C::Incinerator_TrashBurnFinished:

        OM->OnIncineratorTrashBurnFinished(Payload.TargetInstanceId);

        break;


        // -----------
        // 물탱크
        // ----------

    case EObjectEvent_S2C::WaterTank_FillFinished:

        OM->OnWaterTankFillFinished(Payload.TargetInstanceId);

        break;


        // ------------
        // 양동이
        // -----------

    case EObjectEvent_S2C::Bucket_ScoopWater:

        OM->OnBucketScoopWater(Payload.TargetInstanceId);

        break;


        // ----------------------------
        // Object Spawn / Destroy
        // ---------------------------

    case EObjectEvent_S2C::Object_Spawned:

        OM->OnObjectSpawned(Payload.TargetInstanceId);

        break;


    case EObjectEvent_S2C::Object_Destroyed:

        OM->OnObjectDestroyed(Payload.TargetInstanceId);

        break;


    case EObjectEvent_S2C::Object_SpawnDataReady:

        OM->OnObjectSpawnDataReady();

        break;


        // ---------------------
        // Interaction UI
        // -------------------

    case EObjectEvent_S2C::Object_InteractableHint:

        OM->OnObjectInteractableHint(Payload.ObjectTypeId, Payload.TargetInstanceId);

        break;


    case EObjectEvent_S2C::Object_InteractionRejected:

        OM->OnObjectInteractionRejected(Payload.RejectReason, Payload.TargetInstanceId);

        break;


    default:
        break;
    }
}


// ==================
// Player Event
// Client -> Server
// ====================

void URPCRouterComponent::Server_PlayerEvent_Implementation(
    EPlayerEvent_C2S EventType, const FPlayerPayload_C2S& Payload
)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }


    APlayerController* PC = Cast<APlayerController>(GetOwner());

    if (!PC)
        return;


    AInGameGameState* GS = GetInGameState();

    if (!GS)
        return;


    UGPlayerManager* PM = GS->GetPlayerManager();

    if (!PM)
        return;


    // 실제 Player Event 처리
    PM->HandlePlayerEvent_C2S(
        PC,
        EventType,
        Payload
    );
}


// ====================
// Server -> Client
// ===================

void URPCRouterComponent::Client_PlayerEvent_Implementation(
    EPlayerEvent_S2C EventType, const FPlayerPayload_S2C& Payload)
{
    AInGameGameState* GS =  GetInGameState();

    if (!GS)
        return;


    UGPlayerManager* PM = GS->GetPlayerManager();

    if (!PM)
        return;


    // PlayerIndex는 현재 SeatIndex를 의미
    APawn* PlayerPawn = GS->GetPawnBySeat(Payload.PlayerIndex);

    if (!PlayerPawn)
        return;


    switch (EventType)
    {
        // ---------------------
        // Animation State
        // --------------------

    case EPlayerEvent_S2C::NotifyAnimStateChanged:

        /*
         * Payload:
         *
         * PlayerIndex
         * AnimState
         *
         * PlayerIndex(SeatIndex)로 해당 Pawn을 찾은 뒤 AnimState 변경
         *
         * Character에서 상태에 맞는 Animation을 재생하도록 연결
         */

        break;


        // ----------------
        // Held Item
        // ---------------

    case EPlayerEvent_S2C::NotifyHeldItemChanged:

        /*
         * Payload:
         *
         * PlayerIndex
         * HeldItem
         *
         * 해당 캐릭터가 들고 있는 아이템 Mesh / 상태 갱신
         */

        break;


        // ------------------------
        // One Shot Animation
        // -----------------------

    case EPlayerEvent_S2C::PlayOneShot:

        /*
         * Payload:
         *
         * PlayerIndex
         * OneShotId
         *
         * AGOCLEANCharacter에서
         * Montage 등을 재생
         */

        break;


        // ----------------------
        // Death
        // --------------------

    case EPlayerEvent_S2C::NotifyDeath:

        /*
         * Payload: PlayerIndex
         *
         * PlayerSessionState: bIsAlive = false
         *
         * 단, 실제 사망 판정 및 SetAlive(false)는
         * Server 측 InGameGameMode에서 처리
         *
         * Client에서는:
         *
         * - 사망 Animation
         * - Character 상태
         * - UI
         *
         * 등의 표현만 처리
         */

        break;


        // ---------------
        // Reject
        // -------------

    case EPlayerEvent_S2C::ActionRejected:

        /*
         * Payload:
         *
         * RejectReason
         */

        break;


    default:
        break;
    }
}


// =======================
// Cleaning Event
// Client -> Server
// ======================

void URPCRouterComponent::Server_CleaningEvent_Implementation(
    ECleaningEvent_C2S EventType, const FCleaningPayload_C2S& Payload)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }


    APlayerController* PC = Cast<APlayerController>(GetOwner());

    if (!PC)
        return;


    AInGameGameState* GS = GetInGameState();

    if (!GS)
        return;


    UGObjectManager* OM = GS->GetObjectManager();

    if (!OM)
        return;


    // ==============
    // Debug
    // =============

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            2.f,
            FColor::Green,
            FString::Printf(
                TEXT(
                    "[Server] CleaningEvent=%d Target=%d"
                ),
                static_cast<int32>(EventType),
                Payload.TargetInstanceId
            )
        );
    }


    switch (EventType)
    {
        // -----------------
        // 장비 사용
        // ----------------

    case ECleaningEvent_C2S::UseEquipmentOnObject:

        /*
         * Payload:
         *
         * EquipmentTypeId
         * TargetInstanceId
         */

        OM->HandleUseEquipmentOnObject(PC, Payload.EquipmentTypeId, Payload.TargetInstanceId);

        break;


        // ----------------
        // 아이템 사용
        // ----------------

    case ECleaningEvent_C2S::UseItemOnObject:

        /*
         * Payload:
         *
         * ItemId
         * TargetInstanceId
         *
         * 소비형 / 설치형 여부는
         * ObjectManager 내부에서 ItemId로 판단
         */

        OM->HandleUseItemOnObject(PC, Payload.ItemId, Payload.TargetInstanceId);

        break;


    default:
        break;
    }
}