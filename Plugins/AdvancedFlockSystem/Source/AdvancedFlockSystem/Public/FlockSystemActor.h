// Copyright 2020-2021 Fly Dream Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/HAL/Runnable.h"
#include "FlockSystemActor.generated.h"

USTRUCT(BlueprintType)
struct FlockMemberData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    int InstanceIndex = 0;

    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY()
    FVector WanderPosition = FVector::ZeroVector;

    UPROPERTY()
    FTransform Transform;

    UPROPERTY()
    float ElapsedTimeSinceLastWander = 0.f;

    UPROPERTY()
    bool bIsFlockLeader = false;

    UPROPERTY()
    TArray<AActor*> AttackedActors;

    FlockMemberData()
    {
        InstanceIndex = 0;
        Velocity = FVector::ZeroVector;
        ElapsedTimeSinceLastWander = 0.0f;
        WanderPosition = FVector::ZeroVector;
        bIsFlockLeader = false;
    };
};

USTRUCT(BlueprintType)
struct FlockMembersArrays
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY()
    int InstanceIndex = 0;

    UPROPERTY()
    TArray<FlockMemberData> FlockMembersArr;

    FlockMembersArrays()
    {
        InstanceIndex = 0;
        FlockMembersArr.Empty();
    };

    void AddData(int Index, TArray<FlockMemberData> NewFlockData)
    {
        InstanceIndex = Index;
        FlockMembersArr = NewFlockData;
    };
};

UENUM(BlueprintType)
enum class EPriority: uint8
{
    Normal				UMETA(DisplayName = "Normal"),
    AboveNormal			UMETA(DisplayName = "AboveNormal"),
    BelowNormal			UMETA(DisplayName = "BelowNormal"),
    Highest				UMETA(DisplayName = "Highest"),
    Lowest				UMETA(DisplayName = "Lowest"),
    SlightlyBelowNormal UMETA(DisplayName = "SlightlyBelowNormal"),
    TimeCritical		UMETA(DisplayName = "TimeCritical")
};

USTRUCT(BlueprintType)
struct FlockMemberParameters
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    EPriority ThreadPriority = EPriority::Normal;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    bool bUseOneLeader = false;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    AActor* FollowActor = nullptr;
    // Use for only more flock mates!
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    bool ExperimentalOptimization = false;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    bool bUseAquarium = true;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    float FlockWanderInRandomRadius = 3000.0f;
    // If you want add all not mobile actors for avoidance. 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Flock Parameters")
    bool bAutoAddComponentsInArray = true;
    // If need react on pawn.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Flock Parameters")
    bool bReactOnPawn = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Flock Parameters")
    bool bFollowToPawn = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Flock Parameters")
    bool bCanAttackPawn = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Flock Parameters")
    float AttackRadius = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Fly Insects Parameters")
    float DamageValue = 0.001f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Fly Insects Parameters")
    TSubclassOf<class UDamageType> DamageType;
    
    UPROPERTY()
    float AttackRadiusSquared = 0.f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    bool bUseMaxHeight = false;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    float MaxHeight = 0.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    float MoveSpeedInterpInThread = 5.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FlockMaxSpeed = 40.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FlockOffsetSpeed = 5.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float EscapeMaxSpeedMultiply = 2.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FollowScale = 1.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FlockMateAwarenessRadius = 400.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float AlignScale = 0.4f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float CohesionScale = 0.6f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float SeparationScale = 5.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FleeScale = 10.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FleeScaleAvoidance = 10.0f;

    float AvoidancePrimitiveDistance = 50.f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float SeparationRadius = 6.0f;
    // Delay before leader find new wander location.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FlockWanderUpdateRate = 2.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FlockMinWanderDistance = 50.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FlockMateRotationRate = 0.6f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float EscapeMateRotationRate = 10.f;

    float FlockMaxSteeringForce = 100.0f;
    
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FlockEnemyAwarenessRadius = 200.0f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float FollowPawnAwarenessRadius = 500.0f;

    float FleeScaleAquarium = 10.0f;
    float StrengthAquariumOffsetValue = 100.f;


    FlockMemberParameters()
    {
        AttackRadiusSquared = FMath::Square(AttackRadius);
    };
};

UCLASS()
class ADVANCEDFLOCKSYSTEM_API AFlockSystemActor : public AActor
{
    GENERATED_BODY()

public:


    // constructor
    AFlockSystemActor();

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    //************************************************************************
    // Component                                                                  
    //************************************************************************

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component")
    class USphereComponent* SphereComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Component")
    class UBoxComponent* BoxComponent;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Component")
    class UInstancedStaticMeshComponent* StaticMeshInstanceComponent;

    //************************************************************************

    void GenerateFlockThread();

    void AddAvoidanceComponentsTimer();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    UStaticMesh* StaticMesh;
    // Recommended - 1 Thread = (2000 - 2500) mates. 
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn", meta=(ClampMin="1", ClampMax="32"))
    int MaxUseThreads = 1;
    // Recommended - 1 Thread = (2000 - 2500) mates.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    int32 FlockMateInstances = 1000;
    // Random mesh scale.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    float MinMeshScale = 1.f;
    // Random mesh scale.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Spawn")
    float MaxMeshScale = 2.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float InterpMoveAnimRate = 200.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    float InterpRotateAnimRate = 25.f;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Advanced Flock Parameters")
    FlockMemberParameters FlockParameters;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced Flock Parameters")
    TArray<AActor*> AvoidanceActorRootArr;
    UPROPERTY(BlueprintReadOnly, Category = "Advanced Flock Parameters")
    TArray<FlockMemberData> FlockMemberDataArr;
    // Add an instance to this component. Transform is given in world space. 
    void AddFlockMemberWorldSpace(const FTransform& WorldTransform);
	// MD
    int32 NumFlock;

    UPROPERTY()
    TArray<UPrimitiveComponent*> AllOverlappingComponentsArr;

    TArray<FlockMembersArrays> AllFlockMembersArrays;

    void DivideFlockArrayForThreads();

    UPROPERTY()
    TArray<AActor*> DangerActors;

protected:

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void BeginDestroy() override;

    FTimerHandle AddAvoidanceActor_Timer;

private:

    TArray<class FlockThread*> FlockActorPoolThreadArr;

};

// Thread
class FlockThread : public FRunnable
{
public:

    //================================= THREAD =====================================
    
    //Constructor
    FlockThread(AActor *NewActor);
    //Destructor
    ~FlockThread();

    //Use this method to kill the thread!!
    void EnsureCompletion();
    //Pause the thread 
    void PauseThread();
    //Continue/UnPause the thread
    void ContinueThread();
    //FRunnable interface.
    bool Init();
    uint32 Run();
    void Stop();
    bool IsThreadPaused() const;

    //================================= FLOCK =====================================
    class UBoxComponent* BoxComponentRef;

    TArray<FlockMemberData> GetFlockMembersData();

    void InitFlockParameters(TArray<FlockMemberData> SetFlockMembersArr, FlockMemberParameters NewParameters, UBoxComponent* SetBoxComponent);

    void SetOverlappingComponents(TArray<UPrimitiveComponent*> OverlappingComponentsArr, TArray<AActor*> DangerActors);
    void SetAvoidanceActor(TArray<AActor*> AvoidanceActorRootArr);

    FVector SteeringAquarium(FlockMemberData& FlockMember) const;
    FVector SteeringAvoidanceComponent(FlockMemberData& FlockMember) const;
    FVector SteeringWander(FlockMemberData& FlockMember) const;
    FVector GetRandomWanderLocation() const;
    FVector SteeringFollow(FlockMemberData& FlockMember, int32 FlockLeader);
    TArray<int32> GetNearbyFlockMates(int32 FlockMember);
    FVector SteeringAlign(FlockMemberData& FlockMember, TArray<int32>& FlockMates);
    FVector SteeringSeparate(FlockMemberData& FlockMember, TArray<int32>& FlockMates);
    FVector SteeringCohesion(FlockMemberData& FlockMember, TArray<int32>& FlockMates);
    FVector SteeringFlee(FlockMemberData& FlockMember);
    FVector SteeringAvoidance(FlockMemberData& FlockMember) const;
    FVector SteeringMaxHeight(FlockMemberData& FlockMember) const;
    FVector SteeringFollowPawn(FlockMemberData& FlockMember);

    void InitFlockLeader();

    TArray<FlockMemberData> FlockThreadMembersArr;
    FlockMemberParameters FlockParametersTHR;
    TArray<UPrimitiveComponent*> AllOverlappingComponentsArrTHR;
    TArray<AActor*> DangerActorsTHR;
    TArray<AActor*> AvoidanceActorRootArrTHR;

    float ThreadDeltaTime = 0.f;
    //================================= FLOCK =====================================

    void SetPoolThread(TArray<class FlockThread*> SetPoolThreadArr);
    

private:

    //Thread to run the worker FRunnable on
    FRunnableThread* Thread;

    FCriticalSection Mutex;
    FEvent* Semaphore;

    int ChunkCount;
    int Amount;
    int MinInt;
    int MaxInt;

    float ThreadSleepTime = 0.01f;

    //As the name states those members are Thread safe
    FThreadSafeBool Kill;
    FThreadSafeBool Pause;

    TArray<class FlockThread*> PoolThreadArr;
};
