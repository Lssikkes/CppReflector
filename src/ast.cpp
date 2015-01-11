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

const char* ASTNode::GetTypeString() const
{
	switch (type)
	{
		case ASTNode::Type::Root: return "ROOT";
		case ASTNode::Type::File: return "FILE";
		case ASTNode::Type::Private: return "PRIVATE";
		case ASTNode::Type::Public: return "PUBLIC";
		case ASTNode::Type::Protected: return "PROTECTED";
		case ASTNode::Type::Class: return "CLASS";
		case ASTNode::Type::Struct: return "STRUCT";
		case ASTNode::Type::Union: return "UNION";
		case ASTNode::Type::UnionFwdDcl: return "UNION_FORWARD_DECLARATION";
		case ASTNode::Type::StructFwdDcl: return "STRUCT_FORWARD_DECLARATION";
		case ASTNode::Type::ClassFwdDcl: return "CLASS_FORWARD_DECLARATION";
		case ASTNode::Type::Instances: return "INSTANCES";
		case ASTNode::Type::DclSub: return "DCL_SUB";
		case ASTNode::Type::Template: return "TEMPLATE";
		case ASTNode::Type::TemplateArgs: return "TEMPLATE_ARGS";
		case ASTNode::Type::TemplateArg: return "TEMPLATE_ARG";
		case ASTNode::Type::TemplateContent: return "TEMPLATE_CONTENT";
		case ASTNode::Type::Namespace: return "NAMESPACE";
		case ASTNode::Type::NamespaceUsing: return "USING_NAMESPACE";
		case ASTNode::Type::Using: return "USING";
		case ASTNode::Type::Friend: return "FRIEND";
		case ASTNode::Type::Typedef: return "TYPEDEF";
		case ASTNode::Type::TypedefHead: return "TYPEDEF_HEAD";
		case ASTNode::Type::TypedefSub: return "TYPEDEF_SUB";
		case ASTNode::Type::Enum: return "ENUM";
		case ASTNode::Type::EnumClass: return "ENUM_CLASS";
		case ASTNode::Type::EnumDef: return "ENUM_DEFINITION";
		case ASTNode::Type::Init: return "INIT";
		case ASTNode::Type::Parent: return "INHERIT_FROM";
		case ASTNode::Type::Inherit: return "INHERIT";
		case ASTNode::Type::CInitFuncDcl: return "DCL_FUNC_CINIT";
		case ASTNode::Type::CInitVar: return "CINIT_VAR";
		case ASTNode::Type::CInitSet: return "CINIT_SET";
		case ASTNode::Type::TemplateArgDcl: return "DCL_TEMPLATE_ARGS";
		case ASTNode::Type::FuncPtrArgDcl: return "DCL_FPTR_ARGS";
		case ASTNode::Type::FuncArgDcl: return "DCL_FUNC_ARGS";
		case ASTNode::Type::FuncModDcl: return "DCL_FUNC_MODS";
		case ASTNode::Type::Modifier: return "MODIFIER";
		case ASTNode::Type::FuncDcl: return "DCL_FUNC_DECLARATION";
		case ASTNode::Type::DclHead: return "DCL_HEAD";
		case ASTNode::Type::VarArgDcl: return "DCL_VARARGS";
		case ASTNode::Type::ArgDcl: return "DCL_ARG";
		case ASTNode::Type::ArgNonTypeDcl: return "DCL_ARG_NONTYPE";
		case ASTNode::Type::CtorArgsDcl: return "DCL_CTOR_ARGS";
		case ASTNode::Type::CtorArgDcl: return "DCL_CTOR_ARG";
		case ASTNode::Type::AntFwd: return "ANNOTATION_FWD";
		case ASTNode::Type::AntBack: return "ANNOTATION_BACK";
		case ASTNode::Type::AntArgs: return "ANT_ARGS";
		case ASTNode::Type::AntArg: return "ANT_ARG";
		default:
		{
			return "UNKNOWN";
		}
	}
}

std::vector<ASTNode*> ASTNode::GatherAnnotations() const
{
	std::vector<ASTNode*> ret;
	i_InnerGatherAnnotations(ret);
	return ret;
}

void ASTNode::i_InnerGatherAnnotations(std::vector<ASTNode*>& list) const
{
	auto* prev = GetPreviousSibling();
	while (prev && prev->GetType() == Type::AntFwd)
	{
		list.insert(list.begin(), prev);
		prev = prev->GetPreviousSibling();
	}

	auto* next = GetNextSibling();
	while (next && next->GetType() == Type::AntBack)
	{
		list.push_back(next);
		next = next->GetNextSibling();
	}

	auto* parent = GetParent();
	if (parent)
	{
		switch (parent->GetType())
		{
			case ASTNode::Type::Typedef:
			case ASTNode::Type::TypedefHead:
			case ASTNode::Type::Template:
			case ASTNode::Type::TemplateContent:
			case ASTNode::Type::DclHead:
				parent->i_InnerGatherAnnotations(list);
				break;
			default:
				break;
		}
	}
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

	/*if (head != 0)
	{
		GetHeadSub();

		return t.ToString();
	}*/

	// type modifiers
	ret += ToModifiersString();

	// namespace symbol
	/*if (typeName.size() > 0 &&
		tokenSource->Tokens[typeName[0]].TokenType != Token::Type::BuiltinType &&
		tokenSource->Tokens[typeName[0]].TokenType != Token::Type::Void)
	{
		ret += "::";
	}*/

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
	tools::appendSpaceIfNeeded(ret);

	// function modifiers
	ret += ToFunctionModifiersString();

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

std::string ASTType::ToFunctionModifiersString()
{
	if (ndFuncModifierList == 0)
		return "";

	std::string ret;
	for (auto it : ndFuncModifierList->Children())
	{
		ret += it->ToString();
		ret += " ";
	}
	if (ret.size() > 0)
		ret.pop_back(); // remove trailing space
	return ret;
}

std::string ASTType::ToNameString()
{
	std::string ret;
	for (size_t i = 0; i < typeName.size(); i++)
	{
		CxxToken* nextToken = 0;
		if (i + 1 < typeName.size())
			nextToken = &tokenSource->Tokens[typeName[i + 1]];

		ret += tokenSource->Tokens[typeName[i]].TokenData;
		if (tokenSource->Tokens[typeName[i]].TokenType != CxxToken::Type::Doublecolon)
		{
			if (nextToken && nextToken->TokenType == CxxToken::Type::Keyword)
				ret.push_back(' ');
			else if (nextToken && nextToken->TokenType == CxxToken::Type::BuiltinType)
				ret.push_back(' ');
			else if (nextToken && nextToken->TokenType == CxxToken::Type::Void)
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
				if (it->GetType() == ASTNode::Type::VarArgDcl)
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
	if (ret.size() > 0 && ret.back() == ' ')
		ret.pop_back();
	return ret;
}
