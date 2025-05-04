// Fill out your copyright notice in the Description page of Project Settings.


#include "GhostAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"

AGhostAIController::AGhostAIController()
{
	
}

void AGhostAIController::BeginPlay()
{
	Super::BeginPlay();

	RunGhostBT();
}

void AGhostAIController::RunGhostBT()
{
	// !!!! BeginPlay()���� ȣ���� �� ��������.
	BB = GetBlackboardComponent();
	if(UseBlackboard(BT->BlackboardAsset, BB)) RunBehaviorTree(BT);
}