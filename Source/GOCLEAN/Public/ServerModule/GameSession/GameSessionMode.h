// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameSessionMode.generated.h"

class APlayerSessionState;

/**
  * Lobby / InGame GameMode 공통 기반 클래스
 *
 * 실제 게임 규칙은
 * - ALobbyGameMode
 * - AInGameGameMode
 * 
 * 에서 처리
 */

UCLASS()
class GOCLEAN_API AGameSessionMode : public AGameModeBase
{
	GENERATED_BODY()

public:

    AGameSessionMode();


protected:

    // PlayerController에서 PlayerSessionState를 가져오는 공통 Helper
    APlayerSessionState* GetPlayerSessionState(AController* Controller) const;


    // 현재 접속한 PlayerSessionState 개수
    int32 GetSessionPlayerCount() const;
};
