#include "tools.h"
#include <iostream>

#ifdef VLD_MEM_DEBUGGER
#include <vld.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#include "modules.h"
#include "ast.h"
#include "cxxAstParser.h"

// TODO: pointer to member objects

int main(int argc, char** argv)
{
	
	// parse command line arguments
	tools::CommandLineParser opts;
	tools::CommandLineParser::parse(opts, argc, argv);

	// create root objects
	std::vector<std::unique_ptr<ASTCxxParser>> parsers;
	ASTNode superRoot;
	superRoot.SetType(ASTNode::Type::Root);

	// check whether help is needed
	if (opts.optionsWithValues["module"].size() == 0)
	{
		fprintf(stderr, "Supported modules:\n");
		auto& modules = ModuleRegistration::Modules();
		for (auto it : modules)
			fprintf(stderr, " * \"%s\"\n", it.first.c_str());
	}

	// execute modules
	auto& modules = ModuleRegistration::Modules();
	for (auto itModule : opts.optionsWithValues["module"])
	{
		auto mod = modules.find(itModule);
		if (mod == modules.end())
		{
			fprintf(stderr, "Could not find module \"%s\"", itModule.c_str());
			continue;
		}

		(*mod).second->Handler()->Execute(opts, &superRoot, parsers);
	}

}
