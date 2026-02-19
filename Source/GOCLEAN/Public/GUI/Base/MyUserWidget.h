// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GUI/Base/GUILayer.h"
#include "MyUserWidget.generated.h"

/**
 * 
 */
UCLASS()
class GOCLEAN_API UGHUDWidget : public UGUILayer
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintImplementableEvent)
	void OnGameLevelReady();

};
