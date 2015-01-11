#include <stdio.h>
#include "..\modules.h"
#include "..\ast.h"

class ModuleCxxReflector : public IModule
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
			node->GetType() == ASTNode::Type::Friend ||
			node->GetType() == ASTNode::Type::Using ||
			node->GetType() == ASTNode::Type::NamespaceUsing ||
			node->GetType() == ASTNode::Type::Instances ||
			node->GetType() == ASTNode::Type::Typedef ||
			node->GetType() == ASTNode::Type::TypedefHead ||
			node->GetType() == ASTNode::Type::TypedefSub ||
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
		fprintf(stderr, "********************* CPP REFLECTOR ***********************\n");
		WalkAST(rootNode);
	}

};

static ModuleRegistration gModulePrintStructure("cpp_reflector", new ModuleCxxReflector());