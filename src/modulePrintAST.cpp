#include "modules.h"
#include "astProcessor.h"

class ModulePrintAST: public IModule
{
public:
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTParser>>& parsers)
	{
		fprintf(stderr, "********************* PRINT AST ***********************\n");
		ASTProcessor::Print(stdout, rootNode);
	}

};

static ModuleRegistration gModulePrintAST("print_ast", new ModulePrintAST());