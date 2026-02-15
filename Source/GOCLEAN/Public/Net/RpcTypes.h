#pragma once

#include "CoreMinimal.h"
#include "Online/CoreOnline.h"
#include "RpcTypes.generated.h"

UENUM(BlueprintType)
enum class ERpcModule : uint8
{
    Mainstream, // 메인 게임 (퇴마 등)
    Map,        // 맵 관련
    Object,     // 오브젝트
    Cleaning,   // 청소
    Ghost,      // 귀신
    Player,     // 플레이어
};


/// ==============
/// ObjectEvent
/// ==============

UENUM(BlueprintType)
enum class EObjectEvent_C2S : uint8
{
    // 고정 오브젝트
    Incinerator_ThrowTrash,          // 폐기물 투입 (투입된 오브젝트 인스턴스ID 리스트)

    Cabinet_Enter,                   // 캐비닛 입장 (캐비닛 인스턴스ID)
    Cabinet_Exit,                    // 캐비닛 퇴장 (캐비닛 인스턴스ID)

    WaterTank_StartFill,             // 물 담기 시작 (물탱크ID?)

    Vending_SelectItem,              // 아이템 선택 (ItemTypeId)

    // 양동이
    Bucket_PourWater,                // 물 쏟기 (양동이 인스턴스ID)
    Bucket_EmptyWater,               // 물 비우기 (양동이 인스턴스ID)
    Bucket_IncreaseContamination,    // 물 오염도 증가 (양동이 인스턴스ID)

    // 바구니
    Basket_PutTrash,                 // 폐기물 담기 (바구니 인스턴스ID)
    Basket_EmptyTrash,               // 폐기물 비움 (바구니 인스턴스ID)

    // 공통 오브젝트
    Object_ActorSpawnReady,          // 오브젝트 액터 스폰 완료 (초기 복제/리소스 로드 완료)

    // 공통 로직 
    Object_TryInteract,              // 상호작용 실행 (대상 오브젝트 인스턴스ID)

};


UENUM(BlueprintType)
enum class EObjectEvent_S2C : uint8
{
    // 고정 오브젝트
    Incinerator_TrashBurnFinished,   // 폐기물 소각 완료 (완료된 인스턴스ID)

    WaterTank_FillFinished,          // 물 담기 완료 (서버 판정 결과 알림)

    // 양동이
    Bucket_ScoopWater,               // 물 뜨기 (양동이 인스턴스ID)

    // 공통 오브젝트 
    Object_Spawned,                  // 오브젝트 생성됨 (생성된 오브젝트 인스턴스ID)
    Object_Destroyed,                // 오브젝트 파괴됨 (파괴된 오브젝트 인스턴스ID)

    Object_SpawnDataReady,           // 오브젝트 스폰값 설정 완료 (맵 초기 데이터 준비 완료)

    // 인터랙션
    Object_InteractableHint,         // (조건 충족 시) 상호작용 안내 UI 띄워라 (ObjectTypeId 등)
    Object_InteractionRejected,      // (조건 불충족) 상호작용 거부됨 전달 (디버깅)
};


USTRUCT(BlueprintType)
struct FObjectRpcPayload
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FUniqueNetIdRepl SenderId;

    UPROPERTY(BlueprintReadWrite)
    FUniqueNetIdRepl TargetId;

    UPROPERTY(BlueprintReadWrite)
    int32 ObjectInstanceId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    TArray<int32> ObjectInstanceIdList;

    UPROPERTY(BlueprintReadWrite)
    int32 CompletedInstanceId = INDEX_NONE;

    UPROPERTY(BlueprintReadWrite)
    FName ItemTypeId = NAME_None;
};

USTRUCT(BlueprintType)
struct FObjectPayload_C2S
{
    GENERATED_BODY()

    // 플레이어 ID
    UPROPERTY(BlueprintReadWrite)
    FUniqueNetIdRepl SenderId;

    // 플레이어 인덱스
    UPROPERTY(BlueprintReadWrite)
    int32 PlayerIndex = -1;

    // 대상 오브젝트 인스턴스 ID (캐비닛/양동이/바구니/인터랙션 등)
    UPROPERTY(BlueprintReadWrite)
    int32 TargetInstanceId = INDEX_NONE;

    // 소각로 투입 같은 경우 사용 array
    UPROPERTY(BlueprintReadWrite)
    TArray<int32> TargetInstanceIds;

    // 벤딩머신 아이템 타입
    UPROPERTY(BlueprintReadWrite)
    FName ItemTypeId = NAME_None;

    // 예비용 데이터 공간
    UPROPERTY(BlueprintReadWrite)
    int32 ParamInt = 0;

    UPROPERTY(BlueprintReadWrite)
    float ParamFloat = 0.f;
};


UENUM(BlueprintType)
enum class EObjectRejectReason : uint8
{
    None,               // 정상 처리
    NotInteractable,    // 상호작용 불가 상태
    OutOfRange,         // 거리 초과
    Cooldown,           // 쿨타임 중
    InvalidTarget,      // 잘못된 대상
    NotAllowedState,    // 현재 상태에서 불가
};

USTRUCT(BlueprintType)
struct FObjectPayload_S2C
{
    GENERATED_BODY()

    // 특정 플레이어만 대상 (비어있을 경우 브로드캐스트)
    UPROPERTY(BlueprintReadWrite)
    FUniqueNetIdRepl TargetPlayerId; 

    // 플레이어 인덱스
    UPROPERTY(BlueprintReadWrite)
    int32 PlayerIndex = -1;

    // 완료/생성/파괴/거부 대상 등
    UPROPERTY(BlueprintReadWrite)
    int32 TargetInstanceId = INDEX_NONE; 

    // UI 안내용 (상호작용 가능 오브젝트 type id)
    UPROPERTY(BlueprintReadWrite)
    FName ObjectTypeId = NAME_None;

    // 거부 디버깅
    UPROPERTY(BlueprintReadWrite)
    EObjectRejectReason RejectReason = EObjectRejectReason::None;
};



/// ==============
/// PlayerEvent
/// ==============


UENUM(BlueprintType)
enum class EPlayerAnimState : uint8
{
    Idle,
    Stand,
    Crouch,
    Dead,
};

UENUM(BlueprintType)
enum class EPlayerOneShot : uint8
{
    None,

    // idle 제외, active 기본
    Axe,
    Cleaner,
    Cleaner_activetoldle,
    Hold_drop,
    Hold_pick,
    Holdx_interaction,
    Mop,
    Mop_idletoActive,
    Mop_swap,
    OneHandHold,
    GhostAHunt,
    Spray,
    Spray_activeToIdle,
    Spray_idleToActive
};

// 현재 소지 아이템 상태
UENUM(BlueprintType)
enum class EPlayerHeldItem : uint8
{
    Hand,			// 맨손
    OVariables,		// 오브젝트
    Flashilight,	// 손전등
    Pad,			// 귀신노트
    Mop,			// 물걸레
    Axe,			// 도끼
    Cleaner,		// 비단 빗자루
    Spray,			// 닭피 스프레이
    TBowl,			// 닭피가 담긴 사발
    TAmulet,		// 부적
    TPile,			// 쇠말뚝
    VBill,			// 지전
    VDoll,			// 제웅
    VAlcoholGlass,	// 부적을 탄 술
    VCoin,			// 엽전 묶음
    AutoMop,		// 전동 물걸레
};

UENUM(BlueprintType)
enum class EPlayerRejectReason : uint8
{
    None,
    InvalidState,   // 죽음/스턴/상호작용 중 등으로 불가
    Cooldown,       // 쿨타임
    NotEnoughStat,  // 스태미나/정신력 부족 등
    InvalidParam,   // 파라미터 이상
};

UENUM(BlueprintType)
enum class EPlayerEvent_C2S : uint8
{
    RequestSetAnimState,         // 클라 입력에 따라 애니 상태 변경 요청    
    RequestSetHeldItem,          // 클라 입력에 따라 소지 아이템 변경 요청
    RequestPlayOneShot,          // 던지기/청소/아이템 사용 등 한 번 재생 요청
};

UENUM(BlueprintType)
enum class EPlayerEvent_S2C : uint8
{
    NotifyAnimStateChanged,      // 플레이어 애니 상태 변경 브로드캐스트 (지속 상태: 달리기, 웅크리기 등)
    NotifyHeldItemChanged,       // 플레이어 소지 아이템 변경 브로드캐스트

    PlayOneShot,                 // 1회성 연출 재생 (아이템 사용, 사망 등)
    NotifyDeath,                 // 사망 알림 (GameState 쪽 사망상태)

    ActionRejected,              // 입력 요청 거절
};


USTRUCT(BlueprintType)
struct FPlayerPayload_C2S
{
    GENERATED_BODY()

    // 요청 애니 상태
    UPROPERTY(BlueprintReadWrite)
    EPlayerAnimState AnimState = EPlayerAnimState::Idle;

    // 요청 소지 아이템
    UPROPERTY(BlueprintReadWrite)
    EPlayerHeldItem HeldItem = EPlayerHeldItem::Hand;

    UPROPERTY(BlueprintReadWrite)
    EPlayerOneShot OneShot = EPlayerOneShot::None;

    // 예비용 데이터 공간
    UPROPERTY(BlueprintReadWrite)
    int32 ParamInt = 0;

    UPROPERTY(BlueprintReadWrite)
    float ParamFloat = 0.f;
  
};

USTRUCT(BlueprintType)
struct FPlayerPayload_S2C
{
    GENERATED_BODY()

    // 대상 플레이어 인덱스
    UPROPERTY(BlueprintReadWrite)
    int32 PlayerIndex = -1;

    // 애니 상태
    UPROPERTY(BlueprintReadWrite)
    EPlayerAnimState AnimState = EPlayerAnimState::Idle;

    // 소지 아이템
    UPROPERTY(BlueprintReadWrite)
    EPlayerHeldItem HeldItem = EPlayerHeldItem::Hand;

    // 1회성 연출 ID (청소, 물건 드랍 등)
    UPROPERTY(BlueprintReadWrite)
    EPlayerOneShot OneShotId = EPlayerOneShot::None;

    // 거절 사유
    UPROPERTY(BlueprintReadWrite)
    EPlayerRejectReason RejectReason = EPlayerRejectReason::None;

    // 예비용 데이터 공간
    UPROPERTY(BlueprintReadWrite)
    int32 ParamInt = 0;

    UPROPERTY(BlueprintReadWrite)
    float ParamFloat = 0.f;

};