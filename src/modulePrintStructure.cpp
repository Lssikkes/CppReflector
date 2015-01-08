#include "modules.h"
#include "astProcessor.h"

class ModulePrintStructure : public IModule
{
public:

	void WalkAST(ASTNode* node, int level=0)
	{
		int levelIncrement = 0;

#ifdef CONST_STR_CMP
		if (node->GetTypeStr() == "ROOT" || 
			node->GetTypeStr() == "FILE" ||
			node->GetTypeStr() == "CLASS" || 
			node->GetTypeStr() == "STRUCT" || 
			node->GetTypeStr() == "UNION" || 
			node->GetTypeStr() == "TEMPLATE" || 
			node->GetTypeStr() == "NAMESPACE" || 
			node->GetTypeStr() == "INHERIT" ||
			node->GetTypeStr() == "INHERIT_FROM" ||
			node->GetTypeStr() == "PUBLIC" ||
			node->GetTypeStr() == "PRIVATE" ||
			node->GetTypeStr() == "PROTECTED" ||
			node->GetTypeStr() == "FRIEND" ||		// Sup
			node->GetTypeStr() == "USING" ||
			node->GetTypeStr() == "USING_NAMESPACE" ||
			node->GetTypeStr() == "INSTANCES" ||
			node->GetTypeStr() == "FRIEND" ||		// Hi friend
			node->GetTypeStr() == "TYPEDEF" ||
			node->GetTypeStr() == "TYPEDEF_HEAD" ||
			node->GetTypeStr() == "TYPEDEF_SUB" ||
			node->GetTypeStr() == "ANNOTATION_FWD" ||
			node->GetTypeStr() == "ANNOTATION_BACK" ||
			node->GetTypeStr() == "DCL_HEAD" ||
			node->GetTypeStr() == "DCL_SUB")
		{
#else
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
			node->GetType() == ASTNode::Type::AntFwd ||
			node->GetType() == ASTNode::Type::AntBack ||
			node->GetType() == ASTNode::Type::DclHead ||
			node->GetType() == ASTNode::Type::DclSub)
		{
#endif
			for (int i = 0; i < level; i++)
				putc(' ', stdout);
			printf("%s %s\n", node->GetTypeStr().c_str(), node->ToString().c_str());
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