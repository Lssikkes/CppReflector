#include <vld.h>
#include <iostream>
#include "tokenizer.h"
#include <fstream>
#include "astGenerator.h"
#include <ppl.h>
#include <omp.h>

// TODO: friend keyword
// TODO: friend class keyword

const int v=10;
std::string readFromFile(std::string filename)
{
	std::ifstream ifs(filename);
	std::string content( (std::istreambuf_iterator<char>(ifs) ), ( std::istreambuf_iterator<char>() ) );
	return content;
}

void Print(ASTNode* node, int level=0)
{
	std::string allData;
	for(auto it : node->GetData())
		allData += it + "|" ;
	if(allData.size() > 0)
		allData.pop_back();

	char padding[32];
	memset(padding, ' ', 32);
	padding[level*2] = 0;

	printf("%s * %s (%s)\n", padding, node->GetType().c_str(), allData.c_str());

	for(auto it: node->Children())
		Print(it, level+1);
}



#include <windows.h>
int main(int argc, char** argv)
{	
	int v;
	StringTokenizer stokenizer(readFromFile("test1.xh"));
	ASTParser parser(stokenizer);
	parser.Verbose = true;

	//#pragma omp parallel for
	for (int i = 0; i<10000; i++)
	{
		ASTNode root;

		ASTParser::ASTPosition position(parser);
		try
		{
			if (parser.Parse(&root, position))
			{
				if (i == 0)
					Print(&root);
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
}