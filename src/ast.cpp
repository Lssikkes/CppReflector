#include "ast.h"
#include "tools.h"

std::vector<ASTNode*> ASTNode::GatherChildrenRecursively() const
{
	std::vector<ASTNode*> ret;

	for (auto& it : m_children)
	{
		auto subChildren = it->GatherChildrenRecursively();
		ret.push_back(it);
		ret.insert(ret.end(), subChildren.begin(), subChildren.end());
	}
	return ret;
}

ASTNode* ASTNode::GetPreviousSibling() const
{
	if (parentIndex == -1)
		return 0;
	if (parentIndex == 0) 
		return 0;
	else 
		return parent->Children()[parentIndex - 1];
}

ASTNode* ASTNode::GetNextSibling() const
{
	if (parentIndex == -1)
		return 0;
	if (parentIndex + 1 >= parent->Children().size()) 
		return 0; 
	else 
		return parent->Children()[parentIndex + 1];
}

void ASTNode::DestroyChildren()
{
	for (auto it : m_children) 
	{ 
		it->DestroyChildren(); 
		delete it; 
	} 
	m_children.clear();
}

void ASTNode::StealNodesFrom(ASTNode* node)
{
	AddNodes(node->Children());
	node->m_children.clear();
}

std::string ASTPointerType::ToString()
{
	std::string ret;
	if (pointerType == Type::Pointer)
		ret = "*";
	else
		ret = "&";
	for (auto& it : pointerModifiers)
		ret += " " + it.TokenData;
	return ret;
}

std::string ASTType::ToString()
{
	std::string ret;

	if (head != 0)
	{
		ASTType t(tokenSource);
		t.MergeData(this);
		t.MergeData(head);
		return t.ToString();
	}

	// type modifiers
	ret += ToModifiersString();

	// namespace symbol
	if (typeName.size() > 0 &&
		tokenSource->Tokens[typeName[0]].TokenType != Token::Type::BuiltinType &&
		tokenSource->Tokens[typeName[0]].TokenType != Token::Type::Void)
	{
		ret += "::";
	}

	// name symbols
	ret += ToNameString();
	tools::appendSpaceIfNeeded(ret);

	// template arguments
	ret += ToTemplateArgumentsString();
	tools::appendSpaceIfNeeded(ret);

	// pointer symbols (including pointer modifiers)
	ret += ToPointersString();
	tools::appendSpaceIfNeeded(ret);



	// function pointer prefix
	if (!typeIdentifierScopedPointers.empty())
		ret += "(";

	// pointer identifier tokens
	ret += ToPointerIdentifierScopedString();
	tools::appendSpaceIfNeeded(ret);

	// identifier
	ret += ToIdentifierString();

	// function pointer suffix
	if (!typeIdentifierScopedPointers.empty())
		ret += ")";
	tools::appendSpaceIfNeeded(ret);

	// array tokens
	ret += ToArrayTokensString();
	tools::appendSpaceIfNeeded(ret);

	// bitfield
	if (typeBitfieldTokens.size() > 0) ret += ": ";
	ret += ToBitfieldString();

	// operator
	ret += ToOperatorString();
	tools::appendSpaceIfNeeded(ret);

	// function arguments
	ret += ToArgumentsString();

	// remove trailing space
	if (ret.size() > 0 && ret.back() == ' ')
		ret.pop_back();


	return ret;

}


void ASTType::MergeData(ASTType* other)
{
	typeName.insert(typeName.end(), other->typeName.begin(), other->typeName.end());
	typeIdentifier.insert(typeIdentifier.end(), other->typeIdentifier.begin(), other->typeIdentifier.end());
	typeModifiers.insert(typeModifiers.end(), other->typeModifiers.begin(), other->typeModifiers.end());
	typeOperatorTokens.insert(typeOperatorTokens.end(), other->typeOperatorTokens.begin(), other->typeOperatorTokens.end());
	typePointers.insert(typePointers.end(), other->typePointers.begin(), other->typePointers.end());
	typeArrayTokens.insert(typeArrayTokens.end(), other->typeArrayTokens.begin(), other->typeArrayTokens.end());
	typeTemplateIndices.insert(typeTemplateIndices.end(), other->typeTemplateIndices.begin(), other->typeTemplateIndices.end());
	typeBitfieldTokens.insert(typeBitfieldTokens.end(), other->typeBitfieldTokens.begin(), other->typeBitfieldTokens.end());
	typeIdentifierScopedPointers.insert(typeIdentifierScopedPointers.end(), other->typeIdentifierScopedPointers.begin(), other->typeIdentifierScopedPointers.end());
	typeFunctionPointerArgumentIndices.insert(typeFunctionPointerArgumentIndices.end(), other->typeFunctionPointerArgumentIndices.begin(), other->typeFunctionPointerArgumentIndices.end());

	if (other->ndTemplateArgumentList)
		ndTemplateArgumentList = other->ndTemplateArgumentList;
	if (other->ndFuncArgumentList)
		ndFuncArgumentList = other->ndFuncArgumentList;
	if (other->ndFuncModifierList)
		ndFuncModifierList = other->ndFuncModifierList;
	if (other->ndFuncPointerArgumentList)
		ndFuncPointerArgumentList = other->ndFuncPointerArgumentList;
}


std::string ASTType::ToModifiersString()
{
	std::string ret;
	for (auto& it : typeModifiers)
	{
		for (size_t i = it.first; i <= it.second; i++)
			ret += tokenSource->Tokens[i].TokenData + "";
		ret += " ";
	}
	return ret;
}

std::string ASTType::ToNameString()
{
	std::string ret;
	for (size_t i = 0; i < typeName.size(); i++)
	{
		Token* nextToken = 0;
		if (i + 1 < typeName.size())
			nextToken = &tokenSource->Tokens[typeName[i + 1]];

		ret += tokenSource->Tokens[typeName[i]].TokenData;
		if (tokenSource->Tokens[typeName[i]].TokenType != Token::Type::Doublecolon)
		{
			if (nextToken && nextToken->TokenType == Token::Type::Keyword)
				ret.push_back(' ');
			else if (nextToken && nextToken->TokenType == Token::Type::BuiltinType)
				ret.push_back(' ');
			else if (nextToken && nextToken->TokenType == Token::Type::Void)
				ret.push_back(' ');

		}
	}
	return ret;
}

std::string ASTType::ToTemplateArgumentsString()
{
	std::string ret;
	if (ndTemplateArgumentList)
	{
		if (ret.size() > 0 && ret.back() == ' ')
			ret.pop_back(); // remove trailing space

		auto& children = ndTemplateArgumentList->Children();
		ret += "<";
		for (auto& it : children)
		{
			ret += it->ToString();
			if (it != children.back())
				ret += ", ";
		}
		ret += "> ";
	}
	return ret;
}

std::string ASTType::ToArrayTokensString()
{
	std::string ret;
	for (auto& it : typeArrayTokens)
	{
		ret.push_back('[');
		for (auto& it2 : it)
			ret.append(tokenSource->Tokens[it2].TokenData);
		ret.push_back(']');
	}
	return ret;
}

std::string ASTType::ToPointerIdentifierScopedString()
{
	std::string ret;
	for (auto& it : typeIdentifierScopedPointers)
		ret += it.ToString() + " ";
	if (ret.size() > 0)
		ret.pop_back(); // remove trailing space
	return ret;
}

std::string ASTType::ToIdentifierString()
{
	std::string ret;
	for (auto& it : typeIdentifier)
		ret += tokenSource->Tokens[it].TokenData;
	return ret;
}

std::string ASTType::ToBitfieldString()
{
	std::string ret;
	for (auto& it : typeBitfieldTokens)
		ret += tokenSource->Tokens[it].TokenData + " ";
	if (ret.size() > 0)
		ret.pop_back(); // remove trailing space
	return ret;
}

std::string ASTType::ToOperatorString()
{
	std::string ret;
	for (auto& it : typeOperatorTokens)
		ret += tokenSource->Tokens[it].TokenData;
	return ret;
}

std::string ASTType::ToArgumentsString()
{
	std::string ret;
	ASTNode* args = 0;
	if (args == 0 && ndFuncArgumentList)
		args = ndFuncArgumentList;
	if (args == 0 && ndFuncPointerArgumentList)
		args = ndFuncPointerArgumentList;
	if (args)
	{
		auto& children = args->Children();
		ret += "(";
		for (auto& it : children)
		{
			ASTType* t = dynamic_cast<ASTType*>(it);
			if (t == 0)
			{
				if (it->GetTypeStr() == "DCL_VARARGS")
					ret += "...";
				continue; // skip non-types
			}

			ret += t->ToString();
			if (it != children.back())
				ret += ", ";
		}
		ret += ")";
	}
	return ret;
}

std::string ASTType::ToPointersString()
{
	std::string ret;
	for (auto& it : typePointers)
		ret += it.ToString() + " ";

	// remove trailing space
	if (ret.size() > 0)
		ret.pop_back();
	return ret;
}



std::string ASTTokenNode::ToString()
{
	std::string ret;
	for (auto it : Tokens)
		ret.append(tokenSource->Tokens[it].TokenData);
	return ret;
}


std::string ASTDataNode::ToString()
{
	std::string ret;
	for (auto it : data)
	{
		ret += it;
		ret += " ";
	}
	return ret;
}
