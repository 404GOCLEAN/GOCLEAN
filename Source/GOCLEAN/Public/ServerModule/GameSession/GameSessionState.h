// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"

#include "GameSessionState.generated.h"

class APlayerSessionState;


/**
 * Lobby / InGame GameState의 공통 기반 클래스
 *
 * 공통 PlayerState / Seat 조회 기능만 담당
 *
 * 실제 Lobby 데이터는 ALobbyGameState
 * 실제 InGame 데이터는 AInGameGameState에서 관리
 */

UCLASS()
class GOCLEAN_API AGameSessionState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

    AGameSessionState();


    // ========================
    // Player / Seat Helper
    // =========================

    // SeatIndex -> PlayerSessionState
    UFUNCTION(BlueprintCallable, Category = "Session|Player")
    APlayerSessionState* GetPlayerSessionStateBySeat( int32 SeatIndex ) const;


    // PlayerState -> SeatIndex
    UFUNCTION(BlueprintCallable, Category = "Session|Player")
    int32 GetSeatIndexOfPlayerState( const APlayerState* PlayerState ) const;


    // Pawn -> SeatIndex
    UFUNCTION(BlueprintCallable, Category = "Session|Player")
    int32 GetSeatIndexOfPawn( const APawn* Pawn ) const;


    // SeatIndex -> Pawn
    UFUNCTION(BlueprintCallable, Category = "Session|Player")
    APawn* GetPawnBySeat( int32 SeatIndex ) const;


    // 현재 로컬 플레이어 SeatIndex
    UFUNCTION(BlueprintCallable, Category = "Session|Player")
    int32 GetLocalSeatIndex() const;


    // SeatIndex -> PlayerController
    // PlayerController는 서버에만 존재하는 정보가 있으므로 서버 전용으로 사용
    APlayerController* GetPlayerControllerBySeat( int32 SeatIndex ) const;
};