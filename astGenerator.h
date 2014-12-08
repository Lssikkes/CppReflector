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

	void DestroyChildren() { for(auto it : children) { it->DestroyChildren(); delete it; } children.clear(); }
	void DestroyChildrenAndSelf() { DestroyChildren(); delete this; }
	void ClearChildrenWithoutDestruction() { children.clear(); }


	std::vector<ASTNode*> children;
	virtual const std::string& GetType() const { return type; }
	virtual const std::vector<std::string>& GetData() { return data; }
protected:
	friend class ASTParser;
	std::string type;
	std::vector<std::string> data;
	
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
	ASTType(ASTTokenSource* src) : tokenSource(src) {}
	ASTTokenSource* tokenSource;
	std::vector<ASTTokenIndex> typeNamespaces;
	std::vector<ASTTokenIndex> typeName;
	std::vector<ASTTokenIndex> typeModifiers;
	std::vector<ASTPointerType> typePointers;
	std::vector<std::vector<ASTTokenIndex>> typeArrayTokens;
	int typeTemplateSubtypeCount;

	virtual const std::vector<std::string>& GetData();
	std::string ToString();
	virtual const std::string& GetType() const;
};


class ASTParser: public ASTTokenSource
{

public:
	class ASTPosition
	{
	public:
		ASTPosition(ASTParser& parser);
		ASTPosition& operator = (const ASTPosition& o) { this->Position = o.Position; return *this; }

		static bool FilterWhitespaceComments(Token& token) { if (token.TokenType == Token::Type::Newline || token.TokenType == Token::Type::Whitespace || token.TokenType == Token::Type::CommentMultiLine || token.TokenType == Token::Type::CommentSingleLine) return false; return true; }
		static bool FilterComments(Token& token) { if (token.TokenType == Token::Type::CommentMultiLine || token.TokenType == Token::Type::CommentSingleLine) return false; return true; }
		static bool FilterNone(Token& token) { return true; }

		void Increment(int count=1, bool (*filterAllows)(Token& token)=&FilterWhitespaceComments);
		
		Token& GetToken();
		Token& GetNextToken();
		ASTTokenIndex GetTokenIndex() { return Position; }

		ASTParser& Parser;
		ASTTokenIndex Position;
		operator unsigned int() { return Position; }
	};
	ASTParser() {}
	ASTParser(Tokenizer& fromTokenizer);

	
	bool Parse(ASTNode* parent, ASTPosition& position);
protected:
	bool ParseRoot(ASTNode* parent, ASTPosition& position);
	
	bool ParseEnum(ASTNode* parent, ASTPosition& position);
	bool ParseEnumDefinition(ASTNode* parent, ASTPosition& position);
	bool ParseClass(ASTNode* parent, ASTPosition& position);
	bool ParseClassInheritance(int &inheritancePublicPrivateProtected, ASTNode* parent, ASTPosition& position);
	bool ParsePrivatePublicProtected(int& privatePublicProtected, ASTNode* parent, ASTPosition& cposition );
	bool ParsePreprocessor(ASTNode* parent, ASTPosition& position);
	bool ParseFunctionArguments(ASTNode* varNode, int privatePublicProtected, ASTPosition &position);
	bool ParseFunctionRemainder(ASTNode* varNode, ASTPosition &cposition, int privatePublicProtected);
	bool ParseFunctionFinalizer(ASTNode* varNode, ASTPosition& position, int privatePublicProtected);
	bool ParseFunctionOperatorType(ASTNode* parent, ASTPosition& cposition, std::vector<ASTTokenIndex>& ctokens);
	bool ParseConstructorInitializer(ASTNode* parent, ASTPosition& cposition);
	bool ParseDeclaration(int privatePublicProtected, ASTNode* parent, ASTPosition& cposition );
	bool ParseModifierToken(ASTPosition& position, std::vector<Token>& modifierTokens);
	bool ParseModifierToken(ASTPosition& position, std::vector<ASTTokenIndex>& modifierTokens);

	bool ParseSubType(int privatePublicProtected, std::unique_ptr<ASTNode>& varNode, ASTPosition &position, std::unique_ptr<ASTType> &varType, int mode);

	bool ParseClassConstructorDestructor(ASTNode* parent, ASTPosition &position, int privatePublicProtected);
	bool ParsePointerReferenceSymbol(ASTPosition &position, std::vector<Token> &pointerTokens, std::vector<Token> &pointerModifierTokens );
	bool ParseSpecificScopeInner(ASTPosition& cposition, std::vector<ASTTokenIndex> &insideBracketTokens, Token::Type tokenTypeL, Token::Type tokenTypeR, bool(*filterAllows)(Token& token) /*= &ASTPosition::FilterWhitespaceComments*/);
	bool ParseNTypeBase(ASTPosition &position, ASTType* typeNode);
	bool ParseNTypeSinglePointersAndReferences(ASTPosition &position, ASTType* typeNode);
	bool ParseNTypePointersAndReferences(ASTPosition &position, ASTType* typeNode);
	bool ParseNTypeArrayDefinitions(ASTPosition &position, ASTType* typeNode);

	bool ParseUnknown(ASTNode* parent, ASTPosition& position);
	bool ParseIgnored(ASTNode* parent, ASTPosition& position);
	bool ParseEndOfStream(ASTNode* parent, ASTPosition& position);


};
