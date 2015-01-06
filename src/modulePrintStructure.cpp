#include "modules.h"
#include "astProcessor.h"

class ModulePrintStructure : public IModule
{
public:

	void WalkAST(ASTNode* node, int level=0)
	{
		int levelIncrement = 0;

		if (node->GetType() == "ROOT" || 
			node->GetType() == "FILE" ||
			node->GetType() == "CLASS" || 
			node->GetType() == "STRUCT" || 
			node->GetType() == "UNION" || 
			node->GetType() == "TEMPLATE" || 
			node->GetType() == "NAMESPACE" || 
			node->GetType() == "INHERIT" ||
			node->GetType() == "INHERIT_FROM" ||
			node->GetType() == "PUBLIC" ||
			node->GetType() == "PRIVATE" ||
			node->GetType() == "PROTECTED" ||
			node->GetType() == "FRIEND" ||
			node->GetType() == "USING" ||
			node->GetType() == "USING_NAMESPACE" ||
			node->GetType() == "INSTANCES" ||
			node->GetType() == "FRIEND" ||
			node->GetType() == "TYPEDEF" ||
			node->GetType() == "TYPEDEF_HEAD" ||
			node->GetType() == "TYPEDEF_SUB" ||
			node->GetType() == "ANNOTATION_FWD" ||
			node->GetType() == "ANNOTATION_BACK" ||
			node->GetType() == "DCL_HEAD" ||
			node->GetType() == "DCL_SUB")
		{
			for (int i = 0; i < level; i++)
				putc(' ', stdout);
			printf("%s %s\n", node->GetType().c_str(), node->ToString().c_str());
			levelIncrement++;
		}
		

		auto& children = node->Children();
		for (int i = 0; i < children.size(); i++)
		{
			WalkAST(children[i], level + levelIncrement);
		}
	}
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTParser>>& parsers)
	{
		fprintf(stderr, "********************* PRINT STRUCTURE ***********************\n");
		WalkAST(rootNode);
	}

};

static ModuleRegistration gModulePrintStructure("print_structure", new ModulePrintStructure());