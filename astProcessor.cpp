#include "astProcessor.h"
#include <stdlib.h>
#include <string.h>

void ASTProcessor::Print(ASTNode* node, int level)
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
