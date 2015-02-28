#pragma once

// TODO: Template specialization & Partial template specialization support

#include <vector>
#include <memory>
#include "cxxTokenizer.h"

class ASTTokenSource
{
public:
	std::vector<CxxToken> Tokens;
	virtual const char* SourceIdentifier() { return "UNKNOWN"; }
	size_t AddToken(const CxxToken& token);
};

typedef size_t ASTTokenIndex;

class ASTNode
{
public:
	enum class Type
	{
		Root,
		File,
		Instances,	// whats this

		Public,
		Private,
		Protected,
		Class,
		Struct,
		Union,
		ClassFwdDcl,
		StructFwdDcl,
		UnionFwdDcl,
		Template,
		TemplateArgs,
		TemplateArg,
		TemplateArgDcl,
		TemplateContent,
		Namespace,
		NamespaceUsing,
		Using,
		Friend,
		VarType,	// Type
		Typedef,
		TypedefHead,
		TypedefSub,
		Enum,
		EnumClass,
		EnumDef,
		Init,
		Parent,
		Inherit,
		CInitFuncDcl,
		CInitVar,
		CInitSet,
		FuncPtrArgDcl,
		FuncDcl,
		FuncArgDcl,
		FuncModDcl,
		Modifier,
		CtorArgsDcl,
		CtorArgDcl,
		DclHead,
		DclSub,
		ArgDcl,
		ArgNonTypeDcl,
		VarArgDcl,
		AntFwd,
		AntBack,
		AntArgs,
		AntArg,

		TypeCount
	};

	ASTNode() { parent = 0; }
	virtual ~ASTNode() { DestroyChildren(); }

	void DestroyChildren();
	void DestroyChildrenAndSelf() { DestroyChildren(); delete this; }
	void ClearChildrenWithoutDestruction() { m_children.clear(); }

	void AddNode(ASTNode* node) { node->parent = this; node->parentIndex = m_children.size(); m_children.push_back(node); }
	void AddNodes(const std::vector<ASTNode*>& nodes) { for (auto it : nodes) AddNode(it); }
	void StealNodesFrom(ASTNode* node);
	const std::vector<ASTNode*>& Children() const { return m_children; }
	std::vector<ASTNode*> GatherChildrenRecursively() const;
	std::vector<ASTNode*> GatherParents() const;
	std::vector<ASTNode*> GatherAnnotations() const;

	virtual const char* GetTypeString() const;
	virtual const ASTNode::Type GetType() const { return type; };

	ASTNode* GetParent() const { return parent; }
	
	ASTNode* GetNextSibling() const;
	ASTNode* GetPreviousSibling() const;
	
	void SetType( ASTNode::Type inType) { type = inType; }

	void RebuildParentIndices();

	virtual std::string ToString() { return ""; }
protected:
	void i_InnerGatherAnnotations(std::vector<ASTNode*>& list) const;
	ASTNode::Type type;
	
	ASTNode* parent;
	size_t parentIndex = -1;
	std::vector<ASTNode*> m_children;
private:
	
};



class ASTPointerType
{
public:
	enum class Type
	{
		Pointer,
		Reference
	};

	Type pointerType;
	CxxToken pointerToken;
	std::vector<CxxToken> pointerModifiers;

	std::string ToString();
};

class ASTDataNode : public ASTNode
{
public:
	void AddData(const std::string& a_dataValue) { data.push_back(a_dataValue); }
	const std::vector<std::string>& Data() const { return data; }
	virtual std::string ToString();
protected:
	friend class ASTCxxParser;
	std::vector<std::string> data;
};

class ASTTokenNode : public ASTNode
{
public:
	ASTTokenNode(ASTTokenSource* src) : tokenSource(src) { }
	ASTTokenSource* tokenSource;
	std::vector<ASTTokenIndex> Tokens;
	virtual std::string ToString();
};

class ASTType : public ASTNode
{
public:
	struct ASTTokenIndexTemplated { ASTTokenIndex Index; ASTNode* TemplateArguments; };
	ASTType(ASTTokenSource* src) : tokenSource(src) { SetType(ASTNode::Type::VarType); }
	ASTTokenSource* tokenSource;
	ASTType* head = 0;

	// Parser variables
	ASTNode* ndFuncArgumentList = 0;
	ASTNode* ndFuncModifierList = 0;
	ASTNode* ndFuncPointerArgumentList = 0;
	std::vector<ASTTokenIndexTemplated> typeName;
	std::vector<ASTTokenIndex> typeIdentifier;
	std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> > typeModifiers;
	std::vector<ASTTokenIndex> typeOperatorTokens;
	std::vector<ASTTokenIndex> typeBitfieldTokens;
	std::vector<ASTPointerType> typePointers;
	std::vector<ASTPointerType> typeIdentifierScopedPointers;
	std::vector< std::vector<ASTTokenIndex> > typeArrayTokens;
	std::vector<int> typeTemplateIndices;
	std::vector<int> typeFunctionPointerArgumentIndices;

	// Transfiguration variables
	ASTNode* resolvedType = 0;

	bool HasType();
	bool HasModifier(CxxToken::Type modifierType);
	bool IsBuiltinType();

	virtual std::string ToString() { return ToString(true); }
	std::string ToString(bool withIdentifier);

	bool IsDeclarationHead() { return head != 0; }
	ASTType CombineWithHead();

	std::string ToPointersString();
	std::string ToArgumentsString();
	std::string ToOperatorString();
	std::string ToBitfieldString();
	std::string ToIdentifierString();
	std::string ToTemplateArgumentsString(ASTNode* args);
	std::string ToPointerIdentifierScopedString();
	std::string ToNameString(bool includeTemplateArguments=true);
	std::string ToModifiersString();
	std::string ToFunctionModifiersString();
	std::string ToArrayTokensString();

	void MergeData(ASTType* other);
};
