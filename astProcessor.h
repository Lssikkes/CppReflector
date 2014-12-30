#pragma once

#include "ast.h"

class ASTProcessor
{
public:
	static void Print(ASTNode* node, int level=0);
};
