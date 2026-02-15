#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Net/RpcTypes.h"
#include "GObjectManager.generated.h"


class AGFixedObject;
class AGNonfixedObject;


UCLASS()
class GOCLEAN_API UGObjectManager : public UWorldSubsystem
{
	GENERATED_BODY()



	// constructor
public:
	UGObjectManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;



protected:
	// Variables: nonfixed Object
	UPROPERTY()
	TArray<TObjectPtr<AGFixedObject>> NfixedObjPool;

	UPROPERTY(VisibleAnywhere, Category = "NonfixedObject Pool");
	int32 NfixedObjCnt;

	UPROPERTY(EditDefaultsOnly, Category = "NonfixedObject Pool");
	int32 PoolSize = 500;

	UPROPERTY(EditDefaultsOnly, Category = "NonfixedObject Pool");
	FVector PoolLocation = FVector(0.0, -1000.0, 0.0);

	// pool로 반환된 가장 최신 index를 push, spawn 시 pop 하여 해당 index의 data를 재사용
	TArray<int32> FreeObjsStack;

	UPROPERTY()
	TMap<int32, TObjectPtr<AGFixedObject>> NfixedObjects;	// key: IID | value: pool mapping

	// 1. Obj가 Destory 상태가 되면 즉시 Pool로 보내지 말고 해당 Queue에 넣는다.
	// 2. 해당 Queue는 최근 삭제된 10개의 오브젝트만 보관한다.
	// 3. Dequeue된 index의 NFixedObj는 Map에서 삭제되고 Pool로 복귀한다.
	TQueue<int32> DestroyedObjs;



	// Variables: fixed Object
	//		1. 드럼통소각로: 쓰레기 타입의 비고정 오브젝트를 소각하는 용도. 고정 위치 스폰.
	UPROPERTY()
	TObjectPtr<AGFixedObject> Fireplace;

	//		2. 물탱크: 물양동이를 들고 상호작용할 시, 물을 채우는 용도. 고정 위치 스폰.
	UPROPERTY()
	TObjectPtr<AGFixedObject> WaterTank;

	//		3. 벤딩머신: 벤딩아이템을 스폰하는 용도. 고정 위치 스폰.
	UPROPERTY()
	TObjectPtr<AGFixedObject> VendingMachine;

	//		4. 캐비넷: 플레이어가 귀신을 피해 숨는 용도. 고정 위치 스폰.
	UPROPERTY()
	TArray<TObjectPtr<AGFixedObject>> Cabinets;

	//		5. 퇴마진: 퇴마를 진행하는 용도. 랜덤 위치 스폰.
	UPROPERTY()
	TObjectPtr<AGFixedObject> ExocismCircle;



	// functions
protected:
	void InitNonfixedObjects();
	void InitFixedObjects();


public:
	// 비고정오브젝트 풀과 고정오브젝트에 UGObjectData를 할당 
	void InitiateObjects(UObject*);
	// 스폰이 완료된 오브젝트 데이터를 
	void SetObjectDatas();


	//UFUNCTION(BlueprintCallable)
	//AGNonfixedObject* SpawnNonfixedObject(
	//	FName TID,
	//	const FVector& Location,
	//	const FRotator& Rotation
	//);

public:
	// C -> S (서버 처리)

	// 특정 오브젝트에 대한 상호작용 요청 처리
	void HandleTryInteract(class APlayerController* PC, int32 TargetInstanceId);

	// 소각로에 폐기물 투입 요청 처리
	void HandleIncineratorThrowTrash(class APlayerController* PC, const TArray<int32>& TrashInstanceIds);

	// 캐비닛 입장 요청 처리
	void HandleCabinetEnter(class APlayerController* PC, int32 CabinetInstanceId);

	// 캐비닛 퇴장 요청 처리
	void HandleCabinetExit(class APlayerController* PC, int32 CabinetInstanceId);

	// 물탱크에서 물 담기 시작 요청 처리
	void HandleWaterTankStartFill(class APlayerController* PC, int32 WaterTankInstanceId);

	// 벤딩머신 아이템 선택 요청 처리
	void HandleVendingSelectItem(class APlayerController* PC, FName ItemTypeId);

	// 양동이로 물 쏟기 요청 처리
	void HandleBucketPourWater(class APlayerController* PC, int32 BucketInstanceId);

	// 양동이 물 비우기 요청 처리
	void HandleBucketEmptyWater(class APlayerController* PC, int32 BucketInstanceId);

	// 양동이 오염도 증가 요청 처리
	void HandleBucketIncreaseContamination(class APlayerController* PC, int32 BucketInstanceId, int32 Delta);

	// 바구니에 폐기물 담기 요청 처리
	void HandleBasketPutTrash(class APlayerController* PC, int32 BasketInstanceId, int32 TrashParam);

	// 바구니 폐기물 비우기 요청 처리
	void HandleBasketEmptyTrash(class APlayerController* PC, int32 BasketInstanceId);

	// 오브젝트 액터 스폰 준비 완료 알림 처리
	void HandleObjectActorSpawnReady(class APlayerController* PC, int32 ObjectInstanceId);

public:
	// S -> C (클라에서 수신 처리)

	// 소각 완료된 오브젝트 처리
	void OnIncineratorTrashBurnFinished(int32 CompletedInstanceId);

	// 물 담기 완료 처리
	void OnWaterTankFillFinished(int32 WaterTankInstanceId);

	// 양동이 물 뜨기 결과 처리
	void OnBucketScoopWater(int32 BucketInstanceId);

	// 오브젝트 생성 처리
	void OnObjectSpawned(int32 SpawnedInstanceId);

	// 오브젝트 파괴 처리
	void OnObjectDestroyed(int32 DestroyedInstanceId);

	// 오브젝트 스폰 데이터 준비 완료 처리
	void OnObjectSpawnDataReady();

	// 상호작용 가능 오브젝트 안내 처리
	void OnObjectInteractableHint(FName ObjectTypeId, int32 TargetInstanceId);

	// 상호작용 거부 결과 처리
	void OnObjectInteractionRejected(EObjectRejectReason Reason, int32 TargetInstanceId);
};

