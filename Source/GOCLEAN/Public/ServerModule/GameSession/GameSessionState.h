// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "GUI/Base/MyUserWidget.h"

#include "GameSessionState.generated.h"

class APlayerSessionState;

UENUM(BlueprintType)
enum class ESessionPhase : uint8
{
    Lobby,          // 로비
    LobbyCountdown, // 로비 준비 완료 (모두 레디 상태)
    Loading,        // 로딩 중
    InGame,         // 인게임
    Ending,         // 인게임 종료
};

UENUM(BlueprintType)
enum class EInGamePhase : uint8
{
    None,
    Cleaning,           // 청소중
    ExorcismStart,      // 퇴마 시작
    ExorcismInProgress, // 퇴마 중
    ExorcismEnd,        // 퇴마 종료
};

// 구매 벤딩 아이템 관리 구조체
USTRUCT(BlueprintType)
struct FPurchasedVendingEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerId = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 ItemId = -1;
};


class UGObjectManager;
class UGPlayerManager;
class UGMapManager;

class UCharacterStatsComponent;


/**
 * 
 */
UCLASS()
class GOCLEAN_API AGameSessionState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	AGameSessionState();

public:

    UGObjectManager* GetObjectManager() const { return ObjectManager; }
    UGPlayerManager* GetPlayerManager() const { return PlayerManager; }
    UGMapManager* GetMapManager() const { return MapManager; }

protected:
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

public:

    // SeatIndex -> PlayerState
    UFUNCTION(BlueprintCallable, Category = "Session")
    APlayerSessionState* GetPlayerSessionStateBySeat(int32 SeatIndex) const;

    // SeatIndex -> Pawn
    UFUNCTION(BlueprintCallable, Category = "Session")
    APawn* GetPawnBySeat(int32 SeatIndex) const;

    // Pawn -> SeatIndex
    UFUNCTION(BlueprintCallable, Category = "Session")
    int32 GetSeatIndexOfPawn(const APawn* Pawn) const;

    // PlayerState -> SeatIndex
    UFUNCTION(BlueprintCallable, Category = "Session")
    int32 GetSeatIndexOfPlayerState(const APlayerState* PlayerState) const;

    // 로컬 플레이어 SeatIndex (플레이어가 자기 자신 인덱스 찾기)
    UFUNCTION(BlueprintCallable, Category = "Session")
    int32 GetLocalSeatIndex() const;

    // SeatIndex -> PlayerController
    UFUNCTION(BlueprintCallable, Category = "Session")
    APlayerController* GetPlayerControllerBySeat(int32 SeatIndex) const;

    // 타이머
    UFUNCTION(BlueprintPure, Category = "Session")
    float BP_GetPostExorcismTimeRemaining() const { return PostExorcismTimeRemaining; }



private:
    UPROPERTY()
    UGObjectManager* ObjectManager;

    UPROPERTY()
    UGPlayerManager* PlayerManager = nullptr;


    UPROPERTY()
    UGMapManager* MapManager = nullptr;


    // *HUD*
    UPROPERTY()
    UGHUDWidget* HUD = nullptr;

    UPROPERTY(ReplicatedUsing="OnRep_bReady")
    bool bReady = false;

    UFUNCTION()
    void OnRep_bReady();



public:
    // Getters (Client/Server)
    UFUNCTION(BlueprintCallable)
    float GetSpiritualGauge() const { return SpiritualGauge; }

    UFUNCTION(BlueprintCallable)
    float GetRestGauge() const { return RestGauge; }
    float GetExorcismProgress() const { return ExorcismProgress; }
    float GetPostExorcismTimeRemaining() const { return PostExorcismTimeRemaining; }
    int32 GetAliveSurvivorCount() const { return AliveSurvivorCount; }
    int32 GetFinalRewardMoney() const { return FinalRewardMoney; }

    UFUNCTION(BlueprintCallable)
    void SetHUD(UGHUDWidget* Target) 
    { 
        HUD = Target; 
        PlayHUD();
    }
    void PlayHUD();


public:
    // GameMode에서 호출 (Server-only mutators)
    // 게이지 증감 (클램프 포함)
    void AddSpiritualGauge(float Delta, float Min = 0.f, float Max = 100.f);
    void AddRestGauge(float Delta, float Min = 0.f, float Max = 100.f);

    // 진행도 설정/증가
    void SetExorcismProgress(float NewProgress, float Min = 0.f, float Max = 100.f);
    void AddExorcismProgress(float Delta, float Min = 0.f, float Max = 100.f);

    // 퇴마 성공 후 남은 시간
    void StartPostExorcismTimer(float DurationSeconds);   // Duration으로 시작
    void SetPostExorcismTimeRemaining(float NewRemainingSeconds); // 강제 설정
    void AddPostExorcismTimeRemaining(float DeltaSeconds); // 증감

    // 생존자 수
    void SetAliveSurvivorCount(int32 NewCount);
    void AddAliveSurvivorCount(int32 Delta);

    // 보상 (최종)
    void SetFinalRewardMoney(int32 NewMoney);


protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Setters (Server-only internal)
    void SetSpiritualGauge_Internal(float NewValue);
    void SetRestGauge_Internal(float NewValue);
    void SetExorcismProgress_Internal(float NewValue);
    void SetPostExorcismTimeRemaining_Internal(float NewValue);
    void SetAliveSurvivorCount_Internal(int32 NewValue);
    void SetFinalRewardMoney_Internal(int32 NewValue);
    


    // OnRep 
    UFUNCTION() 
    void OnRep_SpiritualGauge();

    UFUNCTION() 
    void OnRep_RestGauge();

    UFUNCTION() 
    void OnRep_ExorcismProgress();

    UFUNCTION() 
    void OnRep_PostExorcismTimeRemaining();

    UFUNCTION() 
    void OnRep_AliveSurvivorCount();

    UFUNCTION() 
    void OnRep_FinalRewardMoney();


protected:
    // 의뢰 번호
    UPROPERTY(ReplicatedUsing = OnRep_SelectedContractId, BlueprintReadOnly)
    int32 SelectedContractId = 0;

    // 세션 참가 코드
    UPROPERTY(ReplicatedUsing = OnRep_SessionJoinCode, BlueprintReadOnly)
    FString SessionJoinCode;

    // 구매 벤딩 아이템(전역 풀)
    UPROPERTY(ReplicatedUsing = OnRep_PurchasedVending, BlueprintReadOnly)
    TArray<FPurchasedVendingEntry> PurchasedVending;

    // 영적 게이지
    UPROPERTY(ReplicatedUsing = OnRep_SpiritualGauge)
    float SpiritualGauge = 0.f;

    // 안식 게이지
    UPROPERTY(ReplicatedUsing = OnRep_RestGauge)
    float RestGauge = 0.f;

    // 퇴마 진행도
    UPROPERTY(ReplicatedUsing = OnRep_ExorcismProgress)
    float ExorcismProgress = 0.f;

    // 퇴마 성공 후 남은 시간
    UPROPERTY(ReplicatedUsing = OnRep_PostExorcismTimeRemaining)
    float PostExorcismTimeRemaining = 0.f;

    // 생존자 수
    UPROPERTY(ReplicatedUsing = OnRep_AliveSurvivorCount)
    int32 AliveSurvivorCount = 0;

    // 추가금 (최종 보상)
    UPROPERTY(ReplicatedUsing = OnRep_FinalRewardMoney)
    int32 FinalRewardMoney = 0;
	

public:
    UFUNCTION(BlueprintCallable, Category = "Session")
    ESessionPhase GetSessionPhase() const { return SessionPhase; }

    UFUNCTION(BlueprintCallable, Category = "Session")
    EInGamePhase GetInGamePhase() const { return InGamePhase; }


    // GameMode에서만 호출
    UFUNCTION(BlueprintCallable, Category = "Session")
    void SetSessionPhase(ESessionPhase NewPhase);

    UFUNCTION(BlueprintCallable, Category = "Session")
    void SetInGamePhase(EInGamePhase NewPhase);

protected:
    // 클라에서 상태 바뀌면 UI/연출 갱신 트리거
    UPROPERTY(ReplicatedUsing = OnRep_SessionPhase, BlueprintReadOnly, Category = "Session")
    ESessionPhase SessionPhase = ESessionPhase::Lobby;

    UPROPERTY(ReplicatedUsing = OnRep_InGamePhase, BlueprintReadOnly, Category = "Session")
    EInGamePhase InGamePhase = EInGamePhase::None;

    UFUNCTION()
    void OnRep_SessionPhase();

    UFUNCTION()
    void OnRep_InGamePhase();

    // 블루프린트 호출
    UFUNCTION(BlueprintImplementableEvent, Category = "Session")
    void BP_OnSessionPhaseChanged(ESessionPhase NewPhase);

    UFUNCTION(BlueprintImplementableEvent, Category = "Session")
    void BP_OnInGamePhaseChanged(EInGamePhase NewPhase);


public:
    // Selected Contract (의뢰 번호)
    UFUNCTION(BlueprintCallable, Category = "Session|Contract")
    int32 GetSelectedContractId() const { return SelectedContractId; }

    // 의뢰 번호 변경
    void SetSelectedContractId(int32 NewContractId);

    // Session Join Code 
    UFUNCTION(BlueprintCallable, Category = "Session|Join")
    const FString& GetSessionJoinCode() const { return SessionJoinCode; }

    // 조인 코드 설정
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Session|Join")
    void SetSessionJoinCode(const FString& NewCode);


    // 구매 여부
    UFUNCTION(BlueprintCallable, Category = "Session|Vending")
    bool HasPurchasedVendingByPlayerId(int32 PlayerId) const;

    // 아이템 토글
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Session|Vending")
    bool TogglePurchasedVending(int32 PlayerId, int32 ItemId);

    // 구매 아이템 조회
    UFUNCTION(BlueprintCallable, Category = "Session|Vending")
    bool GetPurchasedVendingItemId(int32 PlayerId, int32& OutItemId) const;

    // 구매 추가(1인 1개 제한)
    bool TryAddPurchasedVending(int32 PlayerId, int32 ItemId);

    // 구매 제거(튕김/환불 등)
    bool RemovePurchasedVending(int32 PlayerId);

    // 전체 초기화(새 라운드 시작 등)
    void ClearPurchasedVending();
        

protected:
    UFUNCTION()
    void OnRep_SelectedContractId();

    UFUNCTION()
    void OnRep_SessionJoinCode();

    UFUNCTION()
    void OnRep_PurchasedVending();

public:
    // 영적/안식 게이지 초기화
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void ResetSpiritualAndRestGauge(float SpiritualStartValue = 100.f);

    // 영적 / 안식 게이지 값 증감
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    void ApplySpiritualOrRestGauge(float Amount, float SpiritualMin = 0.f, float SpiritualMax = 100.f, float RestMin = 0.f, float RestMax = 100.f);

public:
    // SeatIndex로 해당 플레이어의 현재 목숨(Life) 가져오기 (클라 UI에서도 사용 가능)
    UFUNCTION(BlueprintCallable, Category="Session|Stats")
    int32 GetCurrentLifeBySeat(int32 SeatIndex) const;

    // 로컬 플레이어 현재 목숨
    UFUNCTION(BlueprintCallable, Category="Session|Stats")
    int32 GetLocalCurrentLife() const;



    // 밴딩 아이템 재고 관리
public:
    // 무제한은 -1로 약속
    UFUNCTION(BlueprintCallable)
    void ResetVendingStock();

    UFUNCTION(BlueprintCallable)
    int32 GetVendingRemainingByIndex(int32 ItemIndex) const;

    // 구매 시도
    UFUNCTION(BlueprintCallable)
    bool TryPurchaseVendingByIndex(int32 ItemIndex);

    // 인덱스로 남은 재고 조회
    UFUNCTION(BlueprintCallable)
    int32 GetItemStockByIndex(int32 ItemIndex) const;

    UFUNCTION(BlueprintCallable)
    void SetBuyVendingItem(FName TID);

protected:
    UPROPERTY(Replicated, BlueprintReadOnly)
    TArray<int32> VendingRemaining;


    // 퇴마 연결 로직
public:
    UPROPERTY(ReplicatedUsing = OnRep_ExorcismPhase, BlueprintReadOnly, Category = "Session|Exorcism")
    EInGamePhase ExorcismPhase = EInGamePhase::Cleaning;

    UFUNCTION()
    void OnRep_ExorcismPhase();

    UFUNCTION(BlueprintPure, Category = "Session|Exorcism")
    EInGamePhase BP_GetExorcismPhase() const { return ExorcismPhase; }

    // 서버에서만 바꾸도록(Mode에서만 호출)
    void SetExorcismPhase_ServerOnly(EInGamePhase NewPhase);
};