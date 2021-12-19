// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClassTest.h"
#include "ClassTestStyle.h"
#include "ClassTestCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName ClassTestTabName("ClassTest");

#define LOCTEXT_NAMESPACE "FClassTestModule"

void FClassTestModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FClassTestStyle::Initialize();
	FClassTestStyle::ReloadTextures();

	FClassTestCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FClassTestCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FClassTestModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FClassTestModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ClassTestTabName, FOnSpawnTab::CreateRaw(this, &FClassTestModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FClassTestTabTitle", "ClassTest"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FClassTestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FClassTestStyle::Shutdown();

	FClassTestCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ClassTestTabName);
}

TSharedRef<SDockTab> FClassTestModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FClassTestModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("ClassTest.cpp"))
		);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(WidgetText)
			]
		];
}

void FClassTestModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ClassTestTabName);
}

void FClassTestModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FClassTestCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FClassTestCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FClassTestModule, ClassTest)