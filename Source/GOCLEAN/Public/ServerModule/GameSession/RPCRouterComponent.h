// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/RpcTypes.h"
#include "RPCRouterComponent.generated.h"

class AInGameGameState;

/**
 * Client <-> Server RPC Router
 *
 * 실제 게임 처리 로직을 직접 수행하지 않고,
 * 각 Manager / GameMode 쪽으로 요청을 전달
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GOCLEAN_API URPCRouterComponent : public UActorComponent
{
	GENERATED_BODY()

public:

    URPCRouterComponent();


protected:

    virtual void BeginPlay() override;


public:

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


    // ================
    // Object Event
    // ================

    // Client -> Server
    UFUNCTION(Server, Reliable)
    void Server_ObjectEvent(EObjectEvent_C2S EventType, const FObjectPayload_C2S& Payload);


    // Server -> Client
    UFUNCTION(Client, Reliable)
    void Client_ObjectEvent(EObjectEvent_S2C EventType, const FObjectPayload_S2C& Payload);


    // =================
    // Player Event
    // =================

    // Client -> Server
    UFUNCTION(Server, Reliable)
    void Server_PlayerEvent(EPlayerEvent_C2S EventType, const FPlayerPayload_C2S& Payload);


    // Server -> Client
    UFUNCTION(Client, Reliable)
    void Client_PlayerEvent(EPlayerEvent_S2C EventType, const FPlayerPayload_S2C& Payload);


    // ====================
    // Cleaning Event
    // ====================

    // Client -> Server
    UFUNCTION(Server, Reliable)
    void Server_CleaningEvent(ECleaningEvent_C2S EventType, const FCleaningPayload_C2S& Payload);


private:

    /**
     * 현재 InGameGameState를 가져옴
     *
     * Lobby에서는 nullptr이 반환되는 것이 정상
     * 해당 RPC들은 인게임 기능이므로 InGame State에서만 동작
     */
    AInGameGameState* GetInGameState() const;
		
};
