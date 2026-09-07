// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/PlayerSessionState.h"
#include "ServerModule/GameSession/GameSessionMode.h"
#include "ServerModule/GameSession/LobbyGameMode.h"
#include <Net/UnrealNetwork.h>

//APlayerSessionState::APlayerSessionState()
//{
//    bReplicates = true;
//}


// =================
// Replication
// ===============

void APlayerSessionState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Common
    DOREPLIFETIME(APlayerSessionState, SeatIndex);
    DOREPLIFETIME(APlayerSessionState, Nickname);
    DOREPLIFETIME(APlayerSessionState, bIsHost);

    // Character
    DOREPLIFETIME(APlayerSessionState, CharacterType);
    DOREPLIFETIME(APlayerSessionState, Gender);

    // Lobby
    DOREPLIFETIME(APlayerSessionState, bIsReady);
    DOREPLIFETIME(APlayerSessionState, LoadState);

    // InGame
    DOREPLIFETIME(APlayerSessionState, bIsAlive);
    DOREPLIFETIME(APlayerSessionState, bHasEscaped);
}


// ===============
// Ready Request
// ===============

void APlayerSessionState::RequestSetReady(bool bNewReady)
{
    if (HasAuthority())
    {
        ALobbyGameMode* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;

        if (LobbyGM)
        {
            LobbyGM->RequestSetPlayerReady(
                this,
                bNewReady
            );
        }

        return;
    }


    Server_SetReady(bNewReady);
}


void APlayerSessionState::Server_SetReady_Implementation(bool bNewReady)
{
    ALobbyGameMode* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;

    if (!LobbyGM)
        return;


    LobbyGM->RequestSetPlayerReady(this, bNewReady);

    /*SetReady(bNewReady);*/
}

// ============================
// Load State Request
// ============================

void APlayerSessionState::RequestSetLoadState( EPlayerLoadState NewState )
{
    if (HasAuthority())
    {
        SetLoadState(NewState);
        return;
    }

    Server_SetLoadState(NewState);
}


void APlayerSessionState::Server_SetLoadState_Implementation( EPlayerLoadState NewState )
{
    SetLoadState(NewState);
}


// ================
// Common Setter
// =================

void APlayerSessionState::SetSeatIndex( int32 NewSeatIndex )
{
    if (!HasAuthority())
        return;

    if (SeatIndex == NewSeatIndex)
        return;

    SeatIndex = NewSeatIndex;

    // Listen Server Host에서도 변경 이벤트 적용
    OnRep_SeatIndex();
}


void APlayerSessionState::SetNickname( const FString& NewNickname )
{
    if (!HasAuthority())
        return;

    if (Nickname == NewNickname)
        return;

    Nickname = NewNickname;

    OnRep_Nickname();
}


void APlayerSessionState::SetIsHost( bool bNewIsHost )
{
    if (!HasAuthority())
        return;

    if (bIsHost == bNewIsHost)
        return;

    bIsHost = bNewIsHost;

    OnRep_IsHost();
}


// =====================
// Character Setter
// =====================

void APlayerSessionState::SetCharacterType( EPlayerCharacterType NewCharacterType )
{
    if (!HasAuthority())
        return;

    if (NewCharacterType == EPlayerCharacterType::None)
        return;

    if (CharacterType == NewCharacterType)
        return;

    CharacterType = NewCharacterType;

    OnRep_CharacterType();
}


void APlayerSessionState::SetGender( EPlayerGender NewGender )
{
    if (!HasAuthority())
        return;

    if (NewGender == EPlayerGender::None)
        return;

    if (Gender == NewGender)
        return;

    Gender = NewGender;

    OnRep_Gender();
}


// =====================
// Lobby Setter
// =====================

void APlayerSessionState::SetReady( bool bNewReady )
{
    if (!HasAuthority())
        return;

    if (bIsReady == bNewReady)
        return;

    bIsReady = bNewReady;

    OnRep_Ready();
}


void APlayerSessionState::SetLoadState( EPlayerLoadState NewState )
{
    if (!HasAuthority())
        return;

    if (LoadState == NewState)
        return;

    LoadState = NewState;

    OnRep_LoadState();
}


// ===============
// InGame Setter
// ================

void APlayerSessionState::SetAlive( bool bNewAlive )
{
    if (!HasAuthority())
        return;

    if (bIsAlive == bNewAlive)
        return;

    bIsAlive = bNewAlive;

    OnRep_IsAlive();
}


void APlayerSessionState::SetEscaped( bool bNewEscaped )
{
    if (!HasAuthority())
        return;

    if (bHasEscaped == bNewEscaped)
        return;

    bHasEscaped = bNewEscaped;

    OnRep_HasEscaped();
}


// ===================
// OnRep - Common
// ===================

void APlayerSessionState::OnRep_SeatIndex()
{
    BP_OnSeatIndexChanged(SeatIndex);
}


void APlayerSessionState::OnRep_Nickname()
{
    BP_OnNicknameChanged(Nickname);
}


void APlayerSessionState::OnRep_IsHost()
{
    BP_OnHostChanged(bIsHost);
}


// ======================
// OnRep - Character
// =======================

void APlayerSessionState::OnRep_CharacterType()
{
    BP_OnCharacterTypeChanged(CharacterType);
}


void APlayerSessionState::OnRep_Gender()
{
    BP_OnGenderChanged(Gender);
}


// ========================
// OnRep - Lobby
// ========================

void APlayerSessionState::OnRep_Ready()
{
    BP_OnReadyChanged(bIsReady);
}


void APlayerSessionState::OnRep_LoadState()
{
    BP_OnLoadStateChanged(LoadState);
}


// =====================
// OnRep - InGame
// =====================

void APlayerSessionState::OnRep_IsAlive()
{
    BP_OnAliveChanged(bIsAlive);
}


void APlayerSessionState::OnRep_HasEscaped()
{
    BP_OnEscapedChanged(bHasEscaped);
}