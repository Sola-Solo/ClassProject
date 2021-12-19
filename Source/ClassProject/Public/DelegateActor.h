// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DelegateActor.generated.h"

DECLARE_DELEGATE_OneParam(FTestDelegate, int32);//声明
DECLARE_DYNAMIC_DELEGATE_OneParam(FTestDynamicDelegate, int32, InPar);

UCLASS()
class CLASSPROJECT_API ADelegateActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADelegateActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static void Test(int32 InIntPar);


	UFUNCTION()
	void TestDynamic(int32 InPar);
	
	
	//--------------------------------
	FTestDelegate TestDelegate;//创建一个单播代理
	FTestDynamicDelegate TestDynamicDelegate;

};

