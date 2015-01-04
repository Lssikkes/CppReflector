#include "astParser.h"
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

template <class T, class T2> void Parse_ScopeAware(ASTParser::ASTPosition& position, const T& checkFunc, const T2& emitFunction, bool(*filterAllows)(Token& token) = &ASTParser::ASTPosition::FilterWhitespaceComments)
{
	int count[4] = { 0, 0, 0, 0 };

	while (position.GetToken().TokenType != Token::Type::EndOfStream)
	{
		if (count[0] == 0 && count[1] == 0 && count[2] == 0 && count[3] == 0 && checkFunc(position) == false)
			break;
		if (position.GetToken().TokenType == Token::Type::LBrace)
			count[0]++;
		if (position.GetToken().TokenType == Token::Type::LBracket)
			count[1]++;
		if (position.GetToken().TokenType == Token::Type::LParen)
			count[2]++;
		if (position.GetToken().TokenType == Token::Type::LArrow)
			count[3]++;
		if (position.GetToken().TokenType == Token::Type::RBrace)
		{
			count[0]--;
			if (count[0] < 0)
				break;
			//throw std::runtime_error("too many right braces <}> found");
		}
		if (position.GetToken().TokenType == Token::Type::RBracket)
		{
			count[1]--;
			if (count[1] < 0)
				break;
			//throw std::runtime_error("too many right brackets <]> found");
		}
		if (position.GetToken().TokenType == Token::Type::RParen)
		{
			count[2]--;
			if (count[2] < 0)
				break;
			//throw std::runtime_error("too many right parentheses <)> found");
		}
		if (position.GetToken().TokenType == Token::Type::RArrow)
		{
			count[3]--;
			if (count[3] < 0)
				break;
			//throw std::runtime_error("too many right parentheses \">\" found");
		}

		emitFunction(position);
		position.Increment();
	}
	return;
}

template <class T> std::string CombineWhile_ScopeAware(ASTParser::ASTPosition& position, const T& checkFunc, bool(*filterAllows)(Token& token) = &ASTParser::ASTPosition::FilterWhitespaceComments)
{
	std::string combiner;
	Parse_ScopeAware(position, checkFunc, [&combiner](ASTParser::ASTPosition& pos) { combiner += pos.GetToken().TokenData; }, filterAllows);
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
	// in root scope bit fields are disallowed, but copy constructors are not.
	static ASTDeclarationParsingOptions declOpts(true, false, true);
	
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
	else if (ParseDeclaration(parent, position, declOpts))
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
	ParseArgumentAttribute(position, declspecGcc);

	if (position.GetToken().TokenType != Token::Type::Class && position.GetToken().TokenType != Token::Type::Struct  && position.GetToken().TokenType != Token::Type::Union)
		return false;

	// TODO: do something with this declspec (keyword modifier)
	ParseArgumentAttribute(position, declspecGcc);

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

	// in class scope bit fields are allowed, but copy constructors are not.
	static ASTDeclarationParsingOptions declOpts(false, true, true);

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
		else if (ParseDeclaration(currentScope, position, declOpts)) {}
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
				fprintf(stderr, "[PARSER] discarding unknown scope in class/struct: %s", CombineTokens(this, tokens, "").c_str());
		}
		else if (position.GetToken().TokenType == Token::Type::EndOfStream)
		{
		
			if (Verbose)
				fprintf(stderr, "[PARSER] end of stream reached during class parse - something is wrong\n");
			break;
		}
			
		else
		{
			
			ParseUnknown(currentScope, position);
		}
	}

	// parse instances
	if (position.GetNextToken().TokenType != Token::Type::Semicolon)
	{
		// nothing fancy is allowed in instances
		static ASTDeclarationParsingOptions instanceOpts(false, false, true);

		std::unique_ptr<ASTNode> subNodeInstances(new ASTNode());
		subNodeInstances->type = "INSTANCES";
		while (true)
		{
			std::unique_ptr<ASTType> subInstance(new ASTType(this));
			subInstance->type = "DCL_SUB";
			if (ParseDeclarationSub(subNodeInstances.get(), position, subInstance.get(), 0, instanceOpts) == false)
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
	ParseArgumentAttribute(position, declspecGcc);

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
		ASTDeclarationParsingOptions opts;
		if (ParseDeclarationHead(parent, position, subType.get(), opts) == false)
			return false;

		ParseDeclarationSub(parent, position, subType.get(), 0, opts);
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

	// in template scope bit fields are not allowed, neither are copy constructors.
	static ASTDeclarationParsingOptions declOpts(false, false, true);

	if (ParseClass(subNodeContent.get(), position))
	{
	}
	else if (ParseDeclaration( subNodeContent.get(), position, declOpts))
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

	ASTDeclarationParsingOptions opts;
	ParseDeclarationHead(subNode.get(), position, subNode.get(), opts);
	ParseDeclarationSub(subNode.get(), position, subNode.get(), 0, opts);
	
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
		ASTDeclarationParsingOptions opts;
		// typedef HEAD SUB,SUB,SUB;

		std::unique_ptr<ASTType> subType(new ASTType(this));
		subType->type = "DCL_HEAD";
		// parse HEAD
		if (ParseDeclarationHead(parent, position, subType.get(), opts) == false)
			return false;

		// parse SUB, SUB, SUB, ...
		while (true)
		{
			std::unique_ptr<ASTType> subType2(new ASTType(this));
			subType2->type = "DCL_SUB";
			if (ParseDeclarationSub(parent, position, subType2.get(), subType.get(), opts) == false)
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
	
	if (position.GetToken().TokenType != Token::Type::Keyword && position.GetToken().TokenType != Token::Type::Doublecolon)
		return false;
	

	// parse class/struct/union name (including namespaces)
	std::unique_ptr<ASTNode> subNode(new ASTNode());
	std::unique_ptr<ASTType> subType(new ASTType(this));
	ASTDeclarationParsingOptions opts;
	if (ParseDeclarationHead(subNode.get(), position, subType.get(), opts) == false)
		return false;
	subType->type = "DCL_INHERIT";
	subNode->AddNode(subType.release());

	// create subnode
	subNode->type = "INHERIT";
	if(inheritancePublicPrivateProtected == 0)
		subNode->data.push_back("public");
	else if(inheritancePublicPrivateProtected == 1)
		subNode->data.push_back("private");
	else if(inheritancePublicPrivateProtected == 2)
		subNode->data.push_back("protected");


	parent->AddNode(subNode.release());
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

bool ASTParser::ParseDeclarationHead(ASTNode* parent, ASTPosition& cposition, ASTType* type, ASTDeclarationParsingOptions opts)
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
		if (ParseDeclarationSubArgumentsScopedWithNonTypes(position, argNode.get(), Token::Type::LArrow, Token::Type::RArrow) == false)
			return false; // it must be a pointer instead? (e.g. void (*test); );

		argNode->type = "DCL_TEMPLATE_ARGS";
		type->ndTemplateArgumentList = argNode.get();
		type->AddNode(argNode.release());
		
	}

	// parse final modifier tokens if present
	while (ParseModifierToken(position, type->typeModifiers)) {}

	// apply type and return true
	type->typeName = tempType.typeName;
	type->typeModifiers = tempType.typeModifiers;
	type->StealNodesFrom(&tempType);
	cposition = position;
	return true;
}

bool ASTParser::ParseDeclarationSub(ASTNode* parent, ASTPosition& cposition, ASTType* type, ASTType* headType, ASTDeclarationParsingOptions opts)
{
	ASTPosition position = cposition;

	// store head
	type->head = headType; 
	//if (headType != 0)
		//type->MergeData(headType);

	// parse pointers/references & pointer modifiers
	ParseNTypePointersAndReferences(position, type, false);

	bool parsedArguments = false;

	if (ParseNTypeFunctionPointer(position, type))
	{
		// function pointer parsed, parse function pointer arguments

		std::unique_ptr<ASTNode> argNode(new ASTNode());

		if (ParseDeclarationSubArgumentsScoped(position, argNode.get(), Token::Type::LParen, Token::Type::RParen))
		{
			argNode->type = "DCL_FPTR_ARGS";
			type->ndFuncPointerArgumentList = argNode.get();
			type->AddNode(argNode.release());
			parsedArguments = true;

		}
		else
		{
			// it must be a pointer instead.. (e.g. void (*test); );
		}
	}
	else
	{
		// parse identifier if present
		bool hasIdent = ParseNTypeIdentifier(position, type);
		if (opts.RequireIdentifier && hasIdent == false)
			return false;

		// parse array tokens if present
		ParseNTypeArrayDefinitions(position, type);
	}


	// parse function arguments if present
	if (position.GetToken().TokenType == Token::Type::LParen)
	{
		std::unique_ptr<ASTNode> argNode(new ASTNode());
		
		if (ParseDeclarationSubArgumentsScoped(position, argNode.get(), Token::Type::LParen, Token::Type::RParen) == false)
		{
			// parsing failed
			if (opts.AllowCtor == false)
				return false; // if it's impossible to be a ctor (such as in class/struct/union scope, or function argument scope), this is invalid.

			argNode->type = "DCL_CTOR_ARGS";
			if (ParseConstructorArguments(argNode.get(), position) == false)
				return false;
		}
		else
		{
			argNode->type = "DCL_FUNC_ARGS";
			type->ndFuncArgumentList = argNode.get();
		}
			
		type->AddNode(argNode.release());

		parsedArguments = true;

	}

	// parse bitfield if allowed
	if (opts.AllowBitfield && position.GetToken().TokenType == Token::Type::Colon && parsedArguments == false)
	{
		position.Increment(); // skip Colon

		// parse 
		Parse_ScopeAware(position,
			[](ASTPosition& pos) { return pos.GetToken().TokenType != Token::Type::Comma &&  pos.GetToken().TokenType != Token::Type::Semicolon;  },
			[type](ASTPosition& pos) { type->typeBitfieldTokens.push_back(pos.GetTokenIndex()); },
			&ASTPosition::FilterWhitespaceComments);

	}

	if (parsedArguments)
	{
		// parse function modifiers if present
		std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> > funcModifiers;
		while (ParseModifierToken(position, funcModifiers)) {}

		if (funcModifiers.size() > 0)
		{
			std::unique_ptr<ASTNode> funcModifierList(new ASTNode());
			funcModifierList->type = "DCL_FUNC_MODS";
			type->ndFuncModifierList = funcModifierList.get();

			// fill modifier data
			for (int i = 0; i < funcModifiers.size(); i++)
			{
				std::unique_ptr<ASTTokenNode> funcModifier(new ASTTokenNode(this));
				funcModifier->type = "MODIFIER";

				std::string modifierData;
				for (int j = funcModifiers[i].first; j <= funcModifiers[i].second; j++)
				{
					if (ASTPosition::FilterWhitespaceComments(Tokens[j]) == false) // dont include whitespace and comments
						continue;
					funcModifier->Tokens.push_back(j);
				}
				
				funcModifierList->AddNode(funcModifier.release());
			}

			type->AddNode(funcModifierList.release());

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

bool ASTParser::ParseDeclaration(ASTNode* parent, ASTPosition& cposition, ASTDeclarationParsingOptions opts)
{
	// parse
	// HEAD
	// then SUB,SUB,SUB,... until
	// ; or { FUNCTION_DEFINITION } or : CONSTRUCTOR_INITIALIZER { FUNCTION_DEFINITION }

	ASTPosition position = cposition;
	std::unique_ptr<ASTType> headType(new ASTType(this));
	int lastSubID = -1;
	
	// HEAD then SUB,SUB,SUB,...
	if (_ParseDeclaration_HeadSubs(position, parent, headType, lastSubID, opts) == false)
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

bool ASTParser::_ParseDeclaration_HeadSubs(ASTPosition &position, ASTNode* parent, std::unique_ptr<ASTType> &headType, int &lastSubID, ASTDeclarationParsingOptions opts)
{
	ASTPosition beforeHeadPosition = position;
	ParseDeclarationHead(parent, position, headType.get(), opts);

	int subCount = 0;
	do
	{
		std::unique_ptr<ASTType> subtype(new ASTType(this));
		subtype->typeIdentifier.clear();

		if (ParseDeclarationSub(parent, position, subtype.get(), headType.get(), opts) == false)
		{
			// Support for constructors/destructors/default-int
			// ----------

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
			if (subCount == 0 && ParseDeclarationSub(parent, beforeHeadPosition, subtype2.get(), 0, opts))
			{
				// it probably was since we can parse the sub now - clear head types
				headType.get()->typeName.clear();
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
	switch (position.GetToken().TokenType)
	{
	case Token::Type::Const:
	case Token::Type::Inline:
	case Token::Type::Extern:
	case Token::Type::Virtual:
	case Token::Type::Volatile:
	case Token::Type::Unsigned:
	case Token::Type::Signed:
	
	case Token::Type::Static:
	case Token::Type::Mutable:
	case Token::Type::Class:
	case Token::Type::Struct:
	case Token::Type::Union:
	case Token::Type::Thread:
	case Token::Type::GCCExtension:
	case Token::Type::MSVCForceInline:
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
		break;
	case Token::Type::MSVCDeclspec:
	case Token::Type::GCCAttribute:
	case Token::Type::Throw:
	{
		std::pair<ASTTokenIndex, ASTTokenIndex> declspecTokens;
		if (ParseArgumentAttribute(position, declspecTokens) == false)
			return false;
		modifierTokens.push_back(declspecTokens);
		cposition = position;
		return true;
	}
	default:
		return false;
	}


	position.Increment();
	cposition = position;
	return true;
}

bool ASTParser::ParseNTypeBase(ASTPosition &position, ASTType* typeNode)
{
	std::vector<ASTTokenIndex>& typeTokens = typeNode->typeName;
	std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> >& modifierTokens = typeNode->typeModifiers;
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
			typeTokens.push_back(position.GetTokenIndex());  // namespace support
			typeWordIndex = -1; // reset type word index
		}
		else
			return typeWordIndex != -1;
		position.Increment();
	}

	return true;
}

bool ASTParser::ParseNTypeSinglePointersAndReferences(ASTPosition &cposition, ASTType* typeNode, bool identifierScope)
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
	
	if (identifierScope)
		typeNode->typeIdentifierScopedPointers.push_back(ptrData);
	else
		typeNode->typePointers.push_back(ptrData);
	return true;
}

bool ASTParser::ParseNTypePointersAndReferences(ASTPosition &position, ASTType* typeNode, bool identifierScope)
{
	bool res = false;
	while (ParseNTypeSinglePointersAndReferences(position, typeNode, identifierScope))
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

	bool hasPtrTokens = false;

	if (position.GetToken().TokenType == Token::Type::Ampersand || position.GetToken() == Token::Type::Asterisk)
	{
		// parse pointer tokens
		while (ParseNTypePointersAndReferences(position, typeNode, true)) {}
		hasPtrTokens = true;
	}
	if (position.GetToken().TokenType == Token::Type::LParen)
	{
		if (ParseNTypeFunctionPointer(position, typeNode) == false)
			return false;	
	}
	else
	{
		if (hasPtrTokens == false)
			return false; // no ampersand or asterisk - this cannot be a function pointer
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
		ASTDeclarationParsingOptions opts;
		if (ParseDeclarationHead(parent, position, subType.get(), opts) == false)
			return false;

		ParseDeclarationSub(parent, position, subType.get(), 0, opts);
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

bool ASTParser::ParseDeclarationSubArgumentsScopedWithNonTypes(ASTPosition &cposition, ASTNode* parent, Token::Type leftScope, Token::Type rightScope)
{
	ASTPosition position = cposition;
	if (position.GetToken().TokenType != leftScope)
		return false;
	position.Increment();

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
		ASTDeclarationParsingOptions opts;
		if (ParseDeclarationHead(parent, position, subType.get(), opts))
		{
			ParseDeclarationSub(parent, position, subType.get(), 0, opts);
			subType->type = "DCL_ARG";
			parent->AddNode(subType.release());
		}
		else
		{
			std::unique_ptr<ASTTokenNode> tokNode(new ASTTokenNode(this));
			tokNode->type = "DCL_NONTYPE_ARG";
			Parse_ScopeAware(position,
				[rightScope](ASTPosition& p) { return p.GetToken().TokenType != Token::Type::Comma && p.GetToken().TokenType != rightScope; },
				[&tokNode](ASTPosition& p) { tokNode->Tokens.push_back(p.GetTokenIndex()); },
				&ASTParser::ASTPosition::FilterComments);
			parent->AddNode(tokNode.release());
		}

		if (position.GetToken().TokenType == Token::Type::Comma)
		{
			position.Increment();
			continue;
		}
		else break;
	}

	if (position.GetToken().TokenType != rightScope)
		return false;
	position.Increment();

	cposition = position;
	return true;
}

bool ASTParser::ParseArgumentAttribute(ASTPosition &position, std::pair<ASTTokenIndex, ASTTokenIndex>& outTokenStream)
{
	ASTTokenIndex idxStart = position.GetTokenIndex();
	if (position.GetToken().TokenType != Token::Type::GCCAttribute 
		&& position.GetToken().TokenType != Token::Type::MSVCDeclspec
		&& position.GetToken().TokenType != Token::Type::GCCAssembly
		&& position.GetToken().TokenType != Token::Type::Throw)
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

bool ASTParser::ParseConstructorArguments(ASTNode* parent, ASTPosition &position)
{
	position.Increment(); // skip first lparen

	// parse ctor args
	do
	{
		ASTTokenNode* tokNode = new ASTTokenNode(this);
		tokNode->type = "DCL_CTOR_ARG";
		Parse_ScopeAware(position,
			[](ASTPosition& p) { return p.GetToken().TokenType != Token::Type::Comma && p.GetToken().TokenType != Token::Type::RParen; },
			[tokNode](ASTPosition& p) { tokNode->Tokens.push_back(p.GetTokenIndex()); },
			&ASTParser::ASTPosition::FilterComments);
		parent->AddNode(tokNode);
		if (position.GetToken().TokenType == Token::Type::Comma)
		{
			position.Increment();
			continue; // reloop on comma
		}
		break;
	} while (true); // escape otherwise

	// sanity check
	if (position.GetToken().TokenType != Token::Type::RParen)
		return false;

	// skip RParen
	position.Increment();
	return true;
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

ASTParser::ASTPosition::ASTPosition(ASTParser& parser) : Parser(parser)
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