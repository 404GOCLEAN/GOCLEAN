// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/InGameGameMode.h"

#include "ServerModule/GameSession/PlayerSessionState.h"
#include "ServerModule/GameSession/GameSessionInstance.h"

AInGameGameMode::AInGameGameMode()
{
    GameStateClass = AInGameGameState::StaticClass();

    PlayerStateClass = APlayerSessionState::StaticClass();

    bUseSeamlessTravel = true;
}


// ==============
// BeginPlay
// =============

void AInGameGameMode::BeginPlay()
{
    Super::BeginPlay();

    InitializeGame();
}


// ================
// Initialize
// ================

void AInGameGameMode::InitializeGame()
{
    InitializeGameState();
    InitializePlayers();
}


void AInGameGameMode::InitializeGameState()
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    UGameSessionInstance* GI = GetGameInstance<UGameSessionInstance>();


    if (GI)
    {
        /*
         * 기존 GameInstance 함수에 맞춰 연결
         *
         * IGS->SetSelectedContractId(
         *     GI->GetPendingContractId()
         * );
         */
    }


    IGS->SetGamePhase(EGamePhase::Cleaning);

    IGS->SetGhostState(EGhostState::Normal);

    IGS->SetExorcismState(EExorcismState::None);

    IGS->SetSpiritualGauge(100.f);
    IGS->SetRestGauge(0.f);

    IGS->SetExorcismProgress(0.f);

    IGS->SetExtractionTimeRemaining(0.f);

    IGS->SetFinalRewardMoney(0);
}


void AInGameGameMode::InitializePlayers()
{
    if (!GameState)
        return;


    for (APlayerState* PS : GameState->PlayerArray)
    {
        APlayerSessionState* PSS = Cast<APlayerSessionState>(PS);

        if (!PSS)
            continue;


        PSS->SetAlive(true);
        PSS->SetEscaped(false);
        PSS->SetReady(false);
    }


    RecalculateAliveSurvivors();
}


// ============
// Start
// ============

void AInGameGameMode::StartGame()
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    IGS->SetGamePhase(EGamePhase::Cleaning);
}


// ===========
// Gauge
// ==========

void AInGameGameMode::ChangeSpiritualGauge(float Delta)
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    IGS->AddSpiritualGauge(Delta);


    CheckExorcismCondition();
}


void AInGameGameMode::ChangeRestGauge(float Delta)
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    IGS->AddRestGauge(Delta);
}


void AInGameGameMode::ChangeExorcismProgress(float Delta)
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    IGS->AddExorcismProgress(Delta);


    if (IGS->GetExorcismProgress() >= 100.f)
    {
        StartExtractionPhase();
    }
}


// =================
// Exorcism
// ==================

void AInGameGameMode::CheckExorcismCondition()
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    if (IGS->GetGamePhase() != EGamePhase::Cleaning)
    {
        return;
    }


    // 나중에 게이지 수치 수정 필요
    if (IGS->GetSpiritualGauge() <= 20.f)
    {
        StartExorcismPhase();
    }
}


void AInGameGameMode::StartExorcismPhase()
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    IGS->SetGamePhase(EGamePhase::Exorcism);

    IGS->SetExorcismState(EExorcismState::InProgress);
}


// ===================
// Extraction
// ==================

void AInGameGameMode::StartExtractionPhase()
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    IGS->SetExorcismState(EExorcismState::Success);

    IGS->SetGamePhase(EGamePhase::Extraction);


    /*
     * TODO:
     * 실제 철수 제한시간 설정
     * IGS->SetExtractionTimeRemaining(60.f);
     */
}


// ===================
// Player Death
// ==================

void AInGameGameMode::EliminatePlayer(AController* Controller)
{
    APlayerSessionState* PSS = GetPlayerSessionState(Controller);

    if (!PSS)
        return;


    if (!PSS->IsAlive())
        return;


    PSS->SetAlive(false);


    RecalculateAliveSurvivors();


    /*
     * TODO: 
     * Controller 관전 상태 전환
     * 장비 Drop
     */


    CheckGameEndCondition();
}


// ==================
// Player Escape
// =================

void AInGameGameMode::EscapePlayer(AController* Controller)
{
    APlayerSessionState* PSS = GetPlayerSessionState(Controller);

    if (!PSS)
        return;


    if (!PSS->IsAlive())
        return;


    if (PSS->HasEscaped())
        return;


    PSS->SetEscaped(true);


    /*
     * Alive는 true 유지
     *
     * 살아서 탈출했다는 의미:
     *
     * Alive   = true
     * Escaped = true
     */


    CheckGameEndCondition();
}


// ==============
// Alive
// =============

void AInGameGameMode::RecalculateAliveSurvivors()
{
    AInGameGameState* IGS = GetInGameState();

    if (!IGS || !GameState)
        return;


    int32 AliveCount = 0;


    for (APlayerState* PS : GameState->PlayerArray)
    {
        const APlayerSessionState* PSS = Cast<APlayerSessionState>(PS);

        if (!PSS)
            continue;


        if (PSS->IsAlive())
        {
            ++AliveCount;
        }
    }


    IGS->SetAliveSurvivorCount(
        AliveCount
    );
}


// ================
// Game End
// ===============

void AInGameGameMode::CheckGameEndCondition()
{
    if (bGameFinished)
        return;


    AInGameGameState* IGS =
        GetInGameState();

    if (!IGS)
        return;


    // 전멸
    if (IGS->GetAliveSurvivorCount() <= 0)
    {
        FinishGame(false);
        return;
    }


    // Extraction 중 살아있는 플레이어가 모두 탈출했는지 검사
     

    if (IGS->GetGamePhase() == EGamePhase::Extraction)
    {
        bool bAllFinished = true;


        for (APlayerState* PS : GameState->PlayerArray)
        {
            const APlayerSessionState* PSS = Cast<APlayerSessionState>(PS);

            if (!PSS)
                continue;


            if (PSS->IsAlive()
                && !PSS->HasEscaped())
            {
                bAllFinished = false;
                break;
            }
        }


        if (bAllFinished)
        {
            FinishGame(true);
        }
    }
}


void AInGameGameMode::FinishGame(bool bSuccess)
{
    if (bGameFinished)
        return;


    bGameFinished = true;


    AInGameGameState* IGS = GetInGameState();

    if (!IGS)
        return;


    IGS->SetGamePhase(EGamePhase::End);


    /*
     * TODO:
     *
     * FinalRewardMoney 계산
     *
     * 결과 UI 표시
     *
     * 이후 Title 이동
     */
}


// ====================
// Login / Logout
// ===================

void AInGameGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // 추후 재접속 기능 구현 시 사용
}


void AInGameGameMode::Logout(AController* Exiting)
{
    // 필요한 시스템에 넘길 경우 여기서 처리

    Super::Logout(Exiting);

    RecalculateAliveSurvivors();
    CheckGameEndCondition();
}


// =============
// Helper
// ============

AInGameGameState* AInGameGameMode::GetInGameState() const
{
    return GetGameState<AInGameGameState>();
}