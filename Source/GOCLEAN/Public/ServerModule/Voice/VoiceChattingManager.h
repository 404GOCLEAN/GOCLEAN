// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "VivoxCore.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "VoiceChattingManager.generated.h"

// ========================
// Voice Channel Mode
// ========================

UENUM(BlueprintType)
enum class EVoiceChannelMode : uint8
{
    None        UMETA(DisplayName = "None"),

    // Lobby
    // 거리/방향 관계없이 동일하게 들리는 음성
    Lobby2D     UMETA(DisplayName = "Lobby 2D"),

    // InGame
    // 플레이어 위치 기반 공간 음성
    InGame3D    UMETA(DisplayName = "InGame 3D")
};

/**
 * Vivox Voice Manager
 *
 * GameInstanceSubsystem이므로 ServerTravel 이후에도 유지된다.
 *
 * Lobby
 *  -> NonPositional Channel
 *
 * InGame
 *  -> Positional Channel
 *  -> 현재 Pawn을 다시 찾아 3D Position Update 시작
 */

UCLASS()
class GOCLEAN_API UVoiceChattingManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(
        FSubsystemCollectionBase& Collection
    ) override;

    virtual void Deinitialize() override;


    // =======================
    // Vivox Login / Basic
    // ======================

public:

    UFUNCTION(BlueprintCallable, Category = "Vivox")
    void Vivox_LoginSimple(
        const FString& PlayerIdOverride
    );

    UFUNCTION(BlueprintCallable, Category = "Vivox")
    void Vivox_JoinVoice(
        const FString& ChannelName,
        bool bPositional
    );


    UFUNCTION(BlueprintCallable, Category = "Vivox")
    void Vivox_LeaveVoice();


    UFUNCTION(BlueprintCallable, Category = "Vivox")
    void Vivox_StartVoice(
        const FString& PlayerIdOverride,
        const FString& ChannelName,
        bool bPositional
    );


    UFUNCTION(BlueprintCallable, Category = "Vivox")
    void Vivox_SetMicActive(bool bMuted);


    // ================
    // Voice Mode
    // =============

public:

    UFUNCTION(BlueprintCallable, Category = "Vivox|Mode")
    void Vivox_EnterLobbyVoice( const FString& PlayerId, const FString& SessionVoiceId);



    UFUNCTION(BlueprintCallable, Category = "Vivox|Mode")
    void Vivox_EnterInGameVoice(const FString& PlayerId, const FString& SessionVoiceId);


    UFUNCTION(BlueprintPure, Category = "Vivox|Mode")
    EVoiceChannelMode GetCurrentVoiceMode() const
    {
        return CurrentVoiceMode;
    }


    UFUNCTION(BlueprintPure, Category = "Vivox|Mode")
    FString GetCurrentVoiceChannelName() const
    {
        return CurrentVoiceChannelName;
    }


    // ==============
    // 3D Voice
    // =============

public:

    UFUNCTION(BlueprintCallable, Category = "Vivox|3D")
    void Vivox_Update3DFromActors(
        AActor* SpeakerActor,
        AActor* ListenerActor
    );


    UFUNCTION(BlueprintCallable, Category = "Vivox|3D")
    void Vivox_Start3DUpdate(
        AActor* SpeakerActor,
        AActor* ListenerActor,
        float Interval = 0.1f
    );


    UFUNCTION(BlueprintCallable, Category = "Vivox|3D")
    void Vivox_Stop3DUpdate();


    // =============
    // Debug
    // ============

public:

    UFUNCTION(BlueprintCallable, Category = "Vivox|Debug")
    void Vivox_SetDebugHudEnabled(
        bool bEnabled
    );


    // =====================
    // Voice Cache
    // ==================

public:

    void StartVoiceCacheTimer();

    void StopVoiceCacheTimer();

    void TickVoiceCache();

    float ApplyVoiceLevelCorrection(
        float Raw01
    ) const;

    void PushVoiceCacheToGameInstance();


    void SetMicMuted(bool bMuted)
    {
        bMicMuted = bMuted;
    }

    bool IsMicMuted() const
    {
        return bMicMuted;
    }


private:

    // ==========================
    // Vivox Initialization
    // ==========================

    void InitVivox();

    bool bVivoxInitialized = false;

    IClient* VivoxVoiceClient = nullptr;

    AccountId LoggedInUserId;


    // =================
    // Login State
    // ===============

private:

    bool bLoginInProgress = false;

    bool bIsLoggedIn = false;


    FString PendingPlayerId;

    FString PendingChannelName;

    bool bPendingPositional = false;


    void Vivox_BeginLoginInternal(const FString& PlayerIdOverride);


    // ==================
    // ===================

private:

    ChannelId JoinedChannel;

    bool bHasJoinedChannel = false;


    EVoiceChannelMode CurrentVoiceMode = EVoiceChannelMode::None;


    FString CurrentVoiceChannelName;

    bool bCurrentChannelPositional = false;


    FTimerHandle ChannelSwitchTimerHandle;

    void SchedulePendingChannelJoin(float DelaySeconds = 0.25f);


    void JoinPendingChannel();

    void DisconnectCurrentChannel(bool bClearMode);


    // ================
    // 3D Internal
    // ===============

private:

    void Vivox_TryAutoStart3DUpdate();

    void Vivox_Update3D_Internal();


    TWeakObjectPtr<AActor> CachedSpeaker;

    TWeakObjectPtr<AActor> CachedListener;


    FTimerHandle Vivox3DTimer;

    FTimerHandle PawnRetryTimerHandle;


    bool bAutoStart3DUpdate = true;

    float Auto3DUpdateInterval = 0.1f;


    // ===========
    // Debug
    // =========

private:

    void Vivox_ScreenLog(
        const FString& Msg,
        const FColor& Color = FColor::Green,
        float Time = 6.f
    );


    void Vivox_EnableEnergyMeter();

    void Vivox_TickDebugHud();


    bool bDebugHudEnabled = true;

    FTimerHandle DebugHudTimerHandle;


    double LastSelfAudioEnergy = 0.0;

    bool bLastSelfSpeechDetected = false;


    // ================
    // Voice Cache
    // ==============

private:

    float VoiceCacheUpdateInterval = 0.1f;

    FTimerHandle VoiceCacheTimerHandle;


    bool bMicMuted = false;


    float LastCorrectedVoiceLevel = 0.f;


    /**
     * PlayerIndex
     * -> Location + Corrected Voice Level
     */
    TMap<int32, TPair<FVector, float>>
        LastStimuliMap;


    bool TryGetPlayerLocationByIndex(
        int32 SeatIndex,
        FVector& OutLocation
    ) const;
};
