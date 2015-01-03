#pragma once

#include "ast.h"

class ASTProcessor
{
public:
	static void Print(FILE* dev, ASTNode* node, int level=0);
};
