// Fill out your copyright notice in the Description page of Project Settings.


#include "DelegateActor.h"

// Sets default values
ADelegateActor::ADelegateActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADelegateActor::BeginPlay()
{
	Super::BeginPlay();
	TestDelegate.BindStatic(Test);
	
	//TestDelegate.ExecuteIfBound(1);

	if(TestDelegate.IsBound())
	{
		TestDelegate.Execute(1);
	}

	TestDynamicDelegate.BindDynamic(this, &ADelegateActor::TestDynamic);
	
	if(TestDynamicDelegate.IsBound())
	{
		TestDynamicDelegate.Execute(1);
	}
}

// Called every frame
void ADelegateActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADelegateActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	TestDelegate.Unbind();
	TestDynamicDelegate.Unbind();
}

void ADelegateActor::Test(int32 InIntPar)
{
	UE_LOG(LogTemp, Warning, TEXT("Test : %d"), InIntPar);
}

void ADelegateActor::TestDynamic(int32 InPar)
{
	UE_LOG(LogTemp, Warning, TEXT("TestDynamic : %d"), InPar);
}

