// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "PlayerSessionState.generated.h"

// 플레이어 로딩 상태
UENUM(BlueprintType)
enum class EPlayerLoadState : uint8
{
    None,
    Loading,
    Loaded
};

// 캐릭터 종류
UENUM(BlueprintType)
enum class EPlayerCharacterType : uint8
{
    None        UMETA(DisplayName = "None"),

    Character01 UMETA(DisplayName = "Character 01"),
    Character02 UMETA(DisplayName = "Character 02"),
    Character03 UMETA(DisplayName = "Character 03"),
    Character04 UMETA(DisplayName = "Character 04")
};


// 플레이어 성별
UENUM(BlueprintType)
enum class EPlayerGender : uint8
{
    None    UMETA(DisplayName = "None"),
    Male    UMETA(DisplayName = "Male"),
    Female  UMETA(DisplayName = "Female")
};


/**
* 로비 / 인게임에서 공통으로 유지되는 플레이어 상태
 *
 * [공통]
 * - SeatIndex
 * - Nickname
 * - Host 여부
 * - CharacterType
 * - Gender
 *
 * [Lobby]
 * - Ready
 * - LoadState
 *
 * [InGame]
 * - Alive
 * - Escaped
 */

UCLASS()
class GOCLEAN_API APlayerSessionState : public APlayerState
{
	GENERATED_BODY()

public:

    // =====================
    // Common - Getter
    // =====================

    UFUNCTION(BlueprintPure, Category = "Player|Common")
    int32 GetSeatIndex() const
    {
        return SeatIndex;
    }


    UFUNCTION(BlueprintPure, Category = "Player|Common")
    const FString& GetNickname() const
    {
        return Nickname;
    }


    UFUNCTION(BlueprintPure, Category = "Player|Common")
    bool IsHost() const
    {
        return bIsHost;
    }

    // ===================
    // Character Getter
    // ===================

    UFUNCTION(BlueprintPure, Category = "Player|Character")
    EPlayerCharacterType GetCharacterType() const
    {
        return CharacterType;
    }


    UFUNCTION(BlueprintPure, Category = "Player|Character")
    EPlayerGender GetGender() const
    {
        return Gender;
    }


    // =================
    // Lobby - Getter
    // =================

    UFUNCTION(BlueprintPure, Category = "Player|Lobby")
    bool IsReady() const
    {
        return bIsReady;
    }

    UFUNCTION(BlueprintPure, Category = "Player|Load")
    EPlayerLoadState GetLoadState() const
    {
        return LoadState;
    }

    UFUNCTION(BlueprintPure, Category = "Player|Load")
    bool IsLoadingComplete() const
    {
        return LoadState == EPlayerLoadState::Loaded;
    }


    // =====================
    // InGame - Getter
    // =====================

    UFUNCTION(BlueprintPure, Category = "Player|InGame")
    bool IsAlive() const
    {
        return bIsAlive;
    }

    UFUNCTION(BlueprintPure, Category = "Player|InGame")
    bool HasEscaped() const
    {
        return bHasEscaped;
    }


    // ==========================
    // Client -> Server Request
    // ==========================

     // Ready 변경
    UFUNCTION(BlueprintCallable, Category = "Player|Lobby")
    void RequestSetReady(bool bNewReady);

    UFUNCTION(Server, Reliable)
    void Server_SetReady(bool bNewReady);


    // 로딩 상태 보고
    UFUNCTION(BlueprintCallable, Category = "Player|Load")
    void RequestSetLoadState(EPlayerLoadState NewState);

    UFUNCTION(Server, Reliable)
    void Server_SetLoadState(EPlayerLoadState NewState);


    // ========================
    // Server-only Setter
    // GameMode등의 서버로직에서 사용
    // ========================

    void SetSeatIndex(int32 NewSeatIndex);

    void SetNickname(const FString& NewNickname);

    void SetIsHost(bool bNewIsHost);

    // 캐릭터 종류는 서버가 배정
    void SetCharacterType(EPlayerCharacterType NewCharacterType);

    void SetGender(EPlayerGender NewGender);

    void SetReady(bool bNewReady);

    void SetLoadState(EPlayerLoadState NewState);

    void SetAlive(bool bNewAlive);

    void SetEscaped(bool bNewEscaped);



protected:

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


    // ==================
    // OnRep - Common
    // ==================

    UFUNCTION()
    void OnRep_SeatIndex();

    UFUNCTION()
    void OnRep_Nickname();

    UFUNCTION()
    void OnRep_IsHost();


    // ===================
    // OnRep - Character
    // ===================

    UFUNCTION()
    void OnRep_CharacterType();

    UFUNCTION()
    void OnRep_Gender();


    // ===================
    // OnRep - Lobby
    // ===================

    UFUNCTION()
    void OnRep_Ready();

    UFUNCTION()
    void OnRep_LoadState();


    // ================
    // OnRep - InGame
    // ================

    UFUNCTION()
    void OnRep_IsAlive();

    UFUNCTION()
    void OnRep_HasEscaped();


    // ============================================================
   // Blueprint Events
   //
   // UI / Character BP에서 상태 변경을 바로 받을 수 있게 사용
   // ============================================================

    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Common")
    void BP_OnSeatIndexChanged(int32 NewSeatIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Common")
    void BP_OnNicknameChanged(const FString& NewNickname);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Common")
    void BP_OnHostChanged(bool bNewIsHost);


    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Character")
    void BP_OnCharacterTypeChanged(EPlayerCharacterType NewCharacterType);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Character")
    void BP_OnGenderChanged(EPlayerGender NewGender);


    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Lobby")
    void BP_OnReadyChanged(bool bNewReady);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player|Load")
    void BP_OnLoadStateChanged(EPlayerLoadState NewState);


    UFUNCTION(BlueprintImplementableEvent, Category = "Player|InGame")
    void BP_OnAliveChanged(bool bNewAlive);

    UFUNCTION(BlueprintImplementableEvent, Category = "Player|InGame")
    void BP_OnEscapedChanged(bool bNewEscaped);


private:

    // ============
    // Common
    // ============

    // 플레이어 좌석
    // 0 ~ 3
    UPROPERTY(ReplicatedUsing = OnRep_SeatIndex, BlueprintReadOnly, Category = "Player|Common", meta = (AllowPrivateAccess = "true"))
    int32 SeatIndex = INDEX_NONE;


    // Steam 닉네임
    UPROPERTY(ReplicatedUsing = OnRep_Nickname, BlueprintReadOnly, Category = "Player|Common", meta = (AllowPrivateAccess = "true"))
    FString Nickname;


    // Listen Server Host 여부
    UPROPERTY(ReplicatedUsing = OnRep_IsHost, BlueprintReadOnly, Category = "Player|Common", meta = (AllowPrivateAccess = "true"))
    bool bIsHost = false;


    // ============
    // Character
    // ============
    
    // 4종 캐릭터 중 서버가 배정한 캐릭터
    UPROPERTY(ReplicatedUsing = OnRep_CharacterType, BlueprintReadOnly, Category = "Player|Character", meta = (AllowPrivateAccess = "true"))
    EPlayerCharacterType CharacterType = EPlayerCharacterType::None;

    // 캐릭터 성별
    UPROPERTY(ReplicatedUsing = OnRep_Gender, BlueprintReadOnly, Category = "Player|Customization", meta = (AllowPrivateAccess = "true"))
    EPlayerGender Gender = EPlayerGender::None;


    // =============
    // Lobby
    // =============

    // 준비 상태
    UPROPERTY(ReplicatedUsing = OnRep_Ready, BlueprintReadOnly, Category = "Player|Lobby", meta = (AllowPrivateAccess = "true"))
    bool bIsReady = false;


    // 로딩 상태
    UPROPERTY(ReplicatedUsing = OnRep_LoadState, BlueprintReadOnly, Category = "Player|Load", meta = (AllowPrivateAccess = "true"))
    EPlayerLoadState LoadState = EPlayerLoadState::None;


    // ============
    // InGame
    // ============

    // 현재 생존 여부
    UPROPERTY(ReplicatedUsing = OnRep_IsAlive, BlueprintReadOnly, Category = "Player|InGame", meta = (AllowPrivateAccess = "true"))
    bool bIsAlive = true;


    // 철수 완료 여부
    UPROPERTY(ReplicatedUsing = OnRep_HasEscaped, BlueprintReadOnly,Category = "Player|InGame", meta = (AllowPrivateAccess = "true"))
    bool bHasEscaped = false;
	
};
