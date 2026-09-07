// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ServerModule/GameSession/GameSessionMode.h"
#include "ServerModule/GameSession/InGameGameState.h"
#include "InGameGameMode.generated.h"

class APlayerSessionState;

/**
 * 
 */
UCLASS()
class GOCLEAN_API AInGameGameMode : public AGameSessionMode
{
	GENERATED_BODY()
	
public:

    AInGameGameMode();


    // ===============
    // Game Flow
    // ==============

    UFUNCTION(BlueprintCallable, Category = "Game|Flow")
    void StartGame();

    void StartExorcismPhase();

    void StartExtractionPhase();

    void FinishGame(bool bSuccess);


    // ===========
    // Gauge
    // =========

    void ChangeSpiritualGauge(float Delta);

    void ChangeRestGauge(float Delta);

    void ChangeExorcismProgress(float Delta);


    // ===============
    // Player
    // ==============

    void EliminatePlayer(AController* Controller);

    void EscapePlayer(AController* Controller);

    void RecalculateAliveSurvivors();


protected:

    virtual void BeginPlay() override;

    virtual void PostLogin(APlayerController* NewPlayer) override;

    virtual void Logout(AController* Exiting) override;


    void InitializeGame();

    void InitializePlayers();

    void InitializeGameState();


    // ===============
    // Condition
    // ===============

    void CheckExorcismCondition();

    void CheckGameEndCondition();


    // =============
    // Helper
    // ============

    AInGameGameState* GetInGameState() const;


private:

    bool bGameFinished = false;
};
