// Fill out your copyright notice in the Description page of Project Settings.


#include "ServerModule/GameSession/LobbyGameMode.h"

#include "ServerModule/GameSession/LobbyGameState.h"
#include "ServerModule/GameSession/GameSessionInstance.h"

#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

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

    // ★ 임시 테스트용 계약 ID
    if (HasAuthority())
    {
        if (ALobbyGameState* LGS =
            GetLobbyGameState())
        {
            LGS->SetSelectedContractId(1);

            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[Lobby] Test ContractId set to 1")
            );
        }
    }

    // 로비 월드와 TargetPoint가 모두 준비된 다음 검사
    GetWorldTimerManager().SetTimerForNextTick(
        this,
        &ALobbyGameMode::EnsureAllLobbyCharactersSpawned
    );
}


// =====================
// Character Order
// ====================

void ALobbyGameMode::InitializeCharacterOrder()
{
    // 이미 정상적으로 생성됐다면 다시 섞지 않음
    if (CharacterOrder.Num() == 4)
    {
        return;
    }

    CharacterOrder.Empty();

    CharacterOrder.Add(EPlayerCharacterType::Character01);
    CharacterOrder.Add(EPlayerCharacterType::Character02);
    CharacterOrder.Add(EPlayerCharacterType::Character03);
    CharacterOrder.Add(EPlayerCharacterType::Character04);

    // Fisher-Yates 방식으로 순서 섞기
    for (int32 Index = CharacterOrder.Num() - 1; Index > 0; --Index)
    {
        const int32 SwapIndex =
            FMath::RandRange(0, Index);

        CharacterOrder.Swap(Index, SwapIndex);
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[LobbyAssign] CharacterOrder initialized: %s(%d), %s(%d), %s(%d), %s(%d)"),
        *UEnum::GetValueAsString(CharacterOrder[0]),
        static_cast<int32>(CharacterOrder[0]),
        *UEnum::GetValueAsString(CharacterOrder[1]),
        static_cast<int32>(CharacterOrder[1]),
        *UEnum::GetValueAsString(CharacterOrder[2]),
        static_cast<int32>(CharacterOrder[2]),
        *UEnum::GetValueAsString(CharacterOrder[3]),
        static_cast<int32>(CharacterOrder[3])
    );
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


    // PlayerArray와 로비 월드 상태가 반영된 다음 검사
    GetWorldTimerManager().SetTimerForNextTick(
        this,
        &ALobbyGameMode::EnsureAllLobbyCharactersSpawned
    );

    if (ALobbyGameState* LGS = GetLobbyGameState())
    {
        LGS->NotifyLobbyPlayersChanged();
    }

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

const FVendingItemData* ALobbyGameMode::FindVendingItemData(int32 ItemId) const
{
    if (!VendingItemDataTable)
        return nullptr;


    const TArray<FName> RowNames = VendingItemDataTable->GetRowNames();


    for (const FName& RowName : RowNames)
    {
        const FVendingItemData* Row = VendingItemDataTable->FindRow<FVendingItemData>(
                RowName,
                TEXT("FindVendingItemData")
            );


        if (!Row)
            continue;


        if (Row->VendingItemId == ItemId)
        {
            return Row;
        }
    }


    return nullptr;
}

void ALobbyGameMode::EnsureAllLobbyCharactersSpawned()
{
    if (!HasAuthority())
        return;

    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return;

    for (APlayerState* PlayerState : LGS->PlayerArray)
    {
        APlayerSessionState* PSS =
            Cast<APlayerSessionState>(PlayerState);

        if (!PSS)
            continue;

        const int32 SeatIndex = PSS->GetSeatIndex();

        if (SeatIndex < 0)
            continue;

        if (PSS->GetCharacterType() == EPlayerCharacterType::None)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[LobbySpawn] CharacterType is None. Reassigning: Seat=%d"),
                SeatIndex
            );

            AssignCharacter(PSS);
        }

        if (PSS->GetCharacterType() == EPlayerCharacterType::None)
        {
            UE_LOG(
                LogTemp,
                Error,
                TEXT("[LobbySpawn] Reassignment failed: Seat=%d"),
                SeatIndex
            );

            continue;
        }

        SpawnLobbyCharacter(PSS);

        // 이미 해당 자리에 정상적인 캐릭터가 있으면 스폰하지 않음
        if (TObjectPtr<AActor>* FoundCharacter =
            SpawnedLobbyCharacters.Find(SeatIndex))
        {
            if (IsValid(FoundCharacter->Get()))
            {
                continue;
            }

            SpawnedLobbyCharacters.Remove(SeatIndex);
        }

        SpawnLobbyCharacter(PSS);
    }
}



void ALobbyGameMode::SpawnLobbyCharacter(APlayerSessionState* PlayerState)
{
    if (!HasAuthority() || !PlayerState)
        return;


    const int32 SeatIndex = PlayerState->GetSeatIndex();

    if (SeatIndex < 0) return;


    // 해당 Seat에 기존 캐릭터가 있으면 제거
    DestroyLobbyCharacter(SeatIndex);


    const EPlayerCharacterType CharacterType = PlayerState->GetCharacterType();


    const TSubclassOf<AActor> CharacterClass = GetLobbyCharacterClass(CharacterType);

    if (!CharacterClass)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Lobby Character Class is not assigned. CharacterType: %d"),
            static_cast<int32>(CharacterType)
        );

        return;
    }


    AActor* SpawnPoint = FindLobbyCharacterSpawnPoint(SeatIndex);

    if (!SpawnPoint)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Lobby spawn point not found. SeatIndex: %d"),
            SeatIndex
        );

        return;
    }


    FActorSpawnParameters SpawnParams;

    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


    AActor* SpawnedCharacter =
        GetWorld()->SpawnActor<AActor>(
            CharacterClass,
            SpawnPoint->GetActorTransform(),
            SpawnParams
        );


    if (!SpawnedCharacter) return;


    SpawnedLobbyCharacters.Add(SeatIndex, SpawnedCharacter);
}



void ALobbyGameMode::DestroyLobbyCharacter(int32 SeatIndex)
{
    TObjectPtr<AActor>* Found = SpawnedLobbyCharacters.Find(SeatIndex);

    if (!Found) return;

    AActor* Character = Found->Get();

    if (IsValid(Character))
    {
        Character->Destroy();
    }


    SpawnedLobbyCharacters.Remove(SeatIndex);
}

AActor* ALobbyGameMode::FindLobbyCharacterSpawnPoint(int32 SeatIndex) const
{
    if (!GetWorld())
        return nullptr;

    const FName TargetTag( *FString::Printf( TEXT("LobbySeat%d"), SeatIndex ));

    TArray<AActor*> TargetPoints;

    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), TargetPoints);

    for (AActor* Target : TargetPoints)
    {
        if (Target && Target->ActorHasTag(TargetTag))
        {
            return Target;
        }
    }

    return nullptr;
}

TSubclassOf<AActor> ALobbyGameMode::GetLobbyCharacterClass(EPlayerCharacterType CharacterType) const
{
    switch (CharacterType)
    {
    case EPlayerCharacterType::Character01:
        return Character01LobbyClass;

    case EPlayerCharacterType::Character02:
        return Character02LobbyClass;

    case EPlayerCharacterType::Character03:
        return Character03LobbyClass;

    case EPlayerCharacterType::Character04:
        return Character04LobbyClass;

    default:
        return nullptr;
    }
}


// =============
// Character
// =============

void ALobbyGameMode::AssignCharacter(APlayerSessionState* PlayerState)
{
    if (!HasAuthority() || !PlayerState)
        return;

    const int32 SeatIndex = PlayerState->GetSeatIndex();

    if (SeatIndex < 0)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[LobbyAssign] Failed: Invalid SeatIndex=%d"),
            SeatIndex
        );
        return;
    }

    // 호스트 PostLogin이 BeginPlay보다 먼저 실행되는 경우 대비
    if (CharacterOrder.Num() == 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LobbyAssign] CharacterOrder is empty. Initializing now.")
        );

        InitializeCharacterOrder();
    }

    if (!CharacterOrder.IsValidIndex(SeatIndex))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[LobbyAssign] Failed: Invalid CharacterOrder index. Seat=%d, Count=%d"),
            SeatIndex,
            CharacterOrder.Num()
        );
        return;
    }

    const EPlayerCharacterType AssignedType = CharacterOrder[SeatIndex];

    PlayerState->SetCharacterType(AssignedType);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[LobbyAssign] Success: Player=%s, Seat=%d, Type=%s(%d)"),
        *GetNameSafe(PlayerState),
        SeatIndex,
        *UEnum::GetValueAsString(AssignedType),
        static_cast<int32>(AssignedType)
    );
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


        // 방장을 포함해 한 명이라도 Ready가 아니면 시작 불가
        if (!PSS->IsReady())
        {
            return false;
        }


        //// Host는 Ready가 필요 없음
        //
        //if (PSS->IsHost())
        //    continue;

        //if (!PSS->IsReady())
        //{
        //    return false;
        //}
    }


    return PlayerCount > 0;
}


// =======================
// Start Condition
// =======================

//bool ALobbyGameMode::CanStartGame() const
//{
//    if (bGameStarting)
//        return false;
//
//
//    if (GetSessionPlayerCount() < MinPlayersToStart)
//        return false;
//
//
//    if (!AreAllPlayersReady())
//        return false;
//
//
//    const ALobbyGameState* LGS =
//        GetGameState<ALobbyGameState>();
//
//    if (!LGS)
//        return false;
//
//
//    if (LGS->GetSelectedContractId() <= 0)
//        return false;
//
//
//    return true;
//}


bool ALobbyGameMode::CanStartGame() const
{
    if (bGameStarting)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[CanStartGame] Failed: Game already starting")
        );

        return false;
    }


    const int32 PlayerCount =
        GetSessionPlayerCount();

    if (PlayerCount < MinPlayersToStart)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[CanStartGame] Failed: PlayerCount=%d, Min=%d"
            ),
            PlayerCount,
            MinPlayersToStart
        );

        return false;
    }


    if (!AreAllPlayersReady())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[CanStartGame] Failed: Not all players ready")
        );

        return false;
    }


    const ALobbyGameState* LGS =
        GetGameState<ALobbyGameState>();

    if (!LGS)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[CanStartGame] Failed: LobbyGameState is null")
        );

        return false;
    }


    const int32 ContractId =
        LGS->GetSelectedContractId();

    if (ContractId <= 0)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT(
                "[CanStartGame] Failed: SelectedContractId=%d"
            ),
            ContractId
        );

        return false;
    }


    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[CanStartGame] Success: Players=%d, Contract=%d"
        ),
        PlayerCount,
        ContractId
    );

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


    // 게임 시작 중
    if (bGameStarting)
        return false;


    // Ready 이후 구매 금지
    if (PSS->IsReady())
        return false;


    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return false;


    // 최대 5종
    if (LGS->GetPurchasedVendingCount() >= MaxVendingPurchaseCount)
    {
        return false;
    }


    // 해당 종류 이미 구매됨
    if (LGS->IsVendingItemPurchased(ItemId))
    {
        return false;
    }


    // DataTable
    const FVendingItemData* ItemData = FindVendingItemData(ItemId);

    if (!ItemData)
        return false;


    if (!ItemData->bAvailableInVending)
        return false;


    // 재화 부족
    if (PSS->GetLobbyMoney() < ItemData->Price)
    {
        return false;
    }


    // 먼저 돈 차감
    if (!PSS->SpendLobbyMoney(ItemData->Price))
    {
        return false;
    }


    // 구매 상태 등록
    if (!LGS->AddVendingPurchase(ItemId, PSS->GetSeatIndex()))
    {
        // 실패했다면 환불
        PSS->RefundLobbyMoney(ItemData->Price);

        return false;
    }


    return true;
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


    if (bGameStarting)
        return false;


    if (PSS->IsReady())
        return false;


    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return false;


    // 본인이 구매한 것만 취소 가능
    if (!LGS->DidSeatPurchaseItem(PSS->GetSeatIndex(), ItemId))
    {
        return false;
    }


    const FVendingItemData* ItemData = FindVendingItemData(ItemId);

    if (!ItemData)
        return false;


    if (!LGS->RemoveVendingPurchase(ItemId, PSS->GetSeatIndex()))
    {
        return false;
    }


    // 환불
    PSS->RefundLobbyMoney(ItemData->Price);


    return true;
}


// ================
// Start Game
// ==============

bool ALobbyGameMode::RequestStartGame(APlayerController* Requester)
{
    // 서버에서만 처리
    if (!HasAuthority())
        return false;


    if (!Requester)
        return false;


    APlayerSessionState* PSS = GetPlayerSessionState(Requester);

    // 방장만 시작 가능
    if (!IsHost(PSS))
        return false;


    // 인원, Ready, 계약 선택 여부 검사
    if (!CanStartGame())
        return false;


    ALobbyGameState* LGS = GetLobbyGameState();

    if (!LGS)
        return false;


    UGameSessionInstance* GI = GetGameInstance<UGameSessionInstance>();

    if (!GI)
        return false;


    if (GameMapPath.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Lobby] GameMapPath is empty.")
        );

        return false;
    }


    bGameStarting = true;


    // ===================
    // 선택 계약 보존
    // ===================

    GI->SetPendingContractId(LGS->GetSelectedContractId());


    // ==========================
    // 선택 벤딩 아이템 보존
    // ==========================

    TArray<int32> SelectedVendingItemIds;

    for (const FVendingPurchaseState& Purchase :
        LGS->GetPurchasedVendingItems())
    {
        if (Purchase.ItemId != INDEX_NONE)
        {
            SelectedVendingItemIds.Add(Purchase.ItemId);
        }
    }


    if (SelectedVendingItemIds.Num() > MaxVendingPurchaseCount)
    {
        SelectedVendingItemIds.SetNum(MaxVendingPurchaseCount);
    }


    GI->SetPendingVendingItemIds(
        SelectedVendingItemIds
    );


    // ==========================
    // 모든 플레이어 인게임 이동
    // ==========================

    const FString TravelURL = GameMapPath + TEXT("?listen");


    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[Lobby] ServerTravel: %s"),
        *TravelURL
    );


    if (!GetWorld()->ServerTravel(TravelURL))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[Lobby] ServerTravel failed.")
        );

        bGameStarting = false;

        return false;
    }


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

        DestroyLobbyCharacter(LeavingSeat);

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

    // 제거가 끝난 후 로비 UI 갱신
    if (ALobbyGameState* LGS = GetLobbyGameState())
    {
        LGS->NotifyLobbyPlayersChanged();
    }
}


// =============
// Helper
// ============

ALobbyGameState* ALobbyGameMode::GetLobbyGameState() const
{
    return GetGameState<ALobbyGameState>();
}