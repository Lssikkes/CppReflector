#include "../modules.h"
#include "../tools.h"
#include "../cxxTokenizer.h"
#include "../cxxAstParser.h"
#include "../astProcessor.h" 
#include <mutex>

#include <omp.h>

class ModuleCppParser : public IModule
{
public:
	ModuleCppParser(bool a_MT) : Multithreaded(a_MT) {}
	bool Multithreaded;
	virtual void Execute(tools::CommandLineParser& opts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
	{
		fprintf(stderr, "********************* CPP PARSER (%s) ***********************\n", Multithreaded ? "MT" : "ST");

		std::mutex lkSuperRoot;

		// parse files
		if (Multithreaded)
		{
			#pragma omp parallel for
			for (size_t i = 0; i < opts.names.size(); i++)
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

	void ParseFile(tools::CommandLineParser &opts, size_t i, std::vector<std::unique_ptr<ASTCxxParser>> &parsers, std::mutex &lkSuperRoot, ASTNode* rootNode)
	{
		fprintf(stderr, "[PARSER] Parsing file \"%s\"\n", opts.names[i].c_str());
		CxxStringTokenizer stokenizer(opts.names[i], tools::readFromFile(opts.names[i]));
		std::unique_ptr<ASTCxxParser> parser(new ASTCxxParser(stokenizer));

		// enable verbosity
		if (opts.options.find("verbose") != opts.options.end())
			parser->Verbose = true;

		std::unique_ptr<ASTDataNode> root(new ASTDataNode);
		root->SetType(ASTNode::Type::File);
		root->AddData(opts.names[i]);

		ASTCxxParser::ASTPosition position(*parser.get());
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
				fprintf(stderr, "Error: Parsing failed.\n");

		}
		catch (std::exception e)
		{
			fprintf(stderr, "Error: Fatal error during parse (line %d): %s\n", position.GetToken().TokenLine, e.what());
		}
	}

};

static ModuleRegistration gModuleParser("cpp_parser", new ModuleCppParser(false));
static ModuleRegistration gModuleParserMT("cpp_parser_mt", new ModuleCppParser(true) );

