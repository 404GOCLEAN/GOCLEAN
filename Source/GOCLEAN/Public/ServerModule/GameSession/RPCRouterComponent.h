// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/RpcTypes.h"
#include "RPCRouterComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GOCLEAN_API URPCRouterComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URPCRouterComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	// C -> S 오브젝트 이벤트
	UFUNCTION(Server, Reliable)
	void Server_ObjectEvent(EObjectEvent_C2S EventType, const FObjectPayload_C2S& Payload);

	// S -> C 오브젝트 이벤트
	UFUNCTION(Client, Reliable)
	void Client_ObjectEvent(EObjectEvent_S2C EventType, const FObjectPayload_S2C& Payload);

	// C -> S 플레이어 이벤트
	UFUNCTION(Server, Reliable)
	void Server_PlayerEvent(EPlayerEvent_C2S EventType, const FPlayerPayload_C2S& Payload);

	// S -> C 플레이어 이벤트
	UFUNCTION(Client, Reliable)
	void Client_PlayerEvent(EPlayerEvent_S2C EventType, const FPlayerPayload_S2C& Payload);
		
};
