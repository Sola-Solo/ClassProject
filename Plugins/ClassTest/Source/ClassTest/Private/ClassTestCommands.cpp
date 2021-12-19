// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClassTestCommands.h"

#define LOCTEXT_NAMESPACE "FClassTestModule"

void FClassTestCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "ClassTest", "Bring up ClassTest window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
