// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/GameSessionMode.h"

#include "ServerModule/GameSession/PlayerSessionState.h"

#include "GameFramework/GameStateBase.h"

AGameSessionMode::AGameSessionMode()
{
    PlayerStateClass = APlayerSessionState::StaticClass();
}


APlayerSessionState* AGameSessionMode::GetPlayerSessionState(AController* Controller) const
{
    if (!Controller)
        return nullptr;

    return Controller->GetPlayerState<APlayerSessionState>();
}


int32 AGameSessionMode::GetSessionPlayerCount() const
{
    if (!GameState)
        return 0;

    int32 Count = 0;

    for (APlayerState* PS : GameState->PlayerArray)
    {
        if (Cast<APlayerSessionState>(PS))
        {
            ++Count;
        }
    }

    return Count;
}