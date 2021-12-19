// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ClassTestStyle.h"

class FClassTestCommands : public TCommands<FClassTestCommands>
{
public:

	FClassTestCommands()
		: TCommands<FClassTestCommands>(TEXT("ClassTest"), NSLOCTEXT("Contexts", "ClassTest", "ClassTest Plugin"), NAME_None, FClassTestStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};