/*
	Reconstructing code from AST trees
*/

#pragma once

#include "ast.h"
#include "modules.h"

class ASTConstructor : public IModule
{
public:
	ASTConstructor();

	void	WalkAST(ASTNode* node);

	void Recurse(ASTNode* node);

	void	Write(const std::string& inStr);

	// TODO: Remove module stuff
	void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers);
private:

};
