#include "GCharacter/GOCLEANPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

#include "Net/RpcTypes.h"
#include "ServerModule/GameSession/RPCRouterComponent.h"

AGOCLEANPlayerController::AGOCLEANPlayerController()
{
	RPCRouter = CreateDefaultSubobject<URPCRouterComponent>(TEXT("RPCRouter"));
}


void AGOCLEANPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);
	}
}

void AGOCLEANPlayerController::ShowTitleUI()
{
    // 기존 위젯 제거
    if (CurrentWidget)
    {
        CurrentWidget->RemoveFromParent();
        CurrentWidget = nullptr;
    }

    if (!TitleWidgetClass) return;

    CurrentWidget = CreateWidget<UUserWidget>(this, TitleWidgetClass);
    if (CurrentWidget)
    {
        CurrentWidget->AddToViewport();

        bShowMouseCursor = true;
        FInputModeUIOnly Mode;
        SetInputMode(Mode);
    }
}

void AGOCLEANPlayerController::ShowLobbyUI()
{
    // 위젯 사용 추가 필요

    if (CurrentWidget)
    {
        CurrentWidget->RemoveFromParent();
        CurrentWidget = nullptr;
    }
}
