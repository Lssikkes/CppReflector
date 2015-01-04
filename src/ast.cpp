#include "ast.h"

std::vector<ASTNode*> ASTNode::GatherAllChildren() const
{
	std::vector<ASTNode*> ret;

	for (auto& it : m_children)
	{
		auto subChildren = it->GatherAllChildren();
		ret.push_back(it);
		ret.insert(ret.end(), subChildren.begin(), subChildren.end());
	}
	return ret;
}

const std::string& ASTType::GetType() const
{
	return type;
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
	for (auto& it : typeModifiers)
	{
		for (int i = it.first; i <= it.second; i++)
			ret += tokenSource->Tokens[i].TokenData + "";
		ret += " ";
	}

	if (typeName.size() > 0 && tokenSource->Tokens[typeName[0]].TokenType != Token::Type::BuiltinType && tokenSource->Tokens[typeName[0]].TokenType != Token::Type::Void) ret += "::";

	// name symbols
	for (auto& it : typeName)
		ret += tokenSource->Tokens[it].TokenData + "";
	if (typeName.empty() == false) ret += " ";

	// template arguments
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

	// pointer symbols (including pointer modifiers)
	for (auto& it : typePointers)
		ret += it.ToString() + " ";

	// array tokens
	for (auto& it : typeArrayTokens)
	{
		ret += "[";
		for (auto& it2 : it)
			ret += tokenSource->Tokens[it2].TokenData;
		ret += "]";
	}

	if (typeArrayTokens.size() > 0) ret += " ";

	// function pointer prefix
	if (!typeIdentifierScopedPointers.empty())
		ret += "(";

	for (auto& it : typeIdentifierScopedPointers)
		ret += it.ToString() + " ";

	// identifier
	for (auto& it : typeIdentifier)
		ret += tokenSource->Tokens[it].TokenData;

	// function pointer suffix
	if (!typeIdentifierScopedPointers.empty())
		ret += ")";
	else if (typeIdentifier.size() > 0) ret += " ";

	// operator
	for (auto& it : typeOperatorTokens)
		ret += tokenSource->Tokens[it].TokenData;

	if (typeOperatorTokens.size() > 0) ret += " ";

	// function arguments
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
				if (it->GetType() == "DCL_VARARGS")
					ret += "...";
				continue; // skip non-types
			}

			ret += t->ToString();
			if (it != children.back())
				ret += ", ";
		}
		ret += ")";
	}

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


const std::vector<std::string>& ASTType::GetData()
{
	if (data.empty())
	{
		data.clear();
		data.push_back(ToString());
	}
	return data;
}


const std::vector<std::string>& ASTTokenNode::GetData()
{
	if (data.empty())
	{
		data.clear();
		for (auto it : Tokens)
			data.push_back(tokenSource->Tokens[it].TokenData);
	}
	return data;
}


std::string ASTTokenNode::ToString()
{
	std::string ret;
	for (auto it : Tokens)
		ret.append(tokenSource->Tokens[it].TokenData);
	return ret;
}

