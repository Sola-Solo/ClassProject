// Copyright 2020-2021 Fly Dream Dev. All Rights Reserved.

#include "FlockSystemActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "TimerManager.h"

AFlockSystemActor::AFlockSystemActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereComponent->SetGenerateOverlapEvents(false);

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Overlap);
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);

	BoxComponent->SetBoxExtent(FVector(1000.f, 1000.f, 1000.f), true);

	StaticMeshInstanceComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StaticMeshInstanceComponent"));
	StaticMeshInstanceComponent->SetupAttachment(RootComponent);
	StaticMeshInstanceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshInstanceComponent->SetGenerateOverlapEvents(false);
}

FlockThread::FlockThread(AActor* NewActor)
{
	Kill = false;
	Pause = false;

	//Initialize FEvent (as a cross platform (Confirmed Mac/Windows))
	Semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	EThreadPriority ThreadPriority;

	switch (FlockParametersTHR.ThreadPriority)
	{
	case EPriority::Normal:
		ThreadPriority = TPri_Normal;
		break;
	case EPriority::Highest:
		ThreadPriority = TPri_Highest;
		break;
	case EPriority::Lowest:
		ThreadPriority = TPri_Lowest;
		break;
	case EPriority::AboveNormal:
		ThreadPriority = TPri_AboveNormal;
		break;
	case EPriority::BelowNormal:
		ThreadPriority = TPri_BelowNormal;
		break;
	case EPriority::SlightlyBelowNormal:
		ThreadPriority = TPri_SlightlyBelowNormal;
		break;
	case EPriority::TimeCritical:
		ThreadPriority = TPri_TimeCritical;
		break;

	default:
		ThreadPriority = TPri_AboveNormal;
		break;
	}

	Thread = FRunnableThread::Create(this, (TEXT("%s_FSomeRunnable"), *NewActor->GetName()), 0, ThreadPriority);
}

FlockThread::~FlockThread()
{
	if (Semaphore)
	{
		//Cleanup the FEvent
		FGenericPlatformProcess::ReturnSynchEventToPool(Semaphore);
		Semaphore = nullptr;
	}

	if (Thread)
	{
		//Cleanup the worker thread
		delete Thread;
		Thread = nullptr;
	}
}

void FlockThread::EnsureCompletion()
{
	Stop();

	if (Thread)
	{
		Thread->WaitForCompletion();
	}
}

void FlockThread::PauseThread()
{
	Pause = true;
}

void FlockThread::ContinueThread()
{
	Pause = false;

	if (Semaphore)
	{
		//Here is a FEvent signal "Trigger()" -> it will wake up the thread.
		Semaphore->Trigger();
	}
}

bool FlockThread::Init()
{
	return true;
}

uint32 FlockThread::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(FMath::RandRange(0.05f, 0.5f));

	ThreadDeltaTime = 0.f;

	while (!Kill)
	{
		if (Pause)
		{
			//FEvent->Wait(); will "sleep" the thread until it will get a signal "Trigger()"
			Semaphore->Wait();

			if (Kill)
			{
				return 0;
			}
		}
		else
		{
			uint64 const TimePlatform = FPlatformTime::Cycles64();

			TArray<FlockMemberData> FlockMembersArr = FlockThreadMembersArr;

			for (int32 FlockMemberID = 0; FlockMemberID < FlockMembersArr.Num(); ++FlockMemberID)
			{				
				bool bIsAvoidance(false);
				FlockMemberData& FlockMember = FlockMembersArr[FlockMemberID];

				// Clear attacked actors 
				FlockMember.AttackedActors.Empty();

				FVector FollowVec = FVector::ZeroVector;
				FVector CohesionVec = FVector::ZeroVector;
				FVector AlignmentVec = FVector::ZeroVector;
				FVector SeparationVec = FVector::ZeroVector;
				FVector FleeVec = FVector::ZeroVector;
				FVector NewVelocity = FVector::ZeroVector;

				FVector const FlockMemberLocation = FlockMember.Transform.GetLocation();


				// Follow to Leader
				if (FlockMember.bIsFlockLeader)
				{
					NewVelocity += SteeringWander(FlockMember);

					FlockMember.ElapsedTimeSinceLastWander += ThreadDeltaTime;
				}
				else
				{
					if (FlockParametersTHR.FollowScale > 0.0f)
					{
						// Leader following (seek)
						FollowVec = SteeringFollow(FlockMember, 0) * FlockParametersTHR.FollowScale;
					}

					// Other forces need nearby flock mates
					TArray<int32> Mates = GetNearbyFlockMates(FlockMemberID);

					if (FlockParametersTHR.CohesionScale > 0.0f)
					{
						// Cohesion - staying near nearby flock mates
						CohesionVec = SteeringCohesion(FlockMember, Mates) * FlockParametersTHR.CohesionScale;
					}

					if (FlockParametersTHR.AlignScale > 0.0f)
					{
						// Alignment =  aligning with the heading of nearby flock mates
						AlignmentVec = SteeringAlign(FlockMember, Mates) * FlockParametersTHR.AlignScale;
					}

					if (FlockParametersTHR.SeparationScale > 0.0f)
					{
						// Separation = trying to not get too close to flock mates
						SeparationVec = SteeringSeparate(FlockMember, Mates) * FlockParametersTHR.SeparationScale;
					}
				}
				// Flee = running away from enemies!
				if (!FlockParametersTHR.bFollowToPawn && FlockParametersTHR.FleeScale > 0.0f)
				{
					FleeVec = SteeringFlee(FlockMember) * FlockParametersTHR.FleeScale;
					if (FleeVec != FVector::ZeroVector)
					{
						bIsAvoidance = true;
					}
				}
				// Flee = running away from primitive object collision from root component!
				if (FlockParametersTHR.FleeScaleAvoidance > 0.f && AllOverlappingComponentsArrTHR.Num() > 0 && FlockParametersTHR.bAutoAddComponentsInArray)
				{
					FVector AvoidVec = SteeringAvoidanceComponent(FlockMember) * FlockParametersTHR.FleeScaleAvoidance;

					if (AvoidVec != FVector::ZeroVector)
					{
						bIsAvoidance = true;
						FleeVec = AvoidVec;
					}
				}

				// Flee = running away from primitive object collision from root component!
				if (FlockParametersTHR.FleeScaleAvoidance > 0.f && AvoidanceActorRootArrTHR.Num() > 0)
				{
					FVector AvoidVec = SteeringAvoidance(FlockMember) * FlockParametersTHR.FleeScaleAvoidance;

					if (AvoidVec != FVector::ZeroVector)
					{
						bIsAvoidance = true;
						FleeVec = AvoidVec;
					}
				}
				// Avoidance Aquarium. 
				if (FlockParametersTHR.FleeScaleAquarium > 0.f && FlockParametersTHR.bUseAquarium)
				{
					if (!UKismetMathLibrary::IsPointInBox(FlockMemberLocation, BoxComponentRef->GetComponentLocation(), BoxComponentRef->GetScaledBoxExtent()))
					{
						// Flee = running away from Aquarium wall!
						FleeVec = SteeringAquarium(FlockMember) * FlockParametersTHR.FleeScaleAquarium;
						if (FleeVec != FVector::ZeroVector)
						{
							bIsAvoidance = true;
						}
					}
				}
				// Flee = running away from max height!
				if (FlockParametersTHR.bUseMaxHeight)
				{
					if (FlockMemberLocation.Z >= FlockParametersTHR.MaxHeight)
					{
						// Flee = running away from Aquarium wall!
						FleeVec = SteeringMaxHeight(FlockMember) * FlockParametersTHR.FleeScaleAquarium;
						if (FleeVec != FVector::ZeroVector)
						{
							bIsAvoidance = true;
						}
					}
				}
				// Follow to leader.
				NewVelocity += FleeVec;
				if (FleeVec.SizeSquared() <= 0.1f)
				{
					NewVelocity += FollowVec;
					NewVelocity += CohesionVec;
					NewVelocity += AlignmentVec;
					NewVelocity += SeparationVec;
				}
				// Truncate the new force calculated in newVelocity so we don't go crazy
				NewVelocity = NewVelocity.GetClampedToSize(0.0f, FlockParametersTHR.FlockMaxSteeringForce);

				FVector TargetVelocity = FlockMember.Velocity + NewVelocity;

				float FlockRotRate(FlockParametersTHR.FlockMateRotationRate);
				if (bIsAvoidance)
				{
					FlockRotRate = FlockParametersTHR.EscapeMateRotationRate;
				}
				// Rotate the flock member towards the Velocity direction vector
				// get the rotation value for our desired target Velocity (i.e. if we were in that direction)
				// Interpolate our current rotation towards the desired Velocity vector based on rotation speed * time
				FRotator Rot = FRotationMatrix::MakeFromX((FlockMemberLocation + TargetVelocity) - FlockMemberLocation).Rotator();
				FRotator Final = FMath::RInterpTo(FlockMember.Transform.Rotator(), Rot, ThreadDeltaTime, FlockRotRate);

				FlockMember.Transform.SetRotation(Final.Quaternion());

				FVector Forward = FlockMember.Transform.GetUnitAxis(EAxis::X);
				Forward.Normalize();
				FlockMember.Velocity = Forward * TargetVelocity.Size();

				// Clamp our new Velocity to be within min->max speeds
				if (FlockMember.Velocity.Size() > FlockParametersTHR.FlockMaxSpeed)
				{
					FlockMember.Velocity = FlockMember.Velocity.GetSafeNormal() * FMath::RandRange(FlockParametersTHR.FlockMaxSpeed - FlockParametersTHR.FlockOffsetSpeed,
					                                                                                 FlockParametersTHR.FlockMaxSpeed + FlockParametersTHR.FlockOffsetSpeed);
				}
				// If need escape from danger actor.
				if (bIsAvoidance)
				{
					FlockMember.Velocity = FlockMember.Velocity * FlockParametersTHR.EscapeMaxSpeedMultiply;
				}
				FVector SetSpeed = FlockMemberLocation + FlockMember.Velocity;

				FlockMember.Transform.SetLocation(FMath::VInterpTo(FlockMemberLocation, SetSpeed, ThreadDeltaTime, FlockParametersTHR.MoveSpeedInterpInThread));

				// Save all parameters.
				FlockThreadMembersArr[FlockMemberID] = FlockMember;
			}

			ThreadDeltaTime = FTimespan(FPlatformTime::Cycles64() - TimePlatform).GetTotalSeconds();

			if (FlockParametersTHR.ExperimentalOptimization)
			{
				ThreadDeltaTime += ThreadSleepTime;
			}

			//Critical section:
			Mutex.Lock();
			//We are locking our FCriticalSection so no other thread will access it
			//And thus it is a thread-safe access now

			FlockThreadMembersArr = FlockMembersArr;

			//Unlock FCriticalSection so other threads may use it.
			Mutex.Unlock();

			//Pause Condition - if we RandomVectors contains more vectors than Amount we shall pause the thread to release system resources.
			Pause = true;

			if (FlockParametersTHR.ExperimentalOptimization)
			{
				//A little sleep between the chunks (So CPU will rest a bit -- (may be omitted))
				FPlatformProcess::Sleep(ThreadSleepTime);
			}
		}
	}
	return 0;
}

void FlockThread::Stop()
{
	Kill = true; //Thread kill condition "while (!Kill){...}"
	Pause = false;

	if (Semaphore)
	{
		//We shall signal "Trigger" the FEvent (in case the Thread is sleeping it shall wake up!!)
		Semaphore->Trigger();
	}
}

bool FlockThread::IsThreadPaused() const
{
	return (bool)Pause;
}

TArray<FlockMemberData> FlockThread::GetFlockMembersData()
{
	Mutex.Lock();

	TArray<FlockMemberData> actualArray_ = FlockThreadMembersArr;

	this->ContinueThread();

	Mutex.Unlock();

	return actualArray_;
}

void FlockThread::InitFlockParameters(TArray<FlockMemberData> SetFlockMembersArr, FlockMemberParameters NewParameters, UBoxComponent* SetBoxComponent)
{
	FlockParametersTHR = NewParameters;
	BoxComponentRef = SetBoxComponent;
	FlockThreadMembersArr = SetFlockMembersArr;

	FlockThreadMembersArr[0].bIsFlockLeader = true;
}

void FlockThread::SetOverlappingComponents(TArray<UPrimitiveComponent*> OverlappingComponentsArr, TArray<AActor*> DangerActors)
{
	AllOverlappingComponentsArrTHR = OverlappingComponentsArr;
	DangerActorsTHR = DangerActors;
}

void FlockThread::SetAvoidanceActor(TArray<AActor*> AvoidanceActorRootArr)
{
	AvoidanceActorRootArrTHR = AvoidanceActorRootArr;
}

void AFlockSystemActor::BeginPlay()
{
	Super::BeginPlay();

	StaticMeshInstanceComponent->SetWorldScale3D(FVector(1.f, 1.f, 1.f));

	StaticMeshInstanceComponent->SetStaticMesh(StaticMesh);

	FVector const StartLoc(GetActorLocation());

	for (int i = 0; i < FlockMateInstances; ++i)
	{
		FVector const Direction(FMath::VRand());
		FRotator const Rotation(UKismetMathLibrary::RandomRotator());
		float const Distance(FMath::RandRange(0.01f, SphereComponent->GetScaledSphereRadius()));
		FVector const NewLoc(StartLoc + Direction * Distance);
		float const RandScale(FMath::RandRange(MinMeshScale, MaxMeshScale));

		AddFlockMemberWorldSpace(FTransform(Rotation, NewLoc, FVector(RandScale, RandScale, RandScale)));
	}

	if (FlockParameters.bAutoAddComponentsInArray || FlockParameters.bReactOnPawn)
	{
		GetWorldTimerManager().SetTimer(AddAvoidanceActor_Timer, this, &AFlockSystemActor::AddAvoidanceComponentsTimer, 1.f, true, 0.5f);
	}

	DivideFlockArrayForThreads();

	for (int i = 0; i < FlockActorPoolThreadArr.Num(); i++)
	{
		FlockActorPoolThreadArr[i] = nullptr;
	}

	GenerateFlockThread();
}

void AFlockSystemActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (int i = 0; i < FlockActorPoolThreadArr.Num(); i++)
	{
		if (FlockActorPoolThreadArr[i])
		{
			FlockActorPoolThreadArr[i]->EnsureCompletion();
			delete FlockActorPoolThreadArr[i];
			FlockActorPoolThreadArr[i] = nullptr;
		}
	}
	FlockActorPoolThreadArr.Empty();

	Super::EndPlay(EndPlayReason);
}

void AFlockSystemActor::BeginDestroy()
{
	for (int i = 0; i < FlockActorPoolThreadArr.Num(); i++)
	{
		if (FlockActorPoolThreadArr[i])
		{
			FlockActorPoolThreadArr[i]->EnsureCompletion();
			delete FlockActorPoolThreadArr[i];
			FlockActorPoolThreadArr[i] = nullptr;
		}
	}
	FlockActorPoolThreadArr.Empty();

	Super::BeginDestroy();
}

void AFlockSystemActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!StaticMeshInstanceComponent) return;

	TArray<FlockMemberData> FlockMembersDataArr;

	for (int i = 0; i < FlockActorPoolThreadArr.Num(); i++)
	{
		if (FlockActorPoolThreadArr[i])
		{
			TArray<FlockMemberData> TempArr = FlockActorPoolThreadArr[i]->GetFlockMembersData();
			for (int ID = 0; ID < TempArr.Num(); ID++)
			{
				FlockMembersDataArr.Add(TempArr[ID]);
			}
		}
	}
	// Move flock members. 
	for (int32 FlockMemberID = 0; FlockMemberID < FlockMembersDataArr.Num(); ++FlockMemberID)
	{
		if (FlockMembersDataArr[FlockMemberID].InstanceIndex > StaticMeshInstanceComponent->GetInstanceCount()) continue; // don't do anything if we haven't got an instance in range...

		FTransform InterpFlockTransform;

		// Interpolate Vector and Rotate
		FVector InterpVector = FMath::VInterpConstantTo(FlockMemberDataArr[FlockMemberID].Transform.GetLocation(),
		                                                FlockMembersDataArr[FlockMemberID].Transform.GetLocation(), DeltaTime, InterpMoveAnimRate);

		// FRotator InterpRotator = FMath::RInterpTo(FlockMemberDataArr[flockMemberID_].Transform.Rotator(),
		//                                           flockMembersDataArr_[flockMemberID_].Transform.Rotator(), DeltaTime, interpRotateAnimRate);

		InterpFlockTransform.SetLocation(InterpVector);
		InterpFlockTransform.SetRotation(FQuat(FlockMembersDataArr[FlockMemberID].Transform.Rotator()));
		FlockMemberDataArr[FlockMemberID].Transform.SetLocation(InterpVector);
		FlockMemberDataArr[FlockMemberID].Transform.SetRotation(FQuat(FlockMembersDataArr[FlockMemberID].Transform.Rotator()));

		StaticMeshInstanceComponent->UpdateInstanceTransform(FlockMemberDataArr[FlockMemberID].InstanceIndex, FlockMemberDataArr[FlockMemberID].Transform, true, false);

		// Attack Pawn.
		if (FlockParameters.bCanAttackPawn)
		{
			for (int AttackedID = 0; AttackedID < FlockMembersDataArr[FlockMemberID].AttackedActors.Num(); ++AttackedID)
			{
				if (FlockMembersDataArr[FlockMemberID].AttackedActors[AttackedID])
				{
					UGameplayStatics::ApplyDamage(FlockMembersDataArr[FlockMemberID].AttackedActors[AttackedID], FlockParameters.DamageValue,
												nullptr, this, FlockParameters.DamageType);	
				}
			}
		}
	}

	StaticMeshInstanceComponent->MarkRenderStateDirty();
}

void AFlockSystemActor::GenerateFlockThread()
{
	if (MaxUseThreads > FlockMateInstances)
	{
		FlockActorPoolThreadArr.Add(new FlockThread(this));
		FlockActorPoolThreadArr[0]->InitFlockParameters(AllFlockMembersArrays[0].FlockMembersArr, FlockParameters, BoxComponent);
		FlockActorPoolThreadArr[0]->SetPoolThread(FlockActorPoolThreadArr);
		FlockActorPoolThreadArr[0]->InitFlockLeader();
		if (AvoidanceActorRootArr.Num() > 0)
		{
			FlockActorPoolThreadArr[0]->SetAvoidanceActor(AvoidanceActorRootArr);
		}
		return;
	}
	for (int i = 0; i < MaxUseThreads; i++)
	{
		FlockActorPoolThreadArr.Add(new FlockThread(this));
		FlockActorPoolThreadArr[i]->InitFlockParameters(AllFlockMembersArrays[i].FlockMembersArr, FlockParameters, BoxComponent);

		if (AvoidanceActorRootArr.Num() > 0)
		{
			FlockActorPoolThreadArr[i]->SetAvoidanceActor(AvoidanceActorRootArr);
		}
	}

	// Set pool thread for control flock from first thread.
	for (int i = 0; i < FlockActorPoolThreadArr.Num(); i++)
	{
		FlockActorPoolThreadArr[i]->SetPoolThread(FlockActorPoolThreadArr);
	}
	if (FlockParameters.bUseOneLeader)
	{
		for (int i = 0; i < FlockActorPoolThreadArr.Num(); i++)
		{
			FlockActorPoolThreadArr[i]->InitFlockLeader();
		}
	}
}

void AFlockSystemActor::AddAvoidanceComponentsTimer()
{
	if (FlockParameters.bAutoAddComponentsInArray)
	{
		AllOverlappingComponentsArr.Empty();
		DangerActors.Empty();

		TArray<UPrimitiveComponent*> OverlappingComponentsArr;


		BoxComponent->GetOverlappingComponents(OverlappingComponentsArr);

		for (int i = 0; i < OverlappingComponentsArr.Num(); i++)
		{
			APawn* CheckPawn = Cast<APawn>(OverlappingComponentsArr[i]->GetOwner());
			if (CheckPawn)
			{
				if (FlockParameters.bReactOnPawn)
				{
					DangerActors.AddUnique(CheckPawn);
				}
			}
			else
			{
				AllOverlappingComponentsArr.Add(OverlappingComponentsArr[i]);
			}
		}
	}

	for (int i = 0; i < FlockActorPoolThreadArr.Num(); i++)
	{
		if (FlockActorPoolThreadArr[i])
		{
			FlockActorPoolThreadArr[i]->SetOverlappingComponents(AllOverlappingComponentsArr, DangerActors);
		}
	}
}

FVector FlockThread::SteeringAquarium(FlockMemberData& FlockMember) const
{
	FRotator const RotationToCenter(UKismetMathLibrary::FindLookAtRotation(FlockMember.Transform.GetLocation(), BoxComponentRef->GetComponentLocation()));
	FVector Direction = UKismetMathLibrary::Conv_RotatorToVector(RotationToCenter);
	Direction.Normalize();
	FVector NewVec = Direction * ((FlockParametersTHR.FlockEnemyAwarenessRadius / FlockParametersTHR.StrengthAquariumOffsetValue) * FlockParametersTHR.FleeScaleAquarium);
	return NewVec;
}

FVector FlockThread::SteeringAvoidanceComponent(FlockMemberData& FlockMember) const
{
	FVector NewVec(FVector::ZeroVector);

	for (int i = 0; i < AllOverlappingComponentsArrTHR.Num(); ++i)
	{
		if (AllOverlappingComponentsArrTHR[i])
		{
			UPrimitiveComponent* PrimComp(Cast<UPrimitiveComponent>(AllOverlappingComponentsArrTHR[i]));
			if (PrimComp)
			{
				FVector ClosestPoint;
				PrimComp->GetClosestPointOnCollision(FlockMember.Transform.GetLocation(), ClosestPoint);

				if ((ClosestPoint - FlockMember.Transform.GetLocation()).Size() < FlockParametersTHR.AvoidancePrimitiveDistance)
				{
					FRotator const RotationToCenter(UKismetMathLibrary::FindLookAtRotation(ClosestPoint, FlockMember.Transform.GetLocation()));
					FVector Direction = UKismetMathLibrary::Conv_RotatorToVector(RotationToCenter);
					Direction.Normalize();

					NewVec = Direction * ((FlockParametersTHR.FlockEnemyAwarenessRadius / FlockParametersTHR.StrengthAquariumOffsetValue) * FlockParametersTHR.FleeScaleAquarium);
				}
			}
		}
	}

	return NewVec;
}

FVector FlockThread::SteeringWander(FlockMemberData& FlockMember) const
{
	FVector NewVec = FlockMember.WanderPosition - FlockMember.Transform.GetLocation();

	if (FlockMember.ElapsedTimeSinceLastWander >= FlockParametersTHR.FlockWanderUpdateRate || NewVec.Size() <= FlockParametersTHR.FlockMinWanderDistance)
	{
		FlockMember.WanderPosition = GetRandomWanderLocation(); // + GetActorLocation();
		FlockMember.ElapsedTimeSinceLastWander = 0.0f;
		NewVec = FlockMember.WanderPosition - FlockMember.Transform.GetLocation();
	}
	return NewVec;
}

FVector FlockThread::SteeringAlign(FlockMemberData& FlockMember, TArray<int32>& FlockMates)
{
	FVector Vel = FVector(0, 0, 0);
	if (FlockMates.Num() == 0) return Vel;

	for (int32 i = 0; i < FlockMates.Num(); i++)
	{
		Vel += FlockThreadMembersArr[FlockMates[i]].Velocity;
	}
	Vel /= (float)FlockMates.Num();

	return Vel;
}

FVector FlockThread::SteeringSeparate(FlockMemberData& FlockMember, TArray<int32>& FlockMates)
{
	FVector Force = FVector(0, 0, 0);
	if (FlockMates.Num() == 0) return Force;

	for (int32 i = 0; i < FlockMates.Num(); i++)
	{
		FVector Diff = FlockMember.Transform.GetLocation() - FlockThreadMembersArr[FlockMates[i]].Transform.GetLocation();
		float const Scale = Diff.Size();
		Diff.Normalize();
		Diff = Diff * (FlockParametersTHR.SeparationRadius / Scale);
		Force += Diff;
	}
	return Force;
}

FVector FlockThread::SteeringCohesion(FlockMemberData& FlockMember, TArray<int32>& FlockMates)
{
	FVector AvgPos = FVector(0, 0, 0);
	if (FlockMates.Num() == 0) return AvgPos;

	for (int32 i = 0; i < FlockMates.Num(); i++)
	{
		AvgPos += FlockThreadMembersArr[FlockMates[i]].Transform.GetLocation();
	}

	AvgPos /= (float)FlockMates.Num();

	return AvgPos - FlockMember.Transform.GetLocation();
}

FVector FlockThread::SteeringFlee(FlockMemberData& FlockMember)
{
	FVector NewVec = FVector(0, 0, 0);

	for (int i = 0; i < DangerActorsTHR.Num(); ++i)
	{
		if (DangerActorsTHR[i])
		{
			// calculate flee from this threat
			FVector FromEnemy = FlockMember.Transform.GetLocation() - DangerActorsTHR[i]->GetActorLocation();
			float const DistanceToEnemy = FromEnemy.Size();
			FromEnemy.Normalize();

			// enemy inside our enemy awareness threshold, so evade them
			if (DistanceToEnemy < FlockParametersTHR.FlockEnemyAwarenessRadius)
			{
				NewVec += FromEnemy * ((FlockParametersTHR.FlockEnemyAwarenessRadius / DistanceToEnemy) * FlockParametersTHR.FleeScale);
			}
		}
	}
	return NewVec;
}

FVector FlockThread::GetRandomWanderLocation() const
{
	FVector ReturnVector;

	if (FlockParametersTHR.bUseAquarium)
	{
		ReturnVector = UKismetMathLibrary::RandomPointInBoundingBox(BoxComponentRef->GetComponentLocation(), BoxComponentRef->GetScaledBoxExtent());
	}
	else
	{
		ReturnVector = (BoxComponentRef->GetComponentLocation() + FVector(FMath::RandRange(-FlockParametersTHR.FlockWanderInRandomRadius, FlockParametersTHR.FlockWanderInRandomRadius),
		                                                                   FMath::RandRange(-FlockParametersTHR.FlockWanderInRandomRadius, FlockParametersTHR.FlockWanderInRandomRadius),
		                                                                   FMath::RandRange(-FlockParametersTHR.FlockWanderInRandomRadius, FlockParametersTHR.FlockWanderInRandomRadius)));
	}

	if (FlockParametersTHR.bUseMaxHeight)
	{
		if (ReturnVector.Z >= FlockParametersTHR.MaxHeight)
		{
			ReturnVector.Z = FlockParametersTHR.MaxHeight - 50.f;
		}
	}

	return ReturnVector;
}

FVector FlockThread::SteeringFollow(FlockMemberData& FlockMember, int32 FlockLeader)
{
	bool bIsFollowToEnemy(false);
	FVector NewVec = FVector::ZeroVector;

	FlockMemberData NewFlockLeader;
	if (FlockParametersTHR.bUseOneLeader)
	{
		NewFlockLeader = PoolThreadArr[0]->FlockThreadMembersArr[FlockLeader];
	}
	else
	{
		NewFlockLeader = FlockThreadMembersArr[FlockLeader];
	}

	// Follow to pawn
	if (FlockParametersTHR.bFollowToPawn)
	{
		for (int i = 0; i < DangerActorsTHR.Num(); ++i)
		{
			if (DangerActorsTHR[i])
			{
				// calculate flee from this threat
				FVector FromEnemy = DangerActorsTHR[i]->GetActorLocation() - FlockMember.Transform.GetLocation();
				float const DistanceToEnemy = FromEnemy.Size();
				FromEnemy.Normalize();
	
				// enemy inside our enemy awareness threshold, so evade them
				if (DistanceToEnemy < FlockParametersTHR.FollowPawnAwarenessRadius)
				{
					NewVec = DangerActorsTHR[i]->GetActorLocation() - FlockMember.Transform.GetLocation();
					NewVec.Normalize();
					NewVec *= FlockParametersTHR.FlockMaxSpeed;
					NewVec -= FlockMember.Velocity;
	
					// Add attacked actors in array.
					if (FlockParametersTHR.bCanAttackPawn)
					{
						if ((DangerActorsTHR[i]->GetActorLocation() - FlockMember.Transform.GetLocation()).SizeSquared() < FlockParametersTHR.AttackRadiusSquared)
						{
							FlockMember.AttackedActors.AddUnique(DangerActorsTHR[i]);
						}
						
						// UPrimitiveComponent* primComp_(Cast<UPrimitiveComponent>(DangerActorsTHR[i]->GetRootComponent()));
						// if (primComp_)
						// {							
						// 	FVector closestPoint_;
						// 	primComp_->GetClosestPointOnCollision(FlockMember.Transform.GetLocation(), closestPoint_);
						//
						// 	if ((closestPoint_ - FlockMember.Transform.GetLocation()).SizeSquared() < FlockParametersTHR.AttackRadiusSquared)
						// 	{								
						// 		FlockMember.AttackedActors.AddUnique(DangerActorsTHR[i]);
						// 	}
						// }
					}
	
					bIsFollowToEnemy = true;
					break;
				}
			}
		}
	}

	if (!bIsFollowToEnemy)
	{
		if (FlockParametersTHR.FollowActor)
		{
			NewVec = FlockParametersTHR.FollowActor->GetActorLocation() - FlockMember.Transform.GetLocation();
			NewVec.Normalize();
			NewVec *= FlockParametersTHR.FlockMaxSpeed;
			NewVec -= FlockMember.Velocity;
		}
		else if (FlockLeader <= FlockThreadMembersArr.Num() && FlockLeader >= 0)
		{
			NewVec = NewFlockLeader.Transform.GetLocation() - FlockMember.Transform.GetLocation();
			NewVec.Normalize();
			NewVec *= FlockParametersTHR.FlockMaxSpeed;
			NewVec -= FlockMember.Velocity;
		}
	}

	return NewVec;
}

FVector FlockThread::SteeringMaxHeight(FlockMemberData& FlockMember) const
{
	FRotator const RotationToDeep(UKismetMathLibrary::FindLookAtRotation(FlockMember.Transform.GetLocation(),
	                                                                      FVector(BoxComponentRef->GetComponentLocation().X, BoxComponentRef->GetComponentLocation().Y, FlockParametersTHR.MaxHeight)));
	FVector Direction = UKismetMathLibrary::Conv_RotatorToVector(RotationToDeep);
	Direction.Normalize();
	FVector NewVec = Direction * ((FlockParametersTHR.FlockEnemyAwarenessRadius / FlockParametersTHR.StrengthAquariumOffsetValue) * FlockParametersTHR.FleeScaleAquarium);
	return NewVec;
}

FVector FlockThread::SteeringFollowPawn(FlockMemberData& FlockMember)
{
	FVector NewVec = FVector(0, 0, 0);

	for (int i = 0; i < DangerActorsTHR.Num(); ++i)
	{
		if (DangerActorsTHR[i])
		{
			// calculate flee from this threat
			FVector FromEnemy = DangerActorsTHR[i]->GetActorLocation() - FlockMember.Transform.GetLocation();
			float const DistanceToEnemy = FromEnemy.Size();
			FromEnemy.Normalize();

			// enemy inside our enemy awareness threshold, so evade them
			if (DistanceToEnemy < FlockParametersTHR.FollowPawnAwarenessRadius)
			{
				NewVec += FromEnemy * ((FlockParametersTHR.FollowPawnAwarenessRadius / DistanceToEnemy) * FlockParametersTHR.FleeScale);
			}

			// Add attacked actors in array.
			if (FlockParametersTHR.bCanAttackPawn && DistanceToEnemy)
			{
				UPrimitiveComponent* PrimComp(Cast<UPrimitiveComponent>(DangerActorsTHR[i]->GetRootComponent()));
				if (PrimComp)
				{
					FVector ClosestPoint;
					PrimComp->GetClosestPointOnCollision(FlockMember.Transform.GetLocation(), ClosestPoint);

					if ((ClosestPoint - FlockMember.Transform.GetLocation()).Size() < FlockParametersTHR.AttackRadius)
					{
						FlockMember.AttackedActors.AddUnique(DangerActorsTHR[i]);
					}
				}
			}
		}
	}
	return NewVec;
}

void FlockThread::InitFlockLeader()
{
	if (FlockParametersTHR.FollowActor)
	{
		for (int i = 0; i < PoolThreadArr.Num(); i++)
		{
			PoolThreadArr[i]->FlockThreadMembersArr[0].bIsFlockLeader = false;
		}
	}
	else if (FlockParametersTHR.bUseOneLeader)
	{
		for (int i = 0; i < PoolThreadArr.Num(); i++)
		{
			PoolThreadArr[i]->FlockThreadMembersArr[0].bIsFlockLeader = false;
		}
		PoolThreadArr[0]->FlockThreadMembersArr[0].bIsFlockLeader = true;
	}
}

void FlockThread::SetPoolThread(TArray<FlockThread*> SetPoolThreadArr)
{
	PoolThreadArr = SetPoolThreadArr;
}

FVector FlockThread::SteeringAvoidance(FlockMemberData& FlockMember) const
{
	FVector NewVec(FVector::ZeroVector);

	for (int i = 0; i < AvoidanceActorRootArrTHR.Num(); ++i)
	{
		if (AvoidanceActorRootArrTHR[i])
		{
			UPrimitiveComponent* PrimComp(Cast<UPrimitiveComponent>(AvoidanceActorRootArrTHR[i]->GetRootComponent()));
			if (PrimComp)
			{
				FVector ClosestPoint;
				PrimComp->GetClosestPointOnCollision(FlockMember.Transform.GetLocation(), ClosestPoint);

				if ((ClosestPoint - FlockMember.Transform.GetLocation()).Size() < FlockParametersTHR.AvoidancePrimitiveDistance)
				{
					FRotator const RotationToCenter(UKismetMathLibrary::FindLookAtRotation(ClosestPoint, FlockMember.Transform.GetLocation()));
					FVector Direction = UKismetMathLibrary::Conv_RotatorToVector(RotationToCenter);
					Direction.Normalize();

					NewVec = Direction * ((FlockParametersTHR.FlockEnemyAwarenessRadius / FlockParametersTHR.StrengthAquariumOffsetValue) * FlockParametersTHR.FleeScaleAquarium);
				}
			}
		}
	}

	return NewVec;
}

void AFlockSystemActor::DivideFlockArrayForThreads()
{
	TArray<FlockMemberData> TempFlockData;
	FlockMembersArrays NewFlockData;

	if (MaxUseThreads > FlockMateInstances)
	{
		NewFlockData.AddData(0, TempFlockData);
		AllFlockMembersArrays.Add(NewFlockData);
		AllFlockMembersArrays[0].FlockMembersArr = FlockMemberDataArr;
		return;
	}

	int FlockID(0);
	int FlockPart = FlockMemberDataArr.Num() / MaxUseThreads;
	int const FlockPartIncrement = FlockPart;

	for (int i = 0; i < MaxUseThreads; i++)
	{
		for ( ; FlockID < FlockPart; FlockID++)
		{
			TempFlockData.Add(FlockMemberDataArr[FlockID]);
		}

		NewFlockData.AddData(i, TempFlockData);
		AllFlockMembersArrays.Add(NewFlockData);
		TempFlockData.Empty();

		FlockPart += FlockPartIncrement;
	}

	// Adding the remainder of the division.
	if (FlockID < FlockMemberDataArr.Num() - 1 && FlockID == (FlockPartIncrement * MaxUseThreads))
	{
		for ( ; FlockID < FlockMemberDataArr.Num(); FlockID++)
		{
			AllFlockMembersArrays[MaxUseThreads - 1].FlockMembersArr.Add(FlockMemberDataArr[FlockID]);
		}
	}
}

void AFlockSystemActor::AddFlockMemberWorldSpace(const FTransform& WorldTransform)
{
	StaticMeshInstanceComponent->AddInstanceWorldSpace(WorldTransform);

	FlockMemberData FlockMember;
	FlockMember.InstanceIndex = NumFlock;
	FlockMember.Transform = WorldTransform;
	//   flockMember_.Velocity = flockMember_.WanderPosition - flockMember_.Transform.GetLocation();
	if (NumFlock == 0)
	{
		FlockMember.bIsFlockLeader = true;
	}
	FlockMemberDataArr.Add(FlockMember);
	NumFlock++;
}

TArray<int32> FlockThread::GetNearbyFlockMates(int32 FlockMember)
{
	TArray<int32> Mates;
	if (FlockMember > FlockThreadMembersArr.Num()) return Mates;
	if (FlockMember < 0) return Mates;

	for (int32 i = 0; i < FlockThreadMembersArr.Num(); i++)
	{
		if (i != FlockMember)
		{
			FVector diff = FlockThreadMembersArr[i].Transform.GetLocation() - FlockThreadMembersArr[FlockMember].Transform.GetLocation();
			if (FMath::Abs(diff.Size()) < FlockParametersTHR.FlockMateAwarenessRadius)
			{
				Mates.Add(i);
			}
		}
	}

	return Mates;
}
