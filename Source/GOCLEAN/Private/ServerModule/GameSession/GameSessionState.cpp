// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/GameSessionState.h"
#include <Net/UnrealNetwork.h>
#include "ServerModule/GameSession/PlayerSessionState.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"


AGameSessionState::AGameSessionState()
{
    bReplicates = true;
}


// ======================
// Seat -> PlayerState
// ======================

APlayerSessionState*
AGameSessionState::GetPlayerSessionStateBySeat(
    int32 SeatIndex
) const
{
    for (APlayerState* PlayerState : PlayerArray)
    {
        APlayerSessionState* PSS =
            Cast<APlayerSessionState>(PlayerState);

        if (!PSS)
            continue;

        if (PSS->GetSeatIndex() == SeatIndex)
        {
            return PSS;
        }
    }

    return nullptr;
}


// ========================
// PlayerState -> Seat
// =========================

int32 AGameSessionState::GetSeatIndexOfPlayerState(
    const APlayerState* PlayerState
) const
{
    if (!PlayerState)
        return INDEX_NONE;

    const APlayerSessionState* PSS =
        Cast<APlayerSessionState>(PlayerState);

    if (!PSS)
        return INDEX_NONE;

    return PSS->GetSeatIndex();
}


// ===================
// Pawn -> Seat
// ====================

int32 AGameSessionState::GetSeatIndexOfPawn(
    const APawn* Pawn
) const
{
    if (!Pawn)
        return INDEX_NONE;

    const APlayerState* PS =
        Pawn->GetPlayerState();

    if (!PS)
        return INDEX_NONE;

    return GetSeatIndexOfPlayerState(PS);
}


// ==================
// Seat -> Pawn
// ==================

APawn* AGameSessionState::GetPawnBySeat(
    int32 SeatIndex
) const
{
    APlayerSessionState* PSS =
        GetPlayerSessionStateBySeat(SeatIndex);

    if (!PSS)
        return nullptr;

    if (APawn* Pawn = PSS->GetPawn())
    {
        return Pawn;
    }

    // PlayerController를 이용하는 탐색은 서버에서만
    if (HasAuthority())
    {
        if (APlayerController* PC =
            GetPlayerControllerBySeat(SeatIndex))
        {
            return PC->GetPawn();
        }
    }

    return nullptr;
}


// ======================
// Local Player Seat
// =======================

int32 AGameSessionState::GetLocalSeatIndex() const
{
    if (!GetWorld())
        return INDEX_NONE;

    APlayerController* LocalPC =
        GetWorld()->GetFirstPlayerController();

    if (!LocalPC)
        return INDEX_NONE;

    APlayerSessionState* PSS =
        LocalPC->GetPlayerState<APlayerSessionState>();

    if (!PSS)
        return INDEX_NONE;

    return PSS->GetSeatIndex();
}


// ===========================
// Seat -> PlayerController
// ===========================

APlayerController*
AGameSessionState::GetPlayerControllerBySeat(
    int32 SeatIndex
) const
{
    if (!HasAuthority())
        return nullptr;

    for (APlayerState* PlayerState : PlayerArray)
    {
        APlayerSessionState* PSS =
            Cast<APlayerSessionState>(PlayerState);

        if (!PSS)
            continue;

        if (PSS->GetSeatIndex() != SeatIndex)
            continue;

        return Cast<APlayerController>(PSS->GetOwner());
    }

    return nullptr;
}