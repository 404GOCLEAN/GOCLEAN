// Copyright Epic Games, Inc. All Rights Reserved.

#include "GOCLEANGameMode.h"
#include "GCharacter/GOCLEANCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGOCLEANGameMode::AGOCLEANGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/Seohyeon/Character/Blueprints/BP_CharacterManA"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}