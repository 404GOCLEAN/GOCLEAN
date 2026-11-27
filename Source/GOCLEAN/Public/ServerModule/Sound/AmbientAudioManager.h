// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Engine/DataTable.h"
#include "AmbientAudioManager.generated.h"


class UAudioComponent;
class USoundBase;
class UGMapManager;
class APawn;


USTRUCT(BlueprintType)
struct FAmbientZoneAudioRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    // 랜덤으로 재생할 환경 SFX(3D)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<TObjectPtr<USoundBase>> RandomSfxList;

    // 랜덤 재생 최소/최대 간격(초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
    float MinInterval = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
    float MaxInterval = 20.0f;

    // 간격마다 재생될 확률(0~1)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PlayChance = 0.5f;

    // 플레이어 주변에서 재생할지
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bPlayNearPlayer = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bPlayNearPlayer", ClampMin = "0.0"))
    float NearPlayerRadius = 250.0f;
};

/**
 * 
 */
UCLASS()
class GOCLEAN_API UAmbientAudioManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // 켜고/끄기
    UFUNCTION(BlueprintCallable, Category = "Ambient")
    void StartAmbient(USoundBase* InIndoorLoop, USoundBase* InOutdoorLoop, UDataTable* InZoneTable);

    /*UFUNCTION(BlueprintCallable, Category = "Ambient")
    void StartAmbient();*/

    UFUNCTION(BlueprintCallable, Category = "Ambient")
    void StopAmbient();

    // 디버그
    UFUNCTION(BlueprintPure, Category = "Ambient")
    FName GetCurrentZoneId() const { return CurrentZoneId; }

protected:
    // 2D 베이스 루프 (실내/실외)
    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Base")
    TObjectPtr<USoundBase> IndoorBaseLoop = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Base")
    TObjectPtr<USoundBase> OutdoorBaseLoop = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Base", meta = (ClampMin = "0.0"))
    float BaseLoopVolume = 0.4f;

    // ZoneID -> 랜덤 SFX 설정 DataTable
    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Event")
    TObjectPtr<UDataTable> ZoneAudioTable = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Event", meta = (ClampMin = "0.0"))
    float EventSfxVolume = 1.0f;


    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Context")
    FName InitialZoneId = TEXT("Zone_Outdoor");

    // 플레이어 위치/Zone 상태 폴링 주기
    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Tuning", meta = (ClampMin = "0.05"))
    float ContextPollInterval = 0.25f;

    // 이벤트 SFX 전체 ON/OFF
    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Tuning")
    bool bEnableEventSfx = true;

    // 로그 보고싶으면 true
    UPROPERTY(EditDefaultsOnly, Category = "Ambient|Debug")
    bool bDebugLog = false;

private:
    // 타이머
    FTimerHandle ContextPollTimer;
    FTimerHandle EventSfxTimer;

    // 상태 캐시
    bool bRunning = false;
    bool bIsOutdoor = false;
    bool bBaseLoopInitialized = false;
    TWeakObjectPtr<UWorld> AmbientWorld;
    TWeakObjectPtr<APawn> ContextPawn;

    FName CurrentZoneId = NAME_None;

    // 2D 루프 컴포넌트
    UPROPERTY()
    TObjectPtr<UAudioComponent> BaseLoopComp = nullptr;

private:
    UWorld* GetWorldSafe() const;
    UGMapManager* GetMapManager() const;
    APawn* GetLocalPawn(UWorld* World) const;

    // 주기적으로 플레이어 위치/존을 갱신
    void TickContext();

    // 실내/실외 베이스 루프 교체
    void UpdateBaseLoop(bool bNowOutdoor);

    // ZoneIDs 중 대표 ZoneID 선택
    FName ChooseDominantZoneId(const TArray<FName>& ZoneIds, UGMapManager* MapMgr) const;

    // 현재 ZoneID에 맞게 이벤트 SFX 타이머 재설정
    void RescheduleEventSfx();

    // 이벤트 SFX 한 번 시도
    void TryPlayEventSfx();

    // DataTable에서 ZoneID 설정 가져오기
    const FAmbientZoneAudioRow* FindZoneAudioRow(FName ZoneId) const;

    // 랜덤 SFX 선택
    static USoundBase* PickRandom(const TArray<TObjectPtr<USoundBase>>& List);
};
