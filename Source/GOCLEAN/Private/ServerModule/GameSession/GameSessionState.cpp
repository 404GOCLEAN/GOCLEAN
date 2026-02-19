// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/GameSessionState.h"
#include <Net/UnrealNetwork.h>
#include "ServerModule/GameSession/PlayerSessionState.h"

#include <GObjectSystem/Server/GObjectManager.h>
#include <GPlayerSystem/Server/GPlayerManager.h>
#include <GMapSystem/Server/GMapManager.h>

#include "GCharacter/StatsComponent/CharacterStatsComponent.h"
#include "GCharacter/GOCLEANPlayerController.h"
#include <ServerModule/GameSession/GameSessionMode.h>

AGameSessionState::AGameSessionState()
{
    ObjectManager = nullptr;
    PlayerManager = nullptr;
    MapManager = nullptr;

	bReplicates = true;
}

void AGameSessionState::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
        PlayerManager = NewObject<UGPlayerManager>(this);
        MapManager = GetWorld()->GetSubsystem<UGMapManager>();
    }
}

void AGameSessionState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    
    if (HasAuthority())
    {
        if (ObjectManager) 
        {
            ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();
        }

        if (!PlayerManager)
        {
            PlayerManager = NewObject<UGPlayerManager>(this);
        }

        if (!MapManager)
        {
            MapManager = GetWorld()->GetSubsystem<UGMapManager>();
        }
    }
}


void AGameSessionState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AGameSessionState, SpiritualGauge);
    DOREPLIFETIME(AGameSessionState, RestGauge);
    DOREPLIFETIME(AGameSessionState, ExorcismProgress);
    DOREPLIFETIME(AGameSessionState, PostExorcismTimeRemaining);
    DOREPLIFETIME(AGameSessionState, AliveSurvivorCount);
    DOREPLIFETIME(AGameSessionState, FinalRewardMoney);

    DOREPLIFETIME(AGameSessionState, SessionPhase);
    DOREPLIFETIME(AGameSessionState, InGamePhase);

    DOREPLIFETIME(AGameSessionState, SelectedContractId);
    DOREPLIFETIME(AGameSessionState, SessionJoinCode);
    DOREPLIFETIME(AGameSessionState, PurchasedVending);


    DOREPLIFETIME(AGameSessionState, VendingRemaining);
    DOREPLIFETIME(AGameSessionState, ExorcismPhase);
}


APlayerSessionState* AGameSessionState::GetPlayerSessionStateBySeat(int32 SeatIndex) const
{
    for (APlayerState* PS : PlayerArray)
    {
        APlayerSessionState* PSS = Cast<APlayerSessionState>(PS);
        if (PSS && PSS->GetSeatIndex() == SeatIndex)
        {
            return PSS;
        }
    }
    return nullptr;
}

APawn* AGameSessionState::GetPawnBySeat(int32 SeatIndex) const
{
    if (APlayerSessionState* PSS = GetPlayerSessionStateBySeat(SeatIndex))
    {
        if (APawn* Pawn = PSS->GetPawn())
        {
            return Pawn;
        }

        // 서버에선 PlayerController 통해 찾기
        if (HasAuthority())
        {
            if (APlayerController* PC = GetPlayerControllerBySeat(SeatIndex))
            {
                return PC->GetPawn();
            }
        }
    }

    return nullptr;
}

int32 AGameSessionState::GetSeatIndexOfPawn(const APawn* Pawn) const
{
    if (!Pawn)
        return -1;

    if (const APlayerState* PS = Pawn->GetPlayerState())
    {
        return GetSeatIndexOfPlayerState(PS);
    }

    return -1;
}

int32 AGameSessionState::GetSeatIndexOfPlayerState(const APlayerState* PlayerState) const
{
    const APlayerSessionState* PSS = Cast<APlayerSessionState>(PlayerState);
    if (PSS)
    {
        return PSS->GetSeatIndex();
    }
    return -1;
}

int32 AGameSessionState::GetLocalSeatIndex() const
{
    if (!GetWorld())
        return -1;

    APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
    if (!LocalPC)
        return -1;

    APlayerSessionState* PSS = Cast<APlayerSessionState>(LocalPC->PlayerState);
    if (!PSS)
        return -1;

    return PSS->GetSeatIndex();
}

APlayerController* AGameSessionState::GetPlayerControllerBySeat(int32 SeatIndex) const
{
    if (!HasAuthority())
        return nullptr;

    for (APlayerState* PS : PlayerArray)
    {
        APlayerSessionState* PSS = Cast<APlayerSessionState>(PS);
        if (PSS && PSS->GetSeatIndex() == SeatIndex)
        {
            return Cast<APlayerController>(PSS->GetOwner());
        }
    }

    return nullptr;
}


static float ClampFloat(float Value, float Min, float Max)
{
    return FMath::Clamp(Value, Min, Max);
}

void AGameSessionState::AddSpiritualGauge(float Delta, float Min, float Max)
{
    if (!HasAuthority()) return;
    SetSpiritualGauge_Internal(ClampFloat(SpiritualGauge + Delta, Min, Max));
}

void AGameSessionState::AddRestGauge(float Delta, float Min, float Max)
{
    if (!HasAuthority()) return;
    SetRestGauge_Internal(ClampFloat(RestGauge + Delta, Min, Max));
}

void AGameSessionState::SetExorcismProgress(float NewProgress, float Min, float Max)
{
    if (!HasAuthority()) return;
    SetExorcismProgress_Internal(ClampFloat(NewProgress, Min, Max));
}

void AGameSessionState::AddExorcismProgress(float Delta, float Min, float Max)
{
    if (!HasAuthority()) return;
    SetExorcismProgress_Internal(ClampFloat(ExorcismProgress + Delta, Min, Max));
}

void AGameSessionState::StartPostExorcismTimer(float DurationSeconds)
{
    if (!HasAuthority()) return;
    SetPostExorcismTimeRemaining_Internal(FMath::Max(0.f, DurationSeconds));
}

void AGameSessionState::SetPostExorcismTimeRemaining(float NewRemainingSeconds)
{
    if (!HasAuthority()) return;
    SetPostExorcismTimeRemaining_Internal(FMath::Max(0.f, NewRemainingSeconds));
}

void AGameSessionState::AddPostExorcismTimeRemaining(float DeltaSeconds)
{
    if (!HasAuthority()) return;
    SetPostExorcismTimeRemaining_Internal(FMath::Max(0.f, PostExorcismTimeRemaining + DeltaSeconds));
}

void AGameSessionState::SetAliveSurvivorCount(int32 NewCount)
{
    if (!HasAuthority()) return;
    SetAliveSurvivorCount_Internal(FMath::Max(0, NewCount));
}

void AGameSessionState::AddAliveSurvivorCount(int32 Delta)
{
    if (!HasAuthority()) return;
    SetAliveSurvivorCount_Internal(FMath::Max(0, AliveSurvivorCount + Delta));
}

void AGameSessionState::SetFinalRewardMoney(int32 NewMoney)
{
    if (!HasAuthority()) return;
    SetFinalRewardMoney_Internal(FMath::Max(0, NewMoney));
}



// Internal setters (server-only)
void AGameSessionState::SetSpiritualGauge_Internal(float NewValue)
{
    SpiritualGauge = NewValue;
    
    OnRep_SpiritualGauge();

    // 영적 게이지 20% 이하 -> 퇴마 시작 단계 진입 트리거
    if (HasAuthority())
    {
        if (SpiritualGauge <= 20.f) // 0~100 기준
        {
            if (AGameSessionMode* GM = GetWorld()->GetAuthGameMode<AGameSessionMode>())
            {
                if (GetInGamePhase() == EInGamePhase::Cleaning)
                {
                    GM->StartExorcismPhase();
                }
            }
        }
    }
}

void AGameSessionState::SetRestGauge_Internal(float NewValue)
{
    RestGauge = NewValue;

    OnRep_RestGauge();
}

void AGameSessionState::SetExorcismProgress_Internal(float NewValue)
{
    ExorcismProgress = NewValue;

    OnRep_ExorcismProgress();
}

void AGameSessionState::SetPostExorcismTimeRemaining_Internal(float NewValue)
{
    PostExorcismTimeRemaining = NewValue;

    OnRep_PostExorcismTimeRemaining();
}

void AGameSessionState::SetAliveSurvivorCount_Internal(int32 NewValue)
{
    AliveSurvivorCount = NewValue;

    OnRep_AliveSurvivorCount();
}

void AGameSessionState::SetFinalRewardMoney_Internal(int32 NewValue)
{
    FinalRewardMoney = NewValue;

    OnRep_FinalRewardMoney();
}

void AGameSessionState::SetSessionPhase(ESessionPhase NewPhase)
{
    if (!HasAuthority())
        return;

    if (SessionPhase == NewPhase)
        return;

    SessionPhase = NewPhase;

    OnRep_SessionPhase();
}

void AGameSessionState::SetInGamePhase(EInGamePhase NewPhase)
{
    if (!HasAuthority())
        return;

    if (InGamePhase == NewPhase)
        return;

    InGamePhase = NewPhase;

    OnRep_InGamePhase();
}

void AGameSessionState::SetSelectedContractId(int32 NewContractId)
{
    if (!HasAuthority()) return;
    if (SelectedContractId == NewContractId) return;

    SelectedContractId = NewContractId;
    OnRep_SelectedContractId();
}



void AGameSessionState::SetSessionJoinCode(const FString& NewCode)
{
    if (!HasAuthority()) return;
    if (SessionJoinCode == NewCode) return;

    SessionJoinCode = NewCode;
    OnRep_SessionJoinCode();
}

bool AGameSessionState::HasPurchasedVendingByPlayerId(int32 PlayerId) const
{
    for (const FPurchasedVendingEntry& E : PurchasedVending)
    {
        if (E.PlayerId == PlayerId)
            return true;
    }
    return false;
}

bool AGameSessionState::TogglePurchasedVending(int32 PlayerId, int32 ItemId)
{
    if (!HasAuthority()) return false;
    if (PlayerId < 0) return false;

    int32 OwnedItemId = -1;
    const bool bHas = GetPurchasedVendingItemId(PlayerId, OwnedItemId);

    if (!bHas)
    {
        // 구매한 적 없으면 추가
        return TryAddPurchasedVending(PlayerId, ItemId);
    }

    // 이미 구매한 게 있음
    if (OwnedItemId == ItemId)
    {
        // 같은 걸 다시 누르면 해제
        return RemovePurchasedVending(PlayerId);
    }

    // 다른 아이템은 구매 불가
    return false;
}

bool AGameSessionState::GetPurchasedVendingItemId(int32 PlayerId, int32& OutItemId) const
{
    for (const FPurchasedVendingEntry& E : PurchasedVending)
    {
        if (E.PlayerId == PlayerId)
        {
            OutItemId = E.ItemId;
            return true;
        }
    }
    return false;
}

bool AGameSessionState::TryAddPurchasedVending(int32 PlayerId, int32 ItemId)
{
    if (!HasAuthority()) return false;
    if (PlayerId < 0) return false;

    // 1인 1개 제한
    if (HasPurchasedVendingByPlayerId(PlayerId))
        return false;

    FPurchasedVendingEntry NewEntry;
    NewEntry.PlayerId = PlayerId;
    NewEntry.ItemId = ItemId;

    PurchasedVending.Add(NewEntry);

    OnRep_PurchasedVending();
    return true;
}

bool AGameSessionState::RemovePurchasedVending(int32 PlayerId)
{
    if (!HasAuthority()) return false;

    const int32 Removed = PurchasedVending.RemoveAll(
        [PlayerId](const FPurchasedVendingEntry& E)
        {
            return E.PlayerId == PlayerId;
        });

    if (Removed > 0)
    {
        OnRep_PurchasedVending();
        return true;
    }
    return false;
}

void AGameSessionState::ClearPurchasedVending()
{
    if (!HasAuthority()) return;

    if (PurchasedVending.Num() == 0)
        return;

    PurchasedVending.Reset();
    OnRep_PurchasedVending();
}



// OnRep (Client UI 갱신)
void AGameSessionState::OnRep_SpiritualGauge()
{

}

void AGameSessionState::OnRep_RestGauge()
{
    
}

void AGameSessionState::OnRep_ExorcismProgress()
{

}

void AGameSessionState::OnRep_PostExorcismTimeRemaining()
{

}

void AGameSessionState::OnRep_AliveSurvivorCount()
{

}

void AGameSessionState::OnRep_FinalRewardMoney()
{
    // UI 갱신(결과창)
}

void AGameSessionState::OnRep_SessionPhase()
{
    // 로비 위젯 교체, 입력 제한 토글, BGM 전환 등

    BP_OnSessionPhaseChanged(SessionPhase);
}

void AGameSessionState::OnRep_InGamePhase()
{
    // 청소 시작/퇴마 시작/퇴마 종료 연출 트리거 등

    BP_OnInGamePhaseChanged(InGamePhase);
}

void AGameSessionState::OnRep_SelectedContractId()
{

}

void AGameSessionState::OnRep_SessionJoinCode()
{

}

void AGameSessionState::OnRep_PurchasedVending()
{

}

void AGameSessionState::ResetSpiritualAndRestGauge(float SpiritualStartValue)
{
    if (!HasAuthority()) return;

    // 영적=100(기본), 안식=0
    SetSpiritualGauge_Internal(FMath::Max(0.f, SpiritualStartValue));
    // SetSpiritualGauge_Internal(FMath::Max(0.f, 1.0f));
    SetRestGauge_Internal(0.f);

    bReady = true;
}
void AGameSessionState::PlayHUD()
{
    bReady = false;
    bReady = true;
    OnRep_bReady();
}
void AGameSessionState::OnRep_bReady()
{
    if (HUD && bReady) HUD->OnGameLevelReady();
}

void AGameSessionState::ApplySpiritualOrRestGauge( float Amount, float SpiritualMin, float SpiritualMax, float RestMin, float RestMax)
{
    if (!HasAuthority()) return;

    if (Amount <= 0.f) return;

    if (SpiritualGauge > SpiritualMin)
    {
        AddSpiritualGauge(-Amount, SpiritualMin, SpiritualMax);
        return;
    }

    if (SpiritualGauge <= 0)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (AGOCLEANPlayerController* PC = Cast<AGOCLEANPlayerController>(It->Get()))
            {
                PC->Client_ShowResultUI();
                PC->ShowResultUI();
            }
        }
    }

    // 영적이 0(또는 Min)이라면 안식 증가
    // AddRestGauge(Amount, RestMin, RestMax);
}

int32 AGameSessionState::GetCurrentLifeBySeat(int32 SeatIndex) const
{
    APawn* Pawn = GetPawnBySeat(SeatIndex);
    if (!Pawn) return -1;

    UCharacterStatsComponent* Stats = Pawn->FindComponentByClass<UCharacterStatsComponent>();
    if (!Stats) return -1;

    return static_cast<int32>(Stats->GetCurrentLife());
}

int32 AGameSessionState::GetLocalCurrentLife() const
{
    const int32 LocalSeat = GetLocalSeatIndex();
    if (LocalSeat < 0) return -1;

    return GetCurrentLifeBySeat(LocalSeat);
}



void AGameSessionState::ResetVendingStock()
{
    if (!HasAuthority()) return;

    VendingRemaining.SetNum(4);
    VendingRemaining[0] = 1;
    VendingRemaining[1] = 3;
    VendingRemaining[2] = -1; // 무제한
    VendingRemaining[3] = 4;

}

int32 AGameSessionState::GetVendingRemainingByIndex(int32 ItemIndex) const
{
    if (!VendingRemaining.IsValidIndex(ItemIndex)) return 0;
    return VendingRemaining[ItemIndex];
}

bool AGameSessionState::TryPurchaseVendingByIndex(int32 ItemIndex)
{
    if (!HasAuthority()) return false;
    if (!VendingRemaining.IsValidIndex(ItemIndex)) return false;

    const int32 Remaining = VendingRemaining[ItemIndex];

    // 무제한
    if (Remaining < 0)
    {
        return true;
    }

    // 품절
    if (Remaining == 0)
    {
        return false;
    }

    // 재고 차감
    VendingRemaining[ItemIndex] = Remaining - 1;

    return true;
}

int32 AGameSessionState::GetItemStockByIndex(int32 ItemIndex) const
{
    if (VendingRemaining.IsValidIndex(ItemIndex))
    {
        return VendingRemaining[ItemIndex];
    }

    return 0;
}

void AGameSessionState::SetBuyVendingItem(FName TID)
{
    UE_LOG(LogTemp, Warning, TEXT("[SetBuyVendingItem] NetMode=%d HasAuthority=%d TID=%s ObjMgr=%s"),
        (int32)GetNetMode(), HasAuthority(), *TID.ToString(), ObjectManager ? TEXT("Valid") : TEXT("NULL"));

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("Called on CLIENT. This won't affect server."));
    }

    if (!ObjectManager) return;
    ObjectManager->SetSelectedVendingItem(TID);
}


// 퇴마 관련 로직
void AGameSessionState::SetExorcismPhase_ServerOnly(EInGamePhase NewPhase)
{
    if (!HasAuthority()) return;
    if (ExorcismPhase == NewPhase) return;

    ExorcismPhase = NewPhase;
    OnRep_ExorcismPhase();
}

void AGameSessionState::OnRep_ExorcismPhase()
{
    // UI 갱신 트리거용
}