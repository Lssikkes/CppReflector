#include "astGenerator.h"
#include <memory>

#define SUBTYPE_MODE_SUBVARIABLE 0
#define SUBTYPE_MODE_SUBARGUMENT 1

template <class T> std::string CombineWhile(ASTParser::ASTPosition& position, const T& checkFunc, bool(*filterAllows)(Token& token)=&ASTParser::ASTPosition::FilterWhitespaceComments)
{
	std::string combiner;
	while(position.GetToken().TokenType != Token::Type::EndOfStream && checkFunc(position))
	{
		combiner += position.GetToken().TokenData;
		position.Increment();
	}
	return combiner;
}

template <class T> std::string CombineWhile_ScopeAware(ASTParser::ASTPosition& position, const T& checkFunc, bool(*filterAllows)(Token& token) = &ASTParser::ASTPosition::FilterWhitespaceComments)
{
	int count[4] = {0,0,0,0};

	std::string combiner;
	while(position.GetToken().TokenType != Token::Type::EndOfStream )
	{
		if(count[0] == 0 && count[1] == 0 && count[2] == 0 && count[3] == 0 && checkFunc(position) == false)
			break;
		if(position.GetToken().TokenType == Token::Type::LBrace)
			count[0]++;
		if(position.GetToken().TokenType == Token::Type::LBracket)
			count[1]++;
		if(position.GetToken().TokenType == Token::Type::LParen)
			count[2]++;
		if(position.GetToken().TokenType == Token::Type::LArrow)
			count[3]++;
		if(position.GetToken().TokenType == Token::Type::RBrace)
		{
			count[0]--;
			if(count[0] < 0)
				break;
				//throw std::runtime_error("too many right braces <}> found");
		}
		if(position.GetToken().TokenType == Token::Type::RBracket)
		{
			count[1]--;
			if(count[1] < 0)
				break;
				//throw std::runtime_error("too many right brackets <]> found");
		}
		if(position.GetToken().TokenType == Token::Type::RParen)
		{
			count[2]--;
			if(count[2] < 0)
				break;
				//throw std::runtime_error("too many right parentheses <)> found");
		}
		if(position.GetToken().TokenType == Token::Type::RArrow)
		{
			count[3]--;
			if(count[3] < 0)
				break;
				//throw std::runtime_error("too many right parentheses \">\" found");
		}

		combiner += position.GetToken().TokenData;
		position.Increment();
	}
	return combiner;
}

static std::string CombineTokens(ASTTokenSource* source, std::vector<ASTTokenIndex>& tokens, std::string joinSequence)
{
	std::string combiner;
	for(int i=0; i<static_cast<int>(tokens.size())-1; i++)
	{
		combiner += source->Tokens[tokens[i]].TokenData + joinSequence;
	}
	if(tokens.size() != 0)
		combiner += source->Tokens[tokens.back()].TokenData;

	return combiner;
}

ASTParser::ASTParser( Tokenizer& fromTokenizer )
{
	int lineNumber=1;
	Token token;
	while(token.TokenType != Token::Type::EndOfStream)
	{
		token = fromTokenizer.GetNextToken();
		token.TokenLine = lineNumber;
		if(token.TokenType == Token::Type::Newline)
		{
			lineNumber++;
		}
		else
		{
			
		}
		Tokens.push_back(token);
	}
}

bool ASTParser::Parse( ASTNode* parent, ASTPosition& position)
{
	ParseBOM(position);


	while(true) 
	{ 
		if (ParseEndOfStream(parent, position))
			break;

		if (ParseRoot(parent, position))
			continue;

		ParseUnknown(parent, position);
	}

	return true;
}

bool ASTParser::ParseRoot( ASTNode* parent, ASTPosition& position )
{
	if(ParseEnum(parent, position))
	{
		return true;
	}
	else if (ParseTemplate(parent, position))
	{
		return true;
	}
	else if (ParseTypedef(parent, position))
	{
		return true;
	}
	else if (ParseUsing(parent, position))
	{
		return true;
	}
	else if (ParseNamespace(parent, position))
	{
		return true;
	}
	else if(ParseClass(parent, position))
	{
		return true;
	}
	else if (ParseDeclaration(parent, position))
	{
		return true;
	}
	else if (ParsePreprocessor(parent, position))
	{
		return true;
	}
	else if (position.GetToken().TokenType == Token::Type::Semicolon) // valid tokens with no meaning
	{
		position.Increment();
		return true;
	}
	else if(ParseIgnored(parent, position))
	{
		return true;
	}


	return false;
}

bool ASTParser::ParseClass(ASTNode* parent, ASTPosition& cposition)
{

	ASTPosition position = cposition;

	// TODO: do something with this declspec (class/union/struct modifier)
	std::pair<ASTTokenIndex, ASTTokenIndex> declspecGcc;
	ParseMSVCDeclspecOrGCCAttribute(position, declspecGcc);

	if (position.GetToken().TokenType != Token::Type::Class && position.GetToken().TokenType != Token::Type::Struct  && position.GetToken().TokenType != Token::Type::Union)
		return false;

	// TODO: do something with this declspec (keyword modifier)
	ParseMSVCDeclspecOrGCCAttribute(position, declspecGcc);

	int privatePublicProtected = 0;
	if (position.GetToken().TokenType == Token::Type::Struct || position.GetToken().TokenType == Token::Type::Union )
		privatePublicProtected = 1; // struct is default public

	std::unique_ptr<ASTNode> subNode(new ASTNode());
	std::unique_ptr<ASTNode> initialScopeNode(new ASTNode());
	ASTNode* currentScope = initialScopeNode.get();
	bool isStruct = false;
	bool isUnion = false;
	if (position.GetToken().TokenType == Token::Type::Class)
	{
		subNode->type = "CLASS";
		initialScopeNode->type = "PRIVATE";
	}
	else if (position.GetToken().TokenType == Token::Type::Struct)
	{
		subNode->type = "STRUCT";
		initialScopeNode->type = "PUBLIC";
		isStruct = true;
	}
	else if (position.GetToken().TokenType == Token::Type::Union)
	{
		subNode->type = "UNION";
		initialScopeNode->type = "PUBLIC";
		isUnion = true;
	}
	else
		return false;

	position.Increment();

	if (position.GetToken().TokenType == Token::Type::Keyword)
	{
		subNode->data.push_back(position.GetToken().TokenData);
		position.Increment();
	}

	if (position.GetToken().TokenType == Token::Type::LBrace)
	{
		// brace - class definition starts now
	}
	else if (position.GetToken().TokenType == Token::Type::Colon)
	{
		position.Increment();

		// colon - inheritance
		int inheritancePublicPrivateProtected = 1;
		while (ParseClassInheritance(inheritancePublicPrivateProtected, subNode.get(), position))
		{
			if (position.GetToken().TokenType == Token::Type::Comma)
			{
				position.Increment();
				continue;
			}
			else
				break;
		}
	}
	else if (position.GetToken().TokenType == Token::Type::Semicolon)
	{
		if (isUnion)
			subNode->type = "UNION_FORWARD_DECLARATION";
		else if (isStruct)
			subNode->type = "STRUCT_FORWARD_DECLARATION";
		else
			subNode->type = "CLASS_FORWARD_DECLARATION";
		position.Increment();
		parent->AddNode(subNode.release());
		cposition = position;
		return true;
	}
	else
		return false;

	// add default scope node - class definition will be parsed here
	initialScopeNode->data.push_back("initial");
	subNode->AddNode(initialScopeNode.release());

	position.Increment();

	while (true)
	{
		if (ParsePrivatePublicProtected(privatePublicProtected, position))
		{
			// new scope
			currentScope = new ASTNode();
			if (privatePublicProtected == 0)
				currentScope->type = "PRIVATE";
			if (privatePublicProtected == 1)
				currentScope->type = "PUBLIC";
			if (privatePublicProtected == 2)
				currentScope->type = "PROTECTED";
			currentScope->data.push_back("subsequent");
			subNode->AddNode(currentScope);
		}
		else if (ParseTemplate(currentScope, position)) {}	// subclass
		else if (ParseTypedef(currentScope, position)) {}	// typedef
		else if (ParseFriend(subNode.get(), position)) {}	// friend
		else if (ParseClass(currentScope, position)) {}		// subclass
		else if (ParseDeclaration(currentScope, position)) {}
		else if (ParseEnum(currentScope, position)) {}		// sub enum
		else if (ParsePreprocessor(parent, position)) {  }
		else if (position.GetToken().TokenType == Token::Type::Semicolon)
			position.Increment(); // skip stray semicolons
		else if (position.GetToken().TokenType == Token::Type::RBrace)
			break;
		else if (position.GetToken().TokenType == Token::Type::LBrace)
		{
			// unknown scope found
			std::vector<ASTTokenIndex> tokens;
			ParseSpecificScopeInner(position, tokens, Token::Type::LBrace, Token::Type::RBrace, ASTPosition::FilterNone);
			if (Verbose)
				fprintf(stderr, "[PARSER] discarding unknown scope in class/struct: %s", CombineTokens(this, tokens, ""));
		}
		else
		{
			
			ParseUnknown(currentScope, position);
		}
	}

	// parse instances
	if (position.GetNextToken().TokenType != Token::Type::Semicolon)
	{
		std::unique_ptr<ASTNode> subNodeInstances(new ASTNode());
		subNodeInstances->type = "INSTANCES";
		while (true)
		{
			std::unique_ptr<ASTType> subInstance(new ASTType(this));
			subInstance->type = "DCL_SUB";
			if (ParseDeclarationSub(subNodeInstances.get(), position, subInstance.get(), 0, true) == false)
				return false;

			subNodeInstances->AddNode(subInstance.release());

			// reloop on comma
			if (position.GetToken().TokenType != Token::Type::Comma)
				break;
			position.Increment();
		}

		// add to subnode
		subNode->AddNode(subNodeInstances.release());
	}

	// parse final MSVC/GCC modifiers
	// TODO: do something with this declspec (class/union/struct modifier)
	ParseMSVCDeclspecOrGCCAttribute(position, declspecGcc);

	if (position.GetToken().TokenType != Token::Type::Semicolon)
		throw std::runtime_error("expected semicolon to terminate class definition");

	position.Increment();

	// store subnode
	parent->AddNode(subNode.release());
	cposition = position;
	return true;
}


bool ASTParser::ParseTemplate(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition position = cposition;

	// parse template
	if (position.GetToken().TokenType != Token::Type::Template)
		return false;
	position.Increment();

	// parse <
	if (position.GetToken().TokenType != Token::Type::LArrow)
		return false;
	position.Increment();

	// create template tree
	std::unique_ptr<ASTNode> subNode(new ASTNode());
	subNode->type = "TEMPLATE";

	// create arguments tree
	std::unique_ptr<ASTNode> subNodeArgs(new ASTNode());
	subNodeArgs->type = "TEMPLATE_ARGS";

	while (true)
	{
		std::unique_ptr<ASTType> subType(new ASTType(this));

		// TODO: Support nested template definitions

		if (ParseDeclarationHead(parent, position, subType.get()) == false)
			return false;

		ParseDeclarationSub(parent, position, subType.get(), 0);
		subType->type = "TEMPLATE_ARG";
		subNodeArgs->AddNode(subType.release());

		if (position.GetToken().TokenType == Token::Type::Comma)
		{
			position.Increment();
			continue;
		}
		else break;
	}

	if (position.GetToken().TokenType != Token::Type::RArrow)
		return false;

	position.Increment();

	// parsed template definition

	// create arguments tree
	std::unique_ptr<ASTNode> subNodeContent(new ASTNode());
	subNodeContent->type = "TEMPLATE_CONTENT";

	if (ParseClass(subNodeContent.get(), position))
	{
	}
	else if (ParseDeclaration( subNodeContent.get(), position))
	{
	}
	else
		return false; // unknown definition


	// finalize - everything went right.
	//  add nodes to tree
	subNode->AddNode(subNodeArgs.release());
	subNode->AddNode(subNodeContent.release());
	parent->AddNode(subNode.release());

	cposition = position;
	return true;
}

bool ASTParser::ParseNamespace(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != Token::Type::Namespace)
		return false;

	position.Increment();


	// create namespace tree
	std::unique_ptr<ASTNode> subNode(new ASTNode());
	subNode->type = "NAMESPACE";
	if (position.GetToken().TokenType == Token::Type::Keyword)
	{
		subNode->data.push_back(position.GetToken().TokenData);
		position.Increment();
	}
	else if (position.GetToken().TokenType == Token::Type::LBrace)
	{
	}
	else
	{
		throw std::runtime_error("expected left brace after namespace keyword");
		return false; // redundant
	}


	position.Increment();

	while (true)
	{
		// parse inner namespace

		if (ParseEndOfStream(subNode.get() , position))
			return false; // should not reach end of file

		if (ParseRoot(subNode.get(), position))
			continue;

		if (position.GetToken().TokenType == Token::Type::RBrace)
			break; // reached namespace end

		// unknown tokens found; skip
		ParseUnknown(subNode.get(), position);
	}

	// skip past final "right brace" (})
	position.Increment();

	parent->AddNode(subNode.release());

	return true;

}

bool ASTParser::ParseUsing(ASTNode* parent, ASTPosition& cposition)
{
	// TODO: support type aliasing (c++11 feature) http://en.cppreference.com/w/cpp/language/type_alias

	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != Token::Type::Using)
		return false;

	// create tree
	std::unique_ptr<ASTNode> subNode(new ASTNode());
	if (position.GetNextToken().TokenType == Token::Type::Namespace)
	{
		subNode->type = "USING_NAMESPACE";
		position.Increment();
	}
	else
	{
		subNode->type = "USING";
	}

	// check whether using <namespace> is followed by a keyword or a double colon.
	if (position.GetToken().TokenType != Token::Type::Keyword && position.GetToken().TokenType != Token::Type::Doublecolon)
		return false;

	do
	{
		if (position.GetToken().TokenType == Token::Type::Doublecolon)
		{
			subNode->data.push_back(position.GetToken().TokenData);
			position.Increment();
		}

		subNode->data.push_back(position.GetToken().TokenData);
		position.Increment();

	} while (position.GetToken().TokenType == Token::Type::Doublecolon);
	
	if (position.GetToken().TokenType != Token::Type::Semicolon)
		throw std::runtime_error("expected semicolon to finish using namespace declaration (using namespace <definition>;)");

	parent->AddNode(subNode.release());
	cposition = position;
	return true;

}

bool ASTParser::ParseFriend(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != Token::Type::Friend)
		return false;

	position.Increment();
	// create tree
	std::unique_ptr<ASTType> subNode(new ASTType(this));
	subNode->type = "FRIEND";

	ParseDeclarationHead(subNode.get(), position, subNode.get());
	ParseDeclarationSub(subNode.get(), position, subNode.get(), 0);
	
	if (position.GetToken().TokenType != Token::Type::Semicolon)
		return false;
	position.Increment();

	parent->AddNode(subNode.release());
	cposition = position;
	return true;

}

bool ASTParser::ParseTypedef(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != Token::Type::Typedef)
		return false;
	position.Increment();

	// create tree
	std::unique_ptr<ASTNode> subNode(new ASTNode());
	subNode->type = "TYPEDEF";

	bool skipHeadSub = false;
	if (position.GetToken().TokenType == Token::Type::Struct || position.GetToken().TokenType == Token::Type::Class || position.GetToken().TokenType == Token::Type::Union)
	{
		if (ParseClass(subNode.get(), position))
		{
			skipHeadSub = true;
		}
	}

	if (skipHeadSub == false)
	{
		// typedef HEAD SUB,SUB,SUB;

		std::unique_ptr<ASTType> subType(new ASTType(this));
		subType->type = "DCL_HEAD";
		// parse HEAD
		if (ParseDeclarationHead(parent, position, subType.get()) == false)
			return false;

		// parse SUB, SUB, SUB, ...
		while (true)
		{
			std::unique_ptr<ASTType> subType2(new ASTType(this));
			subType2->type = "DCL_SUB";
			if (ParseDeclarationSub(parent, position, subType2.get(), subType.get()) == false)
				return false;

			subType->AddNode(subType2.release());

			if (position.GetToken().TokenType != Token::Type::Comma)
				break;
			else
				position.Increment();
		}

		subNode->AddNode(subType.release());

		if (position.GetToken().TokenType != Token::Type::Semicolon)
			throw std::runtime_error("expected semicolon to finish typedef declaration (typedef <declaration>;)");
	}



	

	parent->AddNode(subNode.release());
	cposition = position;
	return true;

}

bool ASTParser::ParsePreprocessor(ASTNode* parent, ASTPosition& position)
{
	// this is officially not a part of C/C++, however we parse it anyway

	if (position.GetToken().TokenType == Token::Type::Hash)
	{
		std::vector<ASTTokenIndex> preprocessorTokens;

		// parse until newline or end of stream and store tokens
		while (true)
		{
			if (position.GetToken().TokenType == Token::Type::Newline || position.GetToken().TokenType == Token::Type::EndOfStream)
				break;

			preprocessorTokens.push_back(position.GetTokenIndex());
			position.Increment(1, ASTPosition::FilterNone);
		}

		// print a message
		if (Verbose)
			fprintf(stderr, "[PARSER] ignoring preprocessor directive: \"%s\"\n", CombineTokens(this, preprocessorTokens, "").c_str());

		// make sure we are at a non whitespace/comment at the end
		if (ASTPosition::FilterWhitespaceComments(position.GetToken()) == false)
			position.Increment();

		return true;
	}

	return false;
}

bool ASTParser::ParseEnum( ASTNode* parent, ASTPosition& position )
{
	if(position.GetToken().TokenType != Token::Type::Enum)
		return false;

	position.Increment();

	std::unique_ptr<ASTNode> subNode(new ASTNode());
	subNode->type = "ENUM";

	if(position.GetToken().TokenType == Token::Type::Class)
	{
		// c++11 enum class
		subNode->type = "ENUM_CLASS";
		position.Increment();
	}

	if (position.GetToken().TokenType == Token::Type::Keyword)
	{
		subNode->data.push_back(position.GetToken().TokenData);
		position.Increment();
	}

	if(position.GetToken().TokenType != Token::Type::LBrace)
		throw std::runtime_error("expected left brace during enum parse");

	position.Increment();

	while(true)
	{
		if(ParseEnumDefinition(subNode.get(), position))
		{

		}
		else if(position.GetToken().TokenType == Token::Type::RBrace)
			break;
		else
			position.Increment();
	}

	if(position.GetNextToken().TokenType != Token::Type::Semicolon)
		throw std::runtime_error("expected semicolon after enum closing brace");
	
	position.Increment();

	// store subnode
	parent->AddNode(subNode.release());
	return true;
}

bool ASTParser::ParseEnumDefinition( ASTNode* parent, ASTPosition& position )
{
	if(position.GetToken().TokenType != Token::Type::Keyword)
		return false;

	std::unique_ptr < ASTNode> subNode(new ASTNode());
	subNode->type = "ENUM_DEFINITION";

	// store token
	subNode->data.push_back(position.GetToken().TokenData);
	position.Increment();

	if(position.GetToken().TokenType == Token::Type::Equals)
	{
		position.Increment();

		// add the rest
		// add initialization clause
		std::unique_ptr < ASTNode> subSubNode(new ASTNode());
		subSubNode->type = "INIT";

		subSubNode->data.push_back(CombineWhile_ScopeAware(position, [] ( ASTPosition& position) { return position.GetToken().TokenType != Token::Type::Comma && position.GetToken().TokenType != Token::Type::RBrace && position.GetToken().TokenType != Token::Type::Semicolon; }, &ASTParser::ASTPosition::FilterComments));
		subNode->AddNode(subSubNode.release());
	}
	// store subnode
	parent->AddNode(subNode.release());
	return true;
}

bool ASTParser::ParseIgnored( ASTNode* parent, ASTPosition& position )
{
	auto& token = position.GetToken();
	if(token.TokenType == Token::Type::CommentMultiLine || token.TokenType == Token::Type::CommentSingleLine || token.TokenType == Token::Type::Whitespace || token.TokenType == Token::Type::Newline)
	{
		position.Increment();
		return true;
	}
	return false;
}

bool ASTParser::ParseUnknown( ASTNode* parent, ASTPosition& position )
{
	if (Verbose)
		fprintf(stderr, "[PARSER] no grammar match for token: %d (type: %d, line: %d): %s\n", position.Position, position.GetToken().TokenType, position.GetToken().TokenLine, position.GetToken().TokenData.c_str());
	position.Increment();
	return false;
}

bool ASTParser::ParseEndOfStream( ASTNode* parent, ASTPosition& position )
{
	if(position.GetToken().TokenType == Token::Type::EndOfStream)
		return true;

	return false;
}



bool ASTParser::ParseClassInheritance(int &inheritancePublicPrivateProtected, ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition position = cposition;
	if(position.GetToken().TokenType == Token::Type::Public)
	{
		inheritancePublicPrivateProtected = 0;
		position.Increment();
	}
	else if(position.GetToken().TokenType == Token::Type::Private)
	{
		inheritancePublicPrivateProtected = 1;
		position.Increment();
	}
	else if (position.GetToken().TokenType == Token::Type::Protected)
	{
		inheritancePublicPrivateProtected = 2;
		position.Increment();
	}
	
	if (position.GetToken().TokenType != Token::Type::Keyword)
		return false;
	
	ASTNode* subNode = new ASTNode();
	subNode->type = "INHERIT";
	if(inheritancePublicPrivateProtected == 0)
		subNode->data.push_back("public");
	else if(inheritancePublicPrivateProtected == 1)
		subNode->data.push_back("private");
	else if(inheritancePublicPrivateProtected == 2)
		subNode->data.push_back("protected");

	subNode->data.push_back(position.GetToken().TokenData);
	parent->AddNode(subNode);
	position.Increment();
	cposition = position;
	return true;
}


bool ASTParser::ParsePrivatePublicProtected(int& privatePublicProtected, ASTPosition& cposition )
{
	ASTPosition position = cposition;

	if(position.GetToken().TokenType == Token::Type::Private)
	{
		if(position.GetNextToken().TokenType == Token::Type::Colon)
		{
			privatePublicProtected = 0;
			position.Increment();
			cposition = position;
			return true;
		}
	}

	if(position.GetToken().TokenType == Token::Type::Public)
	{
		if(position.GetNextToken().TokenType == Token::Type::Colon)
		{
			privatePublicProtected = 1;
			position.Increment();
			cposition = position;
			return true;
		}
	}

	if(position.GetToken().TokenType == Token::Type::Protected)
	{
		if(position.GetNextToken().TokenType == Token::Type::Colon)
		{
			privatePublicProtected = 2;
			position.Increment();
			cposition = position;
			return true;
		}
	}

	return false;
}

enum class ASTFunctionType
{
	Regular,
	Constructor,
	Destructor,
	Operator
};

bool ASTParser::ParseConstructorInitializer(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition position = cposition;
	std::string name = "";
	
	// find keyword
	if (position.GetToken().TokenType != Token::Type::Keyword)
		return false;

	// store name and increment
	name = position.GetToken().TokenData;
	position.Increment();

	// find left parenthesis
	if (position.GetToken().TokenType != Token::Type::LParen)
		return false;

	std::vector<ASTTokenIndex> scopeTokens;
	if (ParseSpecificScopeInner(position, scopeTokens, Token::Type::LParen, Token::Type::RParen, &ASTPosition::FilterComments) == false)
		return false;

	position.Increment(); // skip rparen

	// everything checked out - add constructor initializer node
	parent->data.push_back(name);
	parent->data.push_back(CombineTokens(this, scopeTokens, ""));

	// advance position to current and return success
	cposition = position;
	return true;
}

bool ASTParser::ParseDeclarationHead(ASTNode* parent, ASTPosition& cposition, ASTType* type)
{
	ASTType tempType(this);
	ASTPosition position = cposition;
	bool valid = true;

	if (ParseNTypeBase(position, &tempType) == false)
	{
		// only take modifiers - head identifier/type must be incorrect
		std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> > modifierTokens;
		while (ParseModifierToken(cposition, modifierTokens)) { modifierTokens.clear(); }
		type->typeModifiers = tempType.typeModifiers;

		return false;
	}

	if (position.GetToken().TokenType == Token::Type::LArrow)
	{
		// parse template arguments
		std::unique_ptr<ASTNode> argNode(new ASTNode());
		if (ParseDeclarationSubArgumentsScoped(position, argNode.get(), Token::Type::LArrow, Token::Type::RArrow) == false)
			return false; // it must be a pointer instead? (e.g. void (*test); );

		argNode->type = "DCL_TEMPLATE_ARGS";
		type->ndTemplateArgumentList = argNode.get();
		type->AddNode(argNode.release());
		
	}

	// apply type and return true
	type->typeNamespaces = tempType.typeNamespaces;
	type->typeName = tempType.typeName;
	type->typeModifiers = tempType.typeModifiers;
	type->StealNodesFrom(&tempType);
	cposition = position;
	return true;
}

bool ASTParser::ParseDeclarationSub(ASTNode* parent, ASTPosition& cposition, ASTType* type, ASTType* headType, bool requireIdentifier)
{
	ASTPosition position = cposition;

	// store head
	type->head = headType; 
	//if (headType != 0)
		//type->MergeData(headType);

	// parse pointers/references & pointer modifiers
	ParseNTypePointersAndReferences(position, type, false);

	if (ParseNTypeFunctionPointer(position, type))
	{
		// function pointer parsed, parse function pointer arguments

		std::unique_ptr<ASTNode> argNode(new ASTNode());

		if (ParseDeclarationSubArgumentsScoped(position, argNode.get(), Token::Type::LParen, Token::Type::RParen) == false)
			return false; // it must be a pointer instead? (e.g. void (*test); );

		argNode->type = "DCL_FPTR_ARGS";
		type->ndFuncPointerArgumentList = argNode.get();
		type->AddNode(argNode.release());
		
	}
	else
	{
		// parse identifier if present
		bool hasIdent = ParseNTypeIdentifier(position, type);
		if (requireIdentifier && hasIdent == false)
			return false;

		// parse array tokens if present
		ParseNTypeArrayDefinitions(position, type);
	}

	// parse function arguments if present
	if (position.GetToken().TokenType == Token::Type::LParen)
	{
		std::unique_ptr<ASTNode> argNode(new ASTNode());
		if (ParseDeclarationSubArgumentsScoped(position, argNode.get(), Token::Type::LParen, Token::Type::RParen) == false)
			return false; // invalid function arguments

		argNode->type = "DCL_FUNC_ARGS";
		type->ndFuncArgumentList = argNode.get();
		type->AddNode(argNode.release());

		// parse function modifiers if present
		std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> > funcModifiers;
		while (ParseModifierToken(position, funcModifiers)) {}

		if (funcModifiers.size() > 0)
		{
			std::unique_ptr<ASTNode> funcModifier(new ASTNode());
			funcModifier->type = "DCL_FUNC_MOD";
			type->ndFuncModifierList = funcModifier.get();

			// fill modifier data
			for (int i = 0; i < funcModifiers.size(); i++)
			{
				std::string modifierData;
				for (int j = funcModifiers[i].first; j <= funcModifiers[i].second; j++)
				{
					if (ASTPosition::FilterWhitespaceComments(Tokens[j]) == false) // dont include whitespace and comments
						continue;
					modifierData += Tokens[j].TokenData;
				}

				funcModifier->AddData(modifierData);
			}

			type->AddNode(funcModifier.release());

		}

	}

	// parse assignment
	if (position.GetToken().TokenType == Token::Type::Equals)
	{
		position.Increment();

		ASTNode* subNode = new ASTNode();
		subNode->type = "INIT";

		subNode->data.push_back(CombineWhile_ScopeAware(position, [](ASTPosition& position) { return position.GetToken().TokenType != Token::Type::Semicolon && position.GetToken().TokenType != Token::Type::Comma; }, &ASTParser::ASTPosition::FilterComments));
		type->AddNode(subNode);
	}

	cposition = position;
	return true;
}

/*
// 1. HEAD|SUB
// 2. HEAD|SUB_FP(HEAD|SUB,HEAD|SUB,HEAD,HEAD...)
// 3. HEAD|SUB_FP(HEAD|SUB,HEAD|SUB,HEAD,HEAD...)(HEAD|SUB,HEAD,...)

// 1.
// HEAD
//	   SUB

// 2.
// HEAD
//	   SUB
//		   HEAD
//			  SUB
//		   HEAD
//		      SUB
//         HEAD
//         HEAD


//  |operator int();
//	void|(*localFP)(int|(*a)(),float|),*test;
//	float|A(),B(),c;
//	std::float|A(),B(),c;
//	std::vector<int>|A(), B(), c;
*/

bool ASTParser::ParseDeclaration(ASTNode* parent, ASTPosition& cposition)
{
	// parse
	// HEAD
	// then SUB,SUB,SUB,... until
	// ; or { FUNCTION_DEFINITION } or : CONSTRUCTOR_INITIALIZER { FUNCTION_DEFINITION }

	ASTPosition position = cposition;
	std::unique_ptr<ASTType> headType(new ASTType(this));
	int lastSubID = -1;
	
	// HEAD then SUB,SUB,SUB,...
	if (_ParseDeclaration_HeadSubs(position, parent, headType, lastSubID) == false)
		return false;

	if (position.GetToken().TokenType == Token::Type::Colon)
	{
		// : CONSTRUCTOR_INITIALIZER
		while (true)
		{
			std::unique_ptr<ASTNode> nd(new ASTNode());
			nd->type = "DCL_FUNC_CONSTRUCTOR_INITIALIZER";

			if (position.GetToken().TokenType == Token::Type::Colon || position.GetToken().TokenType == Token::Type::Comma)
				position.Increment(); // skip past : or ,
			else if (position.GetToken().TokenType == Token::Type::LBrace || position.GetToken().TokenType == Token::Type::Semicolon)
				break;
			else
				throw std::runtime_error("unexpected token in function constructor initializer");

			if (ParseConstructorInitializer(nd.get(), position) == false)
				return false;

			if (lastSubID == -1)
				headType->AddNode(nd.release());
			else
				headType->m_children[lastSubID]->AddNode(nd.release());
		}

	}

	// { FUNCTION_DEFINITION }
	if (position.GetToken().TokenType == Token::Type::LBrace)
	{
		// function declaration
		std::vector<ASTTokenIndex> functionDeclarationTokens;
		if (ParseSpecificScopeInner(position, functionDeclarationTokens, Token::Type::LBrace, Token::Type::RBrace, &ASTPosition::FilterNone) == false)
			return false;

		// continue past final RBrace
		position.Increment();

		ASTNode* nd = new ASTNode();
		nd->type = "DCL_FUNC_DECLARATION";
		nd->data.push_back(CombineTokens(this, functionDeclarationTokens, ""));
		if (lastSubID == -1)
			headType->AddNode(nd);
		else
			headType->m_children[lastSubID]->AddNode(nd);
		
	}
	else if (position.GetToken().TokenType == Token::Type::Semicolon) // ;
		position.Increment();
	else
		return false;
	
	// push to list
	headType->type = "DCL_HEAD";
	parent->AddNode(headType.release());
	cposition = position;
	return true;

}


bool ASTParser::_ParseDeclaration_HeadSubs(ASTPosition &position, ASTNode* parent, std::unique_ptr<ASTType> &headType, int &lastSubID)
{
	ASTPosition beforeHeadPosition = position;
	ParseDeclarationHead(parent, position, headType.get());

	int subCount = 0;
	do
	{
		std::unique_ptr<ASTType> subtype(new ASTType(this));
		subtype->typeIdentifier.clear();

		if (ParseDeclarationSub(parent, position, subtype.get(), headType.get(), true) == false)
		{
			// sub parsing failed - this might be because the head parsing was invalid.. let's find out..
			// P.S. we can't explicitly check whether the head is correct or not since we don't know anything about types (everything is a keyword or a built-in type), so we have to make some educated guesses.
			//  -- then again, if a better way can be found than this method, by all means contribute :)

			if (subCount == 0)
			{
				// skip modifier tokens for sub parsing
				std::vector<std::pair<ASTTokenIndex, ASTTokenIndex>> dummy;
				while (ParseModifierToken(beforeHeadPosition, dummy)) dummy.clear();
			}

			// try parse again when iteration 0 but pretend the head was not valid..
			std::unique_ptr<ASTType> subtype2(new ASTType(this));
			if (subCount == 0 && ParseDeclarationSub(parent, beforeHeadPosition, subtype2.get(), 0, true))
			{
				// it probably was since we can parse the sub now - clear head types
				headType.get()->typeName.clear();
				headType.get()->typeNamespaces.clear();
				position = beforeHeadPosition;
				subtype = std::move(subtype2);
			}
			else
				return false;
		}
		subCount++;

		// apply to head type children
		subtype->type = "DCL_SUB";
		lastSubID = headType->m_children.size();
		headType->AddNode(subtype.release());

		if (position.GetToken().TokenType == Token::Type::Comma)
		{
			position.Increment();
			continue;
		}

		// stop iterating
		break;
	} while (true);

	return true;
}

bool ASTParser::ParseSpecificScopeInner(ASTPosition& cposition, std::vector<ASTTokenIndex> &insideBracketTokens, Token::Type tokenTypeL, Token::Type tokenTypeR, bool(*filterAllows)(Token& token) /*= &ASTPosition::FilterWhitespaceComments*/)
{
	ASTPosition position(cposition);

	if (position.GetToken().TokenType != tokenTypeL)
		return false;
	position.Increment(1, filterAllows);

	while (position.GetToken().TokenType != tokenTypeR)
	{
		if (position.GetToken().TokenType == Token::Type::EndOfStream)
			throw new std::runtime_error("error while parsing scope - end of file found before closing token was found");

		if (position.GetToken().TokenType == tokenTypeL)
		{
			// add LBracket
			insideBracketTokens.push_back(position.GetTokenIndex());

			// add inner
			std::vector<ASTTokenIndex> inside;
			if (ParseSpecificScopeInner(position, inside, tokenTypeL, tokenTypeR, filterAllows) == false)
				return false;
			insideBracketTokens.insert(insideBracketTokens.end(), inside.begin(), inside.end());

			// add RBracket
			insideBracketTokens.push_back(position.GetTokenIndex());
			position.Increment(1, filterAllows);
		}
		else
		{
			insideBracketTokens.push_back(position.GetTokenIndex());
			position.Increment(1, filterAllows);
		}
	}

	cposition = position;
	return true;
}

bool ASTParser::ParseOperatorType(ASTNode* parent, ASTPosition& cposition, std::vector<ASTTokenIndex>& ctokens)
{
	std::vector<ASTTokenIndex> tokens;
	ASTPosition position = cposition;

	// parse operator () (special case)
	if (position.GetToken().TokenType == Token::Type::LParen)
	{
		tokens.push_back(position.GetTokenIndex());
		position.Increment();
		
		if (position.GetToken().TokenType == Token::Type::RParen)
		{
			tokens.push_back(position.GetTokenIndex());
			position.Increment();
		}
		else
			return false;
	}
	else
	{
		// parse other operators
		while (position.GetToken().TokenType != Token::Type::EndOfStream)
		{
			if (position.GetToken().TokenType == Token::Type::LParen)
				break; // start of function arguments found
			else if (position.GetToken().TokenType == Token::Type::Semicolon)
				throw new std::runtime_error("semicolon found while parsing operator arguments - we must have gone too far. invalid operator?");
			else
			{
				tokens.push_back(position.GetTokenIndex());
				position.Increment();
			}
		}

	}

	cposition = position;
	ctokens.swap(tokens);
	return true;

}

bool ASTParser::ParseModifierToken(ASTPosition& cposition, std::vector<std::pair<ASTTokenIndex, ASTTokenIndex>>& modifierTokens)
{
	ASTPosition position = cposition;
	if (position.GetToken().TokenType == Token::Type::Const)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Inline)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Extern)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Virtual)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Volatile)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Unsigned)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Signed)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Static)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Mutable)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Class)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Struct)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::Union)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::GCCExtension)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::MSVCForceInline)
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
	else if (position.GetToken().TokenType == Token::Type::MSVCDeclspec || position.GetToken().TokenType == Token::Type::GCCAttribute)
	{
		std::pair<ASTTokenIndex, ASTTokenIndex> declspecTokens;
		if (ParseMSVCDeclspecOrGCCAttribute(position, declspecTokens) == false)
			return false;
		modifierTokens.push_back(declspecTokens);
		cposition = position;
		return true;
	}
		
	else
		return false;

	position.Increment();
	cposition = position;
	return true;
}

signed short int a;
bool ASTParser::ParseNTypeBase(ASTPosition &position, ASTType* typeNode)
{
	std::vector<ASTTokenIndex>& typeTokens = typeNode->typeName;
	std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> >& modifierTokens = typeNode->typeModifiers;
	std::vector<ASTTokenIndex>& namespaceTokens = typeNode->typeNamespaces;
	int typeWordIndex = -1;
	while (true)
	{
		if (position.GetToken().TokenType == Token::Type::Keyword || position.GetToken().TokenType == Token::Type::BuiltinType || position.GetToken().TokenType == Token::Type::Void)
		{
			if (typeWordIndex == -1)
			{
				typeTokens.push_back(position.GetTokenIndex());
				typeWordIndex = typeTokens.size() - 1;
			}
			else
			{
				// second occurrence of a keyword/builtintype or void keyword

				if (position.GetToken().TokenType == Token::Type::BuiltinType)
				{
					// combined built in types
					if (Tokens[typeTokens.back()].TokenData == "short" && position.GetToken().TokenData == "int")
						typeTokens.push_back(position.GetTokenIndex());
					else if (Tokens[typeTokens.back()].TokenData == "long" && position.GetToken().TokenData == "int")
						typeTokens.push_back(position.GetTokenIndex());
					else if (Tokens[typeTokens.back()].TokenData == "long" && position.GetToken().TokenData == "long")
						typeTokens.push_back(position.GetTokenIndex());
					else if (Tokens[typeTokens.back()].TokenData == "long" && position.GetToken().TokenData == "double")
						typeTokens.push_back(position.GetTokenIndex());
					else
						return false; // invalid type combination
				}
				else if (position.GetToken().TokenType == Token::Type::Void)
					return false;
				else
					break; // this must be the variable name (two keywords not allowed in a type)

			}
		}
		else if ((position.GetToken().TokenType == Token::Type::Typename) && typeWordIndex == -1)
		{
			typeTokens.push_back(position.GetTokenIndex());
			typeWordIndex = typeTokens.size() - 1;
		}
		else if (ParseModifierToken(position, modifierTokens))
			continue;
		else if (position.GetToken().TokenType == Token::Type::Doublecolon)
		{
			if (typeWordIndex != -1)
			{
				if (Tokens[typeTokens[typeWordIndex]].TokenType == Token::Type::Keyword)
				{
					namespaceTokens.push_back(typeTokens[typeWordIndex]);
					typeTokens.clear();
					typeWordIndex = -1; // namespace found - reset type word index
				}
				else if (Tokens[typeTokens[typeWordIndex]].TokenType == Token::Type::BuiltinType)
					throw std::runtime_error("cannot namespace a built-in type");
				else if (Tokens[typeTokens[typeWordIndex]].TokenType == Token::Type::Void)
					throw std::runtime_error("cannot namespace a built-in type");
			}
		}
		else
			return typeWordIndex != -1;
		position.Increment();
	}

	return true;
}

bool ASTParser::ParseNTypeSinglePointersAndReferences(ASTPosition &cposition, ASTType* typeNode, bool fp)
{
	bool isReference = false;
	ASTPosition position(cposition);
	ASTPointerType ptrData;

	Token ptrToken;

	if (position.GetToken() == Token::Type::Asterisk || position.GetToken() == Token::Type::Ampersand)
	{
		if (position.GetToken() == Token::Type::Asterisk)
			ptrData.pointerType = ASTPointerType::Type::Pointer;
		else if (position.GetToken() == Token::Type::Ampersand)
			ptrData.pointerType = ASTPointerType::Type::Reference;
		ptrToken = position.GetToken();
	}
	else
	{
		return false;
	}
	position.Increment();

	// check for pointer/reference modifiers
	while (position.GetToken() == Token::Type::Const || position.GetToken() == Token::Type::Volatile || position.GetToken() == Token::Type::Restrict || position.GetToken() == Token::Type::MSVCRestrict || position.GetToken() == Token::Type::GCCRestrict)
	{
		if (ptrToken == Token::Type::Ampersand)
			throw std::runtime_error("modifiers (const/volatile) are not allowed on a reference");

		if (position.GetToken() == Token::Type::Const) // const applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == Token::Type::Volatile) // volatile applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == Token::Type::Restrict) // restrict applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == Token::Type::MSVCRestrict) // restrict applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == Token::Type::GCCRestrict) // restrict applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());

		position.Increment();
	}

	// parse succeeded
	cposition = position;
	ptrData.pointerToken = ptrToken;
	
	if (fp)
		typeNode->typeFunctionPointerPointers.push_back(ptrData);
	else
		typeNode->typePointers.push_back(ptrData);
	return true;
}

bool ASTParser::ParseNTypePointersAndReferences(ASTPosition &position, ASTType* typeNode, bool fp)
{
	bool res = false;
	while (ParseNTypeSinglePointersAndReferences(position, typeNode, fp))
	{
		res |= true;
	}
	return res;
}

bool ASTParser::ParseNTypeArrayDefinitions(ASTPosition &position, ASTType* typeNode)
{
	// parse arrays
	bool hasArray = false;

	std::vector<ASTTokenIndex> arrayTokens;
	while (ParseSpecificScopeInner(position, arrayTokens, Token::Type::LBracket, Token::Type::RBracket, &ASTParser::ASTPosition::FilterComments))
	{
		// mark that an array has been found
		hasArray = true;

		// add array tokens to array by pushing a new vector and swapping.
		typeNode->typeArrayTokens.push_back(std::vector<ASTTokenIndex>());
		typeNode->typeArrayTokens.back().swap(arrayTokens);

		// go to next token (advance past RBracket )
		position.Increment(); 
	}

	return hasArray;
}

bool ASTParser::ParseNTypeFunctionPointer(ASTPosition &cposition, ASTType* typeNode)
{
	ASTPosition position = cposition;

	if (position.GetToken().TokenType != Token::Type::LParen)
		return false;

	position.Increment();

	if (position.GetToken().TokenType == Token::Type::LParen)
		ParseNTypeFunctionPointer(position, typeNode);	// if we encounter another lparen, recurse
	else if (position.GetToken().TokenType == Token::Type::Ampersand || position.GetToken() == Token::Type::Asterisk)
	{
		// parse pointer tokens
		while (ParseNTypePointersAndReferences(position, typeNode, true))
		{
		}
	}

	// parse identifier if present
	ParseNTypeIdentifier(position, typeNode);

	// parse array tokens if present
	ParseNTypeArrayDefinitions(position, typeNode);

	if (position.GetToken().TokenType != Token::Type::RParen)
		return false;
	position.Increment();

	cposition = position;
	return true;
}

bool ASTParser::ParseNTypeIdentifier(ASTPosition &cposition, ASTType* typeNode)
{
	

	ASTPosition position = cposition;
	std::vector<ASTTokenIndex> tokenIdent;



	// parse namespaces
	while (true)
	{
		if (position.GetToken().TokenType == Token::Type::Doublecolon)
		{
			tokenIdent.push_back(position.GetTokenIndex());
			position.Increment();
		}

		
		// support destructors & operators
		if (position.GetToken().TokenType == Token::Type::Operator)
		{
			tokenIdent.push_back(position.GetTokenIndex());
			position.Increment();
			std::vector<ASTTokenIndex> operatorTokens;
			if (ParseOperatorType(nullptr, position, operatorTokens) == false)
				return false;

			typeNode->typeOperatorTokens = operatorTokens;
			typeNode->typeIdentifier.insert(typeNode->typeIdentifier.end(), tokenIdent.begin(), tokenIdent.end());
			cposition = position;
			return true;
		}
		else if (position.GetToken().TokenType == Token::Type::Tilde)
		{
			tokenIdent.push_back(position.GetTokenIndex());
			position.Increment();
		}

		if (position.GetToken().TokenType == Token::Type::Keyword)
		{
			tokenIdent.push_back(position.GetTokenIndex());
			position.Increment();
		}
		else
			break;
	}

	if (tokenIdent.size() == 0)
		return false;

	// finalize
	cposition = position;
	typeNode->typeIdentifier.insert(typeNode->typeIdentifier.end(), tokenIdent.begin(), tokenIdent.end());
	return true;
}

bool ASTParser::ParseDeclarationSubArguments(ASTPosition &position, ASTNode* parent)
{
	while (true)
	{
		// check for ... (varargs)
		if (position.GetToken().TokenType == Token::Type::DotDotDot)
		{
			std::unique_ptr<ASTNode> subNode(new ASTNode());
			subNode->type = "DCL_VARARGS";
			parent->AddNode(subNode.release());
			position.Increment();
			break;
		}
			
		std::unique_ptr<ASTType> subType(new ASTType(this));
		if (ParseDeclarationHead(parent, position, subType.get()) == false)
			return false;

		ParseDeclarationSub(parent, position, subType.get(), 0);
		subType->type = "DCL_ARG";
		parent->AddNode(subType.release());

		if (position.GetToken().TokenType == Token::Type::Comma)
		{
			position.Increment();
			continue;
		}
		else break;
	}

	return true;
}

bool ASTParser::ParseDeclarationSubArgumentsScoped(ASTPosition &cposition, ASTNode* parent, Token::Type leftScope, Token::Type rightScope)
{
	ASTPosition position = cposition;
	if (position.GetToken().TokenType != leftScope)
		return false;
	position.Increment();

	ParseDeclarationSubArguments(position, parent);

	if (position.GetToken().TokenType != rightScope)
		return false;
	position.Increment();

	cposition = position;
	return true;
}

bool ASTParser::ParseMSVCDeclspecOrGCCAttribute(ASTPosition &position, std::pair<ASTTokenIndex, ASTTokenIndex>& outTokenStream)
{
	ASTTokenIndex idxStart = position.GetTokenIndex();
	if (position.GetToken().TokenType != Token::Type::GCCAttribute && position.GetToken().TokenType != Token::Type::MSVCDeclspec)
		return false;

	position.Increment();

	std::vector<ASTTokenIndex> vtokens;
	if (ParseSpecificScopeInner(position, vtokens, Token::Type::LParen, Token::Type::RParen, ASTPosition::FilterNone) == false)
		return false;

	// continue past final RParen
	position.Increment();

	outTokenStream = std::make_pair(idxStart, vtokens.back()+1);
	return true;
}

void ASTParser::ParseBOM(ASTPosition &position)
{
	// check for byte order marks
	if (position.GetToken().TokenType == Token::Type::BOM_UTF8)
	{
		fprintf(stderr, "[PARSER] File contains UTF-8 BOM.\n");
		IsUTF8 = true;
		position.Increment();
	}
}


std::vector<ASTNode*> ASTNode::GatherAllChildren() const
{
	std::vector<ASTNode*> ret;
	
	for (auto& it : m_children)
	{ 
		auto& subChildren = it->GatherAllChildren(); 
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

	// namespace symbols
	for (auto& it : typeNamespaces)
		ret += tokenSource->Tokens[it].TokenData + "::";
	
	// name symbols
	for (auto& it : typeName)
		ret += tokenSource->Tokens[it].TokenData + " ";

	// template arguments
	if (ndTemplateArgumentList)
	{
		if (ret.size() > 0 && ret.back() == ' ')
			ret.pop_back(); // remove trailing space

		auto& children = ndTemplateArgumentList->Children();
		ret += "<";
		for (auto& it : children)
		{
			ASTType* t = dynamic_cast<ASTType*>(it);
			ret += t->ToString();
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
	if (ndFuncPointerArgumentList)
		ret += "(*";

	// identifier
	for (auto& it : typeIdentifier)
		ret += tokenSource->Tokens[it].TokenData;

	// function pointer suffix
	if (ndFuncPointerArgumentList)
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
	typeNamespaces.insert(typeNamespaces.end(), other->typeNamespaces.begin(), other->typeNamespaces.end());
	typeName.insert(typeName.end(), other->typeName.begin(), other->typeName.end());
	typeIdentifier.insert(typeIdentifier.end(), other->typeIdentifier.begin(), other->typeIdentifier.end());
	typeModifiers.insert(typeModifiers.end(), other->typeModifiers.begin(), other->typeModifiers.end());
	typeOperatorTokens.insert(typeOperatorTokens.end(), other->typeOperatorTokens.begin(), other->typeOperatorTokens.end());
	typePointers.insert(typePointers.end(), other->typePointers.begin(), other->typePointers.end());
	typeArrayTokens.insert(typeArrayTokens.end(), other->typeArrayTokens.begin(), other->typeArrayTokens.end());
	typeTemplateIndices.insert(typeTemplateIndices.end(), other->typeTemplateIndices.begin(), other->typeTemplateIndices.end());
	typeFunctionPointerPointers.insert(typeFunctionPointerPointers.end(), other->typeFunctionPointerPointers.begin(), other->typeFunctionPointerPointers.end());
	typeFunctionPointerArgumentIndices.insert(typeFunctionPointerArgumentIndices.end(), other->typeFunctionPointerArgumentIndices.begin(), other->typeFunctionPointerArgumentIndices.end());

	if (other->ndTemplateArgumentList)
		ndTemplateArgumentList  		=	 other->ndTemplateArgumentList;
	if (other->ndFuncArgumentList)
		ndFuncArgumentList				=	 other->ndFuncArgumentList;
	if (other->ndFuncModifierList)
		ndFuncModifierList				=	 other->ndFuncModifierList;
	if (other->ndFuncPointerArgumentList)
		ndFuncPointerArgumentList		=	 other->ndFuncPointerArgumentList;
}


const std::vector<std::string>& ASTType::GetData()
{
	data.clear();
	data.push_back(ToString());
	return data;
}

#pragma region ASTPosition
void ASTParser::ASTPosition::Increment(int count/*=1*/, bool(*filterAllows)(Token& token)/*=0*/)
{
	for (int i = 0; i < count; i++)
	{
		// check whether we are at the end of the token stream
		if (Position + 1 >= Parser.Tokens.size())
			break;

		++Position;

		if (filterAllows(GetToken()) == false)
		{
			// this token is filtered, go to the next
			i--;
		}
	}
}

ASTParser::ASTPosition::ASTPosition( ASTParser& parser ) : Parser(parser)
{
	Position = 0;
}

Token& ASTParser::ASTPosition::GetNextToken()
{
	Increment();

	return Parser.Tokens[Position];
}

Token& ASTParser::ASTPosition::GetToken()
{
	return Parser.Tokens[Position];
}

#pragma endregion
