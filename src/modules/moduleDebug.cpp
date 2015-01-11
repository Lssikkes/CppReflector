#include "../modules.h"
#include "../astProcessor.h"
#include "../tools.h"

#pragma region ModulePrintAST
class ModulePrintAST: public IModule
{
public:
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
	{
		fprintf(stderr, "********************* PRINT AST ***********************\n");
		ASTProcessor::Print(stdout, rootNode);
	}

};

static ModuleRegistration gModulePrintAST("print_ast", new ModulePrintAST());

#pragma endregion

#pragma region ModulePrintStructure

class ModulePrintStructure : public IModule
{
public:

	void WalkAST(ASTNode* node, int level = 0)
	{
		int levelIncrement = 0;

		if (node->GetType() == ASTNode::Type::Root ||
			node->GetType() == ASTNode::Type::File ||
			node->GetType() == ASTNode::Type::Class ||
			node->GetType() == ASTNode::Type::Struct ||
			node->GetType() == ASTNode::Type::Union ||
			node->GetType() == ASTNode::Type::Template ||
			node->GetType() == ASTNode::Type::Namespace ||
			node->GetType() == ASTNode::Type::Inherit ||
			node->GetType() == ASTNode::Type::Parent ||
			node->GetType() == ASTNode::Type::Public ||
			node->GetType() == ASTNode::Type::Private ||
			node->GetType() == ASTNode::Type::Protected ||
			node->GetType() == ASTNode::Type::Friend ||		// Sup
			node->GetType() == ASTNode::Type::Using ||
			node->GetType() == ASTNode::Type::NamespaceUsing ||
			node->GetType() == ASTNode::Type::Instances ||
			node->GetType() == ASTNode::Type::Friend ||		// Hi friend
			node->GetType() == ASTNode::Type::Typedef ||
			node->GetType() == ASTNode::Type::TypedefHead ||
			node->GetType() == ASTNode::Type::TypedefSub ||
			//node->GetType() == ASTNode::Type::AntFwd ||
			//node->GetType() == ASTNode::Type::AntBack ||
			node->GetType() == ASTNode::Type::ArgDcl ||
			node->GetType() == ASTNode::Type::DclHead ||
			node->GetType() == ASTNode::Type::DclSub)
		{
			for (int i = 0; i < level; i++)
				putc(' ', stdout);



			std::string annotationTypes;
			if (node->GetType() == ASTNode::Type::Class || node->GetType() == ASTNode::Type::DclSub)
			{
				auto annotations = node->GatherAnnotations();
				for (auto it : annotations)
					annotationTypes += "[" + it->ToString() + "] ";
			}

			printf("%s %s %s\n", node->GetTypeString(), node->ToString().c_str(), annotationTypes.c_str());
			levelIncrement++;
		}


		auto& children = node->Children();
		for (size_t i = 0; i < children.size(); i++)
		{
			WalkAST(children[i], level + levelIncrement);
		}
	}
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
	{
		fprintf(stderr, "********************* PRINT STRUCTURE ***********************\n");
		WalkAST(rootNode);
	}

};

static ModuleRegistration gModulePrintStructure("print_structure", new ModulePrintStructure());

#pragma endregion

#pragma region ModulePrintTypes


class ModulePrintTypes : public IModule
{
public:
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
	{
		fprintf(stderr, "********************* PRINT TYPES ***********************\n");

		auto allChildren = rootNode->GatherChildrenRecursively();
		auto typeChildren = tools::LINQSelect(allChildren, [](ASTNode* it) { return dynamic_cast<ASTType*>(it) != nullptr; });

		for (auto& it : typeChildren)
		{
			ASTNode* itNode = it;
			ASTType* itType = reinterpret_cast<ASTType*>(itNode);
			std::string header; ASTNode* nd = itType; while (nd) { header += " "; nd = nd->GetParent(); }
			printf("(%s) %s %s\n",itType->ToNameString().c_str(), header.c_str(), itType->ToString().c_str());

		}
	}

};

static ModuleRegistration gModulePrintTypes("print_types", new ModulePrintTypes());
#pragma endregion