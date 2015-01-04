#include "modules.h"
#include "tools.h"
#include "tokenizer.h"
#include "astProcessor.h"
#include "astParser.h"
#include <mutex>

#include <omp.h>

class ModuleCppParser : public IModule
{
public:
	ModuleCppParser(bool a_MT) : Multithreaded(a_MT) {}
	bool Multithreaded;
	virtual void Execute(tools::CommandLineParser& opts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTParser>>& parsers)
	{
		fprintf(stderr, "********************* CPP PARSER (%s) ***********************\n", Multithreaded ? "MT" : "ST");

		std::mutex lkSuperRoot;

		// parse files
		if (Multithreaded)
		{
			#pragma omp parallel for
			for (int i = 0; i < opts.names.size(); i++)
			{
				ParseFile(opts, i, parsers, lkSuperRoot, rootNode);
			}
		}
		else
		{
			for (size_t i = 0; i < opts.names.size(); i++)
			{
				ParseFile(opts, i, parsers, lkSuperRoot, rootNode);
			}
		}
	}

	void ParseFile(tools::CommandLineParser &opts, size_t i, std::vector<std::unique_ptr<ASTParser>> &parsers, std::mutex &lkSuperRoot, ASTNode* rootNode)
	{
		fprintf(stderr, "[PARSER] Parsing file \"%s\"\n", opts.names[i].c_str());
		StringTokenizer stokenizer(tools::readFromFile(opts.names[i]));
		std::unique_ptr<ASTParser> parser(new ASTParser(stokenizer));

		// enable verbosity
		if (opts.options.find("verbose") != opts.options.end())
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
				lkSuperRoot.lock();
				parsers.push_back(std::move(parser));
				rootNode->AddNode(root.release());
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

};

static ModuleRegistration gModuleParser("cpp_parser", new ModuleCppParser(false));
static ModuleRegistration gModuleParserMT("cpp_parser_mt", new ModuleCppParser(true) );

