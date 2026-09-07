// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ServerModule/GameSession/GameSessionMode.h"
#include "ServerModule/GameSession/PlayerSessionState.h"
#include "LobbyGameMode.generated.h"

class ALobbyGameState;
class AGOCLEANPlayerController;

/**
 * 로비 게임 규칙
 *
 * 담당:
 * - 플레이어 입장 / 퇴장
 * - Seat
 * - Host
 * - Character
 * - Ready
 * - Contract
 * - Vending
 * - 게임 시작
 */

UCLASS()
class GOCLEAN_API ALobbyGameMode : public AGameSessionMode
{
	GENERATED_BODY()

public:

    ALobbyGameMode();


    // =====================
    // Player Connection
    // =====================

    virtual void PostLogin(APlayerController* NewPlayer) override;

    virtual void Logout(AController* Exiting) override;


    // ===============
    // Ready
    // ===============

    bool RequestSetPlayerReady(APlayerSessionState* PlayerState, bool bNewReady);

    UFUNCTION(BlueprintPure, Category = "Lobby|Ready")
    bool AreAllPlayersReady() const;

    UFUNCTION(BlueprintPure, Category = "Lobby|Game")
    bool CanStartGame() const;


    // ==============
    // Contract
    // ==============

    bool RequestSetContract(APlayerController* Requester, int32 ContractId);


    // ============
    // Vending
    // ============

    bool RequestPurchaseVending(APlayerController* Buyer, int32 ItemId);

    bool RequestCancelVendingPurchase(APlayerController* Buyer, int32 ItemId);


    // ===============
    // Start
    // ===============

    bool RequestStartGame(APlayerController* Requester);


protected:

    virtual void BeginPlay() override;


    // ============
    // Seat
    // ===========

    int32 FindNextAvailableSeatIndex() const;

    bool IsSeatOccupied(int32 SeatIndex) const;

    void AssignSeat(APlayerSessionState* PlayerState);


    // ==========================
    // Player Initialization
    // ==========================

    void InitializeLobbyPlayer(APlayerController* Player);

    void AssignHost(APlayerController* Player, APlayerSessionState* PlayerState);

    void AssignCharacter(APlayerSessionState* PlayerState);

    void SetPlayerNickname(APlayerController* Player, APlayerSessionState* PlayerState);


    // ===============
    // Character
    // ===============

    void InitializeCharacterOrder();

    EPlayerGender GetGenderForCharacter(EPlayerCharacterType CharacterType) const;


    // ===========
    // Helper
    // ===========

    ALobbyGameState* GetLobbyGameState() const;

    bool IsHost(const APlayerSessionState* PlayerState) const;


private:

    // 최대 플레이어
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MaxPlayers = 4;


    // 게임 시작 최소 인원
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MinPlayersToStart = 1;


    // 벤딩 전체 구매 가능 개수
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Vending")
    int32 MaxVendingPurchaseCount = 5;


    // SeatIndex와 대응되는 캐릭터 배정 순서
    TArray<EPlayerCharacterType> CharacterOrder;


    bool bGameStarting = false;
	
};
