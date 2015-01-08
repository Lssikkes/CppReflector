#include "modules.h"
#include "astProcessor.h"

#include "tools.h"
class ModulePrintTypes : public IModule
{
public:
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTParser>>& parsers)
	{
		fprintf(stderr, "********************* PRINT TYPES ***********************\n");

		auto allChildren = rootNode->GatherChildrenRecursively();
		auto typeChildren = tools::LINQSelect(allChildren, [](ASTNode* it) { return dynamic_cast<ASTType*>(it) != nullptr; });
		
		for (auto& it : typeChildren)
		{
			ASTNode* itNode = it;
			ASTType* itType = reinterpret_cast<ASTType*>(itNode);
			std::string header; ASTNode* nd = itType; while (nd) { header += " "; nd = nd->GetParent(); }
			printf("%s %s\n", header.c_str(), itType->ToString().c_str());

		}
	}

};

static ModuleRegistration gModulePrintAST("print_types", new ModulePrintTypes());