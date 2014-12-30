#pragma once

#include <vector>
#include "tokenizer.h"
#include <memory>

class ASTTokenSource
{
public:
	std::vector<Token> Tokens;
};

typedef unsigned int ASTTokenIndex;
class ASTNode
{
public:
	ASTNode() { }
	virtual ~ASTNode() { DestroyChildren(); }

	void DestroyChildren() { for (auto it : m_children) { it->DestroyChildren(); delete it; } m_children.clear(); }
	void DestroyChildrenAndSelf() { DestroyChildren(); delete this; }
	void ClearChildrenWithoutDestruction() { m_children.clear(); }

	void AddNode(ASTNode* node) { node->parent = this; m_children.push_back(node); }
	void AddNodes(const std::vector<ASTNode*>& nodes) { for (auto it : nodes) it->parent = this; m_children.insert(m_children.end(), nodes.begin(), nodes.end()); }
	void StealNodesFrom(ASTNode* node) { AddNodes(node->Children()); node->m_children.clear(); }
	const std::vector<ASTNode*>& Children() const { return m_children; }
	virtual const std::string& GetType() const { return type; }
	virtual const std::vector<std::string>& GetData() { return data; }
	ASTNode* GetParent() const { return parent; }
	
	void SetType(const std::string& a_type) { type = a_type; }
	void AddData(const std::string& a_dataValue) { data.push_back(a_dataValue); }
protected:
	friend class ASTParser;
	std::string type;
	std::vector<std::string> data;
	ASTNode* parent;
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

class ASTType : public ASTNode
{
public:
	ASTType(ASTTokenSource* src) : tokenSource(src) { type = "TYPE"; }
	ASTTokenSource* tokenSource;
	std::vector<ASTTokenIndex> typeNamespaces;
	std::vector<ASTTokenIndex> typeName;
	std::vector<ASTTokenIndex> typeIdentifier;
	std::vector<ASTTokenIndex> typeModifiers;
	std::vector<ASTTokenIndex> typeOperatorTokens;
	std::vector<ASTPointerType> typePointers;
	std::vector<std::vector<ASTTokenIndex>> typeArrayTokens;
	std::vector<int> typeTemplateIndices;
	std::vector<ASTPointerType> typeFunctionPointerPointers;
	std::vector<int> typeFunctionPointerArgumentIndices;

	virtual const std::vector<std::string>& GetData();
	std::string ToString();
	virtual const std::string& GetType() const;
};