// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameSessionMode.generated.h"

/**
 * 
 */
UCLASS()
class GOCLEAN_API AGameSessionMode : public AGameModeBase
{
	GENERATED_BODY()

public:
    AGameSessionMode();

protected:
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

public:
    // Ready가 바뀌었을 때 호출(서버)
    void OnPlayerReadyChanged();

protected:
    // 실제 게임 시작(맵 이동 등)
    void StartGameIfPossible();

    // 시작 가능 여부 판단
    bool CanStartGame() const;
    
    // 호스트 판별
    bool IsHostPlayerState(const APlayerState* PS) const;

private:
    // 최대 동시 플레이어 수
    UPROPERTY(EditDefaultsOnly, Category = "Session")
    int32 MaxPlayers = 4;

    // 시작을 위한 최소 레디 인원
    UPROPERTY(EditDefaultsOnly, Category = "Session")
    int32 MinPlayersToStart = 2; 

    // 게임 시작이 가능한지
    bool bGameStarted = false;

    // PlayerState의 PlayerArray를 통해 빈 Index 찾기
    int32 FindNextAvailableSeatIndex() const;

    // 접속 인원수 카운트
    int32 GetCurrentPlayerCount() const;

};
