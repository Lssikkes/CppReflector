#include "tokenizer.h"
#include "astProcessor.h"
#include "astGenerator.h"
#include "tools.h"
#include <omp.h>
#include <iostream>
#include <mutex>

#ifdef VLD_MEM_DEBUGGER
#include <vld.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

int main(int argc, char** argv)
{	
	cmdOptions opts;
	cmdOptions::parse(opts, argc, argv);
	
	std::mutex lkSuperRoot;
	std::vector<std::unique_ptr<ASTParser>> parsers;

	ASTNode superRoot;
	superRoot.SetType("ROOT");
	
	#pragma omp parallel for
	for(int i=0; i<opts.names.size(); i++)
	{
		StringTokenizer stokenizer(readFromFile(opts.names[i]));
		std::unique_ptr<ASTParser> parser(new ASTParser(stokenizer));
		
		// enable verbosity
		if(opts.options.find("verbose") != opts.options.end())
			parser->Verbose = true;
		
		std::unique_ptr<ASTNode> root(new ASTNode);
		root->SetType("FILE");
		root->AddData(opts.names[i]);

		ASTParser::ASTPosition position(*parser.get());
		try
		{
			if (parser->Parse(root.get(), position))
			{
				// store parser - we need the tokens later
				parsers.push_back(std::move(parser));
				
				lkSuperRoot.lock();
				superRoot.AddNode(root.release());
				lkSuperRoot.unlock();
			}
			else
				printf("Error during parse.\n");
			
		}
		catch (std::exception e)
		{
			printf("Fatal error during parse (line %d): %s\n", position.GetToken().TokenLine, e.what());
		}
	}

	printf("Parse successful.\n");
	ASTProcessor::Print(&superRoot);
}
