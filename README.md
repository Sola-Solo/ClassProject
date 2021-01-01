# UE4C++基础

# UEC++ 与标准 C++ 的区别与联系

UEC++ 本质是 C++ 的一个超集，它支持和使用 C++ 的全部特性，但是它在标准特性之上自己构建了一套语法。UEC++ 则是我们开发当中使用的 Epic 在标准 C++ 之上扩展的用法，这里不讨论 GC、反射之类的基于 C++ 之上自己构建的对象体系。在 UE4.23 + 以后，UE 所支持的 C++ 标准是可以被控制的，在 `*.target.cs` 和 `* .build.cs` 中均可以设置`CppStandard` 的值，目前有三个标准可选：`Cpp14`、`Cpp17`、`Latast`，它控制了传递给编译器的 `/std:c++*`值。

# 项目文件结构

![Image/Untitled.png](Image/Untitled.png)

- `*.Target.cs` 是用来控制的是生成的可执行程序的外部编译环境，就是所谓的 Target。比如，生成的是什么 `Type`(Game/Client/Server/Editor/Program)，开不开启 `RTTI` (bForceEnableRTTI)，`CRT` 使用什么方式链接 (bUseStaticCRT) 等等。
- `*.Build.cs` 控制的是 Module 编译过程，由它来控制所属 Module 的对其他 Module 的依赖、文件包含、链接、宏定义等等相关的操作，`.Build.cs` 告诉 UE 的构建系统，它是一个 Module，并且编译的时候要做哪些事情。
- 其余的是代码源文件 (一般情况下头文件放在 `Public/`，实现放在 `Private/`)

# UEC++ 的特殊语法

UEC++ 的特殊语法主要是用于指导 UHT 来产生辅助代码的方式。来聊 UEC++ 的特殊语法之前，需要先明确一点：`UCLASS`/`UFUNCTION`/`UPROPERTY` 等都不是真正有意义的 C++ 宏，他们是 UHT 的标记，在经过 UHT 生成代码之后他们就是空宏了，没有意义。UE 的代码是把 UHT 标记和真正的宏都以宏的形式来表现，从结果来说，它们都是生成了一些代码，但是它们的处理流程不同。UHT 标记是先通过 UHT 进行扫描并生成代码，再通过编译器进行预处理等等，这里存在一个先后的过程，其限制就为：对 UHT 对代码的处理在前，编译器对宏的预处理在后，所以在 UE 中没办法用宏来包裹 UHT 标记。

UE 的 UHT 标记包括但不限于：

- [Function Specifiers](https://docs.unrealengine.com/en-US/Programming/UnrealArchitecture/Reference/Functions/Specifiers/index.html)
- [Metadata Specifiers](https://docs.unrealengine.com/en-US/Programming/UnrealArchitecture/Reference/Metadata/index.html)
- [Property Specifiers](https://docs.unrealengine.com/en-US/Programming/UnrealArchitecture/Reference/Properties/Specifiers/index.html)
- [Struct Specifiers](https://docs.unrealengine.com/en-US/Programming/UnrealArchitecture/Reference/Structs/Specifiers/index.html)
- [Interfaces](https://docs.unrealengine.com/en-US/Programming/UnrealArchitecture/Reference/Interfaces/index.html)

可以从 `Runtime\CoreUObject\Public\UObject\ObjectMacros.h` 看更多的 UHT 标记，有一个简单的分辨方法：如果这个宏是一个空宏，那么它就是一个 UHT 标记：

```cpp
.......

// These macros wrap metadata parsed by the Unreal Header Tool, and are otherwise
// ignored when code containing them is compiled by the C++ compiler
#define UPROPERTY(...)
#define UFUNCTION(...)
#define USTRUCT(...)
#define UMETA(...)
#define UPARAM(...)
#define UENUM(...)
#define UDELEGATE(...)
#define RIGVM_METHOD(...)

// This pair of macros is used to help implement GENERATED_BODY() and GENERATED_USTRUCT_BODY()
#define BODY_MACRO_COMBINE_INNER(A,B,C,D) A##B##C##D
#define BODY_MACRO_COMBINE(A,B,C,D) BODY_MACRO_COMBINE_INNER(A,B,C,D)

// Include a redundant semicolon at the end of the generated code block, so that intellisense parsers can start parsing
// a new declaration if the line number/generated code is out of date.
#define GENERATED_BODY_LEGACY(...) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_GENERATED_BODY_LEGACY);
#define GENERATED_BODY(...) BODY_MACRO_COMBINE(CURRENT_FILE_ID,_,__LINE__,_GENERATED_BODY);

#define GENERATED_USTRUCT_BODY(...) GENERATED_BODY()
#define GENERATED_UCLASS_BODY(...) GENERATED_BODY_LEGACY()
#define GENERATED_UINTERFACE_BODY(...) GENERATED_BODY_LEGACY()
#define GENERATED_IINTERFACE_BODY(...) GENERATED_BODY_LEGACY()

......
```

下面简单列举一些：

1. `.generated.h` 这个文件是 UE 特有的，它是 UHT 生成的代码（大多都是宏）
2. `UCLASS`/`UFUNCTION`/`UPROPERTY`/`UENUM` 等标记在 C++ 中是没有的，它们的作用是指导 UHT 生成什么样的辅助代码；
3. `GENERATED_BODY` 等系列宏标准 C++ 是没有的，它的作用在于把 UHT 生成的代码包含到当前类中来；
4. `BlueprintNativeEvent` 函数的实现需要加`_Implementation`，这个规则是没有的，上面提到了 C++ 中连 `UFUNCTION` 都没有；
5. C++ 中没有 Slate
6. UE 项目中的宏都会生成在 `Intermediate\Build\Win64\UE4Editor\Development\MODULE_NAME` 下，如 `MODULE_NAME_API`是导出符号，在标准 C++ 项目中需要你自己定义导出。
7. 标准 C++ 的接口可以通过抽象类的来实现，并不需要一个特定基类，而且并没有 UE 中不可以提供数据成员的限制（仅从语法的角度，当然从设计思路上接口要无状态）
8. 标准 C++ 没有 UE 中的 DELEGATE
9. `Cast<>` 和 `NewObject<>` 是 UE 特有的，C++ 使用四种标准 cast 和 `new`
10. C++ 也没有 UE 的 Thunk 函数
11. …

### `UCLASS`/`UFUNCTION`/`UPROPERTY`：

[UFUNCTION/UPROPERTY/UCLASS](https://zhuanlan.zhihu.com/p/149392857)

# UE4中的C++编程简介

![Image/Untitled%201.png](Image/Untitled%201.png)

## C++与蓝图

UE4提供了两种创建新Gameplay元素的方法：C++和蓝图视觉脚本。程序员利用C++即可添加基础Gameplay系统，然后设计师可基于这些系统进行构建或利用这些系统为某个特定关卡或游戏本身创建自定义Gameplay。Gameplay API和框架类在这两个系统中都可以使用，可以单独使用，但组合使用，互补长短才能发挥出它们真正的作用。

## 创建类

![Image/Untitled%202.png](Image/Untitled%202.png)

![Image/Untitled%203.png](Image/Untitled%203.png)

![Image/Untitled%204.png](Image/Untitled%204.png)

![Image/Untitled%205.png](Image/Untitled%205.png)

## C++与蓝图进行交互

修改代码为：

```cpp
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	int32 A;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Damage")
	float B;

	UFUNCTION(BlueprintCallable, Category="Damage")
	void MyFunc();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
```

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#include "MyActor.h"

// Sets default values
AMyActor::AMyActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	A = 200;
	B = 1.0f;
}

void AMyActor::MyFunc()
{
	UE_LOG(LogTemp, Warning, TEXT("Hello World"));
}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
```

热编译：

![Image/Untitled%206.png](Image/Untitled%206.png)

通过蓝图扩展C++类：

![Image/Untitled%207.png](Image/Untitled%207.png)

![Image/Untitled%208.png](Image/Untitled%208.png)

![Image/Untitled%209.png](Image/Untitled%209.png)

![Image/Untitled%2010.png](Image/Untitled%2010.png)

![Image/Untitled%2011.png](Image/Untitled%2011.png)

![Image/Untitled%2012.png](Image/Untitled%2012.png)

# UE4中常见的数据类型

### **类命名前缀**

虚幻引擎提供了一些在构建过程中生成代码的工具。这些工具会期待看到一些类命名，并在名称与预期不符时触发警告或错误。以下类前缀列表描述了工具期望的名称。

- 派生自 **Actor** 的类带有 **A** 前缀，如 `AController`。
- 派生自 **Object** 的类带有 **U** 前缀，如 `UComponent`。
- **Enums** 的前缀是 **E**，如 `EFortificationType`。
- **Interface** 的前缀通常是 **I**，如 `IAbilitySystemInterface`。
- **Template** 的前缀是 **T**，如 `TArray`。
- 派生自 **SWidget** 的类（Slate UI）带有前缀 **S**，如 `SButton`。
- 其他类的前缀为字母F，如 `FVector`。

### **数字类型**

由于不同平台有不同的基本类型大小，如 **短整型**、**整型** 和 **长整型**，因此UE4提供以下类型供您备选：

- `int8`/`uint8`：8位有符号/无符号整数
- `int16`/`uint16`：16位有符号/无符号整数
- `int32`/`uint32`：32位有符号/无符号整数
- `int64`/`uint64`：64位有符号/无符号整数

浮点数也支持标准 `float`（32位）和 `double`（64位）类型。

### **FString**

`FString` 是一个可变字符串，类似于 `std::string`。`FString` 拥有很多方法，方便您处理字符串。要创建新的 `FString`，请使用 `TEXT` 宏：

```cpp
FString MyStr = TEXT("Hello, Unreal 4!").
```

[字符串处理](https://docs.unrealengine.com/zh-CN/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/StringHandling/index.html)

### **FText**

`FText` 类似于FString，但旨在用于本地化文本。要创建新的 `FText`，请使用 `NSLOCTEXT` 宏。该宏将使用默认语言的名称空间、键和值。

```cpp
FText MyText = NSLOCTEXT("Game UI", "Health Warning Message", "Low Health!")
```

您还可以使用 `LOCTEXT` 宏，这样只需要每个文件定义一个名称空间即可。确保在文件结束时取消定义。

```cpp
// 在GameUI.cpp中
#define LOCTEXT_NAMESPACE "Game UI"

//...
FText MyText = LOCTEXT("Health Warning Message", "Low Health!")
//...

#undef LOCTEXT_NAMESPACE
// 文件结束
```

### **FName**

`FName` 存储通常反复出现的字符串作为辨识符，以在比较时节省内存和CPU时间。如果有多个对象引用一个字符串，`FName` 使用较小的存储空间索引来映射到给定字符串，而不是在引用它的每个对象中多次存储完整字符串。这样会将字符串内容存储一次，节省在多个对象中使用该字符串时占用的内存。`FName` 比较更快是因为UE4能够检查其索引值来确认其是否匹配，而无须检查每一个字符是否相同。

[FName](https://docs.unrealengine.com/en-US/API/Runtime/Core/UObject/FName/index.html)

### **TCHAR**

`TCHAR` 类型是独立于所用字符集存储字符的方法，字符集或许会因平台而异。实际上，UE4字符串使用 `TCHAR` 数组来存储 **UTF-16** 编码的数据。您可以使用重载的解除引用运算符（它返回 `TCHAR`）来访问原始数据。

某些函数需要使用它，例如 `FString::Printf`，"%s" 字符串格式说明符期待的是 `TCHAR`，而不是 `FString`。

```cpp
FString Str1 = TEXT("World");
int32 Val1 = 123;
FString Str2 = FString::Printf(TEXT("Hello, %s!You have %i points."), *Str1, Val1);
```

`FChar` 类型提供一组静态效用函数，用来处理各个 `TCHAR` 字符。

```cpp
TCHAR Upper('A');
TCHAR Lower = FChar::ToLower(Upper); // 'a'
```

[TChar](https://docs.unrealengine.com/en-US/API/Runtime/Core/Misc/TChar/index.html)

### **容器**

容器是一种类，它的主要功能是存储数据集合。最常见的这些类包括 `TArray`、`TMap` 和 `TSet`。每个类都会自动调节大小，因此增长到您所需的大小。

[Containers](https://docs.unrealengine.com/en-US/API/Runtime/Core/Containers/index.html)

### **TArray**

在所有三个容器中，您在虚幻引擎4中将会使用的主要容器是TArray，它的功能与 **std::vector** 十分相似，但会提供更多功能。以下是一些常见操作：

```cpp
TArray<AActor*> ActorArray = GetActorArrayFromSomewhere();

// 告知当前ActorArray中存储了多少个元素（AActor）。
int32 ArraySize = ActorArray.Num();

// TArray基于0（第一个元素将位于索引0处）
int32 Index = 0;
// 尝试检索给定索引处的元素
AActor* FirstActor = ActorArray[Index];

// 在数组末尾添加新元素
AActor* NewActor = GetNewActor();
ActorArray.Add(NewActor);

// 在数组末尾添加元素，但前提必须是该元素尚不存在于数组中
ActorArray.AddUnique(NewActor); // 不会改变数组，因为已经添加了NewActor。

// 从数组中移除"NewActor"的所有实例
ActorArray.Remove(NewActor);

// 移除指定索引处的元素
// 索引之上的元素将下移一位来填充空白空间
ActorArray.RemoveAt(Index);

// 更高效版本的"RemoveAt"，但不能保持元素的顺序
ActorArray.RemoveAtSwap(Index);

// 移除数组中的所有元素
ActorArray.Empty();
```

TArray 添加了对其元素进行垃圾回收的好处。这样会假设 TArray 存储了 UObject 派生的指针。

```cpp
UCLASS()
class UMyClass : UObject
{
    GENERATED_BODY();

    // ...

    UPROPERTY()
    AActor* GarbageCollectedActor;

    UPROPERTY()
    TArray<AActor*> GarbageCollectedArray;

    TArray<AActor*> AnotherGarbageCollectedArray;
};
```

[TArray：虚幻引擎中的数组](https://docs.unrealengine.com/zh-CN/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/TArrays/index.html)

### **TMap**

`TMap` 是键-值对的集合，类似于 `std::map`。`TMap` 具有一些根据元素键查找、添加和移除元素的快速方法。您可以使用任意类型来表示键，因为它定义有 `GetTypeHash` 函数，我们稍后将进行介绍。

假设您创建了一个基于网格的棋盘游戏，并需要存储和查询每一个正方形上的内容。`TMap` 会为您提供一种简单的可用方法。如果棋盘较小，并且尺寸不变，那么或许会有更有效的方法来达到此目的，但在此示例中，我们假设了一个尺寸较大、带有少量棋子的棋盘。

```cpp
enum class EPieceType
{
    King,
    Queen,
    Rook,
    Bishop,
    Knight,
    Pawn
};

struct FPiece
{
    int32 PlayerId;
    EPieceType Type;
    FIntPoint Position;

    FPiece(int32 InPlayerId, EPieceType InType, FIntVector InPosition) :
        PlayerId(InPlayerId),
        Type(InType),
        Position(InPosition)
    {
    }
};

class FBoard
{
private:

    // 通过使用TMap，我们可以按位置引用每一块
    TMap<FIntPoint, FPiece> Data;

public:
    bool HasPieceAtPosition(FIntPoint Position)
    {
        return Data.Contains(Position);
    }
    FPiece GetPieceAtPosition(FIntPoint Position)
    {
        return Data[Position];
    }

    void AddNewPiece(int32 PlayerId, EPieceType Type, FIntPoint Position)
    {
        FPiece NewPiece(PlayerId, Type, Position);
        Data.Add(Position, NewPiece);
    }

    void MovePiece(FIntPoint OldPosition, FIntPoint NewPosition)
    {
        FPiece Piece = Data[OldPosition];
        Piece.Position = NewPosition;
        Data.Remove(OldPosition);
        Data.Add(NewPosition, Piece);
    }

    void RemovePieceAtPosition(FIntPoint Position)
    {
        Data.Remove(Position);
    }

    void ClearBoard()
    {
        Data.Empty();
    }
};
```

[TMap](https://docs.unrealengine.com/zh-CN/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/TMap/index.html)

### **TSet**

`TSet` 存储唯一值集合，类似于 `std::set`。虽然通过 `TArray` 可通过其 `AddUnique` 和 `Contains` 方法支持类似集的行为，`TSet` 可以更快的实现这些运算且不会自动添加非独有元素。

```cpp
TSet<AActor*> ActorSet = GetActorSetFromSomewhere();

int32 Size = ActorSet.Num();

// 向集添加元素，但前提是集尚未包含这个元素
AActor* NewActor = GetNewActor();
ActorSet.Add(NewActor);

// 检查元素是否已经包含在集中
if (ActorSet.Contains(NewActor))
{
    // ...
}

// 从集移除元素
ActorSet.Remove(NewActor);

// 从集移除所有元素
ActorSet.Empty();

// 创建包含TSet元素的TArray
TArray<AActor*> ActorArrayFromSet = ActorSet.Array();
```

### **容器迭代器**

通过使用迭代器，您可以循环遍历容器的所有元素。以下是该迭代器语法的示例，使用的是 `TSet`。

```cpp
void RemoveDeadEnemies(TSet<AEnemy*>& EnemySet)
{
    // 从集开头处开始，迭代至集末尾
    for (auto EnemyIterator = EnemySet.CreateIterator(); EnemyIterator; ++EnemyIterator)
    {
        // *运算符获取当前元素
        AEnemy* Enemy = *EnemyIterator;
        if (Enemy.Health == 0)
        {
            //"RemoveCurrent"受TSet和TMap支持
            EnemyIterator.RemoveCurrent();
        }
    }
}
```

For-each循环

```cpp
// TArray
TArray<AActor*> ActorArray = GetArrayFromSomewhere();
for (AActor* OneActor :ActorArray)
{
    // ...
}

// TSet——与TArray相同
TSet<AActor*> ActorSet = GetSetFromSomewhere();
for (AActor* UniqueActor :ActorSet)
{
    // ...
}

// TMap——迭代器返回键-值对
TMap<FName, AActor*> NameToActorMap = GetMapFromSomewhere();
for (auto& KVP :NameToActorMap)
{
    FName Name = KVP.Key;
    AActor* Actor = KVP.Value;

    // ...
}
```

---

# 重定向

[可可西](https://www.cnblogs.com/kekec/p/13447788.html)

![Image/Untitled%2013.png](Image/Untitled%2013.png)

![Image/Untitled%2014.png](Image/Untitled%2014.png)

![Image/Untitled%2015.png](Image/Untitled%2015.png)

![Image/Untitled%2016.png](Image/Untitled%2016.png)

![Image/Untitled%2017.png](Image/Untitled%2017.png)

# 深入探索

## UE中的GamePlay框架

![Image/Untitled%2018.png](Image/Untitled%2018.png)

1.  从`UObject`派生下来的`AActor`，拥有了`UObject`的反射序列化网络同步等功能，同时又通过各种`Component`来组装不同组件。 
2. `AActor`中一些需要逻辑控制的成员分化出了`APawn`。`Pawn`就像是棋盘上的棋子，或者是战场中的兵卒。有3个基本的功能：可被`Controller`控制、`PhysicsCollision`表示和`MovementInput`的基本响应接口。代表了基本的逻辑控制物理表示和行走功能。根据这3个功能的定制化不同，可以派生出不同功能的的`DefaultPawn`、`SpectatorPawn`和`Character`。([GamePlay架构（四）Pawn](https://zhuanlan.zhihu.com/p/23321666))
3. `AController`是用来控制`APawn`的一个特殊的`AActor`。同属于`AActor`的设计，可以让`Controller`享受到`AActor`的基本福利，而和`APawn`分离又可以通过组合来提供更大的灵活性，把表示和逻辑分开，独立变化。([GamePlay架构（五）Controller](https://zhuanlan.zhihu.com/p/23480071))。而AController又根据用法和适用对象的不同，分化出了`APlayerController`来充当本地玩家的控制器，而`AAIController`就充当了NPC们的AI智能。([GamePlay架构（六）PlayerController和AIController](https://zhuanlan.zhihu.com/p/23649987))。而数据配套的就是`APlayerState`，可以充当`AController`的可网络复制的状态。
4. 到了Level这一层，UE为我们提供了`ALevelScriptActor`（关卡蓝图）当作关卡静态性的逻辑载体。而对于一场游戏或世界的规则，UE提供的AGameMode就只是一个虚拟的逻辑载体，可以通过`PersistentLevel`上的`AWorldSettings`上的配置创建出我们具体的`AGameMode`子类。AGameMode同时也是负责在具体的`Level`中创建出其他的`Pawn`和`PlayerController`的负责人，在`Level`的切换的时候`AGameMode`也负责协调`Actor`的迁移。配套的数据对象是`AGameState`。([GamePlay架构（七）GameMode和GameState](https://zhuanlan.zhihu.com/p/23707588))
5. `World`构建好了，该派玩家进来了。但游戏的方式多样，玩家的接入方式也多样。UE为了支持各种不同的玩家模式，抽象出了`UPlayer`实体来实际上控制游戏中的玩家`PlayerController`的生成数量和方式。([GamePlay架构（八）Player](https://zhuanlan.zhihu.com/p/23826859))
6. 所有的表示和逻辑汇集到一起，形成了全局唯一的`UGameInstance`对象，代表着整个游戏的开始和结束。同时为了方便开发者进行玩家存档，提供了`USaveGame`进行全局的数据配套。([GamePlay架构（九）GameInstance](https://zhuanlan.zhihu.com/p/24005952))

![Image/Untitled%2019.png](Image/Untitled%2019.png)

由此也可以看出来，UE基于`UObject`的机制出发，构建出了纷繁复杂的游戏世界，几乎所有的重要的类都直接或间接的继承于`UObject`，都能充分利用到`UObject`的反射等功能，大大加强了整体框架的灵活度和表达能力。比如`GamePlay`中最常用到根据某个`Class`配置在运行时创建出特定的对象的行为就是利用了反射功能；而网络里的属性同步也是利用了`UObject`的网络同步RPC调用；一个`Level`想保存成`uasset`文件，或者`USaveGame`想存档，也都是利用了`UObject`的序列化；而利用了`UObject`的`CDO`（Class Default Object），在保存时候也大大节省了内存；这么多Actor对象能在编辑器里方便的编辑，也得益于`UObject`的属性编辑器集成；对象互相引用的从属关系有了`UObject`的垃圾回收之后我们就不用担心会释放问题了。

## 虚幻对象（UObject）

虚幻引擎中的基本构建块叫做UObject。该类结合 `UClass`，可以提供多个最重要的引擎服务：

- 反射属性和方法
- 序列化属性
- 垃圾回收
- 按名称查找 `UObject`
- 属性的可配置值
- 属性和方法的联网支持

从 `UObject` 派生的每个类都会创建有一个 `UClass`，`UClass` 包含有关该类实例的所有元数据。`UObject` 和 `UClass` 一起位于Gameplay对象在其生命周期所有作用的最根部位置。如果要解释 `UClass` 和 `UObject`的差异在哪里，最合适的方法是 `UClass` 描述的是 `UObject` 实例的样子、可序列化和联网的属性等。大多数Gameplay开发不会直接从 `UObject` 派生，而是从AActor和UActorComponent派生。您无需知道 `UClass` 或 `UObject` 工作方式细节，这并不影响编写Gameplay代码，知道这些系统的存在即可。

## AActor

`AActor` 是将会成为Gameplay体验的一部分的 `UObject`。AActor由可以放在关卡中，或者通过Gameplay系统在运行时创建。可以放入关卡的所有对象都是从该类扩展而来的。由于 `AActor` 派生自 `UObject`，因此可以使用上一节所列的所有标准功能。Actor可以显式销毁，方法是使用Gameplay代码（C++或蓝图），或者在所属关卡从内存中卸载时通过标准的垃圾回收机制销毁。Actor负责游戏对象的高级行为。`AActor` 还是可以在联网时复制的基本类型。在网络复制期间，Actor还可以分发其拥有的、需要网络支持或同步的任何 `UActorComponent` 的信息。

Actor还有它们自己的行为（通过继承实现特殊化），但它们也充当Actor Component层级容器（通过复合实现特殊化）。这个过程通过Actor的 `RootComponent` 成员实现，它包含一个 `USceneComponent`，而后者继而包含许多其他成员。在可以将Actor放入关卡之前，它必须包含至少一个Scene Component，Actor可以从后者绘制其平移、旋转和缩放。

Actor包含在AActor生命周期中调用的一系列事件。以下列表是一组简化的事件，描绘了整个生命周期：

- `BeginPlay`：Actor首次在Gameplay中存在时调用。
- `Tick`：每帧调用一次，随着时间的进行持续完成工作。
- `EndPlay`：对象离开Gameplay空间时调用。

### 运行时生命周期

上文讨论了`AActor`生命周期的一小部分。对于关卡中放置的Actor，了解生命周期是很容易想象的到的：Actor加载并存在，最终关卡被卸载后，Actor被销毁。产生Actor比在游戏中创建普通对象稍微复杂一点，因为Actor需要注册到多个运行时系统才能满足其所有需要。需要设置Actor的初始位置和旋转。物理可能需要知道这些信息。负责告诉Actor执行tick事件的管理器也需要知道。诸如此类。因此，我们有一种方法专门用来产生Actor，叫做 `SpawnActor` （`UWorld` 的成员之一）。成功产生Actor后，引擎会调用它的 `BeginPlay` 方法，下一帧调用 `Tick`。

Actor生命周期结束时，您可以调用 `Destroy` 来将它销毁。在该过程中，将调用 `EndPlay`，能在Actor进入回收站之前执行自定义逻辑。另一个控制Actor生命周期时长的方法是使用 `Lifespan` 成员。您可以在对象的构造函数中设置Actor的时间跨度，也可以在运行时使用其他代码进行设置。当这段时间到期后，会自动对该Actor调用 `Destroy`。

[Actor 生命周期](https://docs.unrealengine.com/zh-CN/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/Actors/ActorLifecycle/index.html)

### **UActorComponent**

Actor Component （`UActorComponent` 类）有自己的行为，通常负责在许多类型Actor之间共享的功能，例如，提供视觉网格体、粒子效果、摄像机视角和物理互动。Actor通常提供与其游戏总体角色有关的高级目标，而Actor Component通常执行用于支持这些更高级目标的单独任务。组件也可以与其他组件相连接，或者可以成为Actor的根组件。

### **UStruct**

要使用 `UStruct`，您不必从任何特定类扩展，只需用USTRUCT()标记该结构体，构建工具就会为您完成基本工作。与 `UObject` 不同的是，`UStruct` 实例不会被垃圾回收。如果您要创建它们的动态实例，必须自行管理其生命周期。`UStruct` 应该是纯数据类型，包含 `UObject` 反射支持，可以在虚幻编辑器、蓝图操控、序列化、联网等中编辑。

---

# UE4 delegate初探

[UE4 delegate初探](https://zhuanlan.zhihu.com/p/150117642)

UE4中的[delegate](https://docs.unrealengine.com/zh-CN/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/Delegates/index.html)（[委托](https://docs.unrealengine.com/zh-CN/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/Delegates/index.html)）常用于解耦不同对象之间的关联：委托的触发者不与监听者有直接关联，两者通过委托对象间接地建立联系。

监听者通过将响应函数绑定到委托上，使得委托触发时立即收到通知，并进行相关逻辑处理。

委托，又称代理，本质是一个特殊类的对象，它内部可以储存（一个或多个）函数指针、调用参数和返回值。

委托的作用如同函数指针，但它更安全（支持编译期类型检查），而且更易于使用。

UE4在**DelegateCombinations.h**提供了一些宏来方便开发者快速来自定义一个delegate类型。

大致地使用流程如下：

1. 使用`DECLARE_*`宏声明一个自定义`delegate`类型`FDelegateXXX`
2. 声明一个`FDelegateXXX`类型的代理对象
3. 绑定需要执行的函数指针到代理对象上
4. 触发代理对象中的函数指针会立即执行
5. 不需要某个函数指针时，可将其从代理对象中解绑

UE4提供了五类`Delegate`：

## **1. 单播代理**

`DECLARE_DELEGATE_*`

`DECLARE_DELEGATE_RetVal_*`

不支持反射以及序列化，可以有返回值，支持`Playload`参数，可以绑定一个

### **触发代理方式**

1. 无返回值情况

`bool ExecuteIfBound(...)`

`void Execute(...)`

2. 有返回值情况

`RetValue Execute(...)`

- 注：RetValue为绑定函数的返回值

### **支持的绑定实例类型**

1. `Static`函数 // BindStatic
2. 普通C++对象的成员函数 // BindRaw
3. `Lambda`表达式 // BindLambda // BindWeakLambda
4. 与某个`UObject`对象关联的`Lambda`表达式
5. 共享引用包裹的普通C++对象的成员函数 // BindSP // BindThreadSafeSP
6. 线程安全的共享引用包裹的普通C++对象的成员函数
7. `UObject`对象的成员函数 // BindUObject
8. `UObject`对象的`UFunction`成员函数 // BindUFunction

## **2. 多播代理**

`DECLARE_MULTICAST_DELEGATE_*`

不支持反射以及序列化，无返回值，支持`Playload`参数，可以绑定多个

### **触发代理方式**

`void Broadcast(...)`

### **支持的绑定实例类型**

1. `Static`函数 // AddStatic
2. 普通C++对象的成员函数 // AddRaw
3. `Lambda`表达式 // AddLambda // AddWeakLambda
4. 与某个`UObject`对象关联的Lambda表达式
5. 共享引用包裹的普通C++对象的成员函数 // AddSP // AddThreadSafeSP
6. 线程安全的共享引用包裹的普通C++对象的成员函数
7. `UObject`对象的成员函数 // AddUObject
8. `UObject`对象的`UFunction`成员函数 // AddUFunction
- 另外，可以创建`FDelegate`对象，然后调用Add函数

## **3. 事件**

`DECLARE_EVENT_*`

不支持反射以及序列化，无返回值，支持`Playload`参数，可以绑定多个

### **触发代理方式**

`void Broadcast(...)`

### **支持的绑定实例类型**

同上

## **4. 动态代理**

`DECLARE_DYNAMIC_DELEGATE_*`

`DECLARE_DYNAMIC_DELEGATE_RetVal_*`

支持反射以及序列化，可以有返回值，不支持`Playload`参数，可以绑定一个

### **触发代理方式**

1. 无返回值情况

`bool ExecuteIfBound(...)`

`void Execute(...)`

2. 有返回值情况

`RetValue Execute(...)`

- 注：`RetValue`为绑定函数的返回值

### **支持的绑定实例类型**

// BindUFunction

`UObject`对象的`UFunction`成员函数

- 另外，可以使用`BindDynamic`宏来绑定

## **5. 动态多播代理**

`DECLARE_DYNAMIC_MULTICAST_DELEGATE_*`

支持反射以及序列化，蓝图中可食用，无返回值，不支持`Playload`参数，可以绑定多个

### **触发代理方式**

`void Broadcast(...)`

### **支持的绑定实例类型**

// Add函数、AddDynamic宏

// AddUnique函数、AddUniqueDynamic宏

`UObject`对象的`UFunction`成员函数

**`Payload`**为代理绑定时传递的额外参数变量列表，这些参数会存储在代理对象内部；在触发代理时，`Payload`会紧跟着`Execute`、`ExecuteInBound`或`Broadcast`传入的参数之后，填充到绑定函数指针的参数列表中，然后执行。

```cpp
DECLARE_DELEGATE_OneParam(FTestDelegate, **int**);

**static** **void** **StaticDelegateProc**(**int** nCode)
{
    UE_LOG(LogTemp, Log, TEXT("StaticDelegateProc : %d"), nCode);
}

**static** **void** **StaticDelegateProcTest**(**int** nCode, **float** dis, FString txt)
{
    UE_LOG(LogTemp, Log, TEXT("StaticDelegateProcTest : %d %f %s"), nCode, dis, *****txt);
}

FTestDelegate DelegateObj1;
DelegateObj1.BindStatic(StaticDelegateProc);
DelegateObj1.ExecuteIfBound(1);

*// Payload示例*
FTestDelegate DelegateObj2;
DelegateObj2.BindStatic(StaticDelegateProcTest, 12.0f, FString(TEXT("Hello")));  *// 12.0f、FString(TEXT("Hello"))为绑定时传入的Payload参数*
DelegateObj2.ExecuteIfBound(2);
```

**动态代理、动态多播代理**不支持**`Payload`**，因此，绑定函数与代理对象的参数、返回值必须完全一致。

**下面为一些类型示例：**

**1. 单播代理**

```cpp
DECLARE_DELEGATE( FSimpleDelegate );  *// 无参、无返回值*
DECLARE_DELEGATE_OneParam(FPakEncryptionKeyDelegate, uint8);  *// 1个参数、无返回值*
DECLARE_DELEGATE_TwoParams(FPakSigningKeysDelegate, TArray**<**uint8**>&**, TArray**<**uint8**>&**);  *// 2个参数、无返回值*
DECLARE_DELEGATE_RetVal_ThreeParams(**bool**, FOnMountPak, **const**  FString**&**, int32, IPlatformFile**::**FDirectoryVisitor*****);  *// 3个参数、bool返回值*
```

**2. 多播代理**

```cpp
DECLARE_MULTICAST_DELEGATE( FSimpleMulticastDelegate );  *// 无参*
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTreeStarted, **const**  UBehaviorTreeComponent**&**, **const** UBehaviorTree**&** );  *// 2个参数*
DECLARE_MULTICAST_DELEGATE_FourParams(FOnOpenURL, UIApplication*****, NSURL*****, NSString*****, id); *// 4个参数*
```

**3. 事件**

```cpp
DECLARE_EVENT(UWorld, FOnTickFlushEvent);  *// 无参*
DECLARE_EVENT_OneParam(IWebBrowserWindow, FOnTitleChanged, FString); *// 1个参数*
```

**4. 动态代理**

```cpp
DECLARE_DYNAMIC_DELEGATE(FOnGameWindowCloseButtonClickedDelegate);  *// 无参、无返回值*
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAssetLoaded, **class** **UObject***, Loaded); *// 1个参数、无返回值*
DECLARE_DYNAMIC_DELEGATE_RetVal(EMouseCursor**::**Type, FGetMouseCursor); *// 无参、EMouseCursor::Type返回值*
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(UWidget*****, FGenerateWidgetForObject, UObject*****, Item); *// 1个参数、UWidget*返回值*
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(FEventReply, FOnPointerEvent, FGeometry, MyGeometry, **const** FPointerEvent**&**, MouseEvent);  *// 2个参数、FEventReply返回值*
```

**5. 动态多播代理**

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE( FLevelStreamingLoadedStatus );  *// 无参*
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLandedSignature, **const** FHitResult**&**, Hit);  *// 1个参数*
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMovementModeChangedSignature, **class** **ACharacter***, Character, EMovementMode, PrevMovementMode, uint8, PreviousCustomMode);  *// 3个参数*
```

---

## **单播代理**

1. 定义代理类型

定义返回值为void含1个int类型参数的单播代理类型FCharacterDelegate

```cpp
DECLARE_DELEGATE_OneParam(FCharacterDelegate, **int**); *// TBaseDelegate<void, int>*
**typedef** TBaseDelegate**<void**, **int>** FCharacterDelegate;
```

定义返回值为bool含1个int类型参数的单播代理类型FCharacterDelegate_RetVal

```cpp
DECLARE_DELEGATE_RetVal_OneParam(**bool**, FCharacterDelegate_RetVal, **int**);  *// TBaseDelegate<bool, int>*
**typedef** TBaseDelegate**<bool**, **int>** FCharacterDelegate_RetVal;
```

2. 声明代理对象

```cpp
FCharacterDelegate CharacterDelegate;
FCharacterDelegate_RetVal CharacterDelegateRetVal;
```

以**FCharacterDelegate**为例，讲述单播代理的绑定、触发执行及存在的风险

- TestDelegateCharacter.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/Character.h"**#include** "TestDelegateCharacter.generated.h"UCLASS()
**class** **UDelegatepTestClass** **:** **public** UObject
{
	GENERATED_BODY()
**protected:int** m_nValue **=** 0;
**public:void** DelegateProc1(**int** nCode)
	{
		**this->**m_nValue **=** nCode;
		UE_LOG(LogTemp, Log, TEXT("DelegateProc1 : %d"), nCode);
	}

	UFUNCTION()
	**void** DelegateUFunctionProc1(**int** nCode)
	{
		**this->**m_nValue **=** nCode;
		UE_LOG(LogTemp, Log, TEXT("DelegateUFunctionProc1 : %d"), nCode);
	}
};

**class** **DelegateCppTestClass**{
	**int** m_nValue **=** 0;
**public:void** CppDelegateProc(**int** nCode)
	{
		**this->**m_nValue **=** nCode;
		UE_LOG(LogTemp, Log, TEXT("CppDelegateProc : %d"), nCode);
	}

	**void** **CppDelegateProc2**(**int** nCode)
	{
		**this->**m_nValue **=** nCode;
		UE_LOG(LogTemp, Log, TEXT("CppDelegateProc2 : %d"), nCode);
	}

	**void** **CppDelegateProc3**(**int** nCode)
	{
		**this->**m_nValue **=** nCode;
		UE_LOG(LogTemp, Log, TEXT("CppDelegateProc3 : %d"), nCode);
	}

	**void** **CppDelegateProc4**(**int** nCode)
	{
		**this->**m_nValue **=** nCode;
		UE_LOG(LogTemp, Log, TEXT("CppDelegateProc4 : %d"), nCode);
	}

	**void** **CppDelegateProc5**(**int** nCode)
	{
		**this->**m_nValue **=** nCode;
		UE_LOG(LogTemp, Log, TEXT("CppDelegateProc5 : %d"), nCode);
	}
};

DECLARE_DELEGATE_OneParam(FCharacterDelegate, **int**); *// TBaseDelegate<void, int>
//typedef TBaseDelegate<void, int> FCharacterDelegate;*
DECLARE_DELEGATE_RetVal_OneParam(**bool**, FCharacterDelegate_RetVal, **int**);  *// TBaseDelegate<bool, int>
//typedef TBaseDelegate<bool, int> FCharacterDelegate_RetVal;*
UCLASS(config **=** Game)
**class** **SOMETEST_API** ATestDelegateCharacter : **public** ACharacter
{
	GENERATED_BODY()

**public:static** **void** StaticCharacterDelegateProc(**int** nCode);
	**static** **void** **StaticDelegateProc**(**int** nCode);
	**void** **OnBind**();
	**void** **OnExecute**();
	**void** **OnUnbind**();

	FCharacterDelegate CharacterDelegate1;
	FCharacterDelegate CharacterDelegate2;
	FCharacterDelegate CharacterDelegate3;
	FCharacterDelegate CharacterDelegate4;
	FCharacterDelegate CharacterDelegate5;
	FCharacterDelegate CharacterDelegate6;
	FCharacterDelegate CharacterDelegate7;
	FCharacterDelegate CharacterDelegate8;
	FCharacterDelegate CharacterDelegate9;
	FCharacterDelegate CharacterDelegate10;
	FCharacterDelegate CharacterDelegate11;
	FCharacterDelegate CharacterDelegate12;
	FCharacterDelegate CharacterDelegate13;

};
```

- TestDelegateCharacter.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "TestDelegateCharacter.h"**void** ATestDelegateCharacter**::**StaticCharacterDelegateProc(**int** nCode)
{
	UE_LOG(LogTemp, Log, TEXT("StaticCharacterDelegateProc : %d"), nCode);
}

**void** ATestDelegateCharacter**::**StaticDelegateProc(**int** nCode)
{
	UE_LOG(LogTemp, Log, TEXT("StaticDelegateProc : %d"), nCode);
}

**void** ATestDelegateCharacter**::**OnBind()
{
	*// Bind Static*
	CharacterDelegate1.BindStatic(StaticDelegateProc);

	CharacterDelegate2.BindStatic(ATestDelegateCharacter**::**StaticCharacterDelegateProc);

	*// Bind Raw*
	DelegateCppTestClass Obj1;
	CharacterDelegate3.BindRaw(**&**Obj1, **&**DelegateCppTestClass**::**CppDelegateProc);

	*// Bind Lambda*
	**auto** LambdaDelegateProc **=** [](**int** nCode)**->void**{
		UE_LOG(LogTemp, Log, TEXT("LambdaDelegateProc : %d"), nCode);
	};
	CharacterDelegate4.BindLambda(LambdaDelegateProc);

	CharacterDelegate5.BindLambda(
		[](**int** nCode)**->void**{
		UE_LOG(LogTemp, Log, TEXT("LambdaDelegateProc2 : %d"), nCode);
	}
	);

	*// Bind Weak Lambda*
	**auto** WeakLambdaDelegateProc **=** [](**int** nCode)**->void**{
		UE_LOG(LogTemp, Log, TEXT("WeakLambdaDelegateProc : %d"), nCode);
	};
	UDelegatepTestClass***** UObj1 **=** NewObject**<**UDelegatepTestClass**>**(**this**, UDelegatepTestClass**::**StaticClass());
	CharacterDelegate6.BindWeakLambda(UObj1, WeakLambdaDelegateProc);

	UDelegatepTestClass***** UObj2 **=** NewObject**<**UDelegatepTestClass**>**(**this**, UDelegatepTestClass**::**StaticClass());
	CharacterDelegate7.BindWeakLambda(
		UObj2, [](**int** nCode)**->void**{
		UE_LOG(LogTemp, Log, TEXT("WeakLambdaDelegateProc2 : %d"), nCode);
	}
	);

	*// Bind SP(Shared Pointer)*
	TSharedRef**<**DelegateCppTestClass**>** ObjSP1 **=** MakeShareable(**new** DelegateCppTestClass());
	CharacterDelegate8.BindSP(ObjSP1, **&**DelegateCppTestClass**::**CppDelegateProc2);

	TSharedRef**<**DelegateCppTestClass**>** ObjSP2 **=** MakeShared**<**DelegateCppTestClass**>**();
	CharacterDelegate9.BindSP(ObjSP2, **&**DelegateCppTestClass**::**CppDelegateProc3);

	*// Bind Thread Safe SP(Shared Pointer)*
	TSharedRef**<**DelegateCppTestClass, ESPMode**::**ThreadSafe**>** ObjSafeSP1 **=** MakeShareable(**new** DelegateCppTestClass());
	CharacterDelegate10.BindThreadSafeSP(ObjSafeSP1, **&**DelegateCppTestClass**::**CppDelegateProc4);

	TSharedRef**<**DelegateCppTestClass, ESPMode**::**ThreadSafe**>** ObjSafeSP2 **=** MakeShared**<**DelegateCppTestClass, ESPMode**::**ThreadSafe**>**();
	CharacterDelegate11.BindThreadSafeSP(ObjSafeSP2, **&**DelegateCppTestClass**::**CppDelegateProc5);

	*// Bind UObject*
	UDelegatepTestClass***** UObj3 **=** NewObject**<**UDelegatepTestClass**>**(**this**, UDelegatepTestClass**::**StaticClass());
	CharacterDelegate12.BindUObject(UObj3, **&**UDelegatepTestClass**::**DelegateProc1);

	*// Bind UFunction*
	UDelegatepTestClass***** UObj4 **=** NewObject**<**UDelegatepTestClass**>**(**this**, UDelegatepTestClass**::**StaticClass());
	CharacterDelegate13.BindUFunction(UObj4, STATIC_FUNCTION_FNAME(TEXT("UDelegatepTestClass::DelegateUFunctionProc1")));
}

**void** ATestDelegateCharacter**::**OnExecute()
{
	CharacterDelegate1.ExecuteIfBound(1);
	CharacterDelegate2.ExecuteIfBound(2);
	CharacterDelegate3.ExecuteIfBound(3);
	CharacterDelegate4.ExecuteIfBound(4);
	CharacterDelegate5.ExecuteIfBound(5);
	CharacterDelegate6.ExecuteIfBound(6);
	CharacterDelegate7.ExecuteIfBound(7);
	CharacterDelegate8.ExecuteIfBound(8);
	CharacterDelegate9.ExecuteIfBound(9);
	CharacterDelegate10.ExecuteIfBound(10);
	CharacterDelegate11.ExecuteIfBound(11);
	CharacterDelegate12.ExecuteIfBound(12);
	**if** (CharacterDelegate13.IsBound())
	{
		CharacterDelegate13.Execute(13);
	}
}

**void** ATestDelegateCharacter**::**OnUnbind()
{
	CharacterDelegate1.Unbind();
	CharacterDelegate2.Unbind();
	CharacterDelegate3.Unbind();
	CharacterDelegate4.Unbind();
	CharacterDelegate5.Unbind();
	CharacterDelegate6.Unbind();
	CharacterDelegate7.Unbind();
	CharacterDelegate8.Unbind();
	CharacterDelegate9.Unbind();
	CharacterDelegate10.Unbind();
	CharacterDelegate11.Unbind();
	CharacterDelegate12.Unbind();
	CharacterDelegate13.Unbind();
}
```

- 注1：`BindRaw`函数用于绑定普通c++对象的成员函数，若该c++对象已被销毁，触发代理执行该对象的成员函数，将会导致内存违规操作。
- 注2：`BindLambda`函数用于绑定lambda表达式，若lambda表达式捕获外部变量已被销毁，触发代理执行lambda表达式，将会导致内存违规操作。
- 注3：`BindWeakLambda`、`BindUObject`、`BindUFunction`绑定时会与一个UObject对象进行弱引用关联（不影响该对象被gc回收）。若UObject对象被gc回收，直接调用Execute触发代理，将会导致内存违规操作；可先调用IsBound或调用`ExecuteIfBound`触发代理来检查该UObject的有效性，再来执行代理。
- 注4：`BindSP`、`BindThreadSafeSP`绑定时会与一个智能指针对象进行弱引用关联（不影响该对象的内存回收）。若智能指针对象的内存被回收，直接调用Execute触发代理，将会导致内存违规操作；可先调用IsBound或调用ExecuteIfBound触发代理来检查该智能指针对象的有效性，再来执行代理。
- 注5：单播代理对象在被销毁时，会在其析构函数中调用Unbind进行解绑操作。
- 注6：不同的绑定实例类型定义在：DelegateInstancesImpl.h

![https://pic4.zhimg.com/80/v2-0a946a8bdc3dc4d9cbf858b62d3ef6ef_720w.jpg](https://pic4.zhimg.com/80/v2-0a946a8bdc3dc4d9cbf858b62d3ef6ef_720w.jpg)

---

## **多播代理（组播代理）**

1. 定义返回值为void含1个int类型参数的多播代理类型FCharacterDelegate_Multicase

```cpp
DECLARE_MULTICAST_DELEGATE_OneParam(FCharacterDelegate_Multicast, **int**);  *// TMulticastDelegate<void, int>*
**typedef** TMulticastDelegate**<void**, **int>** FCharacterDelegate_Multicast;
```

![https://pic4.zhimg.com/80/v2-65545900c033bada80c252a86455031f_720w.jpg](https://pic4.zhimg.com/80/v2-65545900c033bada80c252a86455031f_720w.jpg)

2. 声明代理对象

```cpp
FCharacterDelegate_Multicast CharacterDelegateMulticast1;
```

以FCharacterDelegate_Multicast为例，讲述多播代理的绑定、触发执行及存在的风险。

```cpp
.h
**static** **void** StaticDelegateProc2(**int** nCode);

UFUNCTION(BlueprintCallable)
	**void** OnDelegateMulticastTest();

FCharacterDelegate_Multicast CharacterDelegateMulticast1;
FCharacterDelegate_Multicast CharacterDelegateMulticast2;
FCharacterDelegate_Multicast CharacterDelegateMulticast3;
FCharacterDelegate_Multicast CharacterDelegateMulticast4;
FCharacterDelegate_Multicast CharacterDelegateMulticast5;
FCharacterDelegate_Multicast CharacterDelegateMulticast6;
FCharacterDelegate_Multicast CharacterDelegateMulticast7;
FCharacterDelegate_Multicast CharacterDelegateMulticast8;
```

```cpp

void ATestDelegateCharacter::StaticDelegateProc2(int nCode)
{
	UE_LOG(LogTemp, Log, TEXT("StaticDelegateProc2 : %d"), nCode);
}

void ATestDelegateCharacter::OnDelegateMulticastTest()
{
	// Add Static
	bool b1 = CharacterDelegateMulticast1.IsBound(); // false

	FDelegateHandle HandleMC1 = CharacterDelegateMulticast1.AddStatic(StaticDelegateProc); // 绑定实例个数为：1
	CharacterDelegateMulticast1.AddStatic(ATestDelegateCharacter::StaticCharacterDelegateProc); // 绑定实例个数为：2

	bool b2 = CharacterDelegateMulticast1.IsBound(); // true
	bool b3 = CharacterDelegateMulticast1.IsBoundToObject(this); // false

	CharacterDelegateMulticast1.Remove(HandleMC1); // 绑定实例个数为：1

	FCharacterDelegate_Multicast::FDelegate MC1 = FCharacterDelegate_Multicast::FDelegate::CreateStatic(StaticDelegateProc2);
	CharacterDelegateMulticast1.Add(MC1); // 绑定实例个数为：2
	CharacterDelegateMulticast1.Broadcast(1); // 执行绑定实例列表（共2个）  注：执行顺序可能与函数的添加顺序不相同

	// Add Raw
	DelegateCppTestClass ObjMC1;
	CharacterDelegateMulticast2.AddRaw(&ObjMC1, &DelegateCppTestClass::CppDelegateProc); // 绑定实例个数为：1
	CharacterDelegateMulticast2.AddRaw(&ObjMC1, &DelegateCppTestClass::CppDelegateProc2); // 绑定实例个数为：2
	CharacterDelegateMulticast2.AddStatic(ATestDelegateCharacter::StaticCharacterDelegateProc); // 绑定实例个数为：3

	bool b4 = CharacterDelegateMulticast2.IsBoundToObject(&ObjMC1); // true

	CharacterDelegateMulticast2.RemoveAll(&ObjMC1); // CppDelegateProc、CppDelegateProc2被删除，绑定实例个数为：1

	bool b5 = CharacterDelegateMulticast2.IsBoundToObject(&ObjMC1); // false

	CharacterDelegateMulticast2.Broadcast(2); // 执行绑定实例列表（共1个）  注：执行顺序可能与函数的添加顺序不相同

	CharacterDelegateMulticast2.Clear(); // 绑定实例个数为：0

	// Add Lambda
	auto LambdaDelegateMCProc = [](int nCode)->void
	{
		UE_LOG(LogTemp, Log, TEXT("LambdaDelegateMCProc : %d"), nCode);
	};
	CharacterDelegateMulticast3.AddLambda(LambdaDelegateMCProc);// 绑定实例个数为：1

	CharacterDelegateMulticast3.AddLambda(
		[](int nCode)->void
	{
		UE_LOG(LogTemp, Log, TEXT("LambdaDelegateMCProc2 : %d"), nCode);
	}
	);// 绑定实例个数为：2
	CharacterDelegateMulticast3.Broadcast(3);// 执行绑定实例列表（共2个）  注：执行顺序可能与函数的添加顺序不相同

	// Add Weak Lambda
	auto WeakLambdaDelegateMCProc = [](int nCode)->void
	{
		UE_LOG(LogTemp, Log, TEXT("WeakLambdaDelegateMCProc : %d"), nCode);
	};
	UDelegatepTestClass* UObjMC1 = NewObject<UDelegatepTestClass>(this, UDelegatepTestClass::StaticClass());
	CharacterDelegateMulticast4.AddWeakLambda(UObjMC1, WeakLambdaDelegateMCProc);// 绑定实例个数为：1

	UDelegatepTestClass* UObjMC2 = NewObject<UDelegatepTestClass>(this, UDelegatepTestClass::StaticClass());
	CharacterDelegateMulticast4.AddWeakLambda(
		UObjMC2, [](int nCode)->void
	{
		UE_LOG(LogTemp, Log, TEXT("WeakLambdaDelegateMCProc2 : %d"), nCode);
	}
	);// 绑定实例个数为：2
	CharacterDelegateMulticast4.Broadcast(4);// 执行绑定实例列表（共2个）  注：执行顺序可能与函数的添加顺序不相同

	// Add SP(Shared Pointer)
	TSharedRef<DelegateCppTestClass> ObjMCSP1 = MakeShareable(new DelegateCppTestClass());
	CharacterDelegateMulticast5.AddSP(ObjMCSP1, &DelegateCppTestClass::CppDelegateProc2);// 绑定实例个数为：1

	TSharedRef<DelegateCppTestClass> ObjMCSP2 = MakeShared<DelegateCppTestClass>();
	CharacterDelegateMulticast5.AddSP(ObjMCSP2, &DelegateCppTestClass::CppDelegateProc3);// 绑定实例个数为：2
	CharacterDelegateMulticast5.Broadcast(5);

	// Add Thread Safe SP(Shared Pointer)
	TSharedRef<DelegateCppTestClass, ESPMode::ThreadSafe> ObjSafeMCSP1 = MakeShareable(new DelegateCppTestClass());
	CharacterDelegateMulticast6.AddThreadSafeSP(ObjSafeMCSP1, &DelegateCppTestClass::CppDelegateProc4);// 绑定实例个数为：1

	TSharedRef<DelegateCppTestClass, ESPMode::ThreadSafe> ObjSafeMCSP2 = MakeShared<DelegateCppTestClass, ESPMode::ThreadSafe>();
	CharacterDelegateMulticast6.AddThreadSafeSP(ObjSafeMCSP2, &DelegateCppTestClass::CppDelegateProc5);// 绑定实例个数为：2

	CharacterDelegateMulticast6.Broadcast(6);// 执行绑定实例列表（共2个）  注：执行顺序可能与函数的添加顺序不相同

	// Add UObject
	UDelegatepTestClass* UObjMC3 = NewObject<UDelegatepTestClass>(this, UDelegatepTestClass::StaticClass());
	CharacterDelegateMulticast7.AddUObject(UObjMC3, &UDelegatepTestClass::DelegateProc1);// 绑定实例个数为：1
	CharacterDelegateMulticast7.AddUObject(UObjMC3, &UDelegatepTestClass::DelegateProc2);// 绑定实例个数为：2

	CharacterDelegateMulticast7.Broadcast(7);// 执行绑定实例列表（共2个）  注：执行顺序可能与函数的添加顺序不相同

	// Add UFunction
	UDelegatepTestClass* UObjMC4 = NewObject<UDelegatepTestClass>(this, UDelegatepTestClass::StaticClass());
	CharacterDelegateMulticast8.AddUFunction(UObjMC4, STATIC_FUNCTION_FNAME(TEXT("UDelegatepTestClass::DelegateUFunctionProc1")));// 绑定实例个数为：1
	CharacterDelegateMulticast8.Broadcast(8);// 执行绑定实例列表（共1个）  注：执行顺序可能与函数的添加顺序不相同
}
```

- 注1：`AddRaw`函数用于绑定普通c++对象的成员函数，若该c++对象已被销毁，触发代理执行该对象的成员函数，将会导致内存违规操作。
- 注2：`AddLambda`函数用于绑定lambda表达式，若lambda表达式捕获外部变量已被销毁，触发代理执行lambda表达式，将会导致内存违规操作。
- 注3：`AddWeakLambda`、`AddUObject`、`AddUFunction`绑定时会与一个UObject对象进行弱引用关联（不影响该对象被gc回收），在触发代理时，会先检查该UObject的有效性，再来执行代理。
- 注4：`BindSP`、`BindThreadSafeSP`绑定时会与一个智能指针对象进行弱引用关联（不影响该对象的内存回收），在触发代理时，会先检查该智能指针对象的有效性，再来执行代理。
- 注5：可通过`Remove`删除指定Handle的代理对象；通过`RemoveAll`删除相关UObject对象所有的代理对象；通过Clear清除代理对象列表。

![https://pic1.zhimg.com/80/v2-2107e88b54a09caa19a83455d66890e8_720w.jpg](https://pic1.zhimg.com/80/v2-2107e88b54a09caa19a83455d66890e8_720w.jpg)

---

## **事件**

1. 定义返回值为void含1个int类型参数的事件类型FCharacterEvent

```cpp
DECLARE_EVENT_OneParam(ATestDelegateCharacter, FCharacterEvent, **int**);  *// TBaseMulticastDelegate<void, int>*
**class** **FCharacterEvent** **:** **public** TBaseMulticastDelegate**<void**, **int>**{
    **friend** **class** **ATestDelegateCharacter**;
}
```

- 注1：定义事件类型宏的第一个参数为拥有该事件类型A的类型B，即为类型B为事件类型A的友元类，可以访问A中的私有成员。
- 注2：定义事件类型宏一般放在一个类的内部（即访问该类型时需要带上所在类的名称前缀，如ATestDelegateCharacter::FCharacterEvent），可有效减少事件类型名称冲突

2. 声明代理对象

```cpp
FCharacterEvent CharacterEvent;
```

![https://pic4.zhimg.com/80/v2-675cc86768bb9bc4abb007d5d91fb707_720w.jpg](https://pic4.zhimg.com/80/v2-675cc86768bb9bc4abb007d5d91fb707_720w.jpg)

事件本质是一个多播代理，另外，在事件的友元类型ATestDelegateCharacter中，可访问事件的私有成员。

```cpp
.h
DECLARE_EVENT_OneParam(ATestDelegateCharacter, FCharacterEvent, **int**);

**void** **OnTriggerEvent**(**int** nCode);

UFUNCTION(BlueprintCallable)
	**void** OnEventTest();

FCharacterEvent CharacterEvent;

.cpp
**void** ATestDelegateCharacter**::**OnEventTest()
{
	CharacterEvent.AddUObject(**this**, **&**ATestDelegateCharacter**::**OnTriggerEvent);
	CharacterEvent.Broadcast(1);
}
```

![https://pic2.zhimg.com/80/v2-69a3074839019b91aedbf9b8fb0fe6c5_720w.png](https://pic2.zhimg.com/80/v2-69a3074839019b91aedbf9b8fb0fe6c5_720w.png)

---

## **动态代理**

1. 定义返回值为void含1个int类型参数的动态代理类型FCharacterDelegate_Dynamic

```cpp
DECLARE_DYNAMIC_DELEGATE_OneParam(FCharacterDelegate_Dynamic, **int**, nCode); *// TBaseDynamicDelegate<FWeakObjectPtr, void, int>*
```

2. 定义返回值为bool含1个int类型参数的动态代理类型FCharacterDelegate_DynamicRetVal

```cpp
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(**bool**, FCharacterDelegate_DynamicRetVal, **int**, nCode);  *// TBaseDynamicDelegate<FWeakObjectPtr, bool, int>*
```

![https://pic3.zhimg.com/80/v2-ae49ad3e2727e458800a843622a0abbe_720w.jpg](https://pic3.zhimg.com/80/v2-ae49ad3e2727e458800a843622a0abbe_720w.jpg)

以FCharacterDelegate_Dynamic为例，讲述动态代理的绑定、触发执行及存在的风险。

```cpp
.h
UFUNCTION(BlueprintCallable)
	void OnDelegateDynamicTest();
UFUNCTION()//注意此处要加宏
	bool DynamicMulticastProc(int nCode);

FCharacterDelegate_Dynamic CharacterDelegateDynamic;//vs报错请忽略
FCharacterDelegate_DynamicRetVal CharacterDelegateDynamic2;

.cpp
void ATestDelegateCharacter::OnDelegateDynamicTest()
{
	bool bd1 = CharacterDelegateDynamic.IsBound(); // false

	UDelegatepTestClass* UObjDy1 = NewObject<UDelegatepTestClass>(this, UDelegatepTestClass::StaticClass());
	CharacterDelegateDynamic.BindUFunction(UObjDy1, STATIC_FUNCTION_FNAME(TEXT("UDelegatepTestClass::DelegateUFunctionProc1")));// 绑定实例个数为：1

	bool bd2 = CharacterDelegateDynamic.IsBound(); // true

	CharacterDelegateDynamic.ExecuteIfBound(1);

	CharacterDelegateDynamic.Unbind();

	// 使用BindDynamic宏
	CharacterDelegateDynamic2.BindDynamic(this, &ATestDelegateCharacter::DynamicMulticastProc);
	if (CharacterDelegateDynamic2.IsBound()) // true
	{
		CharacterDelegateDynamic2.Execute(2);
	}

	CharacterDelegateDynamic2.Clear(); // 功能与Unbind一样，内部是直接调用Unbind方法
}

bool ATestDelegateCharacter::DynamicMulticastProc(int nCode)
{
	UE_LOG(LogTemp, Log, TEXT("DynamicMulticastProc : %d"), nCode);
	return true;
}
```

![https://pic2.zhimg.com/80/v2-1fab03fe2a568320887181685157c369_720w.jpg](https://pic2.zhimg.com/80/v2-1fab03fe2a568320887181685157c369_720w.jpg)

- 注：BindUFunction绑定时会与一个UObject对象进行弱引用关联（不影响该对象被gc回收）。若UObject对象被gc回收，直接调用Execute触发代理，将会导致内存违规操作；可先调用IsBound或调用ExecuteIfBound触发代理来检查该UObject的有效性，再来执行代理

---

## **动态多播代理**

定义返回值为void含1个int类型参数的动态多播代理类型FCharacterDelegate_DynamicMulticast

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDelegate_DynamicMulticast, **int**, nCode); *// TBaseDynamicMulticastDelegate<FWeakObjectPtr, void, int>*
```

![https://pic4.zhimg.com/80/v2-d785f9839cfda8d169c6bb14df15ea1b_720w.jpg](https://pic4.zhimg.com/80/v2-d785f9839cfda8d169c6bb14df15ea1b_720w.jpg)

FCharacterDelegate_DynamicMulticast为例，讲述动态多播代理的绑定、触发执行及存在的风险。

```cpp
.h
UFUNCTION(BlueprintCallable)
	void OnDelegateDynamicMulticastTest();

UFUNCTION()//注意此处要加宏
	void DynamicMulticastProc1(int nCode);

UFUNCTION()//注意此处要加宏
	void DynamicMulticastProc2(int nCode);

UFUNCTION()//注意此处要加宏
	void DynamicMulticastProc3(int nCode);

FCharacterDelegate_DynamicMulticast CharacterDelegateDynamicMulticast;

.cpp
void ATestDelegateCharacter::OnDelegateDynamicMulticastTest()
{
	UDelegatepTestClass* UObjDMC1 = NewObject<UDelegatepTestClass>(this, UDelegatepTestClass::StaticClass());
	// 使用AddDynamic宏
	//CharacterDelegateDynamicMulticast.AddDynamic(UObjDMC1, &UDelegatepTestClass::DelegateProc1); // 运行时错误：Unable to bind delegate (function might not be marked as a UFUNCTION or object may be pending kill)
	CharacterDelegateDynamicMulticast.AddDynamic(UObjDMC1, &UDelegatepTestClass::DelegateUFunctionProc1); // 绑定实例个数为：1
	//CharacterDelegateDynamicMulticast.AddDynamic(UObjDMC1, &UDelegatepTestClass::DelegateUFunctionProc1); // 运行时错误 加入DelegateUFunctionProc1的绑定实例已存在
	CharacterDelegateDynamicMulticast.AddDynamic(UObjDMC1, &UDelegatepTestClass::DelegateUFunctionProc2); // 绑定实例个数为：2

	// 使用AddUniqueDynamic宏
	CharacterDelegateDynamicMulticast.AddUniqueDynamic(this, &ATestDelegateCharacter::DynamicMulticastProc1); // 绑定实例个数为：3
	CharacterDelegateDynamicMulticast.AddUniqueDynamic(this, &ATestDelegateCharacter::DynamicMulticastProc1); // 加入DynamicMulticastProc的绑定实例已存在
	CharacterDelegateDynamicMulticast.AddUniqueDynamic(this, &ATestDelegateCharacter::DynamicMulticastProc2); // 绑定实例个数为：4

	FScriptDelegate delegateVar1; // FScriptDelegate即为TScriptDelegate<> //typedef TScriptDelegate<> FScriptDelegate;
	delegateVar1.BindUFunction(this, STATIC_FUNCTION_FNAME(TEXT("ATestDelegateCharacter::DynamicMulticastProc3")));
	CharacterDelegateDynamicMulticast.Add(delegateVar1); // 绑定实例个数为：5
	FScriptDelegate delegateVar2;
	delegateVar2.BindUFunction(this, STATIC_FUNCTION_FNAME(TEXT("ATestDelegateCharacter::DynamicMulticastProc3")));
	//CharacterDelegateDynamicMulticast.Add(delegateVar2); // 运行时错误 加入DynamicMulticastProc3的绑定实例已存在
	CharacterDelegateDynamicMulticast.AddUnique(delegateVar2); // 加入DynamicMulticastProc3的绑定实例已存在

	// 使用RemoveDynamic宏
	CharacterDelegateDynamicMulticast.RemoveDynamic(this, &ATestDelegateCharacter::DynamicMulticastProc2); // 绑定实例个数为：4

	// 使用IsAlreadyBound宏
	bool bDMC1 = CharacterDelegateDynamicMulticast.IsAlreadyBound(this, &ATestDelegateCharacter::DynamicMulticastProc3);// true

	CharacterDelegateDynamicMulticast.Remove(delegateVar2); // 绑定实例个数为：3
	CharacterDelegateDynamicMulticast.Remove(UObjDMC1, STATIC_FUNCTION_FNAME(TEXT("UDelegatepTestClass::DelegateUFunctionProc1")));// 绑定实例个数为：2

	bool bDMC2 = CharacterDelegateDynamicMulticast.IsAlreadyBound(this, &ATestDelegateCharacter::DynamicMulticastProc3); // false

	CharacterDelegateDynamicMulticast.RemoveAll(this); // 绑定实例个数为：1

	CharacterDelegateDynamicMulticast.Broadcast(300); // 执行绑定实例列表（共1个）  注：执行顺序可能与函数的添加顺序不相同

	CharacterDelegateDynamicMulticast.Clear(); // 清除所有绑定实例
}

void ATestDelegateCharacter::DynamicMulticastProc1(int nCode)
{
	UE_LOG(LogTemp, Log, TEXT("DynamicMulticastProc1 : %d"), nCode);
}

void ATestDelegateCharacter::DynamicMulticastProc2(int nCode)
{
	UE_LOG(LogTemp, Log, TEXT("DynamicMulticastProc2 : %d"), nCode);
}

void ATestDelegateCharacter::DynamicMulticastProc3(int nCode)
{
	UE_LOG(LogTemp, Log, TEXT("DynamicMulticastProc3 : %d"), nCode);
}
```

![https://pic1.zhimg.com/80/v2-922739bd19919bb20145ae597d8bba80_720w.png](https://pic1.zhimg.com/80/v2-922739bd19919bb20145ae597d8bba80_720w.png)

需要注意的是，只有动态多播代理才能在蓝图中使用（对其进行绑定、解绑、Broadcast、以及清除操作）

```cpp
.h
UFUNCTION(BlueprintCallable)
	**void** BrocastCharacterDelegateDynamicMulticast(**int** nCode);

UPROPERTY(BlueprintAssignable)
	FCharacterDelegate_DynamicMulticast2 CharacterDelegateDynamicMulticast2;

.cpp
**void** ATestDelegateCharacter**::**BrocastCharacterDelegateDynamicMulticast(**int** nCode)
{
	CharacterDelegateDynamicMulticast2.Broadcast(nCode);
	CharacterDelegateDynamicMulticast2.Clear();
}
```

![https://pic1.zhimg.com/80/v2-ca5cc7c532afaf855b1e1b850b58e35c_720w.jpg](https://pic1.zhimg.com/80/v2-ca5cc7c532afaf855b1e1b850b58e35c_720w.jpg)

![https://pic2.zhimg.com/80/v2-be15cec7ebd6b1f881a65962f289f38d_720w.png](https://pic2.zhimg.com/80/v2-be15cec7ebd6b1f881a65962f289f38d_720w.png)

---

# UE4 Slate初探

[UE4 Slate初探](https://zhuanlan.zhihu.com/p/149034739)

## **Slate的准备工作**

创建一个空白的C++项目，打开YourProject.Build.cs文件，添加以下代码：

```cpp
PrivateDependencyModuleNames.AddRange(**new** string[] { "Slate", "SlateCore" });
```

## **创建MainMenuHUD（继承自HUD）**

![https://pic4.zhimg.com/80/v2-19a319aa2349321cd9b1711bd3302123_720w.jpg](https://pic4.zhimg.com/80/v2-19a319aa2349321cd9b1711bd3302123_720w.jpg)

- MainMenuHUD.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/HUD.h"**#include** "MainMenuHUD.generated.h"*/**
 * 
 */*UCLASS()
**class** **SOMETEST_API** AMainMenuHUD : **public** AHUD
{
	GENERATED_BODY()
	*// Initializes the Slate UI and adds it as widget content to the game viewport.*
	**virtual** **void** PostInitializeComponents() **override**;

	*// Reference to the Main Menu Slate UI.*
	TSharedPtr**<class** **SMainMenuUI>** MainMenuUI;

**public:***// Called by SMainMenu whenever the Play Game! button has been clicked.*
	UFUNCTION(BlueprintImplementableEvent, Category **=** "Menus|Main Menu")
		**void** PlayGameClicked();

	*// Called by SMainMenu whenever the Quit Game button has been clicked.*
	UFUNCTION(BlueprintImplementableEvent, Category **=** "Menus|Main Menu")
		**void** QuitGameClicked();
	
};
```

- MainMenuHUD.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "MainMenuHUD.h"**void** AMainMenuHUD**::**PostInitializeComponents()
{
	Super**::**PostInitializeComponents();
}
```

## **创建SMainMenuUI（继承自SCompoundWidget）**

![https://pic4.zhimg.com/80/v2-0580b2548b22f791f9ebafd60d3c8c2b_720w.jpg](https://pic4.zhimg.com/80/v2-0580b2548b22f791f9ebafd60d3c8c2b_720w.jpg)

- MainMenuUI.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "Widgets/SCompoundWidget.h"*/**
 * 
 */***class** **SOMETEST_API** SMainMenuUI : **public** SCompoundWidget
{
**public:**SLATE_BEGIN_ARGS(SMainMenuUI)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr**<class** **AMainMenuHUD>**, MainMenuHUD)

	SLATE_END_ARGS()

	*// Constructs and lays out the Main Menu UI Widget.*
	*// args Arguments structure that contains widget-specific setup information.*
	**void** Construct(**const** FArguments**&** args);

	*// Click handler for the Play Game! button – Calls MenuHUD’s PlayGameClicked() event.*
	FReply **PlayGameClicked**();

	*// Click handler for the Quit Game button – Calls MenuHUD’s QuitGameClicked() event.*
	FReply **QuitGameClicked**();

	*// Stores a weak reference to the HUD controlling this class.*
	TWeakObjectPtr**<class** **AMainMenuHUD>** MainMenuHUD;
};
```

大部分代码是很直接的。注意：并没有将类指定为**`UCLASS()`**，实际上这个类并不需要暴露给蓝图。首先这个类继承自**`SCompoundWidget`，`SCompoundWidget`**是一个Slate控件，可以由其他控件组成——在Slate的API中有很多示例：SVerticalBox，SHorizontalBox，SOverlay等等。相反的控件是**`SLeafWidget`**，它不包含任何控件。

```cpp
SLATE_BEGIN_ARGS(SMainMenuUI)
{}

SLATE_ARGUMENT(TWeakObjectPtr**<class** **AMainMenuHUD>**, MainMenuHUD)

SLATE_END_ARGS()

*// Constructs and lays out the Main Menu UI Widget.
// args Arguments structure that contains widget-specific setup information.*
**void** Construct(**const** FArguments**&** args);
```

如果你不熟悉虚幻的宏，这部分可能看起来很奇怪。这三个宏的作用是用来生成一个结构，其中包含在构建过程中的参数列表。您可能注意到了**`Construct`**函数的参数**`FArguments`**——这些宏就是在定义这个结构。在我们的示例中，我们只有一个参数，一个指向**`AMainMenuHUD`**的弱指针。

- **SLATE_BEGIN_ARGS(WidgetType):带参宏**

Widgets的创建者可以使用**`SLATE_BEGIN_ARGS`**和**`SLATE_END_ARGS`**来声明和构建一个Widget,使得Widget可以通过`SNew()`和`SAssignNew()`来创建一个Widget,从而可以添加到用户视口.

其中参数WidgetType是你正在构建的Widget的类型;

```cpp
**#define SLATE_BEGIN_ARGS( WidgetType ) \
	public: \
	struct FArguments : public TSlateBaseNamedArgs<WidgetType> \
	{ \
		typedef FArguments WidgetArgsType; \
		FORCENOINLINE FArguments()**
```

- **SLATE_ARGUMENT( ArgType, ArgName )**

```cpp
*/**
 * Use this macro to declare a slate argument.
 * Arguments differ from attributes in that they can only be values
 */***#define SLATE_ARGUMENT( ArgType, ArgName ) \
		ArgType _##ArgName; \
		WidgetArgsType& ArgName( ArgType InArg ) \
		{ \
			_##ArgName = InArg; \
			return this->Me(); \
		}**
```

- **SLATE_END_ARGS():无参宏**

```cpp
**#define SLATE_END_ARGS() \
	};**
```

- **回到我们代码：**

```cpp
**void** **Construct**(**const** FArguments**&** args);
```

**`Construct()`**方法接收一个参数，我们使用**`SLATE_*`**宏对**`FArguments`**结构化，结构中包含了控件的所有参数。当我们创建控件时，将调用这个方法，您很快就可以看到如何布置窗口控件。

```cpp
*// Click handler for the Play Game! button – Calls MenuHUD’s PlayGameClicked() event.*
FReply **PlayGameClicked**();

*// Click handler for the Quit Game button – Calls MenuHUD’s QuitGameClicked() event.*
FReply **QuitGameClicked**();
```

这个两个方法是用来处理我们将要添加的按钮**`Play Game`**和**`Quit Game`**，这两个函数处理**`FOnClicked`**事件，它只返回一个**`FReply`**（告诉引擎是否处理事件）不接收任何参数。如果我们想要的话，我们可以为这些方法指定一个参数。在后面的教程，我会教你如何绑定数据并且使用他们。

## **继续我们的主菜单**

我们需要实现MainMenuUI.h中的三个方法(Slate语法在刚开始来看非常奇怪，但是我推荐你习惯性的保持缩进，这对于理解布局是非常有必要的）。

- MainMenuUI.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "SMainMenuUI.h"
**#include** "SlateOptMacros.h"
**#include** "Widgets/SOverlay.h"
**#include** "Widgets/SBoxPanel.h"
**#include** "Widgets/Input/SButton.h"
**#include** "Widgets/Text/STextBlock.h"
**#include** "Engine/Engine.h"
**#include** "MainMenuHUD.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
**void** SMainMenuUI**::**Construct(**const** FArguments**&** InArgs)
{
	MainMenuHUD **=** InArgs._MainMenuHUD;

	ChildSlot
		[
			SNew(SOverlay)
			**+** SOverlay**::**Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.ColorAndOpacity(FLinearColor**::**White)
				.ShadowColorAndOpacity(FLinearColor**::**Black)
				.ShadowOffset(FIntPoint(**-**1, 1))
				.Font(FSlateFontInfo("Arial", 26))
				.Text(FText**::**FromString("Main Menu"))
			]
			**+** SOverlay**::**Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SNew(SVerticalBox)
				**+** SVerticalBox**::**Slot()
				[
					SNew(SButton)
					.Text(FText**::**FromString("Play Game!"))
					.OnClicked(**this**, **&**SMainMenuUI**::**PlayGameClicked)
				]
				**+** SVerticalBox**::**Slot()
				[
					SNew(SButton)
					.Text(FText**::**FromString("Quit Game"))
					.OnClicked(**this**, **&**SMainMenuUI**::**QuitGameClicked)
				]
			]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SMainMenuUI**::**PlayGameClicked()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("PlayGameClicked"));
	}

	*// actually the BlueprintImplementable function of the HUD is not called; uncomment if you want to handle the OnClick via Blueprint*
	*//MainMenuHUD->PlayGameClicked();*
	**return** FReply**::**Handled();
}

FReply SMainMenuUI**::**QuitGameClicked()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("QuitGameClicked"));
	}

	*// actually the BlueprintImplementable function of the HUD is not called; uncomment if you want to handle the OnClick via Blueprint*
	*//MainMenuHUD->QuitGameClicked();*
	**return** FReply**::**Handled();
}
```

你可以看到我所说的Slate语法中缩进对布局的作用。我们从两个事件处理函数看起。如前面所讲，他们首先会调用HUD上的蓝图事件，这样便可以使我们在蓝图中处理这些按钮的点击事件，然后它们返回了**`FReply::Handled()`**——这是让引擎知道我们已经处理了点击事件，所以它不需要对玩家的鼠标输入做任何其他处理。

我们再来看看**`Construct()`**方法，如前面所讲，**`FArguments`**是用**`SLATE*`**宏定义的。在我们的示例中，它添加了一个方法，我们稍后将使用它来指定拥有此控件的**`HUD`**。我们首先要做的事情是捕获这个值，我们只需要将本地值设置为args包含的值就可以了。注意：args里面的实际名称是**`_MainMenuHUD`**，而不是**`MainMenuHUD`**，**`_MainMenuHUD`**是实际设置的变量，所有通过**`SLATE_ARGUMENT`**宏传递的参数都遵循此规定。

简单的说，我们使用**`[]`**运算符来组合子节点控件。对于组合控件，我需要先调用**`SNew(WidgetClass)`**方法，然后使用**`+ WidgetClass::Slot()`**来增加控件。然后我们可以在该**`slot`**中使用**`[]`**为该插槽指定子项。

**`SOverlay`**的第一个子控件是**`STextBlock`**，他与**`TextBlock`**或者**`Label`**非常相似。为此，我们指定了颜色、阴影颜色、阴影偏移量、字体和一些文本。所有这些都是相当显而易见的，但是请注意一下，我们在**`SNew`**后没有任何**`::Slot()`**调用，因为**`TextBlock`**是一个**`SLeafWidget`**，它不能包含子控件。所以我们没有任何插槽可以使用，因此我们只能指定自身属性。

我们的第二个子控件是一个组合控件——**`SVerticalBox`**。垂直框（与它相对应的是水平框），将所有元素由上到下依次排列（对于水平框是从左到右）。在**`SVerticalBox`**的插槽内（记住，**`SVerticalBox`**是个组合控件），我们指定两个参数的**`SButton`**。第一个参数是显示在按钮上的文本，第二个是事件处理的绑定函数（**`PlayGameClicked` / `QuitGameClicked`**），每当点击时会调用该函数。然后我们完成了布局的编码，记得用**;**号结尾。

## **修改MainMenuHUD.cpp**

将MainMenuUI绑定在MainMenuHUD上。

- MainMenuHUD.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "MainMenuHUD.h"**#include** "SMainMenuUI.h"**#include** "Widgets/SWeakWidget.h"**void** AMainMenuHUD**::**PostInitializeComponents()
{
	Super**::**PostInitializeComponents();

	SAssignNew(MainMenuUI, SMainMenuUI).MainMenuHUD(**this**);

	**if** (GEngine**->**IsValidLowLevel())
	{
		GEngine**->**GameViewport**->**AddViewportWidgetContent(SNew(SWeakWidget).PossiblyNullContent(MainMenuUI.ToSharedRef()));
	}
}
```

这里的设置就比较简单了——在确保**Engine**和**Viewport**有效后，我们创建一个**MainMenuUI**的实例，然后将控件作为内容添加到游戏视图上！注意：**SAssignNew**添加**MenuHUD**——这又是我们**SLATE_**宏的结果，记得前面我们提到的SLATE_ARGUMENT宏的**MenuHUD**部分？它不仅设置了我们的变量（**_MainMenuHUD**），也同时生成了我们这里用到的**MainMenuHUD(this)**这个设置方法。

## **创建自定义GameMode**

![https://pic3.zhimg.com/80/v2-0dd7029420e1ff399106e097760993ee_720w.jpg](https://pic3.zhimg.com/80/v2-0dd7029420e1ff399106e097760993ee_720w.jpg)

- WidgetGameMode.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/GameMode.h"**#include** "WidgetGameMode.generated.h"*/**
 * 
 */*UCLASS()
**class** **SOMETEST_API** AWidgetGameMode : **public** AGameMode
{
	GENERATED_BODY()

**public:**AWidgetGameMode();
	
};
```

- WidgetGameMode.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "WidgetGameMode.h"**#include** "MainMenuHUD.h"AWidgetGameMode**::**AWidgetGameMode()
{
	HUDClass **=** AMainMenuHUD**::**StaticClass();
}
```

- **然后再项目里进行设置：**

![https://pic4.zhimg.com/80/v2-680bdf4dee0fa33cb46818fe1617397b_720w.jpg](https://pic4.zhimg.com/80/v2-680bdf4dee0fa33cb46818fe1617397b_720w.jpg)

## **编译运行测试**

![https://pic3.zhimg.com/80/v2-375a56febb5340fbf02b6ba476424862_720w.jpg](https://pic3.zhimg.com/80/v2-375a56febb5340fbf02b6ba476424862_720w.jpg)

点击按钮，左上角出现对应文字，可是上方文字乱码，以后再修复。

## **添加自定义样式**

首先要做的事是设置样式集合，它将用于加载和引用我们的样式。样式本身会在虚幻编辑器中指定，使我们能够对设计进行大量更改，而无需重新编译代码（我们目前必须为样式和布局更改都进行编译）。我们设置的Style Set，需要调用MenuStyles，它是一个纯静态类。

- MenuStyles.h

```cpp
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class FMenuStyles
{
public:
	// Initializes the value of MenuStyleInstance and registers it with the Slate Style Registry.
	static void Initialize();

	// Unregisters the Slate Style Set and then resets the MenuStyleInstance pointer.
	static void Shutdown();

	// Retrieves a reference to the Slate Style pointed to by MenuStyleInstance.
	static const class ISlateStyle& Get();

	// Retrieves the name of the Style Set.
	static FName GetStyleSetName();

private:
	// Creates the Style Set.
	static TSharedRef<class FSlateStyleSet> Create();

	// Singleton instance used for our Style Set.
	static TSharedPtr<class FSlateStyleSet> MenuStyleInstance;

};
```

这些方法注释已经阐明的很清楚了，看名字也大概知道作用。后面我们将在我们的**`Game Module`**中调用**`Initialize()`**和**`Shutdown()`**方法。**`Get()`**方法会在以后我们需要加载特定样式集时使用。**`GetStyleSetName()`**用于引擎检索我们的样式集名称。这些方法的实现同样很简单：

- MenuStyles.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "MenuStyles.h"**#include** "Styling/SlateStyle.h"**#include** "Styling/SlateStyleRegistry.h"**#include** "Slate/SlateGameResources.h"TSharedPtr**<**FSlateStyleSet**>** FMenuStyles**::**MenuStyleInstance **=** NULL;

**void** FMenuStyles**::**Initialize()
{
	**if** (**!**MenuStyleInstance.IsValid())
	{
		MenuStyleInstance **=** Create();
		FSlateStyleRegistry**::**RegisterSlateStyle(*****MenuStyleInstance);
	}
}

**void** FMenuStyles**::**Shutdown()
{
	FSlateStyleRegistry**::**UnRegisterSlateStyle(*****MenuStyleInstance);
	ensure(MenuStyleInstance.IsUnique());
	MenuStyleInstance.Reset();
}

FName FMenuStyles**::**GetStyleSetName()
{
	**static** FName **StyleSetName**(TEXT("MenuStyles"));
	**return** StyleSetName;
}

TSharedRef**<**FSlateStyleSet**>** FMenuStyles**::**Create()
{
	TSharedRef**<**FSlateStyleSet**>** StyleRef **=** FSlateGameResources**::**New(FMenuStyles**::**GetStyleSetName(), "/Game/UI/Styles", "/Game/UI/Styles");
	**return** StyleRef;
}

**const** ISlateStyle**&** FMenuStyles**::**Get()
{
	**return** *****MenuStyleInstance;
}
```

在**`Initialize()`**中，我们判断MenuStyleInstance（我们的单例指针）是否有效（就是不为空）。如果不是有效的，我们就实例化它，并且用**`SlateStyleRegistry`**来注册样式集。在**`Shutdown()`**中，我们做出了相反的，我们取消了注册样式集，确保我们的指针是唯一的(在这种情况下应该是唯一的)，然后我们重置它（设置为空）。**`GetStyleSetName()`**中，我们只需将我们的样式的FName缓存为静态变量，并始终返回该值。 这样，我们就有一个简单的方式来获取我们的样式集单例。

## **加入到Game Module**

如果现在编译代码，它无法正常运行。我们还从来没有调用我们的静态方法！找到你的游戏模块的源文件，里面应该真正只有两行：一个包括你的模块的头文件和类似于以下内容：

```cpp
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl，SlateTutorials，“SlateTutorials”);
```

注意**`FDefaultGameModuleImpl`**,这是用于你的游戏模块的类。很多时候不会在这里去处理任何其他事情 - 但我们需要绑定到游戏模块来初始化我们的样式集！我们如何做到这一点？好吧，Epic的做法（就是我们要使用的）似乎只是简单地定义模块类在这里 - 但要记住，如果你要做的比我们复杂很多，分为 **`header & source`**是一个更好的主意。

```cpp
*// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.*
**#include** "SomeTest.h"
**#include** "Modules/ModuleManager.h"
**#include** "Slate/MenuStyles.h"
**#include** "Styling/SlateStyleRegistry.h"

*//Custom implementation of the Default Game Module.* 
**class** **FSomeTestGameModule** **:** **public** FDefaultGameModuleImpl
{
	*// Called whenever the module is starting up. In here, we unregister any style sets* 
	*// (which may have been added by other modules) sharing our* 
	*// style set's name, then initialize our style set.* 
	**virtual** **void** **StartupModule**() **override**{
		*//Hot reload hack*
		FSlateStyleRegistry**::**UnRegisterSlateStyle(FMenuStyles**::**GetStyleSetName());
		FMenuStyles**::**Initialize();
	}

	*// Called whenever the module is shutting down. Here, we simply tell our MenuStyles to shut down.*
	**virtual** **void** **ShutdownModule**() **override**{
		FMenuStyles**::**Shutdown();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSomeTestGameModule, SomeTest, "SomeTest" );
```

这里没什么难的，对吧？ 我们只是定义一个自定义的模块类，只是扩展我们以前的，并添加一些必要的调用来初始化和关闭我们的游戏模块。 注意，我们花时间去取消注册 - 只是为了防止任何其他模块引入一个相同的名称。

## **创建一个Style类**

现在我们有了样式集，让我们继续创建一个类，我们可以使用它来建立和自定义我们的菜单样式。你有很多很多方法做到这些，来满足您的布局需要。我个人倾向于有一个单一的“全局”样式定义的东西，例如标准按钮样式。然后创建控件特定的样式集，如果自定义控件（比如我们的主菜单UI）只存在一个或两个空格。 那么我们该怎么做呢？ 简单！ 我们将创建一个新的**`GlobalMenuStyle`**类（和一个**`GlobalStyle`**结构…你很快会看到）。

![https://pic1.zhimg.com/80/v2-e3113abcaf72bea9e17b3aba581dc858_720w.jpg](https://pic1.zhimg.com/80/v2-e3113abcaf72bea9e17b3aba581dc858_720w.jpg)

- GlobalMenuStyle.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"
**#include** "Styling/SlateWidgetStyle.h"
**#include** "Styling/SlateTypes.h"
**#include** "Styling/SlateWidgetStyleContainerBase.h"
**#include** "GlobalMenuStyle.generated.h"

*/**
 * 
 */*USTRUCT()
**struct** **FGlobalStyle** **:** **public** FSlateWidgetStyle
{
	GENERATED_USTRUCT_BODY()

	*// FSlateWidgetStyle*
	**virtual** **void** GetResources(TArray**<const** FSlateBrush***>&** OutBrushes) **const** **override**;
	**static** **const** FName TypeName;
	**virtual** **const** FName **GetTypeName**() **const** **override**;
	**static** **const** FGlobalStyle**&** GetDefault();

	*// Style that define the appearance of all menu buttons.* 
	UPROPERTY(EditAnywhere, Category **=** Appearance)
		FButtonStyle MenuButtonStyle;

	*// Style that defines the text on all of our menu buttons.* 
	UPROPERTY(EditAnywhere, Category **=** Appearance)
		FTextBlockStyle MenuButtonTextStyle;

	*// Style that defines the text for our menu title.* 
	UPROPERTY(EditAnywhere, Category **=** Appearance)
		FTextBlockStyle MenuTitleStyle;
};

*/**
 */*UCLASS(hidecategories**=**Object, MinimalAPI)
**class** **UGlobalMenuStyle** **:** **public** USlateWidgetStyleContainerBase
{
	GENERATED_BODY()

**public:***/** The actual data describing the widget appearance. */*UPROPERTY(Category**=**Appearance, EditAnywhere, meta**=**(ShowOnlyInnerProperties))
	FGlobalStyle MenuStyle;

	**virtual** **const** **struct** **FSlateWidgetStyle*** **const** **GetStyle**() **const** **override**{
		**return** **static_cast<** **const** **struct** **FSlateWidgetStyle*** **>**( **&**MenuStyle);
	}
};
```

这一个有点长，但（像前面一样）不是很复杂。首先，我们有**`GetResources`**方法 - 如果你使用任何`Slate Brush`（例如，定义一个**`SImage`**窗口控件的属性），你可以在这里用**`OutBrushes`**添加这些画刷。 在我们的示例中，我们的按钮和文本块样式不是Brush，所以我们不必在这个方法中做任何事情。接下来，我们有**`GetTypeName`**方法 - 这个方法给出了类型的名称，它应该匹配实际的类型名称。 这用于引用这个控件是什么类型。 **`GetDefault()`**方法允许我们设置一些默认值 - 例如，如果我们想要的话我们可以为标题屏幕设置默认字体或大小。最后，我们有三个属性。第一和第二都是关于按钮 - 按钮（SButton控件）实际采取两种样式 - 一个用于按钮本身，一个用于表示按钮上的文本的文本块。第三个属性，然后是我们的菜单标题文本。但是，这一个结构是不够的！我们实际上有一个充当容器基础的类 - 这允许我们在编辑器中可以修改这些结构的公开属性，甚至比定义更简单的实现：

- GlobalMenuStyle.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "GlobalMenuStyle.h"**const** FName FGlobalStyle**::**TypeName **=** TEXT("FGlobalStyle");

**const** FGlobalStyle**&** FGlobalStyle**::**GetDefault()
{
	**static** FGlobalStyle Default;
	**return** Default;
}

**void** FGlobalStyle**::**GetResources(TArray**<const** FSlateBrush***>&** OutBrushes) **const**{
	*// Add any brush resources here so that Slate can correctly atlas and reference them*
}

**const** FName FGlobalStyle**::**GetTypeName() **const**{
	**static** **const** FName InTypeName **=** TEXT("FGlobalStyle");
	**return** InTypeName;
}
```

大多数这些方法是空的 - 毕竟，我们没有任何Brush注册，我同样不会设置默认值（字面上很简单，可以在**GetDefault()**方法中更改您的样式的默认属性）。 只要确保你的**GetTypeName()**返回一个FName匹配你的样式结构的名称！

![https://pic3.zhimg.com/80/v2-ed0245f4418f8a1b8de29975cb4656f2_720w.jpg](https://pic3.zhimg.com/80/v2-ed0245f4418f8a1b8de29975cb4656f2_720w.jpg)

## **定义你的风格**

现在我们已经设置了样式，你不认为是时候去定义它们了吗？对吧！启动虚幻，在内容浏览器中创建一个名为UI的新文件夹，然后在其中创建一个名为Styles的新文件夹。 然后要创建实际的样式定义，创建一个新的**`Slate Widget`**资源（**`用户界面->Slate Widget Style`**）。 将提示您选择控件样式容器 - 选择`GlobalMenuStyle`，并将新资产命名为Global（如果您名为其他名称，请记住您以后使用的名称）。继续打开它，并将您的属性调整到你喜欢的东西 - 随意导入一些图像，为您的按钮使用！

![https://pic4.zhimg.com/80/v2-c00c98c87553603cd88554a46da114ff_720w.jpg](https://pic4.zhimg.com/80/v2-c00c98c87553603cd88554a46da114ff_720w.jpg)

## **使用你的风格**

我们定义了我们的风格，设置了一些漂亮的设置，但是我们如何实际使用这个！

首先，将以下成员变量添加到您的主菜单UI窗口控件：

```cpp
**const** **struct** **FGlobalStyle** ***** MenuStyle;
```

- MainMenuUI.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "MenuStyles.h"**#include** "Widgets/SCompoundWidget.h"**#include** "GlobalMenuStyle.h"*/**
 * 
 */***class** **SOMETEST_API** SMainMenuUI : **public** SCompoundWidget
{
**public:**SLATE_BEGIN_ARGS(SMainMenuUI)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr**<class** **AMainMenuHUD>**, MainMenuHUD)

	SLATE_END_ARGS()

	*// Constructs and lays out the Main Menu UI Widget.*
	*// args Arguments structure that contains widget-specific setup information.*
	**void** Construct(**const** FArguments**&** args);

	*// Click handler for the Play Game! button – Calls MenuHUD’s PlayGameClicked() event.*
	FReply **PlayGameClicked**();

	*// Click handler for the Quit Game button – Calls MenuHUD’s QuitGameClicked() event.*
	FReply **QuitGameClicked**();

	*// Stores a weak reference to the HUD controlling this class.*
	TWeakObjectPtr**<class** **AMainMenuHUD>** MainMenuHUD;

	**const** **struct** **FGlobalStyle*** MenuStyle;
};
```

- MainMenuUI.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "MainMenuUI.h"**#include** "SlateOptMacros.h"**#include** "Widgets/SOverlay.h"**#include** "Widgets/SBoxPanel.h"**#include** "Widgets/Input/SButton.h"**#include** "Widgets/Text/STextBlock.h"**#include** "Engine/Engine.h"**#include** "MainMenuHUD.h"**#include** "Internationalization/Text.h"**#include** "MenuStyles.h"BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
**void** SMainMenuUI**::**Construct(**const** FArguments**&** InArgs)
{
	MainMenuHUD **=** InArgs._MainMenuHUD;
	MenuStyle **=** **&**FMenuStyles**::**Get().GetWidgetStyle**<**FGlobalStyle**>**(TEXT("Global"));

	ChildSlot
		[
			SNew(SOverlay)
			**+** SOverlay**::**Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.TextStyle(**&**MenuStyle**->**MenuTitleStyle)
				.Text(FText**::**FromString("Main Menu"))
			]
			**+** SOverlay**::**Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			[
				SNew(SVerticalBox)
				**+** SVerticalBox**::**Slot()
				[
					SNew(SButton)
					.ButtonStyle(**&**MenuStyle**->**MenuButtonStyle)
					.TextStyle(**&**MenuStyle**->**MenuButtonTextStyle)
					.Text(FText**::**FromString("Play Game!"))
					.OnClicked(**this**, **&**SMainMenuUI**::**PlayGameClicked)
				]
				**+** SVerticalBox**::**Slot()
				[
					SNew(SButton)
					.ButtonStyle(**&**MenuStyle**->**MenuButtonStyle)
					.TextStyle(**&**MenuStyle**->**MenuButtonTextStyle)
					.Text(FText**::**FromString("Quit Game"))
					.OnClicked(**this**, **&**SMainMenuUI**::**QuitGameClicked)
				]
			]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SMainMenuUI**::**PlayGameClicked()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("PlayGameClicked"));
	}

	*// actually the BlueprintImplementable function of the HUD is not called; uncomment if you want to handle the OnClick via Blueprint*
	*//MainMenuHUD->PlayGameClicked();*
	**return** FReply**::**Handled();
}

FReply SMainMenuUI**::**QuitGameClicked()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("QuitGameClicked"));
	}

	*// actually the BlueprintImplementable function of the HUD is not called; uncomment if you want to handle the OnClick via Blueprint*
	*//MainMenuHUD->QuitGameClicked();*
	**return** FReply**::**Handled();
}
```

## **编译运行测试**

瞧！您的菜单现在已设置样式！这里我们做了什么的细节：

首先，在绑定我们的**`MainMenuHUD`**后，实际上通过我们的**`FMenuStyles`**类在布局前面加载**`Slate Widget`**样式。

接下来，我们调整游戏标题的**`STextBlock`**，以添加对**`TextStyle()`**的调用，并往其中传递我们的标题文本样式的地址。

这与Slate的属性调整完全相同！对于我们的两个按钮，我们实际分配两种样式。 首先，我们分配我们的按钮样式，然后我们分配文本样式 - 没有太多担心这里，对吧？你现在有一个菜单，有一些风格化的按钮！

编译并开始你的游戏，然后测试你的主菜单！

![https://pic3.zhimg.com/80/v2-b561bcee10f64d952af3d4e3639abcc6_720w.jpg](https://pic3.zhimg.com/80/v2-b561bcee10f64d952af3d4e3639abcc6_720w.jpg)

## **数据绑定**

什么是数据绑定？ 数据绑定是一种来自软件开发的概念，其中信息输出（例如玩家的当前血量）与实际展示的信息相关联。这样，只要更改数据（例如，对玩家造成伤害），显示就会自动更新。

在我们的示例中，我们将创建一个新的**`Slate UI`**，用作游戏中的HUD，在屏幕的上角显示玩家的当前血量和得分。最初，HUD将只有静态值 - 我们将在下一步中将其更改为绑定数据。 我将粘贴下面的代码，但不会详细介绍它是如何工作的 - 这是所有很简单的内容，已经在过去的教程中已经讲过了。

- GameHUDUI.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"
**#include** "Widgets/SCompoundWidget.h"
**#include** "GlobalMenuStyle.h"
**#include** "GameHUD.h"

*/**
 * 
 */*
**class** **SOMETEST_API** SGameHUDUI : **public** SCompoundWidget
{
**public:**SLATE_BEGIN_ARGS(SGameHUDUI)
		**:** _OwnerHUD()
	{}
	SLATE_ARGUMENT(TWeakObjectPtr**<class** **AGameHUD>**, OwnerHUD);
	SLATE_END_ARGS()

	*/** Constructs this widget with InArgs */***void** Construct(**const** FArguments**&** InArgs);

**private:***/**
	 * Stores a weak reference to the HUD owning this widget.
	 **/*
	TWeakObjectPtr**<class** **AGameHUD>** OwnerHUD;

	*/**
	 * A reference to the Slate Style used for this HUD's widgets.
	 **/*
	**const** **struct** **FGlobalStyle*** HUDStyle;
};
```

- GameHUDUI.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "GameHUDUI.h"**#include** "SlateOptMacros.h"**#include** "MenuStyles.h"**#include** "GlobalMenuStyle.h"**#include** "Widgets/SOverlay.h"**#include** "Widgets/Text/STextBlock.h"BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
**void** SGameHUDUI**::**Construct(**const** FArguments**&** InArgs)
{
	OwnerHUD **=** InArgs._OwnerHUD;

	HUDStyle **=** **&**FMenuStyles**::**Get().GetWidgetStyle**<**FGlobalStyle**>**("Global");

	ChildSlot
		[
			SNew(SOverlay)
			**+** SOverlay**::**Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.TextStyle(**&**HUDStyle**->**MenuTitleStyle)
				.Text(FText**::**FromString("SCORE: 0"))
			]
			**+** SOverlay**::**Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				SNew(STextBlock)
				.TextStyle(**&**HUDStyle**->**MenuTitleStyle)
				.Text(FText**::**FromString("HEALTH: 100"))
			]
		];

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
```

- GameHUD.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/HUD.h"**#include** "GameHUDUI.h"**#include** "GameHUD.generated.h"*/**
 * 
 */*UCLASS()
**class** **SOMETEST_API** AGameHUD : **public** AHUD
{
	GENERATED_BODY()
	
**public:**AGameHUD();

	*/**
	 * Initializes the Slate UI and adds it as a widget to the game viewport.
	 **/***virtual** **void** **PostInitializeComponents**() **override**;

**private:***/**
	 * Reference to the Game HUD UI.
	 **/*TSharedPtr**<class** **SGameHUDUI>** GameHUD;
};
```

- GameHUD.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "GameHUD.h"**#include** "Engine/GameViewportClient.h"**#include** "GameHUDUI.h"**#include** "Widgets/SWeakWidget.h"AGameHUD**::**AGameHUD()
	**:** Super(FObjectInitializer**::**Get())
{
}

**void** AGameHUD**::**PostInitializeComponents()
{
	Super**::**PostInitializeComponents();

	**if** (GEngine **&&** GEngine**->**GameViewport)
	{
		UGameViewportClient***** Viewport **=** GEngine**->**GameViewport;

		SAssignNew(GameHUD, SGameHUDUI).OwnerHUD(TWeakObjectPtr**<**AGameHUD**>**(**this**));

		Viewport**->**AddViewportWidgetContent(
			SNew(SWeakWidget).PossiblyNullContent(GameHUD.ToSharedRef())
		);
	}
}
```

继续构建项目并设置一个新的地图和游戏模式，并让HUD显示出来！

我们有两个信息需要显示，我们想绑定到我们的UI：得分和血量。这两个都是整数，但它们必须绑定为我们的HUD的字符串！我们很快会从游戏模式（积分）和角色（血量）获取此信息，但首先我们将处理数据绑定部分。我们的绑定将有两个重要的任务：第一，它将获取实际数据。接下来，它会将其转换为要应用于文本块控件的FText。

他们都需要两个东西：属性和绑定它的东西。在我们的例子中，因为我们需要对数据进行额外的处理（从一个整数转换为一个字符串），我们将在我们的widget类本身有一个函数来绑定。 将以下私有属性值和方法添加到**`SGameHUDUI`**类中：

- GameHUDUI.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "Widgets/SCompoundWidget.h"**#include** "GlobalMenuStyle.h"**#include** "GameHUD.h"*/**
 * 
 */***class** **SOMETEST_API** SGameHUDUI : **public** SCompoundWidget
{
**public:**SLATE_BEGIN_ARGS(SGameHUDUI)
		**:** _OwnerHUD()
	{}
	SLATE_ARGUMENT(TWeakObjectPtr**<class** **AGameHUD>**, OwnerHUD);
	SLATE_END_ARGS()

	*/** Constructs this widget with InArgs */***void** Construct(**const** FArguments**&** InArgs);

**private:***/**
	 * Stores a weak reference to the HUD owning this widget.
	 **/*TWeakObjectPtr**<class** **AGameHUD>** OwnerHUD;

	*/**
	 * A reference to the Slate Style used for this HUD's widgets.
	 **/***const** **struct** **FGlobalStyle*** HUDStyle;

**private:***/**
	 * Attribute storing the binding for the player's score.
	 **/*TAttribute**<**FText**>** Score;

	*/**
	 * Attribute storing the binding for the player's health.
	 **/*TAttribute**<**FText**>** Health;

	*/**
	 * Our Score will be bound to this function, which will retrieve the appropriate data and convert it into an FText.
	 **/*FText **GetScore**() **const**;

	*/**
	 * Our Health will be bound to this function, which will retrieve the appropriate data and convert it into an FText.
	 **/*FText **GetHealth**() **const**;
};
```

在虚幻中使用**`TAttribute`**类型来提供有一个**`accessor/getter`**的数据绑定。 接下来，我们有两个常量函数，负责检索和格式化数据到UI可以使用的类型！那么我们如何实际做绑定？ 好吧，它很简单 - 事实上，如果你在C++中为Unreal项目完成了输入绑定，你已经做到了。 在SGameHUDUI的Construct方法的顶部，在捕获HUDStyle之后，添加以下内容将我们的**`TAttributes`**绑定到它们适当的函数：

```cpp
Score.Bind(**this**, **&**STutorialGameHUDUI**::**GetScore);
Health.Bind(**this**, **&**STutorialGameHUDUI**::**GetHealth);
```

接下来，我们在UI布局中直接使用TAttributes类型：

```cpp
ChildSlot
	[
		SNew(SOverlay)
		**+** SOverlay**::**Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		[
			SNew(STextBlock)
			.TextStyle(**&**HUDStyle**->**MenuTitleStyle)
			.Text(Score)
		]
		**+** SOverlay**::**Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		[
			SNew(STextBlock)
			.TextStyle(**&**HUDStyle**->**MenuTitleStyle)
			.Text(Health)
		]
	];
```

最后，我们要在绑定函数放入一些临时数据，只是为了确保一切正常工作：

```cpp
FText STutorialGameHUDUI**::**GetScore() **const** { **return** FText**::**FromString("SCORE: --"); }
FText STutorialGameHUDUI**::**GetHealth() **const** { **return** FText**::**FromString("HEALTH: --"); }
```

继续编译肯定一切正常，恭喜你！你刚刚绑定了你的文本块！ 您可以对Slate的所有内容执行此绑定 - 按钮文本，列表项，图像背景，样式等。

![https://pic4.zhimg.com/80/v2-cf8f335db7d448dffd24d0a470493d1b_720w.jpg](https://pic4.zhimg.com/80/v2-cf8f335db7d448dffd24d0a470493d1b_720w.jpg)

## **有用的数据**

如果你只需要学习如何做数据绑定，而不关心本教程的细节，那么你可以跳过本节 - 在这里，我们只是实现了分数和血量功能。

为了获得本教程的得分和血量数据，我添加了以下`GameMode`和**`Character`**类，并将它们设置为与**`GameMap`**关卡一起使用。

- GameMapGameMode.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/GameMode.h"**#include** "GameMapGameMode.generated.h"*/**
 * 
 */*UCLASS()
**class** **SOMETEST_API** AGameMapGameMode : **public** AGameMode
{
	GENERATED_BODY()
**public:**AGameMapGameMode();

	*/**
	 * Retrieves the current Score from the game mode.
	 **/*UFUNCTION(BlueprintPure, BlueprintCallable, Category **=** "Score")
		int32 GetScore();

	*/**
	 * Adds to the game score.
	 **/*UFUNCTION(BlueprintCallable, Category **=** "Score")
		**void** AddPoints(int32 value);

	*/**
	 * Removes from the game score.
	 **/*UFUNCTION(BlueprintCallable, Category **=** "Score")
		**void** DeductPoints(int32 value);

**private:***/**
	 * Stores the current score.
	 **/*int32 CurrentScore;
};
```

- GameMapGameMode.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "GameMapGameMode.h"**#include** "GameCharacter.h"**#include** "GameHUD.h"**#include** <algorithm>AGameMapGameMode**::**AGameMapGameMode()
	**:** Super(FObjectInitializer**::**Get())
	, CurrentScore(0)
{
	HUDClass **=** AGameHUD**::**StaticClass();
	DefaultPawnClass **=** AGameCharacter**::**StaticClass();
}

int32 AGameMapGameMode**::**GetScore()
{
	**return** CurrentScore;
}

**void** AGameMapGameMode**::**AddPoints(int32 value)
{
	**if** (value **>** 0)
		CurrentScore **+=** value;
}

**void** AGameMapGameMode**::**DeductPoints(int32 value)
{
	**if** (value **>** 0)
		CurrentScore **=** std**::**max(CurrentScore **-** value, 0);
}
```

- GameCharacter.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/Character.h"**#include** "GameCharacter.generated.h"UCLASS()
**class** **SOMETEST_API** AGameCharacter : **public** ACharacter
{
	GENERATED_BODY()

	AGameCharacter();
**public:***/**
 * Stores the character's current health.
 **/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category **=** "Combat")
		int32 Health;

};
```

- GameCharacter.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "GameCharacter.h"*// Sets default values*
AGameCharacter**::**AGameCharacter()
{
	Health **=** 100;
}
```

一旦我们有了这些，更新得分和血量的数据是很简单的；

- GameHUDUI.cpp

```cpp
FText SGameHUDUI**::**GetScore() **const**{
	*// NOTE: THIS IS A TERRIBLE WAY TO DO THIS. DO NOT DO IT. IT ONLY WORKS ON SERVERS. USE GAME STATES INSTEAD!*
	AGameMapGameMode***** gameMode **=** Cast**<**AGameMapGameMode**>**(OwnerHUD**->**GetWorldSettings()**->**GetWorld()**->**GetAuthGameMode());

	**if** (gameMode **==** **nullptr**)
		**return** FText**::**FromString(TEXT("SCORE: --"));

	FString score **=** TEXT("SCORE: ");
	score.AppendInt(gameMode**->**GetScore());

	**return** FText**::**FromString(score);
}

FText SGameHUDUI**::**GetHealth() **const**{
	AGameCharacter***** character **=** Cast**<**AGameCharacter**>**(OwnerHUD**->**PlayerOwner**->**GetCharacter());

	**if** (character **==** **nullptr**)
		**return** FText**::**FromString(TEXT("HEALTH: --"));

	FString health **=** TEXT("HEALTH: ");
	health.AppendInt(character**->**Health);

	**return** FText**::**FromString(health);
}
```

## **创建PlayerController，添加按键绑定**

- GameMapPlayerController.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/PlayerController.h"**#include** "GameMapPlayerController.generated.h"*/**
 * 
 */*UCLASS()
**class** **SOMETEST_API** AGameMapPlayerController : **public** APlayerController
{
	GENERATED_BODY()
	
**public:**AGameMapPlayerController();

	**virtual** **void** **SetupInputComponent**() **override**;

	**void** **HomeButtomPressed**();
	**void** **EndButtomPressed**();
	**void** **PgupButtomPressed**();
	**void** **PgdnButtomPressed**();
};
```

- GameMapPlayerController.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "GameMapPlayerController.h"**#include** "GameCharacter.h"AGameMapPlayerController**::**AGameMapPlayerController()
{
	bShowMouseCursor **=** true;
	bEnableClickEvents **=** true;
}

**void** AGameMapPlayerController**::**SetupInputComponent()
{
	Super**::**SetupInputComponent();

	InputComponent**->**BindAction("Home", IE_Pressed, **this**, **&**AGameMapPlayerController**::**HomeButtomPressed);
	InputComponent**->**BindAction("End", IE_Pressed, **this**, **&**AGameMapPlayerController**::**EndButtomPressed);
	InputComponent**->**BindAction("Pgup", IE_Pressed, **this**, **&**AGameMapPlayerController**::**PgupButtomPressed);
	InputComponent**->**BindAction("Pgdn", IE_Pressed, **this**, **&**AGameMapPlayerController**::**PgdnButtomPressed);
}

**void** AGameMapPlayerController**::**HomeButtomPressed()
{
	Cast**<**AGameCharacter**>**(GetCharacter())**->**AddHealth();
}

**void** AGameMapPlayerController**::**EndButtomPressed()
{
	Cast**<**AGameCharacter**>**(GetCharacter())**->**SubHealth();
}

**void** AGameMapPlayerController**::**PgupButtomPressed()
{
	Cast**<**AGameCharacter**>**(GetCharacter())**->**AddScore();
}

**void** AGameMapPlayerController**::**PgdnButtomPressed()
{
	Cast**<**AGameCharacter**>**(GetCharacter())**->**SubScore();
}
```

## **修改GameCharacter**

- GameCharacter.h

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#pragma once
#include** "CoreMinimal.h"**#include** "GameFramework/Character.h"**#include** "GameMapGameMode.h"**#include** "GameCharacter.generated.h"UCLASS()
**class** **SOMETEST_API** AGameCharacter : **public** ACharacter
{
	GENERATED_BODY()

	AGameCharacter();
**public:***/**
 * Stores the character's current health.
 **/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category **=** "Combat")
		int32 Health;

	FORCEINLINE AGameMapGameMode***** **GetGameMode**() { **return** GetWorld() **?** GetWorld()**->**GetAuthGameMode**<**AGameMapGameMode**>**() **:** **nullptr**; }

	**void** **AddHealth**();
	**void** **SubHealth**();
	**void** **AddScore**();
	**void** **SubScore**();
};
```

- GameCharacter.cpp

```cpp
*// Fill out your copyright notice in the Description page of Project Settings.*
**#include** "GameCharacter.h"**#include** "Engine/World.h"*// Sets default values*
AGameCharacter**::**AGameCharacter()
{
	Health **=** 10;
}

**void** AGameCharacter**::**AddHealth()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("AddHealth"));
	}
	Health**++**;
}

**void** AGameCharacter**::**SubHealth()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("SubHealth"));
	}
	
	Health **=** **--**Health **<=** 0 **?** 0 **:** Health;
}

**void** AGameCharacter**::**AddScore()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("AddScore"));
	}
	
	GetGameMode()**->**AddPoints(10);
}

**void** AGameCharacter**::**SubScore()
{
	**if** (GEngine)
	{
		GEngine**->**AddOnScreenDebugMessage(**-**1, 3.f, FColor**::**Yellow, TEXT("SubScore"));
	}

	GetGameMode()**->**DeductPoints(10);
}
```

继续并运行游戏，然后更新得分！（您可以在示例文件中通过按Home/End来调整血量，然后按Page Up/Page Down来调整分数）。

![https://pic3.zhimg.com/80/v2-9f7181ce469e96d575aa2653f6ab1d96_720w.jpg](https://pic3.zhimg.com/80/v2-9f7181ce469e96d575aa2653f6ab1d96_720w.jpg)