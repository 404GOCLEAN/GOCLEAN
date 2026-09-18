// Copyright Epic Games, Inc. All Rights Reserved.
/**
 * @class AGOCLEANPlayerController
 * @brief Player controller
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

#include "GOCLEANPlayerController.generated.h"

class UInputMappingContext;
class URPCRouterComponent;

UCLASS()
class GOCLEAN_API AGOCLEANPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGOCLEANPlayerController();

	UFUNCTION(BlueprintPure, Category = "RPC")
	URPCRouterComponent* GetRPCRouter() const { return RPCRouter; }

protected:

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RPC")
	URPCRouterComponent* RPCRouter = nullptr;

	// Begin Actor interface
protected:

	virtual void BeginPlay() override;

	// End Actor interface

public:

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> TitleWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ResultWidgetClass;


	// 현재 띄운 위젯 인스턴스
	UPROPERTY()
	TObjectPtr<UUserWidget> CurrentWidget;

	// 레벨별 UI 표시 함수
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowTitleUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLobbyUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowResultUI();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Client_ShowLobbyUI();

	// Server -> owning client
	UFUNCTION(Client, Reliable)
	void Client_ShowResultUI();

	void TryDoInteraction();

	void ChangeSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Vending")
	void RequestTakeVendingItem(int32 ItemId);

public:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> VendingWidgetClass;

protected:
	UPROPERTY()
	TObjectPtr<UUserWidget> VendingWidget;

public:
	UFUNCTION(BlueprintCallable, Category = "UI|Vending")
	void OpenVendingUI();

	UFUNCTION(BlueprintCallable, Category = "UI|Vending")
	void CloseVendingUI();

	UFUNCTION(BlueprintCallable, Category = "UI|Vending")
	void ToggleVendingUI();

	UFUNCTION(BlueprintPure, Category = "UI|Vending")
	bool IsVendingUIOpen() const;

protected:
	void EnsureVendingWidget();

};
