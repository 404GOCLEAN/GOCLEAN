// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/LobbyGameMode.h"

#include "ServerModule/GameSession/LobbyGameState.h"
#include "ServerModule/GameSession/GameSessionInstance.h"

#include "Kismet/GameplayStatics.h"

ALobbyGameMode::ALobbyGameMode()
{
    GameStateClass = ALobbyGameState::StaticClass();

    PlayerStateClass = APlayerSessionState::StaticClass();

    bUseSeamlessTravel = true;
}


// ===============
// BeginPlay
// ==============

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    InitializeCharacterOrder();
}


// =====================
// Character Order
// ====================

void ALobbyGameMode::InitializeCharacterOrder()
{
    CharacterOrder.Reset();

    CharacterOrder.Add(EPlayerCharacterType::Character01);

    CharacterOrder.Add(EPlayerCharacterType::Character02);

    CharacterOrder.Add(EPlayerCharacterType::Character03);

    CharacterOrder.Add(EPlayerCharacterType::Character04);


    for (int32 i = CharacterOrder.Num() - 1; i > 0; --i)
    {
        const int32 SwapIndex = FMath::RandRange(0, i);

        CharacterOrder.Swap(i, SwapIndex);
    }
}


// ==============
// ==============

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer)
        return;


    // 최대 인원 검사
    if (GetSessionPlayerCount() > MaxPlayers)
    {
        NewPlayer->ClientReturnToMainMenuWithTextReason(
            FText::FromString(
                TEXT("방이 가득 찼습니다.")
            )
        );

        return;
    }


    InitializeLobbyPlayer(NewPlayer);


    /*
     * PlayerController 구현과 연결 필요
     *
     * AGOCLEANPlayerController* PC = Cast<AGOCLEANPlayerController>(NewPlayer);
     *
     * if (PC)
     * {
     *     PC->Client_ShowLobbyUI();
     * }
     */
}


// ==========================
// Player Initialization
// =========================

void ALobbyGameMode::InitializeLobbyPlayer(APlayerController* Player)
{
    if (!Player)
        return;


    APlayerSessionState* PSS = GetPlayerSessionState(Player);

    if (!PSS)
        return;


    // Seat
    AssignSeat(PSS);


    // Host
    AssignHost(Player, PSS);


    // Character
    AssignCharacter(PSS);


    // Nickname
    SetPlayerNickname(Player, PSS);


    // Lobby 초기 상태
    PSS->SetReady(false);
    PSS->SetLoadState(EPlayerLoadState::None);

    // 인게임 상태도 초기값
    PSS->SetAlive(true);
    PSS->SetEscaped(false);
}


// ================
// Seat
// ================

int32 ALobbyGameMode::FindNextAvailableSeatIndex() const
{
    const ALobbyGameState* LGS = GetGameState<ALobbyGameState>();

    if (!LGS)
        return INDEX_NONE;


    for (int32 Seat = 0; Seat < MaxPlayers; ++Seat)
    {
        if (!LGS->GetPlayerSessionStateBySeat(Seat))
        {
            return Seat;
        }
    }


    return INDEX_NONE;
}


bool ALobbyGameMode::IsSeatOccupied(int32 SeatIndex) const
{
    const ALobbyGameState* LGS = GetGameState<ALobbyGameState>();

    if (!LGS)
        return false;


    return
        LGS->GetPlayerSessionStateBySeat(SeatIndex) != nullptr;
}


void ALobbyGameMode::AssignSeat(APlayerSessionState* PlayerState)
{
    if (!PlayerState)
        return;


    const int32 Seat = FindNextAvailableSeatIndex();


    if (Seat == INDEX_NONE)
        return;


    PlayerState->SetSeatIndex(Seat);
}


// ===========
// Host
// ===========

void ALobbyGameMode::AssignHost(APlayerController* Player, APlayerSessionState* PlayerState)
{
    if (!Player || !PlayerState)
        return;


    // Listen Server의 로컬 Controller = Host
    const bool bHost = Player->IsLocalController();


    PlayerState->SetIsHost(bHost);
}


bool ALobbyGameMode::IsHost(const APlayerSessionState* PlayerState) const
{
    return PlayerState && PlayerState->IsHost();
}


// =============
// Character
// =============

void ALobbyGameMode::AssignCharacter(APlayerSessionState* PlayerState)
{
    if (!PlayerState)
        return;


    const int32 Seat = PlayerState->GetSeatIndex();


    if (!CharacterOrder.IsValidIndex(Seat))
        return;


    const EPlayerCharacterType Character = CharacterOrder[Seat];


    PlayerState->SetCharacterType(Character);


    PlayerState->SetGender(GetGenderForCharacter(Character));
}


EPlayerGender ALobbyGameMode::GetGenderForCharacter(EPlayerCharacterType CharacterType) const
{
    
    // 캐릭터에 맞춰 성별 수정 필요

    switch (CharacterType)
    {
    case EPlayerCharacterType::Character01:
        return EPlayerGender::Male;

    case EPlayerCharacterType::Character02:
        return EPlayerGender::Male;

    case EPlayerCharacterType::Character03:
        return EPlayerGender::Female;

    case EPlayerCharacterType::Character04:
        return EPlayerGender::Female;

    default:
        return EPlayerGender::None;
    }
}


// ==============
// Nickname
// ==============

void ALobbyGameMode::SetPlayerNickname(APlayerController* Player, APlayerSessionState* PlayerState)
{
    if (!Player || !PlayerState)
        return;


    FString Name = PlayerState->GetPlayerName();


    if (Name.IsEmpty())
    {
        Name = FString::Printf( TEXT("Player%d"), PlayerState->GetSeatIndex() + 1);
    }


    PlayerState->SetNickname(Name);
}


// ============
// Ready
// ============

bool ALobbyGameMode::RequestSetPlayerReady(APlayerSessionState* PlayerState, bool bNewReady)
{
    if (!PlayerState)
        return false;


    if (bGameStarting)
        return false;


    PlayerState->SetReady(bNewReady);


    return true;
}


bool ALobbyGameMode::AreAllPlayersReady() const
{
    if (!GameState)
        return false;


    int32 PlayerCount = 0;


    for (APlayerState* PS : GameState->PlayerArray)
    {
        const APlayerSessionState* PSS = Cast<APlayerSessionState>(PS);

        if (!PSS)
            continue;


        ++PlayerCount;


        // Host는 Ready가 필요 없음
        
        if (PSS->IsHost())
            continue;

        if (!PSS->IsReady())
        {
            return false;
        }
    }


    return PlayerCount > 0;
}


// =======================
// Start Condition
// =======================

bool ALobbyGameMode::CanStartGame() const
{
    if (bGameStarting)
        return false;


    if (GetSessionPlayerCount() < MinPlayersToStart)
        return false;


    if (!AreAllPlayersReady())
        return false;


    const ALobbyGameState* LGS =
        GetGameState<ALobbyGameState>();

    if (!LGS)
        return false;


    if (LGS->GetSelectedContractId() <= 0)
        return false;


    return true;
}


// ================
// Contract
// ================

bool ALobbyGameMode::RequestSetContract(APlayerController* Requester, int32 ContractId)
{
    if (!Requester)
        return false;


    APlayerSessionState* PSS = GetPlayerSessionState(Requester);

    if (!IsHost(PSS))
        return false;


    if (bGameStarting)
        return false;


    if (ContractId <= 0)
        return false;


    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return false;


    LGS->SetSelectedContractId(ContractId);


    return true;
}


// =======================
// Vending Purchase
// ======================

bool ALobbyGameMode::RequestPurchaseVending(APlayerController* Buyer, int32 ItemId)
{
    if (!Buyer)
        return false;


    APlayerSessionState* PSS = GetPlayerSessionState(Buyer);

    if (!PSS)
        return false;


    // Ready 이후 구매 불가
    if (PSS->IsReady())
        return false;


    if (bGameStarting)
        return false;


    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return false;


    // 전체 최대 5개
    if (LGS->GetTotalPurchasedCount() >= MaxVendingPurchaseCount)
    {
        return false;
    }


    // 해당 아이템 재고 확인
    if (!LGS->CanPurchaseItem(ItemId))
        return false;


    /*
     * TODO: 플레이어 재화 확인
     *
     * TODO: 실제 재화 차감
     */


    return LGS->AddVendingPurchase(ItemId, PSS->GetSeatIndex());
}


// ===================
// Vending Cancel
// ===================

bool ALobbyGameMode::RequestCancelVendingPurchase(APlayerController* Buyer, int32 ItemId)
{
    if (!Buyer)
        return false;


    APlayerSessionState* PSS = GetPlayerSessionState(Buyer);

    if (!PSS)
        return false;


    if (PSS->IsReady())
        return false;


    if (bGameStarting)
        return false;


    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return false;


    const bool bRemoved = LGS->RemoveVendingPurchase(ItemId, PSS->GetSeatIndex());


    if (bRemoved)
    {
        /*
         * TODO: 구매 취소 시 재화 반환
         */
    }


    return bRemoved;
}


// ================
// Start Game
// ==============

bool ALobbyGameMode::RequestStartGame(APlayerController* Requester)
{
    if (!Requester)
        return false;


    APlayerSessionState* PSS = GetPlayerSessionState(Requester);

    if (!IsHost(PSS))
        return false;


    if (!CanStartGame())
        return false;


    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return false;


    UGameSessionInstance* GI = GetGameInstance<UGameSessionInstance>();

    if (!GI)
        return false;


    bGameStarting = true;


    // ===================
    // 선택 의뢰 확정
    // ===================

    const int32 ContractId = LGS->GetSelectedContractId();

    GI->SetPendingContractId(LGS->GetSelectedContractId());


    return true;
}


// =============
// Logout
// ============

void ALobbyGameMode::Logout(AController* Exiting)
{
    if (!Exiting)
    {
        Super::Logout(Exiting);
        return;
    }


    APlayerSessionState* PSS = GetPlayerSessionState(Exiting);


    if (PSS)
    {
        const int32 LeavingSeat = PSS->GetSeatIndex();


        ALobbyGameState* LGS = GetLobbyGameState();


        if (LGS)
        {
            // 나간 플레이어가 구매한 벤딩 아이템 반환
            LGS->RemoveAllVendingPurchasesBySeat(
                LeavingSeat
            );
        }


        /*
         * 현재는 Seat을 앞으로 당기지 않음.
         *
         * 예:
         *
         * 0 : Player A
         * 1 : Empty
         * 2 : Player C
         *
         * 신규 참가자는 Seat 1을 사용.
         *
         * 이 방식이면 CharacterType과 SeatIndex가
         * 플레이 도중 불필요하게 변경되지 않음.
         */
    }


    Super::Logout(Exiting);
}


// =============
// Helper
// ============

ALobbyGameState* ALobbyGameMode::GetLobbyGameState() const
{
    return GetGameState<ALobbyGameState>();
}