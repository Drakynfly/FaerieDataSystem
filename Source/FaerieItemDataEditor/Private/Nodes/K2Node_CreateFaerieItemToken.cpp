// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#include "Nodes/K2Node_CreateFaerieItemToken.h"

#include "FaerieItemToken.h"

#include "K2Node_CallFunction.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "KismetCompilerMisc.h"
#include "KismetCompiler.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_CreateFaerieItemToken)

#define LOCTEXT_NAMESPACE "K2Node_CreateFaerieItemToken"

UK2Node_CreateFaerieItemToken::UK2Node_CreateFaerieItemToken(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeTooltip = LOCTEXT("CreateFaerieItemTokenTooltip", "Creates a new faerie item token");
}

FText UK2Node_CreateFaerieItemToken::GetBaseNodeTitle() const
{
	return LOCTEXT("CreateFaerieItemToken_BaseTitle", "Create Faerie Item Token");
}

FText UK2Node_CreateFaerieItemToken::GetNodeTitleFormat() const
{
	return LOCTEXT("CreateFaerieItemToken_TitleFormat", "Create {ClassName}");
}

UClass* UK2Node_CreateFaerieItemToken::GetClassPinBaseClass() const
{
	return UFaerieItemToken::StaticClass();
}

FText UK2Node_CreateFaerieItemToken::GetMenuCategory() const
{
	return LOCTEXT("FaerieMenuCategory", "Faerie");
}

FName UK2Node_CreateFaerieItemToken::GetCornerIcon() const
{
	return Super::GetCornerIcon();
	//return TEXT("Graph.Replication.ClientEvent");
}

void UK2Node_CreateFaerieItemToken::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Create ExposeOnSpawn pins
	UEdGraphPin* ClassPin = GetClassPin();
	if (ClassPin->DefaultObject == nullptr)
	{
		ClassPin->DefaultObject = UFaerieItemToken::StaticClass();

		UClass* UseSpawnClass = GetClassToSpawn();
		if (UseSpawnClass != nullptr)
		{
			CreatePinsForClass(UseSpawnClass);
		}
	}
}

void UK2Node_CreateFaerieItemToken::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	static FName Create_FunctionName = GET_FUNCTION_NAME_CHECKED(UFaerieItemToken, CreateFaerieItemToken);
	static FName TokenClass_ParamName(TEXT("TokenClass"));

	UK2Node_CreateFaerieItemToken* CreateOpNode = this;
	UEdGraphPin* SpawnNodeExec = CreateOpNode->GetExecPin();
	UEdGraphPin* SpawnClassPin = CreateOpNode->GetClassPin();
	UEdGraphPin* SpawnNodeThen = CreateOpNode->GetThenPin();
	UEdGraphPin* SpawnNodeResult = CreateOpNode->GetResultPin();

	if (!SpawnNodeExec || !SpawnClassPin || !SpawnNodeThen || !SpawnNodeResult)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("CreateFaerieItemToken_InternalError", "Invalid node @@").ToString(), CreateOpNode);
		CreateOpNode->BreakAllNodeLinks();
		return;
	}

	UClass* SpawnClass = ( SpawnClassPin != nullptr) ? Cast<UClass>(SpawnClassPin->DefaultObject) : nullptr;

	//////////////////////////////////////////////////////////////////////////
	// create 'UFaerieItemToken::CreateFaerieItemToken' call node
	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(CreateOpNode, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(Create_FunctionName, UWidgetBlueprintLibrary::StaticClass());
	CallCreateNode->AllocateDefaultPins();

	UEdGraphPin* CallCreateExec = CallCreateNode->GetExecPin();
	UEdGraphPin* CallCreateTokenClassPin = CallCreateNode->FindPinChecked(TokenClass_ParamName);
	UEdGraphPin* CallCreateResult = CallCreateNode->GetReturnValuePin();

	// Move 'exec' connection from create widget node to 'UFaerieItemToken::CreateFaerieItemToken'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeExec, *CallCreateExec);

	if ( SpawnClassPin->LinkedTo.Num() > 0 )
	{
		// Copy the 'blueprint' connection from the spawn node to 'UFaerieItemToken::CreateFaerieItemToken'
		CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallCreateTokenClassPin);
	}
	else
	{
		// Copy blueprint literal onto 'UFaerieItemToken::CreateFaerieItemToken' call
		CallCreateTokenClassPin->DefaultObject = SpawnClass;
	}

	// Move result connection from spawn node to 'UFaerieItemToken::CreateFaerieItemToken'
	CallCreateResult->PinType = SpawnNodeResult->PinType; // Copy type so it uses the right actor subclass
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeResult, *CallCreateResult);

	//////////////////////////////////////////////////////////////////////////
	// create 'set var' nodes

	// Get 'result' pin from 'begin spawn', this is the actual actor we want to set properties on
	UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, CreateOpNode, CallCreateResult, GetClassToSpawn(), CallCreateTokenClassPin);

	// Move 'then' connection from create widget node to the last 'then'
	CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *LastThen);

	// Break any links to the expanded node
	CreateOpNode->BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE
