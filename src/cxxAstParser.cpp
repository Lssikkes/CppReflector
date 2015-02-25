#include "cxxAstParser.h"
#include <memory>

#define SUBTYPE_MODE_SUBVARIABLE 0
#define SUBTYPE_MODE_SUBARGUMENT 1

template <class T> std::string CombineWhile(ASTCxxParser::ASTPosition& position, const T& checkFunc, bool(*filterAllows)(CxxToken& token) = &ASTCxxParser::ASTPosition::FilterWhitespaceComments)
{
	std::string combiner;
	while (position.GetToken().TokenType != CxxToken::Type::EndOfStream && checkFunc(position))
	{
		combiner += position.GetToken().TokenData;
		position.Increment();
	}
	return combiner;
}

template <class T, class T2> void Parse_ScopeAware(ASTCxxParser::ASTPosition& position, const T& checkFunc, const T2& emitFunction, bool(*filterAllows)(CxxToken& token) = &ASTCxxParser::ASTPosition::FilterWhitespaceComments)
{
	int count[4] = { 0, 0, 0, 0 };

	while (position.GetToken().TokenType != CxxToken::Type::EndOfStream)
	{
		if (count[0] == 0 && count[1] == 0 && count[2] == 0 && count[3] == 0 && checkFunc(position) == false)
			break;
		if (position.GetToken().TokenType == CxxToken::Type::LBrace)
			count[0]++;
		if (position.GetToken().TokenType == CxxToken::Type::LBracket)
			count[1]++;
		if (position.GetToken().TokenType == CxxToken::Type::LParen)
			count[2]++;
		if (position.GetToken().TokenType == CxxToken::Type::LArrow)
			count[3]++;
		if (position.GetToken().TokenType == CxxToken::Type::RBrace)
		{
			count[0]--;
			if (count[0] < 0)
				break;
			//throw std::runtime_error("too many right braces <}> found");
		}
		if (position.GetToken().TokenType == CxxToken::Type::RBracket)
		{
			count[1]--;
			if (count[1] < 0)
				break;
			//throw std::runtime_error("too many right brackets <]> found");
		}
		if (position.GetToken().TokenType == CxxToken::Type::RParen)
		{
			count[2]--;
			if (count[2] < 0)
				break;
			//throw std::runtime_error("too many right parentheses <)> found");
		}
		if (position.GetToken().TokenType == CxxToken::Type::RArrow)
		{
			count[3]--;
			if (count[3] < 0)
				break;
			//throw std::runtime_error("too many right parentheses \">\" found");
		}

		emitFunction(position);
		position.Increment(1, filterAllows);
	}
	return;
}

template <class T> std::string CombineWhile_ScopeAware(ASTCxxParser::ASTPosition& position, const T& checkFunc, bool(*filterAllows)(CxxToken& token) = &ASTCxxParser::ASTPosition::FilterWhitespaceComments)
{
	std::string combiner;
	Parse_ScopeAware(position, checkFunc, [&combiner](ASTCxxParser::ASTPosition& pos) { combiner += pos.GetToken().TokenData; }, filterAllows);
	return combiner;
}
template <class T> void ParseToArray_ScopeAware(std::vector<ASTTokenIndex>& ret, ASTCxxParser::ASTPosition& position, const T& checkFunc, bool(*filterAllows)(CxxToken& token) = &ASTCxxParser::ASTPosition::FilterWhitespaceComments)
{
	Parse_ScopeAware(position, checkFunc, [&ret](ASTCxxParser::ASTPosition& pos) { ret.push_back(pos.GetTokenIndex()); }, filterAllows);

}

static std::string CombineTokens(ASTTokenSource* source, std::vector<ASTTokenIndex>& tokens, std::string joinSequence)
{
	std::string combiner;
	for (int i = 0; i < static_cast<int>(tokens.size()) - 1; i++)
	{
		combiner += source->Tokens[tokens[i]].TokenData + joinSequence;
	}
	if (tokens.size() != 0)
		combiner += source->Tokens[tokens.back()].TokenData;

	return combiner;
}

ASTCxxParser::ASTCxxParser(CxxTokenizer& fromTokenizer)
{
	m_source = fromTokenizer.Identifier;
	int lineNumber = 1;
	CxxToken token;
	while (token.TokenType != CxxToken::Type::EndOfStream)
	{
		token = fromTokenizer.GetNextToken();
		token.TokenLine = lineNumber;
		if (token.TokenType == CxxToken::Type::Newline)
		{
			lineNumber++;
		}
		else
		{

		}
		Tokens.push_back(token);
	}
}

bool ASTCxxParser::Parse(ASTNode* parent, ASTPosition& position)
{
	ParseBOM(position);


	while (true)
	{
		if (ParseEndOfStream(parent, position))
			break;

		if (ParseRootParticle(parent, position))
			continue;

		ParseUnknown(parent, position);
	}

	return true;
}

bool ASTCxxParser::ParseRootParticle(ASTNode* parent, ASTPosition& position)
{
	// in root scope bit fields are disallowed, but copy constructors are not.
	static ASTDeclarationParsingOptions declOpts(true, false, true);

	if (ParseEnum(parent, position))
	{
		return true;
	}
	else if (ParseTemplate(parent, position))
	{
		return true;
	}
	else if (ParseExtensionAnnotation(parent, position))
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
	else if (ParseClass(parent, position))
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
	else if (position.GetToken().TokenType == CxxToken::Type::Semicolon) // valid tokens with no meaning
	{
		position.Increment();
		return true;
	}
	else if (ParseIgnored(parent, position))
	{
		return true;
	}


	return false;
}


int ASTCxxParser::ParseClassParticle(int privatePublicProtected, ASTPosition &position, ASTDataNode*& currentScope, std::unique_ptr<ASTDataNode> &subNode, ASTNode* parent)
{
	static ASTDeclarationParsingOptions declOpts(false, true, true);
	if (ParsePrivatePublicProtected(privatePublicProtected, position))
	{
		// new scope
		currentScope = new ASTDataNode();
		if (privatePublicProtected == 0)
			currentScope->SetType(ASTNode::Type::Private);
		if (privatePublicProtected == 1)
			currentScope->SetType(ASTNode::Type::Public);
		if (privatePublicProtected == 2)
			currentScope->SetType(ASTNode::Type::Protected);
		currentScope->data.push_back("subsequent");
		subNode->AddNode(currentScope);
	}
	else if (ParseExtensionAnnotation(currentScope, position)) {}
	else if (ParseTemplate(currentScope, position)) {}	// subclass
	else if (ParseTypedef(currentScope, position)) {}	// typedef
	else if (ParseFriend(subNode.get(), position)) {}	// friend
	else if (ParseClass(currentScope, position)) {}		// subclass
	else if (ParseDeclaration(currentScope, position, declOpts)) {}
	else if (ParseEnum(currentScope, position)) {}		// sub enum
	else if (ParsePreprocessor(parent, position)) {}
	else if (position.GetToken().TokenType == CxxToken::Type::Semicolon)
		position.Increment(); // skip stray semicolons
	else if (position.GetToken().TokenType == CxxToken::Type::RBrace)
		return 2; // end of class
	else if (position.GetToken().TokenType == CxxToken::Type::LBrace)
	{
		// unknown scope found
		std::vector<ASTTokenIndex> tokens;
		ParseSpecificScopeInner(position, tokens, CxxToken::Type::LBrace, CxxToken::Type::RBrace, ASTPosition::FilterNone);
		if (Verbose)
			fprintf(stderr, "[PARSER] discarding unknown scope in class/struct: %s", CombineTokens(this, tokens, "").c_str());
	}
	else if (position.GetToken().TokenType == CxxToken::Type::EndOfStream)
	{

		if (Verbose)
			fprintf(stderr, "[PARSER] end of stream reached during class parse - something is wrong\n");
		return 2; // end of class
	}

	else
	{
		ParseUnknown(currentScope, position);
		return 1; // unknown particle
	}
	return 0; // everything went fine
}


bool ASTCxxParser::ParseClass(ASTNode* parent, ASTPosition& cposition)
{

	ASTPosition position = cposition;

	// TODO: do something with this declspec (class/union/struct modifier)
	std::pair<ASTTokenIndex, ASTTokenIndex> declspecGcc;
	ParseArgumentAttribute(position, declspecGcc);

	if (position.GetToken().TokenType != CxxToken::Type::Class && position.GetToken().TokenType != CxxToken::Type::Struct  && position.GetToken().TokenType != CxxToken::Type::Union)
		return false;

	// TODO: do something with this declspec (keyword modifier)
	ParseArgumentAttribute(position, declspecGcc);

	int privatePublicProtected = 0;
	if (position.GetToken().TokenType == CxxToken::Type::Struct || position.GetToken().TokenType == CxxToken::Type::Union)
		privatePublicProtected = 1; // struct is default public

	std::unique_ptr<ASTDataNode> subNode(new ASTDataNode());
	std::unique_ptr<ASTDataNode> initialScopeNode(new ASTDataNode());
	ASTDataNode* currentScope = initialScopeNode.get();
	bool isStruct = false;
	bool isUnion = false;
	if (position.GetToken().TokenType == CxxToken::Type::Class)
	{
		subNode->SetType(ASTNode::Type::Class);
		initialScopeNode->SetType(ASTNode::Type::Private);
	}
	else if (position.GetToken().TokenType == CxxToken::Type::Struct)
	{
		subNode->SetType(ASTNode::Type::Struct);
		initialScopeNode->SetType(ASTNode::Type::Public);
		isStruct = true;
	}
	else if (position.GetToken().TokenType == CxxToken::Type::Union)
	{
		subNode->SetType(ASTNode::Type::Union);
		initialScopeNode->SetType(ASTNode::Type::Public);
		isUnion = true;
	}
	else
		return false;

	position.Increment();

	if (position.GetToken().TokenType == CxxToken::Type::Keyword)
	{
		subNode->data.push_back(position.GetToken().TokenData);
		position.Increment();
	}

	if (position.GetToken().TokenType == CxxToken::Type::LBrace)
	{
		// brace - class definition starts now
	}
	else if (position.GetToken().TokenType == CxxToken::Type::Colon)
	{
		position.Increment();

		// colon - inheritance
		int inheritancePublicPrivateProtected = 1;
		while (ParseClassInheritance(inheritancePublicPrivateProtected, subNode.get(), position))
		{
			if (position.GetToken().TokenType == CxxToken::Type::Comma)
			{
				position.Increment();
				continue;
			}
			else
				break;
		}
	}
	else if (position.GetToken().TokenType == CxxToken::Type::Semicolon)
	{
		if (isUnion)
			subNode->SetType(ASTNode::Type::UnionFwdDcl);
		else if (isStruct)
			subNode->SetType(ASTNode::Type::StructFwdDcl);
		else
			subNode->SetType(ASTNode::Type::ClassFwdDcl);
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
	while (true)
	{
		int res = ParseClassParticle(privatePublicProtected, position, currentScope, subNode, parent);
		if (res == 0 || res == 1)
		{
			// everything went fine
		}
		else if (res == 2)
		{
			// we're at the end
			break;
		}

	}

	// parse instances
	if (position.GetNextToken().TokenType != CxxToken::Type::Semicolon)
	{
		// nothing fancy is allowed in instances
		static ASTDeclarationParsingOptions instanceOpts(false, false, true);

		std::unique_ptr<ASTNode> subNodeInstances(new ASTNode());
		subNodeInstances->SetType(ASTNode::Type::Instances);
		while (true)
		{
			std::unique_ptr<ASTType> subInstance(new ASTType(this));
			subInstance->SetType(ASTNode::Type::DclSub);
			if (ParseDeclarationSub(subNodeInstances.get(), position, subInstance.get(), 0, instanceOpts) == false)
				return false;

			subNodeInstances->AddNode(subInstance.release());

			// reloop on comma
			if (position.GetToken().TokenType != CxxToken::Type::Comma)
				break;
			position.Increment();
		}

		// add to subnode
		subNode->AddNode(subNodeInstances.release());
	}

	// parse final MSVC/GCC modifiers
	// TODO: do something with this declspec (class/union/struct modifier)
	ParseArgumentAttribute(position, declspecGcc);

	if (position.GetToken().TokenType != CxxToken::Type::Semicolon)
		throw std::runtime_error("expected semicolon to terminate class definition");

	position.Increment();

	// store subnode
	parent->AddNode(subNode.release());
	cposition = position;
	return true;
}



bool ASTCxxParser::ParseTemplate(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition position = cposition;

	// parse template
	if (position.GetToken().TokenType != CxxToken::Type::Template)
		return false;
	position.Increment();

	// parse <
	if (position.GetToken().TokenType != CxxToken::Type::LArrow)
		return false;
	position.Increment();

	// create template tree
	std::unique_ptr<ASTDataNode> subNode(new ASTDataNode());
	subNode->SetType(ASTNode::Type::Template);

	// create arguments tree
	std::unique_ptr<ASTNode> subNodeArgs(new ASTNode());
	subNodeArgs->SetType(ASTNode::Type::TemplateArgs);

	while (true)
	{
		std::unique_ptr<ASTType> subType(new ASTType(this));

		// TODO: Support nested template definitions
		ASTDeclarationParsingOptions opts;
		if (ParseDeclarationHead(parent, position, subType.get(), opts) == false)
			return false;

		ParseDeclarationSub(parent, position, subType.get(), 0, opts);
		subType->SetType(ASTNode::Type::TemplateArg);
		subNodeArgs->AddNode(subType.release());

		if (position.GetToken().TokenType == CxxToken::Type::Comma)
		{
			position.Increment();
			continue;
		}
		else break;
	}

	if (position.GetToken().TokenType != CxxToken::Type::RArrow)
		return false;

	position.Increment();

	// parsed template definition

	// create arguments tree
	std::unique_ptr<ASTNode> subNodeContent(new ASTNode());
	subNodeContent->SetType(ASTNode::Type::TemplateContent);

	// in template scope bit fields are not allowed, neither are copy constructors.
	static ASTDeclarationParsingOptions declOpts(false, false, true);

	if (ParseClass(subNodeContent.get(), position))
	{
	}
	else if (ParseDeclaration(subNodeContent.get(), position, declOpts))
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

bool ASTCxxParser::ParseNamespace(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != CxxToken::Type::Namespace)
		return false;

	position.Increment();


	// create namespace tree
	std::unique_ptr<ASTDataNode> subNode(new ASTDataNode());
	subNode->SetType(ASTNode::Type::Namespace);
	if (position.GetToken().TokenType == CxxToken::Type::Keyword)
	{
		subNode->data.push_back(position.GetToken().TokenData);
		position.Increment();
	}
	else if (position.GetToken().TokenType == CxxToken::Type::LBrace)
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

		if (ParseEndOfStream(subNode.get(), position))
			return false; // should not reach end of file

		if (ParseRootParticle(subNode.get(), position))
			continue;

		if (position.GetToken().TokenType == CxxToken::Type::RBrace)
			break; // reached namespace end

		// unknown tokens found; skip
		ParseUnknown(subNode.get(), position);
	}

	// skip past final "right brace" (})
	position.Increment();

	parent->AddNode(subNode.release());

	return true;

}

bool ASTCxxParser::ParseUsing(ASTNode* parent, ASTPosition& cposition)
{
	// TODO: support type aliasing (c++11 feature) http://en.cppreference.com/w/cpp/language/type_alias

	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != CxxToken::Type::Using)
		return false;

	// create tree
	std::unique_ptr<ASTDataNode> subNode(new ASTDataNode());
	if (position.GetNextToken().TokenType == CxxToken::Type::Namespace)
	{
		subNode->SetType(ASTNode::Type::NamespaceUsing);
		position.Increment();
	}
	else
	{
		subNode->SetType(ASTNode::Type::Using);
	}

	// check whether using <namespace> is followed by a keyword or a double colon.
	if (position.GetToken().TokenType != CxxToken::Type::Keyword && position.GetToken().TokenType != CxxToken::Type::Doublecolon)
		return false;

	do
	{
		if (position.GetToken().TokenType == CxxToken::Type::Doublecolon)
		{
			subNode->data.push_back(position.GetToken().TokenData);
			position.Increment();
		}

		subNode->data.push_back(position.GetToken().TokenData);
		position.Increment();

	} while (position.GetToken().TokenType == CxxToken::Type::Doublecolon);

	if (position.GetToken().TokenType != CxxToken::Type::Semicolon)
		throw std::runtime_error("expected semicolon to finish using namespace declaration (using namespace <definition>;)");

	parent->AddNode(subNode.release());
	cposition = position;
	return true;

}

bool ASTCxxParser::ParseFriend(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != CxxToken::Type::Friend)
		return false;

	position.Increment();
	// create tree
	std::unique_ptr<ASTType> subNode(new ASTType(this));
	subNode->SetType(ASTNode::Type::Friend);

	ASTDeclarationParsingOptions opts;
	ParseDeclarationHead(subNode.get(), position, subNode.get(), opts);
	ParseDeclarationSub(subNode.get(), position, subNode.get(), 0, opts);

	if (position.GetToken().TokenType != CxxToken::Type::Semicolon)
		return false;
	position.Increment();

	parent->AddNode(subNode.release());
	cposition = position;
	return true;

}

bool ASTCxxParser::ParseTypedef(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition& position = cposition;

	if (position.GetToken().TokenType != CxxToken::Type::Typedef)
		return false;
	position.Increment();

	// create tree
	std::unique_ptr<ASTNode> subNode(new ASTNode());
	subNode->SetType(ASTNode::Type::Typedef);

	bool skipHeadSub = false;
	if (position.GetToken().TokenType == CxxToken::Type::Struct || position.GetToken().TokenType == CxxToken::Type::Class || position.GetToken().TokenType == CxxToken::Type::Union)
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
		subType->SetType(ASTNode::Type::TypedefHead);
		// parse HEAD
		if (ParseDeclarationHead(parent, position, subType.get(), opts) == false)
			return false;

		// parse SUB, SUB, SUB, ...
		while (true)
		{
			std::unique_ptr<ASTType> subType2(new ASTType(this));
			subType2->SetType(ASTNode::Type::TypedefSub);
			if (ParseDeclarationSub(parent, position, subType2.get(), subType.get(), opts) == false)
				return false;

			subType->AddNode(subType2.release());

			if (position.GetToken().TokenType != CxxToken::Type::Comma)
				break;
			else
				position.Increment();
		}

		subNode->AddNode(subType.release());

		if (position.GetToken().TokenType != CxxToken::Type::Semicolon)
			throw std::runtime_error("expected semicolon to finish typedef declaration (typedef <declaration>;)");
	}





	parent->AddNode(subNode.release());
	cposition = position;
	return true;

}

bool ASTCxxParser::ParsePreprocessor(ASTNode* parent, ASTPosition& position)
{
	// this is officially not a part of C/C++, however we parse it anyway

	if (position.GetToken().TokenType == CxxToken::Type::Hash)
	{
		std::vector<ASTTokenIndex> preprocessorTokens;

		// parse until newline or end of stream and store tokens
		while (true)
		{
			if (position.GetToken().TokenType == CxxToken::Type::Newline || position.GetToken().TokenType == CxxToken::Type::EndOfStream)
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

bool ASTCxxParser::ParseEnum(ASTNode* parent, ASTPosition& position)
{
	if (position.GetToken().TokenType != CxxToken::Type::Enum)
		return false;

	position.Increment();

	std::unique_ptr<ASTTokenNode> subNode(new ASTTokenNode(this));
	subNode->SetType(ASTNode::Type::Enum);

	if (position.GetToken().TokenType == CxxToken::Type::Class)
	{
		// c++11 enum class
		subNode->SetType(ASTNode::Type::EnumClass);
		position.Increment();
	}

	if (position.GetToken().TokenType == CxxToken::Type::Keyword)
	{
		subNode->Tokens.push_back(position.GetTokenIndex());
		position.Increment();
	}

	if (position.GetToken().TokenType != CxxToken::Type::LBrace)
		throw std::runtime_error("expected left brace during enum parse");

	position.Increment();

	while (true)
	{
		if (ParseEnumDefinition(subNode.get(), position))
		{

		}
		else if (position.GetToken().TokenType == CxxToken::Type::RBrace)
			break;
		else
			position.Increment();
	}

	if (position.GetNextToken().TokenType != CxxToken::Type::Semicolon)
		throw std::runtime_error("expected semicolon after enum closing brace");

	position.Increment();

	// store subnode
	parent->AddNode(subNode.release());
	return true;
}

bool ASTCxxParser::ParseEnumDefinition(ASTNode* parent, ASTPosition& position)
{
	if (position.GetToken().TokenType != CxxToken::Type::Keyword)
		return false;

	std::unique_ptr < ASTTokenNode> subNode(new ASTTokenNode(this));
	subNode->SetType(ASTNode::Type::EnumDef);

	// store token
	subNode->Tokens.push_back(position.GetTokenIndex());
	position.Increment();

	if (position.GetToken().TokenType == CxxToken::Type::Equals)
	{
		position.Increment();

		// add the rest
		// add initialization clause
		std::unique_ptr < ASTDataNode> subSubNode(new ASTDataNode());
		subSubNode->SetType(ASTNode::Type::Init);

		subSubNode->data.push_back(CombineWhile_ScopeAware(position, [](ASTPosition& position) { return position.GetToken().TokenType != CxxToken::Type::Comma && position.GetToken().TokenType != CxxToken::Type::RBrace && position.GetToken().TokenType != CxxToken::Type::Semicolon; }, &ASTCxxParser::ASTPosition::FilterComments));
		subNode->AddNode(subSubNode.release());
	}
	// store subnode
	parent->AddNode(subNode.release());
	return true;
}

bool ASTCxxParser::ParseIgnored(ASTNode* parent, ASTPosition& position)
{
	auto& token = position.GetToken();
	if (token.TokenType == CxxToken::Type::CommentMultiLine || token.TokenType == CxxToken::Type::CommentSingleLine || token.TokenType == CxxToken::Type::Whitespace || token.TokenType == CxxToken::Type::Newline)
	{
		position.Increment();
		return true;
	}
	return false;
}

bool ASTCxxParser::ParseUnknown(ASTNode* parent, ASTPosition& position)
{
	if (Verbose)
		fprintf(stderr, "[PARSER] no grammar match for token: %d (type: %d, line: %d): %s\n", static_cast<int>(position.Position), position.GetToken().TokenType, static_cast<int>(position.GetToken().TokenLine), position.GetToken().TokenData.c_str());
	position.Increment();
	return false;
}

bool ASTCxxParser::ParseEndOfStream(ASTNode* parent, ASTPosition& position)
{
	if (position.GetToken().TokenType == CxxToken::Type::EndOfStream)
		return true;

	return false;
}

bool ASTCxxParser::ParseClassInheritance(int &inheritancePublicPrivateProtected, ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition position = cposition;
	if (position.GetToken().TokenType == CxxToken::Type::Public)
	{
		inheritancePublicPrivateProtected = 0;
		position.Increment();
	}
	else if (position.GetToken().TokenType == CxxToken::Type::Private)
	{
		inheritancePublicPrivateProtected = 1;
		position.Increment();
	}
	else if (position.GetToken().TokenType == CxxToken::Type::Protected)
	{
		inheritancePublicPrivateProtected = 2;
		position.Increment();
	}

	if (position.GetToken().TokenType != CxxToken::Type::Keyword && position.GetToken().TokenType != CxxToken::Type::Doublecolon)
		return false;


	// parse class/struct/union name (including namespaces)
	std::unique_ptr<ASTDataNode> subNode(new ASTDataNode());
	std::unique_ptr<ASTType> subType(new ASTType(this));
	ASTDeclarationParsingOptions opts;
	if (ParseDeclarationHead(subNode.get(), position, subType.get(), opts) == false)
		return false;
	subType->SetType(ASTNode::Type::Parent);
	subNode->AddNode(subType.release());

	// create subnode
	subNode->SetType(ASTNode::Type::Inherit);
	if (inheritancePublicPrivateProtected == 0)
		subNode->data.push_back("public");
	else if (inheritancePublicPrivateProtected == 1)
		subNode->data.push_back("private");
	else if (inheritancePublicPrivateProtected == 2)
		subNode->data.push_back("protected");


	parent->AddNode(subNode.release());
	cposition = position;
	return true;
}

bool ASTCxxParser::ParsePrivatePublicProtected(int& privatePublicProtected, ASTPosition& cposition)
{
	ASTPosition position = cposition;

	if (position.GetToken().TokenType == CxxToken::Type::Private)
	{
		if (position.GetNextToken().TokenType == CxxToken::Type::Colon)
		{
			privatePublicProtected = 0;
			position.Increment();
			cposition = position;
			return true;
		}
	}

	if (position.GetToken().TokenType == CxxToken::Type::Public)
	{
		if (position.GetNextToken().TokenType == CxxToken::Type::Colon)
		{
			privatePublicProtected = 1;
			position.Increment();
			cposition = position;
			return true;
		}
	}

	if (position.GetToken().TokenType == CxxToken::Type::Protected)
	{
		if (position.GetNextToken().TokenType == CxxToken::Type::Colon)
		{
			privatePublicProtected = 2;
			position.Increment();
			cposition = position;
			return true;
		}
	}

	return false;
}

bool ASTCxxParser::ParseConstructorInitializer(ASTType* parent, ASTPosition& cposition)
{
	std::unique_ptr<ASTNode> ndRoot(new ASTNode());
	ndRoot->SetType(ASTNode::Type::CInitFuncDcl);

	ASTPosition position = cposition;
	std::string name = "";

	// find keyword
	if (position.GetToken().TokenType != CxxToken::Type::Keyword)
		return false;

	// store name and increment
	std::unique_ptr<ASTTokenNode> ndVar(new ASTTokenNode(this));
	ndVar->SetType(ASTNode::Type::CInitVar);
	ndVar->Tokens.push_back(position.GetTokenIndex());
	position.Increment();

	// find left parenthesis
	if (position.GetToken().TokenType != CxxToken::Type::LParen)
		return false;

	std::unique_ptr<ASTTokenNode> ndSet(new ASTTokenNode(this));
	ndSet->SetType(ASTNode::Type::CInitSet);
	if (ParseSpecificScopeInner(position, ndSet->Tokens, CxxToken::Type::LParen, CxxToken::Type::RParen, &ASTPosition::FilterComments) == false)
		return false;

	position.Increment(); // skip rparen

	// advance position to current and return success
	ndRoot->AddNode(ndVar.release());
	ndRoot->AddNode(ndSet.release());
	parent->AddNode(ndRoot.release());
	cposition = position;
	return true;
}

bool ASTCxxParser::ParseDeclarationHead(ASTNode* parent, ASTPosition& cposition, ASTType* type, ASTDeclarationParsingOptions opts)
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

	// parse final modifier tokens if present
	while (ParseModifierToken(position, type->typeModifiers)) {}

	// apply type and return true
	type->typeName = tempType.typeName;
	type->typeModifiers = tempType.typeModifiers;
	type->StealNodesFrom(&tempType);
	cposition = position;
	return true;
}

bool ASTCxxParser::ParseDeclarationSub(ASTNode* parent, ASTPosition& cposition, ASTType* type, ASTType* headType, ASTDeclarationParsingOptions opts)
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

		if (ParseDeclarationSubArgumentsScoped(position, argNode.get(), CxxToken::Type::LParen, CxxToken::Type::RParen))
		{
			argNode->SetType(ASTNode::Type::FuncPtrArgDcl);
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
	if (position.GetToken().TokenType == CxxToken::Type::LParen)
	{
		std::unique_ptr<ASTNode> argNode(new ASTNode());

		if (ParseDeclarationSubArgumentsScoped(position, argNode.get(), CxxToken::Type::LParen, CxxToken::Type::RParen) == false)
		{
			// parsing failed
			if (opts.AllowCtor == false)
				return false; // if it's impossible to be a ctor (such as in class/struct/union scope, or function argument scope), this is invalid.

			argNode->SetType(ASTNode::Type::CtorArgsDcl);
			if (ParseConstructorArguments(argNode.get(), position) == false)
				return false;
		}
		else
		{
			argNode->SetType(ASTNode::Type::FuncArgDcl);
			type->ndFuncArgumentList = argNode.get();
		}

		type->AddNode(argNode.release());

		parsedArguments = true;

	}

	// parse bitfield if allowed
	if (opts.AllowBitfield && position.GetToken().TokenType == CxxToken::Type::Colon && parsedArguments == false)
	{
		position.Increment(); // skip Colon

		// parse 
		Parse_ScopeAware(position,
			[](ASTPosition& pos) { return pos.GetToken().TokenType != CxxToken::Type::Comma &&  pos.GetToken().TokenType != CxxToken::Type::Semicolon;  },
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
			funcModifierList->SetType(ASTNode::Type::FuncModDcl);
			type->ndFuncModifierList = funcModifierList.get();

			// fill modifier data
			for (size_t i = 0; i < funcModifiers.size(); i++)
			{
				std::unique_ptr<ASTTokenNode> funcModifier(new ASTTokenNode(this));
				funcModifier->SetType(ASTNode::Type::Modifier);

				std::string modifierData;
				for (size_t j = funcModifiers[i].first; j <= funcModifiers[i].second; j++)
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
	if (position.GetToken().TokenType == CxxToken::Type::Equals)
	{
		position.Increment();

		ASTTokenNode* subNode = new ASTTokenNode(this);
		subNode->SetType(ASTNode::Type::Init);

		ParseToArray_ScopeAware(subNode->Tokens, position,
			[](ASTPosition& position) { return position.GetToken().TokenType != CxxToken::Type::Semicolon && position.GetToken().TokenType != CxxToken::Type::Comma; },
			&ASTCxxParser::ASTPosition::FilterComments);

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

bool ASTCxxParser::ParseDeclaration(ASTNode* parent, ASTPosition& cposition, ASTDeclarationParsingOptions opts)
{
	// parse
	// HEAD
	// then SUB,SUB,SUB,... until
	// ; or { FUNCTION_DEFINITION } or : CONSTRUCTOR_INITIALIZER { FUNCTION_DEFINITION }

	ASTPosition position = cposition;
	std::unique_ptr<ASTType> headType(new ASTType(this));
	size_t lastSubID = -1;

	// HEAD then SUB,SUB,SUB,...
	if (_ParseDeclaration_HeadSubs(position, parent, headType, lastSubID, opts) == false)
		return false;

	if (position.GetToken().TokenType == CxxToken::Type::Colon)
	{
		// : CONSTRUCTOR_INITIALIZER
		while (true)
		{


			if (position.GetToken().TokenType == CxxToken::Type::Colon || position.GetToken().TokenType == CxxToken::Type::Comma)
				position.Increment(); // skip past : or ,
			else if (position.GetToken().TokenType == CxxToken::Type::LBrace || position.GetToken().TokenType == CxxToken::Type::Semicolon)
				break;
			else
				throw std::runtime_error("unexpected token in function constructor initializer");

			ASTType* ndParent = 0;
			if (lastSubID == -1)
				ndParent = dynamic_cast<ASTType*>(headType.get());
			else
				ndParent = dynamic_cast<ASTType*>(headType->Children()[lastSubID]);

			if (ParseConstructorInitializer(ndParent, position) == false)
				return false;
		}

	}

	// { FUNCTION_DEFINITION }
	if (position.GetToken().TokenType == CxxToken::Type::LBrace)
	{
		// function declaration
		std::vector<ASTTokenIndex> functionDeclarationTokens;
		if (ParseSpecificScopeInner(position, functionDeclarationTokens, CxxToken::Type::LBrace, CxxToken::Type::RBrace, &ASTPosition::FilterNone) == false)
			return false;

		// continue past final RBrace
		position.Increment();

		ASTTokenNode* nd = new ASTTokenNode(this);
		nd->SetType(ASTNode::Type::FuncDcl);
		nd->Tokens.swap(functionDeclarationTokens);
		if (lastSubID == -1)
			headType->AddNode(nd);
		else
			headType->Children()[lastSubID]->AddNode(nd);

	}
	else if (position.GetToken().TokenType == CxxToken::Type::Semicolon) // ;
		position.Increment();
	else
		return false;

	// push to list
	headType->SetType(ASTNode::Type::DclHead);
	parent->AddNode(headType.release());
	cposition = position;
	return true;

}

bool ASTCxxParser::_ParseDeclaration_HeadSubs(ASTPosition &position, ASTNode* parent, std::unique_ptr<ASTType> &headType, size_t &lastSubID, ASTDeclarationParsingOptions opts)
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
		subtype->SetType(ASTNode::Type::DclSub);
		lastSubID = headType->Children().size();
		headType->AddNode(subtype.release());

		if (position.GetToken().TokenType == CxxToken::Type::Comma)
		{
			position.Increment();
			continue;
		}

		// stop iterating
		break;
	} while (true);

	return true;
}

bool ASTCxxParser::ParseSpecificScopeInner(ASTPosition& cposition, std::vector<ASTTokenIndex> &insideBracketTokens, CxxToken::Type tokenTypeL, CxxToken::Type tokenTypeR, bool(*filterAllows)(CxxToken& token) /*= &ASTPosition::FilterWhitespaceComments*/)
{
	ASTPosition position(cposition);

	if (position.GetToken().TokenType != tokenTypeL)
		return false;
	position.Increment(1, filterAllows);

	while (position.GetToken().TokenType != tokenTypeR)
	{
		if (position.GetToken().TokenType == CxxToken::Type::EndOfStream)
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

bool ASTCxxParser::ParseOperatorType(ASTNode* parent, ASTPosition& cposition, std::vector<ASTTokenIndex>& ctokens)
{
	std::vector<ASTTokenIndex> tokens;
	ASTPosition position = cposition;

	// parse operator () (special case)
	if (position.GetToken().TokenType == CxxToken::Type::LParen)
	{
		tokens.push_back(position.GetTokenIndex());
		position.Increment();

		if (position.GetToken().TokenType == CxxToken::Type::RParen)
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
		while (position.GetToken().TokenType != CxxToken::Type::EndOfStream)
		{
			if (position.GetToken().TokenType == CxxToken::Type::LParen)
				break; // start of function arguments found
			else if (position.GetToken().TokenType == CxxToken::Type::Semicolon)
				throw new std::runtime_error("semicolon found while parsing operator arguments - we must have gone too far. invalid operator?");
			else
			{
				tokens.push_back(position.GetTokenIndex());
				position.Increment(1, ASTPosition::FilterComments);
			}
		}

	}

	cposition = position;
	ctokens.swap(tokens);
	return true;

}

bool ASTCxxParser::ParseModifierToken(ASTPosition& cposition, std::vector<std::pair<ASTTokenIndex, ASTTokenIndex>>& modifierTokens)
{
	ASTPosition position = cposition;
	switch (position.GetToken().TokenType)
	{
	case CxxToken::Type::Const:
	case CxxToken::Type::Inline:
	case CxxToken::Type::GCCInline:
	case CxxToken::Type::Extern:
	case CxxToken::Type::Virtual:
	case CxxToken::Type::Volatile:
	case CxxToken::Type::Unsigned:
	case CxxToken::Type::Signed:
	case CxxToken::Type::Typename:
	case CxxToken::Type::Static:
	case CxxToken::Type::Mutable:
	case CxxToken::Type::Class:
	case CxxToken::Type::Struct:
	case CxxToken::Type::Union:
	case CxxToken::Type::Thread:
	case CxxToken::Type::GCCExtension:
	case CxxToken::Type::MSVCForceInline:
		modifierTokens.push_back(std::make_pair(position.GetTokenIndex(), position.GetTokenIndex()));
		break;
	case CxxToken::Type::MSVCDeclspec:
	case CxxToken::Type::GCCAttribute:
	case CxxToken::Type::Throw:
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

bool ASTCxxParser::ParseNTypeBase(ASTPosition &position, ASTType* typeNode)
{
	std::vector<ASTType::ASTTokenIndexTemplated>& typeTokens = typeNode->typeName;
	std::vector<std::pair<ASTTokenIndex, ASTTokenIndex> >& modifierTokens = typeNode->typeModifiers;
	size_t typeWordIndex = -1;
	while (true)
	{
		if (position.GetToken().TokenType == CxxToken::Type::Keyword || position.GetToken().TokenType == CxxToken::Type::BuiltinType || position.GetToken().TokenType == CxxToken::Type::Void)
		{
			if (typeWordIndex == -1)
			{
				ASTType::ASTTokenIndexTemplated tok = { position.GetTokenIndex(), 0 };
				typeTokens.push_back(tok);
				typeWordIndex = typeTokens.size() - 1;
			}
			else
			{
				// second occurrence of a keyword/builtintype or void keyword
				
				if (position.GetToken().TokenType == CxxToken::Type::BuiltinType)
				{
					// combined built in types
					if ((Tokens[typeTokens.back().Index].TokenData == "short" && position.GetToken().TokenData == "int") ||
						(Tokens[typeTokens.back().Index].TokenData == "long" && position.GetToken().TokenData == "int") ||
						(Tokens[typeTokens.back().Index].TokenData == "long" && position.GetToken().TokenData == "long") || 
						(Tokens[typeTokens.back().Index].TokenData == "long" && position.GetToken().TokenData == "double"))
					{
						ASTType::ASTTokenIndexTemplated tok = { position.GetTokenIndex(), 0 };
						typeTokens.push_back(tok);
						typeWordIndex = typeTokens.size() - 1;
					}
					else
						return false; // invalid type combination
				}
				else if (position.GetToken().TokenType == CxxToken::Type::Void)
					return false;
				else
					break; // this must be the variable name (two keywords not allowed in a type)

			}
		}
		else if (ParseModifierToken(position, modifierTokens))
			continue;
		else if (position.GetToken().TokenType == CxxToken::Type::LArrow && typeWordIndex != -1)
		{
			// parse template arguments
			std::unique_ptr<ASTNode> argNode(new ASTNode());
			if (ParseDeclarationSubArgumentsScopedWithNonTypes(position, argNode.get(), CxxToken::Type::LArrow, CxxToken::Type::RArrow) == false)
				return false; // it must be a pointer instead? (e.g. void (*test); );

			argNode->SetType(ASTNode::Type::TemplateArgDcl);
			typeTokens[typeWordIndex].TemplateArguments = argNode.get();
			typeNode->AddNode(argNode.release());
			continue;
		}
		else if (position.GetToken().TokenType == CxxToken::Type::Doublecolon)
		{
			ASTType::ASTTokenIndexTemplated tok = { position.GetTokenIndex(), 0 };
			typeTokens.push_back(tok);
			typeWordIndex = -1; // reset type word index
		}
		else
			return typeWordIndex != -1;
		position.Increment();
	}

	return true;
}

bool ASTCxxParser::ParseNTypeSinglePointersAndReferences(ASTPosition &cposition, ASTType* typeNode, bool identifierScope)
{
	bool isReference = false;
	ASTPosition position(cposition);
	ASTPointerType ptrData;

	CxxToken ptrToken;

	if (position.GetToken() == CxxToken::Type::Asterisk || position.GetToken() == CxxToken::Type::Ampersand)
	{
		if (position.GetToken() == CxxToken::Type::Asterisk)
			ptrData.pointerType = ASTPointerType::Type::Pointer;
		else if (position.GetToken() == CxxToken::Type::Ampersand)
			ptrData.pointerType = ASTPointerType::Type::Reference;
		ptrToken = position.GetToken();
	}
	else
	{
		return false;
	}
	position.Increment();

	// check for pointer/reference modifiers
	while (position.GetToken() == CxxToken::Type::Const || position.GetToken() == CxxToken::Type::Volatile || position.GetToken() == CxxToken::Type::Restrict || position.GetToken() == CxxToken::Type::MSVCRestrict || position.GetToken() == CxxToken::Type::GCCRestrict)
	{
		if (ptrToken == CxxToken::Type::Ampersand)
			throw std::runtime_error("modifiers (const/volatile) are not allowed on a reference");

		if (position.GetToken() == CxxToken::Type::Const) // const applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == CxxToken::Type::Volatile) // volatile applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == CxxToken::Type::Restrict) // restrict applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == CxxToken::Type::MSVCRestrict) // restrict applies to the thing to the left (so it could apply to the pointer)
			ptrData.pointerModifiers.push_back(position.GetToken());
		if (position.GetToken() == CxxToken::Type::GCCRestrict) // restrict applies to the thing to the left (so it could apply to the pointer)
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

bool ASTCxxParser::ParseNTypePointersAndReferences(ASTPosition &position, ASTType* typeNode, bool identifierScope)
{
	bool res = false;
	while (ParseNTypeSinglePointersAndReferences(position, typeNode, identifierScope))
	{
		res |= true;
	}
	return res;
}

bool ASTCxxParser::ParseNTypeArrayDefinitions(ASTPosition &position, ASTType* typeNode)
{
	// parse arrays
	bool hasArray = false;

	std::vector<ASTTokenIndex> arrayTokens;
	while (ParseSpecificScopeInner(position, arrayTokens, CxxToken::Type::LBracket, CxxToken::Type::RBracket, &ASTCxxParser::ASTPosition::FilterComments))
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

bool ASTCxxParser::ParseNTypeFunctionPointer(ASTPosition &cposition, ASTType* typeNode)
{
	ASTPosition position = cposition;

	if (position.GetToken().TokenType != CxxToken::Type::LParen)
		return false;

	position.Increment();

	bool hasPtrTokens = false;

	if (position.GetToken().TokenType == CxxToken::Type::Ampersand || position.GetToken() == CxxToken::Type::Asterisk)
	{
		// parse pointer tokens
		while (ParseNTypePointersAndReferences(position, typeNode, true)) {}
		hasPtrTokens = true;
	}
	if (position.GetToken().TokenType == CxxToken::Type::LParen)
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

	if (position.GetToken().TokenType != CxxToken::Type::RParen)
		return false;
	position.Increment();

	cposition = position;
	return true;
}

bool ASTCxxParser::ParseNTypeIdentifier(ASTPosition &cposition, ASTType* typeNode)
{
	ASTPosition position = cposition;
	std::vector<ASTTokenIndex> tokenIdent;

	// parse namespaces
	while (true)
	{
		if (position.GetToken().TokenType == CxxToken::Type::Doublecolon)
		{
			tokenIdent.push_back(position.GetTokenIndex());
			position.Increment();
		}

		// support destructors & operators
		if (position.GetToken().TokenType == CxxToken::Type::Operator)
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
		else if (position.GetToken().TokenType == CxxToken::Type::Tilde)
		{
			tokenIdent.push_back(position.GetTokenIndex());
			position.Increment();
		}

		if (position.GetToken().TokenType == CxxToken::Type::Keyword)
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

bool ASTCxxParser::ParseDeclarationSubArguments(ASTPosition &position, ASTNode* parent)
{
	while (true)
	{
		// check for ... (varargs)
		if (position.GetToken().TokenType == CxxToken::Type::DotDotDot)
		{
			std::unique_ptr<ASTNode> subNode(new ASTNode());
			subNode->SetType(ASTNode::Type::VarArgDcl);
			parent->AddNode(subNode.release());
			position.Increment();
			break;
		}

		std::unique_ptr<ASTType> subType(new ASTType(this));
		ASTDeclarationParsingOptions opts;
		if (ParseDeclarationHead(parent, position, subType.get(), opts) == false)
			return false;

		ParseDeclarationSub(parent, position, subType.get(), 0, opts);
		subType->SetType(ASTNode::Type::ArgDcl);
		parent->AddNode(subType.release());

		if (position.GetToken().TokenType == CxxToken::Type::Comma)
		{
			position.Increment();
			continue;
		}
		else break;
	}

	return true;
}

bool ASTCxxParser::ParseDeclarationSubArgumentsScoped(ASTPosition &cposition, ASTNode* parent, CxxToken::Type leftScope, CxxToken::Type rightScope)
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

bool ASTCxxParser::ParseDeclarationSubArgumentsScopedWithNonTypes(ASTPosition &cposition, ASTNode* parent, CxxToken::Type leftScope, CxxToken::Type rightScope)
{
	ASTPosition position = cposition;
	if (position.GetToken().TokenType != leftScope)
		return false;
	position.Increment();

	while (true)
	{
		// check for ... (varargs)
		if (position.GetToken().TokenType == CxxToken::Type::DotDotDot)
		{
			std::unique_ptr<ASTNode> subNode(new ASTNode());
			subNode->SetType(ASTNode::Type::VarArgDcl);
			parent->AddNode(subNode.release());
			position.Increment();
			break;
		}

		std::unique_ptr<ASTType> subType(new ASTType(this));
		ASTDeclarationParsingOptions opts;
		if (ParseDeclarationHead(parent, position, subType.get(), opts))
		{
			ParseDeclarationSub(parent, position, subType.get(), 0, opts);
			subType->SetType(ASTNode::Type::ArgDcl);
			parent->AddNode(subType.release());
		}
		else
		{
			std::unique_ptr<ASTTokenNode> tokNode(new ASTTokenNode(this));
			tokNode->SetType(ASTNode::Type::ArgNonTypeDcl);
			Parse_ScopeAware(position,
				[rightScope](ASTPosition& p) { return p.GetToken().TokenType != CxxToken::Type::Comma && p.GetToken().TokenType != rightScope; },
				[&tokNode](ASTPosition& p) { tokNode->Tokens.push_back(p.GetTokenIndex()); },
				&ASTCxxParser::ASTPosition::FilterComments);
			parent->AddNode(tokNode.release());
		}

		if (position.GetToken().TokenType == CxxToken::Type::Comma)
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

bool ASTCxxParser::ParseArgumentAttribute(ASTPosition &position, std::pair<ASTTokenIndex, ASTTokenIndex>& outTokenStream)
{
	ASTTokenIndex idxStart = position.GetTokenIndex();
	if (position.GetToken().TokenType != CxxToken::Type::GCCAttribute
		&& position.GetToken().TokenType != CxxToken::Type::MSVCDeclspec
		&& position.GetToken().TokenType != CxxToken::Type::GCCAssembly
		&& position.GetToken().TokenType != CxxToken::Type::Throw)
		return false;

	position.Increment();

	std::vector<ASTTokenIndex> vtokens;
	if (ParseSpecificScopeInner(position, vtokens, CxxToken::Type::LParen, CxxToken::Type::RParen, ASTPosition::FilterNone) == false)
		return false;

	// continue past final RParen
	position.Increment();

	outTokenStream = std::make_pair(idxStart, vtokens.back() + 1);
	return true;
}

void ASTCxxParser::ParseBOM(ASTPosition &position)
{
	// check for byte order marks
	if (position.GetToken().TokenType == CxxToken::Type::BOM_UTF8)
	{
		fprintf(stderr, "[PARSER] File contains UTF-8 byte order mark.\n");
		IsUTF8 = true;
		position.Increment();
	}
}

bool ASTCxxParser::ParseConstructorArguments(ASTNode* parent, ASTPosition &position)
{
	position.Increment(); // skip first lparen

	// parse ctor args
	do
	{
		ASTTokenNode* tokNode = new ASTTokenNode(this);
		tokNode->SetType(ASTNode::Type::CtorArgDcl);
		Parse_ScopeAware(position,
			[](ASTPosition& p) { return p.GetToken().TokenType != CxxToken::Type::Comma && p.GetToken().TokenType != CxxToken::Type::RParen; },
			[tokNode](ASTPosition& p) { tokNode->Tokens.push_back(p.GetTokenIndex()); },
			&ASTCxxParser::ASTPosition::FilterComments);
		parent->AddNode(tokNode);
		if (position.GetToken().TokenType == CxxToken::Type::Comma)
		{
			position.Increment();
			continue; // reloop on comma
		}
		break;
	} while (true); // escape otherwise

	// sanity check
	if (position.GetToken().TokenType != CxxToken::Type::RParen)
		return false;

	// skip RParen
	position.Increment();
	return true;
}

bool ASTCxxParser::ParseExtensionAnnotation(ASTNode* parent, ASTPosition& cposition)
{
	ASTPosition position = cposition;
	auto annotationType = position.GetToken().TokenType;
	if (annotationType == CxxToken::Type::AnnotationForwardStart || annotationType == CxxToken::Type::AnnotationBackStart)
	{
		position.Increment();

		while (true)
		{
			std::unique_ptr<ASTTokenNode> ndAnnotationRoot(new ASTTokenNode(this));

			if (annotationType == CxxToken::Type::AnnotationForwardStart)
				ndAnnotationRoot->SetType(ASTNode::Type::AntFwd);
			else
				ndAnnotationRoot->SetType(ASTNode::Type::AntBack);

			if (ParseExtensionAnnotationContent(ndAnnotationRoot.get(), position) == false)
				return false;

			// add to root
			parent->AddNode(ndAnnotationRoot.release());

			if (position.GetToken().TokenType == CxxToken::Type::Comma)
			{
				// another annotation incoming 
				position.Increment();
				continue;
			}
			break;
		}

		if (position.GetToken().TokenType != CxxToken::Type::RBracket)
			return false;


	}
	else
		return false;

	cposition = position;
	return true;
}

bool ASTCxxParser::ParseExtensionAnnotationContent(ASTTokenNode* ndAnnotationRoot, ASTPosition &cposition)
{
	ASTPosition position = cposition;
	CxxToken::Type annotationArgScopeOpen = CxxToken::Type::LParen;
	CxxToken::Type annotationArgScopeClose = CxxToken::Type::RParen;

	if (position.GetToken().TokenType != CxxToken::Type::Keyword)
		return false;

	// store annotation name
	ndAnnotationRoot->Tokens.push_back(position.GetTokenIndex());
	position.Increment();

	if (position.GetToken().TokenType == annotationArgScopeOpen)
	{
		position.Increment();

		// parse arguments
		std::unique_ptr<ASTNode> ndAnnotationArguments(new ASTNode());
		ndAnnotationArguments->SetType(ASTNode::Type::AntArgs);

		while (true)
		{
			std::unique_ptr<ASTTokenNode> ndAnnotationArgument(new ASTTokenNode(this));
			ndAnnotationArgument->SetType(ASTNode::Type::AntArg);

			ParseToArray_ScopeAware(ndAnnotationArgument->Tokens, position, [annotationArgScopeClose](ASTPosition& p) { return p.GetToken().TokenType != CxxToken::Type::Comma && p.GetToken().TokenType != annotationArgScopeClose; });

			// add to list
			ndAnnotationArguments->AddNode(ndAnnotationArgument.release());

			if (position.GetToken().TokenType != CxxToken::Type::Comma)
				break;
			position.Increment(); // skip comma


		}

		// add to annotation root
		ndAnnotationRoot->AddNode(ndAnnotationArguments.release());

		// we should be at annotation terminator now
		if (position.GetToken().TokenType != annotationArgScopeClose)
			return false;

		position.Increment();
	}

	cposition = position;
	return true;
}

#pragma region ASTPosition
void ASTCxxParser::ASTPosition::Increment(int count/*=1*/, bool(*filterAllows)(CxxToken& token)/*=0*/)
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

ASTCxxParser::ASTPosition::ASTPosition(ASTCxxParser& parser) : Parser(parser)
{
	Position = 0;
}

CxxToken& ASTCxxParser::ASTPosition::GetNextToken()
{
	Increment();

	return Parser.Tokens[Position];
}

CxxToken& ASTCxxParser::ASTPosition::GetToken()
{
	return Parser.Tokens[Position];
}

#pragma endregion
