#include "GCharacter/GOCLEANPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"

#include "Net/RpcTypes.h"
#include "ServerModule/GameSession/RPCRouterComponent.h"

#include "ServerModule/Sound/AmbientAudioManager.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "ServerModule/GameSession/LobbyGameState.h"
#include "ServerModule/GameSession/PlayerSessionState.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

#include "ServerModule/GameSession/GameSessionInstance.h"

AGOCLEANPlayerController::AGOCLEANPlayerController()
{
    RPCRouter = CreateDefaultSubobject<URPCRouterComponent>(TEXT("RPCRouter"));
    static ConstructorHelpers::FClassFinder<UUserWidget> LobbyUI(
        TEXT("/Game/Hyeona/Widgets/WBP_LobbyMainWidget"));
    if (LobbyUI.Succeeded()) LobbyWidgetClass = LobbyUI.Class;
}


void AGOCLEANPlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (InputMappingContext) Subsystem->AddMappingContext(InputMappingContext, 0);
    }
    if (IsLocalController() && UGameplayStatics::GetCurrentLevelName(this, true) == TEXT("test_Lobby"))
        ShowLobbyUI();
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
    if (!IsLocalController() || !GetWorld()) return;
    if (UGameplayStatics::GetCurrentLevelName(this, true) != TEXT("test_Lobby")) return;

    const APlayerSessionState* PSS = GetPlayerState<APlayerSessionState>();
    // Construct the widget only after its replicated data sources exist locally.
    if (!GetWorld()->GetGameState<ALobbyGameState>() || !PSS || PSS->GetSeatIndex() == INDEX_NONE)
    {
        if (++LobbyUIRetryCount <= 200)
            GetWorldTimerManager().SetTimer(LobbyUIRetryTimer, this,
                &AGOCLEANPlayerController::ShowLobbyUI, 0.1f, false);
        else
            UE_LOG(LogTemp, Warning, TEXT("[LobbyUI] Timed out waiting for local PlayerState/GameState"));
        return;
    }
    if (!LobbyWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[LobbyUI] LobbyWidgetClass is not configured"));
        return;
    }
    GetWorldTimerManager().ClearTimer(LobbyUIRetryTimer);
    LobbyUIRetryCount = 0;
    if (IsValid(CurrentWidget) && CurrentWidget->IsA(LobbyWidgetClass) && CurrentWidget->IsInViewport())
        return;

    // Replace any early Blueprint-created lobby widget so Construct can bind valid state.
    TArray<UUserWidget*> ExistingWidgets;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ExistingWidgets, LobbyWidgetClass, true);
    for (UUserWidget* Widget : ExistingWidgets)
    {
        if (IsValid(Widget) && Widget->GetWorld() == GetWorld() &&
            (!Widget->GetOwningPlayer() || Widget->GetOwningPlayer() == this))
            Widget->RemoveFromParent();
    }
    if (IsValid(CurrentWidget)) CurrentWidget->RemoveFromParent();
    CurrentWidget = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
    if (!CurrentWidget) return;
    CurrentWidget->AddToViewport();
    bShowMouseCursor = true;
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(CurrentWidget->TakeWidget());
    SetInputMode(Mode);
    UE_LOG(LogTemp, Log, TEXT("[LobbyUI] Created for %s Seat=%d Nickname=%s"),
        *GetName(), PSS->GetSeatIndex(), *PSS->GetNickname());
}

void AGOCLEANPlayerController::ShowResultUI()
{
    if (CurrentWidget)
    {
        CurrentWidget->RemoveFromParent();
        CurrentWidget = nullptr;
    }

    if (!ResultWidgetClass) return;

    CurrentWidget = CreateWidget<UUserWidget>(this, ResultWidgetClass);
    if (CurrentWidget)
    {
        CurrentWidget->AddToViewport();

        bShowMouseCursor = true;

        FInputModeUIOnly Mode;
        Mode.SetWidgetToFocus(CurrentWidget->TakeWidget());
        SetInputMode(Mode);
    }
}

void AGOCLEANPlayerController::Client_ShowLobbyUI_Implementation()
{
    ShowLobbyUI();
}

void AGOCLEANPlayerController::Client_ShowResultUI_Implementation()
{
    ShowResultUI();
}


void AGOCLEANPlayerController::TryDoInteraction()
{
    FObjectPayload_C2S Temp;
    RPCRouter->Server_ObjectEvent(EObjectEvent_C2S::Object_TryInteract, Temp);
}

void AGOCLEANPlayerController::ChangeSlot(int32 SlotIndex)
{
    FPlayerPayload_C2S Temp;
    Temp.ParamInt = SlotIndex;

    RPCRouter->Server_PlayerEvent(EPlayerEvent_C2S::RequestChangeCurrentSlotIndex, Temp);
}

void AGOCLEANPlayerController::RequestTakeVendingItem(int32 ItemId)
{
    if (!RPCRouter)
        return;

    FObjectPayload_C2S Payload;
    Payload.ParamInt = ItemId;

    RPCRouter->Server_ObjectEvent(
        EObjectEvent_C2S::Vending_SelectItem,
        Payload
    );
}





void AGOCLEANPlayerController::EnsureVendingWidget()
{
    if (!IsLocalController()) return;
    if (VendingWidget) return;
    if (!VendingWidgetClass) return;

    VendingWidget = CreateWidget<UUserWidget>(this, VendingWidgetClass);
    if (VendingWidget)
    {
        VendingWidget->AddToViewport();
        VendingWidget->SetVisibility(ESlateVisibility::Hidden);
    }
}

bool AGOCLEANPlayerController::IsVendingUIOpen() const
{
    return VendingWidget && VendingWidget->GetVisibility() == ESlateVisibility::Visible;
}

void AGOCLEANPlayerController::OpenVendingUI()
{
    if (!IsLocalController()) return;

    EnsureVendingWidget();
    if (!VendingWidget) return;

    VendingWidget->SetVisibility(ESlateVisibility::Visible);

    bShowMouseCursor = true;
    FInputModeUIOnly Mode;
    SetInputMode(Mode);
}

void AGOCLEANPlayerController::CloseVendingUI()
{
    if (!IsLocalController()) return;
    if (!VendingWidget) return;

    VendingWidget->SetVisibility(ESlateVisibility::Hidden);

    bShowMouseCursor = false;
    FInputModeGameOnly Mode;
    SetInputMode(Mode);
}

void AGOCLEANPlayerController::ToggleVendingUI()
{
    if (IsVendingUIOpen())
    {
        CloseVendingUI();
    }
    else
    {
        OpenVendingUI();
    }
}

void AGOCLEANPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    if (IsLocalController() && UGameplayStatics::GetCurrentLevelName(this, true) == TEXT("test_Lobby"))
        ShowLobbyUI();
}

void AGOCLEANPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld()) GetWorldTimerManager().ClearTimer(LobbyUIRetryTimer);
    Super::EndPlay(EndPlayReason);
}


// Additional Input //

// IA
void AGOCLEANPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EnhancedInputComponent->BindAction(IA_SwitchPlayer,
            ETriggerEvent::Started,
            this,
            &AGOCLEANPlayerController::SwitchSpectatorTarget);
    }
}

void AGOCLEANPlayerController::SwitchSpectatorTarget()
{
    // 관전 중인 플레이어를 변경
}


// IMC
void AGOCLEANPlayerController::SwitchToDefaultIMC()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->ClearAllMappings();

        if (InputMappingContext)
        {
            Subsystem->AddMappingContext(InputMappingContext, 0);
            UE_LOG(LogTemp, Log, TEXT("Change IMC : Spectator"));
        }
    }
}

void AGOCLEANPlayerController::SwitchToCabinetIMC()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->ClearAllMappings();

        if (IMC_Cabinet)
        {
            Subsystem->AddMappingContext(IMC_Cabinet, 0);
            UE_LOG(LogTemp, Log, TEXT("Change IMC : Spectator"));
        }
    }
}

void AGOCLEANPlayerController::SwitchToSpectatorIMC()
{
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        Subsystem->ClearAllMappings();

        if (IMC_Spectator)
        {
            Subsystem->AddMappingContext(IMC_Spectator, 0);
            UE_LOG(LogTemp, Log, TEXT("Change IMC : Spectator"));
        }
    }
}


// Spectator UI
void AGOCLEANPlayerController::ShowSpectatorUI()
{
    // 기존 위젯 제거
    if (CurrentWidget)
    {
        CurrentWidget->RemoveFromParent();
        CurrentWidget = nullptr;
    }

    if (!SpectatorWidgetClass) return;

    CurrentWidget = CreateWidget<UUserWidget>(this, SpectatorWidgetClass);
    if (CurrentWidget)
    {
        CurrentWidget->AddToViewport();

        bShowMouseCursor = false;
        FInputModeGameOnly Mode;
        SetInputMode(Mode);
    }
}

void AGOCLEANPlayerController::ReturnToTitle()
{
    if (!IsLocalController())
        return;

    UGameSessionInstance* GI = GetGameInstance<UGameSessionInstance>();

    if (!GI)
        return;

    GI->ReturnToTitle();
}
