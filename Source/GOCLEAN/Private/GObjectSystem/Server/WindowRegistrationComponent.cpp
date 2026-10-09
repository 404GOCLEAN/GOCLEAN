// Fill out your copyright notice in the Description page of Project Settings.


#include "GObjectSystem/Server/WindowRegistrationComponent.h"
#include "GObjectSystem/Server/GObjectManager.h"

//// Sets default values for this component's properties
//UWindowRegistrationComponent::UWindowRegistrationComponent()
//{
//	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
//	// off to improve performance if you don't need them.
//	PrimaryComponentTick.bCanEverTick = true;
//}


void UWindowRegistrationComponent::BeginPlay()
{
	Super::BeginPlay();

    auto* ObjManager = GetWorld()->GetSubsystem<UGObjectManager>();

    if (ObjManager)
    {
        ObjManager->RegisterWindow(GetOwner());
    }
	
}

