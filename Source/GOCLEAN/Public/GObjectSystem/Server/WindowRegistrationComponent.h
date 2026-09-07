// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WindowRegistrationComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GOCLEAN_API UWindowRegistrationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// UWindowRegistrationComponent();

protected:
	virtual void BeginPlay() override;

		
};
