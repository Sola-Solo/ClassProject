// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class CLASSPROJECT_API AMyActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BlueprintProperty")
	int32 A;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="BlueprintProperty")
	float B;

	UFUNCTION(BlueprintCallable, Category="BlueprintFunc")
	void MyFunc();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta = (DisplayName = "Blueprint Native Event Function"), Category = "BlueprintFunc")
    FString BlueprintNativeEventFunction(AActor* InActor);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
