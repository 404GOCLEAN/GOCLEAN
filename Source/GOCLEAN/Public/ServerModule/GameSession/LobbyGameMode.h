// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ServerModule/GameSession/GameSessionMode.h"
#include "ServerModule/GameSession/PlayerSessionState.h"
#include "ItemModule/Vending/VendingItemData.h"
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

    UFUNCTION(BlueprintCallable, Category = "Lobby|Game")
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

    const FVendingItemData* FindVendingItemData(int32 ItemId) const;


    // =============================
    // Lobby Character Display
    // =============================

    // 현재 접속해 있지만 로비 캐릭터가 없는 플레이어를 확인하고 스폰
    void EnsureAllLobbyCharactersSpawned();

    void SpawnLobbyCharacter(APlayerSessionState* PlayerState);

    void DestroyLobbyCharacter(int32 SeatIndex);

    AActor* FindLobbyCharacterSpawnPoint(int32 SeatIndex) const;

    TSubclassOf<AActor> GetLobbyCharacterClass(EPlayerCharacterType CharacterType) const;


private:

    // 최대 플레이어
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MaxPlayers = 4;


    // 게임 시작 최소 인원
    UPROPERTY(EditDefaultsOnly, Category = "Lobby")
    int32 MinPlayersToStart = 1;

    // 인게임 맵 경로
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Travel")
    FString GameMapPath = TEXT("/Game/FirstPerson/Maps/FirstPersonMap");


    // 벤딩 전체 구매 가능 개수
    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Vending")
    int32 MaxVendingPurchaseCount = 5;


    // SeatIndex와 대응되는 캐릭터 배정 순서
    TArray<EPlayerCharacterType> CharacterOrder;

    // 벤딩 아이템 기본 데이터
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Vending", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UDataTable> VendingItemDataTable;



    // =============================
    // Lobby Character Classes
    // =============================

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Character")
    TSubclassOf<AActor> Character01LobbyClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Character")
    TSubclassOf<AActor> Character02LobbyClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Character")
    TSubclassOf<AActor> Character03LobbyClass;

    UPROPERTY(EditDefaultsOnly, Category = "Lobby|Character")
    TSubclassOf<AActor> Character04LobbyClass;

    UPROPERTY()
    TMap<int32, TObjectPtr<AActor>> SpawnedLobbyCharacters;



    bool bGameStarting = false;
	
};
