// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MenuStyles.h"
#include "Widgets/SCompoundWidget.h"
#include "GlobalMenuStyle.h"

/**
 * 
 */
class CLASSPROJECT_API SMainMenuUI : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMainMenuUI)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<class AMainMenuHUD>, MainMenuHUD)

	SLATE_END_ARGS()
	/*//宏展开
	{
		//#define SLATE_BEGIN_ARGS( SMainMenuUI )
		public:
		struct FArguments : public TSlateBaseNamedArgs<SMainMenuUI>
		{
			typedef FArguments WidgetArgsType;
			FORCENOINLINE FArguments()

            //#define SLATE_ARGUMENT( TWeakObjectPtr<class AMainMenuHUD>, MainMenuHUD )
            TWeakObjectPtr<class AMainMenuHUD> _MainMenuHUD;
			WidgetArgsType& MainMenuHUD( TWeakObjectPtr<class AMainMenuHUD> InArg )
			{
				_MainMenuHUD = InArg;
				return this->Me();
			}
			//#define SLATE_END_ARGS() 
		};
	}*/

	// Constructs and lays out the Main Menu UI Widget.
	// args Arguments structure that contains widget-specific setup information.
	void Construct(const FArguments& args);

	// Click handler for the Play Game! button – Calls MenuHUD’s PlayGameClicked() event.
	FReply PlayGameClicked();

	// Click handler for the Quit Game button – Calls MenuHUD’s QuitGameClicked() event.
	FReply QuitGameClicked();

	// Stores a weak reference to the HUD controlling this class.
	TWeakObjectPtr<class AMainMenuHUD> MainMenuHUD;

	const struct FGlobalStyle* MenuStyle;
};
