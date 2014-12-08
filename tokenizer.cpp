#include "tokenizer.h"

Token Tokenizer::PeekNextToken(int& offset)
{
	Token token;
	token.TokenData = PeekBytes(1, offset);
	token.TokenByteOffset = m_offset + offset;
	offset += token.TokenData.size();

	if(token.TokenData.size() == 0)
		token.TokenType = Token::Type::EndOfStream;
	else if(token.TokenData.at(0) == ' ' || token.TokenData.at(0) == '\t')
		token.TokenType = Token::Type::Whitespace;
	else if(token.TokenData.at(0) == '\n')
		token.TokenType = Token::Type::Newline;
	else if(token.TokenData.at(0) == '[')
		token.TokenType = Token::Type::LBracket;
	else if(token.TokenData.at(0) == ']')
		token.TokenType = Token::Type::RBracket;
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
	else if(token.TokenData.at(0) == '.')
		token.TokenType = Token::Type::Dot;
	else if(token.TokenData.at(0) == ',')
		token.TokenType = Token::Type::Comma;
	else if(token.TokenData.at(0) == '/')
	{
		// COMMENTS
		std::string next = PeekBytes(1, offset);
		if(next == "*")
		{
			token.TokenType = Token::Type::CommentMultiLine;

			offset = AddPart(token, next, offset);

			int counter = 1;
			std::string next = PeekBytes(2, offset);
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
				
				offset = AddPart(token, next.substr(0, 1), offset);

				// continue
				next = PeekBytes(2, offset);
			}
		}
		if(next == "/")
		{
			token.TokenType = Token::Type::CommentSingleLine;

			offset = AddPart(token, next, offset);

			std::string next = PeekBytes(1, offset);
			while(next.size() > 0 && next.at(0) != '\n' && next.at(0) != '\r' )
			{
				offset = AddPart(token, next, offset); // add part
				next = PeekBytes(1, offset);
			}
		}
	}
	else if(token.TokenData.at(0) == '\"' || token.TokenData.at(0) == '\'')
	{
		// STRINGS
		if(token.TokenData.at(0) == '\"')
			token.TokenType = Token::Type::String;
		else 
			token.TokenType = Token::Type::CharConstant;

		std::string next = PeekBytes(1, offset);
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
			else if(next == token.TokenData.substr(0, 1))
				break; // end of string
			else if(next == "")
				throw std::runtime_error("end of line reached while parsing string/character sequence");
			else
			{
				token.TokenParsedData += next;
			}

			next = PeekBytes(1, offset);
		}
	}
	else if((token.TokenData.at(0) >= 'a' && token.TokenData.at(0) <= 'z') || (token.TokenData.at(0) >= 'A' && token.TokenData.at(0) <= 'Z') )
	{
		token.TokenType = Token::Type::Keyword;

		std::string next = PeekBytes(1, offset);
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

		std::string next = PeekBytes(1, offset);

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
		std::string next = PeekBytes(1, offset);
		if(next == ":")
		{
			token.TokenType = Token::Type::Doublecolon;
			offset = AddPart(token, next, offset);
		}	
	}
	else if(token.TokenData.at(0) == '\r')
	{
		token.TokenType = Token::Type::Newline;
		std::string next = PeekBytes(1, offset);
		if(next == "\n")
			offset = AddPart(token, next, offset);
	}

	else
		token.TokenType = Token::Type::Unknown;
	return token;
}


void Tokenizer::ConvertToSpecializedKeyword( Token& token )
{
	if(token.TokenData == "public")
		token.TokenType = Token::Type::Public;
	else if(token.TokenData == "protected")
		token.TokenType = Token::Type::Protected;
	else if(token.TokenData == "private")
		token.TokenType = Token::Type::Private;
	else if(token.TokenData == "class")
		token.TokenType = Token::Type::Class;
	else if(token.TokenData == "const")
		token.TokenType = Token::Type::Const;
	else if(token.TokenData == "unsigned")
		token.TokenType = Token::Type::Unsigned;
	else if(token.TokenData == "null")
		token.TokenType = Token::Type::Null;
	else if(token.TokenData == "void")
		token.TokenType = Token::Type::Void;
	else if(token.TokenData == "int")
		token.TokenType = Token::Type::BuiltinType;
	else if(token.TokenData == "short")
		token.TokenType = Token::Type::BuiltinType;
	else if(token.TokenData == "long")
		token.TokenType = Token::Type::BuiltinType;
	else if(token.TokenData == "float")
		token.TokenType = Token::Type::BuiltinType;
	else if(token.TokenData == "double")
		token.TokenType = Token::Type::BuiltinType;
	else if(token.TokenData == "char")
		token.TokenType = Token::Type::BuiltinType;
	else if(token.TokenData == "undefined")
		token.TokenType = Token::Type::Undefined;
	else if(token.TokenData == "enum")
		token.TokenType = Token::Type::Enum;
	else if(token.TokenData == "virtual")
		token.TokenType = Token::Type::Virtual;
	else if(token.TokenData == "volatile")
		token.TokenType = Token::Type::Volatile;
	else if(token.TokenData == "mutable")
		token.TokenType = Token::Type::Mutable;
	else if(token.TokenData == "extern")
		token.TokenType = Token::Type::Extern;
	else if(token.TokenData == "inline")
		token.TokenType = Token::Type::Inline;
	else if (token.TokenData == "static")
		token.TokenType = Token::Type::Static;
	else if (token.TokenData == "operator")
		token.TokenType = Token::Type::Operator;
	else if (token.TokenData == "template")
		token.TokenType = Token::Type::Template;
	else if (token.TokenData == "typedef")
		token.TokenType = Token::Type::Typedef;
	else if (token.TokenData == "typename")
		token.TokenType = Token::Type::Typename;
}


int Tokenizer::IsCombinableWith( Token& token, Token& nextToken )
{
	if(token.TokenType == Token::Type::Whitespace)
	{
		if(nextToken.TokenType == Token::Type::Whitespace)
			return 2;
		return 1;
	}
	else if(token.TokenType == Token::Type::Unknown)
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

int Tokenizer::AddPart( Token &token, std::string &next, int& offset )
{
	token.TokenData += next;
	offset += next.size();	
	return offset;
}

std::string StringTokenizer::PeekBytes( int numBytes, int offset)
{
	int noffset = m_offset + offset;
	if(noffset + numBytes > Source.size())
		numBytes = Source.size() - noffset;

	if(numBytes <= 0)
		return "";

	return Source.substr(noffset, numBytes);
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
