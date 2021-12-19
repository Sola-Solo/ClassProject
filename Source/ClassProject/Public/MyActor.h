// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UENUM()
enum class EType : uint8
{
	EEE,
};


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

	UFUNCTION(BlueprintCallable, Category="BlueprintFunc")
	void MyFunc2(){}

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta = (DisplayName = "ClassBlueprint Event Function"), Category = "BlueprintFunc")
    FString BlueprintNativeEventFunction(AActor* InActor);

	UFUNCTION(BlueprintImplementableEvent)
	void Test();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	/*{
		TArray Arr;  // --> std::vector
		TMap Map; // std::map
		TSet Set;//std::set
	}*/

	/*TSharedPtr<XXX>  shared_ptr;
	TWeakPtr<XXX>  */

	FText MyText = NSLOCTEXT("Game UI", "Health Warning Message", "Low Health!");
	
};
