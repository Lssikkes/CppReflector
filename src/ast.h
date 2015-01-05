#pragma once

#include <vector>
#include "tokenizer.h"
#include <memory>

class ASTTokenSource
{
public:
	std::vector<Token> Tokens;
};

typedef size_t ASTTokenIndex;

class ASTNode
{
public:
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

	virtual const std::string& GetType() const;

	ASTNode* GetParent() const { return parent; }
	ASTNode* GetNextSibling() const;
	ASTNode* GetPreviousSibling() const;
	
	void SetType(const std::string& a_type) { type = a_type; }

	void RebuildParentIndices();

	virtual std::string ToString() { return ""; }
protected:
	friend class ASTParser;
	std::string type;
	
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
	Token pointerToken;
	std::vector<Token> pointerModifiers;

	std::string ToString();
};

class ASTDataNode : public ASTNode
{
public:
	void AddData(const std::string& a_dataValue) { data.push_back(a_dataValue); }
	const std::vector<std::string>& Data() const { return data; }
	virtual std::string ToString();
protected:
	friend class ASTParser;
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

//@[Serializable]
class ASTType : public ASTNode
{
public:
	ASTType(ASTTokenSource* src) : tokenSource(src) { type = "TYPE"; }
	ASTTokenSource* tokenSource;
	ASTType* head = 0;
	
	ASTNode* ndTemplateArgumentList = 0;
	ASTNode* ndFuncArgumentList = 0;
	ASTNode* ndFuncModifierList = 0;
	ASTNode* ndFuncPointerArgumentList = 0;
	std::vector<ASTTokenIndex> typeName;
	std::vector<ASTTokenIndex> typeIdentifier;
	std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> > typeModifiers;
	std::vector<ASTTokenIndex> typeOperatorTokens;
	std::vector<ASTTokenIndex> typeBitfieldTokens;
	std::vector<ASTPointerType> typePointers;
	std::vector<ASTPointerType> typeIdentifierScopedPointers;
	std::vector<std::vector<ASTTokenIndex>> typeArrayTokens;
	std::vector<int> typeTemplateIndices;
	
	std::vector<int> typeFunctionPointerArgumentIndices;

	virtual std::string ToString();

	std::string ToPointersString();

	std::string ToArgumentsString();
	std::string ToOperatorString();
	std::string ToBitfieldString();
	std::string ToIdentifierString();
	std::string ToTemplateArgumentsString();
	std::string ToPointerIdentifierScopedString();
	std::string ToNameString();
	std::string ToModifiersString();
	std::string ToArrayTokensString();

	virtual const std::string& GetType() const;
	void MergeData(ASTType* other);
};
