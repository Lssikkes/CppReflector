#include "tokenizer.h"
#include <stdexcept>
#include <map>
#include "tools.h"

Token Tokenizer::PeekNextToken(int& offset)
{
	Token token;
	auto dt = PeekBytes(1, offset);
	token.TokenData = std::string(dt.data, dt.length);
	token.TokenByteOffset = m_offset + offset;
	offset += token.TokenData.size();

	if(token.TokenData.size() == 0)
		token.TokenType = Token::Type::EndOfStream;
	else if (token.TokenData.at(0) == ' ' || token.TokenData.at(0) == '\t')
	{
		token.TokenType = Token::Type::Whitespace;
		Data next;
		while (true)
		{
			next = PeekBytes(1, offset);
			if (next == " " || next == "\t")
				AddPart(token, next, offset);
			else
				break;
		}
	}	
	else if(token.TokenData.at(0) == '\n')
		token.TokenType = Token::Type::Newline;
	else if(token.TokenData.at(0) == '[')
		token.TokenType = Token::Type::LBracket;
	else if(token.TokenData.at(0) == ']')
		token.TokenType = Token::Type::RBracket;
	else if (token.TokenData.at(0) == '|')
		token.TokenType = Token::Type::Pipe;
	else if (token.TokenData.at(0) == '%')
		token.TokenType = Token::Type::Percent;
	else if(token.TokenData.at(0) == '{')
		token.TokenType = Token::Type::LBrace;
	else if(token.TokenData.at(0) == '}')
		token.TokenType = Token::Type::RBrace;
	else if (token.TokenData.at(0) == '+')
		token.TokenType = Token::Type::Plus;
	else if (token.TokenData.at(0) == '-')
		token.TokenType = Token::Type::Minus;
	else if(token.TokenData.at(0) == '(')
		token.TokenType = Token::Type::LParen;
	else if(token.TokenData.at(0) == ')')
		token.TokenType = Token::Type::RParen;
	else if(token.TokenData.at(0) == '<')
		token.TokenType = Token::Type::LArrow;
	else if(token.TokenData.at(0) == '>')
		token.TokenType = Token::Type::RArrow;
	else if(token.TokenData.at(0) == ';')
		token.TokenType = Token::Type::Semicolon;
	else if(token.TokenData.at(0) == '!')
		token.TokenType = Token::Type::Exclamation;
	else if(token.TokenData.at(0) == '*')
		token.TokenType = Token::Type::Asterisk;
	else if(token.TokenData.at(0) == '&')
		token.TokenType = Token::Type::Ampersand;
	else if (token.TokenData.at(0) == '~')
		token.TokenType = Token::Type::Tilde;
	else if(token.TokenData.at(0) == '#')
		token.TokenType = Token::Type::Hash;
	else if(token.TokenData.at(0) == '=')
		token.TokenType = Token::Type::Equals;
	else if (token.TokenData.at(0) == '.')
	{
		auto next = PeekBytes(2, offset);
		if (next == "..")
		{
			token.TokenType = Token::Type::DotDotDot;
			AddPart(token, next, offset);
		}
		else 
			token.TokenType = Token::Type::Dot;
	}
	else if(token.TokenData.at(0) == ',')
		token.TokenType = Token::Type::Comma;
	else if(token.TokenData.at(0) == '/')
	{
		// COMMENTS
		auto next = PeekBytes(1, offset);
		if(next == "*")
		{
			token.TokenType = Token::Type::CommentMultiLine;

			offset = AddPart(token, next, offset);

			int counter = 1;
			auto next = PeekBytes(2, offset);
			while(true)
			{
				if(next == "/*")
					counter++;
				if(next == "*/")
				{
					counter--;
					if(counter == 0)
					{
						offset = AddPart(token, next, offset);
						break;
					}
				}

				if(next.size() == 0)
					throw std::runtime_error("end of file reached while parsing multi line comment");
				
				next.length = 1;
				offset = AddPart(token, next, offset);

				// continue
				next = PeekBytes(2, offset);
			}
		}
		else if(next == "/")
		{
			token.TokenType = Token::Type::CommentSingleLine;

			offset = AddPart(token, next, offset);

			auto next = PeekBytes(1, offset);
			while(next.size() > 0 && next.at(0) != '\n' && next.at(0) != '\r' )
			{
				offset = AddPart(token, next, offset); // add part
				next = PeekBytes(1, offset);
			}
		}
		else 
			token.TokenType = Token::Type::Slash;
	}
	else if(token.TokenData.at(0) == '\"' || token.TokenData.at(0) == '\'')
	{
		// STRINGS
		if(token.TokenData.at(0) == '\"')
			token.TokenType = Token::Type::String;
		else 
			token.TokenType = Token::Type::CharConstant;

		auto next = PeekBytes(1, offset);
		while(true)
		{
			offset = AddPart(token, next, offset);

			if(next == "\\")
			{
				next = PeekBytes(1, offset);
				offset = AddPart(token, next, offset);
				if(next == "\"")
				{
					token.TokenParsedData += "\"";
				}
				else if(next == "'")
				{
					token.TokenParsedData += "'";
				}
				else if(next == "\\")
				{
					token.TokenParsedData += "\\";
				}
				else if(next == "n")
				{
					token.TokenParsedData += "\n";
				}
				else if(next == "t")
				{
					token.TokenParsedData += "\t";
				}
				else if(next == "r")
				{
					token.TokenParsedData += "\r";
				}
				else
				{
					// unknown escaped character
					token.TokenParsedData += "?";
				}
			}
			else if(next == token.TokenData.substr(0, 1).c_str())
				break; // end of string
			else if(next == "")
				throw std::runtime_error("end of line reached while parsing string/character sequence");
			else
			{
				token.TokenParsedData += next.str();
			}

			next = PeekBytes(1, offset);
		}
	}
	else if((token.TokenData.at(0) >= 'a' && token.TokenData.at(0) <= 'z') || (token.TokenData.at(0) >= 'A' && token.TokenData.at(0) <= 'Z') || token.TokenData.at(0) == '_' )
	{
		token.TokenType = Token::Type::Keyword;

		auto next = PeekBytes(1, offset);
		while(next.size() > 0 && ((next.at(0) >= 'a' && next.at(0) <= 'z') || (next.at(0) >= 'A' && next.at(0) <= 'Z') || (next.at(0) >= '0' && next.at(0) <= '9') || next.at(0) == '_' ||  next.at(0) == '-') )
		{
			offset = AddPart(token, next, offset);
			next = PeekBytes(1, offset);
		}

		ConvertToSpecializedKeyword(token);
	}
	else if((token.TokenData.at(0) >= '0' && token.TokenData.at(0) <= '9') )
	{
		token.TokenType = Token::Type::Number;

		auto next = PeekBytes(1, offset);

		while(next.size() > 0 && ((next.at(0) >= '0' && next.at(0) <= '9') || next.at(0) == '.') )
		{
			offset = AddPart(token, next, offset);
			next = PeekBytes(1, offset);
			if (next == "f")
			{
				offset = AddPart(token, next, offset);
				break;
			}
		}
	}
	else if(token.TokenData.at(0) == ':')
	{
		token.TokenType = Token::Type::Colon;
		auto next = PeekBytes(1, offset);
		if(next == ":")
		{
			token.TokenType = Token::Type::Doublecolon;
			offset = AddPart(token, next, offset);
		}	
	}
	else if(token.TokenData.at(0) == '\r')
	{
		token.TokenType = Token::Type::Newline;
		auto next = PeekBytes(1, offset);
		if(next == "\n")
			offset = AddPart(token, next, offset);
	}
	else if (token.TokenData.at(0) == static_cast<char>(0xEF))
	{
		// 0xEF, 0xBB, 0xBF
		char bomContinuation[] = { static_cast<char>(0xBB), static_cast<char>(0xBF), 0 };
		auto next = PeekBytes(2, offset);
		if (next == bomContinuation)
		{
			token.TokenType = Token::Type::BOM_UTF8;
			AddPart(token, next, offset);
		}
		else
			token.TokenType = Token::Type::Unknown;
	}
	else
		token.TokenType = Token::Type::Unknown;
	return token;
}

#ifdef _MSC_VER
#define threadlocal __declspec(thread)
#else
#define threadlocal __thread
#endif
threadlocal std::map<unsigned long, Token::Type> *gTokenTypes=0;
void PopulateTokenTypes()
{
	gTokenTypes = new std::map<unsigned long, Token::Type>();
	std::map<unsigned long, Token::Type> &grTokenTypes = *gTokenTypes;
	grTokenTypes[tools::crc32String("true")] = Token::Type::True;
	grTokenTypes[tools::crc32String("false")] = Token::Type::False;
	grTokenTypes[tools::crc32String("public")] = Token::Type::Public;
	grTokenTypes[tools::crc32String("protected")] = Token::Type::Protected;
	grTokenTypes[tools::crc32String("private")] = Token::Type::Private;
	grTokenTypes[tools::crc32String("class")] = Token::Type::Class;
	grTokenTypes[tools::crc32String("struct")] = Token::Type::Struct;
	grTokenTypes[tools::crc32String("union")] = Token::Type::Union;
	grTokenTypes[tools::crc32String("const")] = Token::Type::Const;
	grTokenTypes[tools::crc32String("unsigned")] = Token::Type::Unsigned;
	grTokenTypes[tools::crc32String("signed")] = Token::Type::Signed;
	grTokenTypes[tools::crc32String("null")] = Token::Type::Null;
	grTokenTypes[tools::crc32String("void")] = Token::Type::Void;
	grTokenTypes[tools::crc32String("__int64")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("bool")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("int")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("short")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("long")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("float")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("double")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("char")] = Token::Type::BuiltinType;
	grTokenTypes[tools::crc32String("undefined")] = Token::Type::Undefined;
	grTokenTypes[tools::crc32String("enum")] = Token::Type::Enum;
	grTokenTypes[tools::crc32String("virtual")] = Token::Type::Virtual;
	grTokenTypes[tools::crc32String("volatile")] = Token::Type::Volatile;
	grTokenTypes[tools::crc32String("mutable")] = Token::Type::Mutable;
	grTokenTypes[tools::crc32String("extern")] = Token::Type::Extern;
	grTokenTypes[tools::crc32String("inline")] = Token::Type::Inline;
	grTokenTypes[tools::crc32String("static")] = Token::Type::Static;
	grTokenTypes[tools::crc32String("operator")] = Token::Type::Operator;
	grTokenTypes[tools::crc32String("template")] = Token::Type::Template;
	grTokenTypes[tools::crc32String("typedef")] = Token::Type::Typedef;
	grTokenTypes[tools::crc32String("typename")] = Token::Type::Typename;
	grTokenTypes[tools::crc32String("namespace")] = Token::Type::Namespace;
	grTokenTypes[tools::crc32String("using")] = Token::Type::Using;
	grTokenTypes[tools::crc32String("friend")] = Token::Type::Friend;
	grTokenTypes[tools::crc32String("__forceinline")] = Token::Type::MSVCForceInline;
	grTokenTypes[tools::crc32String("restrict")] = Token::Type::Restrict;
	grTokenTypes[tools::crc32String("__inline")] = Token::Type::Inline;
	grTokenTypes[tools::crc32String("__declspec")] = Token::Type::MSVCDeclspec;
	grTokenTypes[tools::crc32String("__attribute__")] = Token::Type::GCCAttribute;
	grTokenTypes[tools::crc32String("__restrict")] = Token::Type::MSVCRestrict;
	grTokenTypes[tools::crc32String("__restrict__")] = Token::Type::GCCRestrict;
	grTokenTypes[tools::crc32String("__extension__")] = Token::Type::GCCExtension;

}

void Tokenizer::ConvertToSpecializedKeyword( Token& token )
{
	if (gTokenTypes == 0)
		PopulateTokenTypes();

	unsigned long hsh = tools::crc32String(token.TokenData);
	auto loc = (*gTokenTypes).find(hsh);
	if (loc != (*gTokenTypes).end())
		token.TokenType = loc->second;

}

int Tokenizer::IsCombinableWith( Token& token, Token& nextToken )
{
    if(token.TokenType == Token::Type::Unknown)
	{
		if(nextToken.TokenType == Token::Type::Unknown)
			return 2;
		return 1;
	}
	return 0;
}

Token Tokenizer::GetNextToken()
{
	int offset = 0;
	Token nextToken; 
	Token token=PeekNextToken(offset);

	// combine tokens
	if(IsCombinableWith(token,nextToken) >= 1)
	{
		int nextOffset = offset;
		nextToken = PeekNextToken(nextOffset);
		while(IsCombinableWith(token, nextToken) == 2)
		{
			offset = nextOffset;
			token.TokenData += nextToken.TokenData;
			nextToken = PeekNextToken(nextOffset);
		}
	}

	Advance(offset);
	return token;
}



Tokenizer::Data StringTokenizer::PeekBytes(int numBytes, int offset)
{
	int noffset = m_offset + offset;
	if(noffset + numBytes > Source.size())
		numBytes = Source.size() - noffset;

	if (numBytes <= 0)
	{
		Data dt = { "", 0 };
		return dt;
	}
	else
	{
		Data dt = { Source.c_str() + noffset, (size_t)numBytes};
		return dt;
	}
}

int StringTokenizer::Advance( int numBytes )
{
	if(m_offset + numBytes > Source.size())
		numBytes = Source.size() - m_offset;
	m_offset += numBytes;
	return numBytes;
}

void Tokenizer::Debug()
{
	Token nextToken;
	int line = 1;
	try
	{
		while((nextToken = GetNextToken()).TokenType != Token::Type::EndOfStream)
		{
			if(nextToken.TokenType == Token::Type::Whitespace)
				continue;
			if(nextToken.TokenType == Token::Type::Newline)
			{
				line++;
				continue;
			}

			printf("line %d: token type: %d <%s>\n", line, nextToken.TokenType, nextToken.TokenData.c_str());
		}
	}
	catch(std::exception e)
	{
		printf("error during lexing at line %d: %s", line, e.what());
	}
}
