#include "astProcessor.h"
#include <stdlib.h>
#include <string.h>

void ASTProcessor::Print(FILE* dev, ASTNode* node, int level)
{
	std::string allData;
	allData += node->ToString();

	char padding[32];
	memset(padding, ' ', 32);
	padding[level*2] = 0;

	fprintf(dev, "%s * %s (%s)\n", padding, node->GetTypeString(), allData.c_str());

	for(auto it: node->Children())
		Print(dev, it, level+1);
}
