// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/InGameGameState.h"

#include "GObjectSystem/Server/GObjectManager.h"
#include "GPlayerSystem/Server/GPlayerManager.h"
#include "GMapSystem/Server/GMapManager.h"

#include "Net/UnrealNetwork.h"

void AInGameGameState::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
        return;

    ObjectManager = GetWorld()->GetSubsystem<UGObjectManager>();

    PlayerManager = NewObject<UGPlayerManager>(this);

    MapManager = GetWorld()->GetSubsystem<UGMapManager>();
}

AInGameGameState::AInGameGameState()
{
    bReplicates = true;
}


// =================
// Replication
// =================

void AInGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AInGameGameState, SelectedContractId);

    DOREPLIFETIME(AInGameGameState, GamePhase);
    DOREPLIFETIME(AInGameGameState, GhostState);
    DOREPLIFETIME(AInGameGameState, ExorcismState);

    DOREPLIFETIME(AInGameGameState, SpiritualGauge);
    DOREPLIFETIME(AInGameGameState, RestGauge);

    DOREPLIFETIME(AInGameGameState, ExorcismProgress);
    DOREPLIFETIME(AInGameGameState, ExtractionTimeRemaining);

    DOREPLIFETIME(AInGameGameState, AliveSurvivorCount);

    DOREPLIFETIME(AInGameGameState, FinalRewardMoney);
}


// ==============
// Contract
// ==============

void AInGameGameState::SetSelectedContractId(int32 NewContractId)
{
    if (!HasAuthority())
        return;

    if (SelectedContractId == NewContractId)
        return;

    SelectedContractId = NewContractId;

    OnRep_SelectedContractId();
}


// =============
// Phase
// ============

void AInGameGameState::SetGamePhase(EGamePhase NewPhase)
{
    if (!HasAuthority())
        return;

    if (GamePhase == NewPhase)
        return;

    GamePhase = NewPhase;

    OnRep_GamePhase();
}


void AInGameGameState::SetGhostState(EGhostState NewState)
{
    if (!HasAuthority())
        return;

    if (GhostState == NewState)
        return;

    GhostState = NewState;

    OnRep_GhostState();
}


void AInGameGameState::SetExorcismState(EExorcismState NewState)
{
    if (!HasAuthority())
        return;

    if (ExorcismState == NewState)
        return;

    ExorcismState = NewState;

    OnRep_ExorcismState();
}


// ===========
// Gauge
// ===========

void AInGameGameState::SetSpiritualGauge(float NewValue)
{
    if (!HasAuthority())
        return;

    const float Clamped = FMath::Clamp(NewValue, 0.f, 100.f);

    if (FMath::IsNearlyEqual(SpiritualGauge, Clamped))
    {
        return;
    }

    SpiritualGauge = Clamped;

    OnRep_SpiritualGauge();
}


void AInGameGameState::AddSpiritualGauge(float Delta)
{
    SetSpiritualGauge(SpiritualGauge + Delta);
}


void AInGameGameState::SetRestGauge(float NewValue)
{
    if (!HasAuthority())
        return;

    const float Clamped = FMath::Clamp(NewValue, 0.f, 100.f);

    if (FMath::IsNearlyEqual(RestGauge, Clamped))
    {
        return;
    }

    RestGauge = Clamped;

    OnRep_RestGauge();
}


void AInGameGameState::AddRestGauge(float Delta)
{
    SetRestGauge(RestGauge + Delta);
}


// ============
// Exorcism
// ============

void AInGameGameState::SetExorcismProgress(float NewProgress)
{
    if (!HasAuthority())
        return;

    const float Clamped = FMath::Clamp(NewProgress, 0.f, 100.f);

    if (FMath::IsNearlyEqual(ExorcismProgress, Clamped))
    {
        return;
    }

    ExorcismProgress = Clamped;

    OnRep_ExorcismProgress();
}


void AInGameGameState::AddExorcismProgress(float Delta)
{
    SetExorcismProgress(ExorcismProgress + Delta);
}


// ================
// Extraction
// ===============

void AInGameGameState::SetExtractionTimeRemaining(float NewTime)
{
    if (!HasAuthority())
        return;

    const float NewValue = FMath::Max(0.f, NewTime);

    if (FMath::IsNearlyEqual(ExtractionTimeRemaining, NewValue))
    {
        return;
    }

    ExtractionTimeRemaining = NewValue;

    OnRep_ExtractionTimeRemaining();
}


void AInGameGameState::AddExtractionTimeRemaining(float Delta)
{
    SetExtractionTimeRemaining( ExtractionTimeRemaining + Delta );
}


// =================
// Alive Count
// =================

void AInGameGameState::SetAliveSurvivorCount(int32 NewCount)
{
    if (!HasAuthority())
        return;

    const int32 NewValue = FMath::Max(0, NewCount);

    if (AliveSurvivorCount == NewValue)
        return;

    AliveSurvivorCount = NewValue;

    OnRep_AliveSurvivorCount();
}


// ===========
// Reward
// ==========

void AInGameGameState::SetFinalRewardMoney(int32 NewMoney)
{
    if (!HasAuthority())
        return;

    const int32 NewValue = FMath::Max(0, NewMoney);

    if (FinalRewardMoney == NewValue)
        return;

    FinalRewardMoney = NewValue;

    OnRep_FinalRewardMoney();
}


// ============
// OnRep
// ===========

void AInGameGameState::OnRep_SelectedContractId()
{
}


void AInGameGameState::OnRep_GamePhase()
{
    BP_OnGamePhaseChanged(GamePhase);
}


void AInGameGameState::OnRep_GhostState()
{
    BP_OnGhostStateChanged(GhostState);
}


void AInGameGameState::OnRep_ExorcismState()
{
    BP_OnExorcismStateChanged(ExorcismState);
}


void AInGameGameState::OnRep_SpiritualGauge()
{
    BP_OnSpiritualGaugeChanged(SpiritualGauge);
}


void AInGameGameState::OnRep_RestGauge()
{
    BP_OnRestGaugeChanged(RestGauge);
}


void AInGameGameState::OnRep_ExorcismProgress()
{
    BP_OnExorcismProgressChanged(ExorcismProgress);
}


void AInGameGameState::OnRep_ExtractionTimeRemaining()
{
    BP_OnExtractionTimeChanged(ExtractionTimeRemaining);
}


void AInGameGameState::OnRep_AliveSurvivorCount()
{
    BP_OnAliveSurvivorCountChanged(AliveSurvivorCount);
}


void AInGameGameState::OnRep_FinalRewardMoney()
{
    BP_OnFinalRewardChanged(FinalRewardMoney);
}