#include "cxxTokenizer.h"
#include <stdexcept>
#include <map>
#include "tools.h"

CxxToken CxxTokenizer::PeekNextToken(size_t& offset)
{
	CxxToken token;
	auto dt = PeekBytes(1, offset);
	token.TokenData = std::string(dt.data, dt.length);
	token.TokenByteOffset = m_offset + offset;
	offset += token.TokenData.size();

	if(token.TokenData.size() == 0)
		token.TokenType = CxxToken::Type::EndOfStream;
	else if (token.TokenData.at(0) == ' ' || token.TokenData.at(0) == '\t')
	{
		token.TokenType = CxxToken::Type::Whitespace;
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
		token.TokenType = CxxToken::Type::Newline;
	else if(token.TokenData.at(0) == '[')
		token.TokenType = CxxToken::Type::LBracket;
	else if(token.TokenData.at(0) == ']')
		token.TokenType = CxxToken::Type::RBracket;
	else if (token.TokenData.at(0) == '|')
		token.TokenType = CxxToken::Type::Pipe;
	else if (token.TokenData.at(0) == '%')
		token.TokenType = CxxToken::Type::Percent;
	else if(token.TokenData.at(0) == '{')
		token.TokenType = CxxToken::Type::LBrace;
	else if(token.TokenData.at(0) == '}')
		token.TokenType = CxxToken::Type::RBrace;
	else if (token.TokenData.at(0) == '+')
		token.TokenType = CxxToken::Type::Plus;
	else if (token.TokenData.at(0) == '-')
		token.TokenType = CxxToken::Type::Minus;
	else if(token.TokenData.at(0) == '(')
		token.TokenType = CxxToken::Type::LParen;
	else if(token.TokenData.at(0) == ')')
		token.TokenType = CxxToken::Type::RParen;
	else if(token.TokenData.at(0) == '<')
		token.TokenType = CxxToken::Type::LArrow;
	else if(token.TokenData.at(0) == '>')
		token.TokenType = CxxToken::Type::RArrow;
	else if(token.TokenData.at(0) == ';')
		token.TokenType = CxxToken::Type::Semicolon;
	else if(token.TokenData.at(0) == '!')
		token.TokenType = CxxToken::Type::Exclamation;
	else if(token.TokenData.at(0) == '*')
		token.TokenType = CxxToken::Type::Asterisk;
	else if(token.TokenData.at(0) == '&')
		token.TokenType = CxxToken::Type::Ampersand;
	else if (token.TokenData.at(0) == '~')
		token.TokenType = CxxToken::Type::Tilde;
	else if(token.TokenData.at(0) == '#')
		token.TokenType = CxxToken::Type::Hash;
	else if(token.TokenData.at(0) == '=')
		token.TokenType = CxxToken::Type::Equals;
	else if (token.TokenData.at(0) == '.')
	{
		auto next = PeekBytes(2, offset);
		if (next == "..")
		{
			token.TokenType = CxxToken::Type::DotDotDot;
			AddPart(token, next, offset);
		}
		else 
			token.TokenType = CxxToken::Type::Dot;
	}
	else if(token.TokenData.at(0) == ',')
		token.TokenType = CxxToken::Type::Comma;
	else if(token.TokenData.at(0) == '/')
	{
		// COMMENTS
		auto next = PeekBytes(1, offset);
		if(next == "*")
		{
			token.TokenType = CxxToken::Type::CommentMultiLine;

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
			offset = AddPart(token, next, offset);
			auto annotationPrefix = PeekBytes(2, offset);
			if (WithAnnotations && annotationPrefix == "@[")
			{
				token.TokenType = CxxToken::Type::AnnotationForwardStart;
				offset = AddPart(token, annotationPrefix, offset);
			}
			else if (WithAnnotations && annotationPrefix == "@<" && PeekBytes(1, offset + 2) == "[")
			{
				annotationPrefix = PeekBytes(3, offset);
				token.TokenType = CxxToken::Type::AnnotationBackStart;
				offset = AddPart(token, annotationPrefix, offset);
			}
			else
			{
				token.TokenType = CxxToken::Type::CommentSingleLine;

				auto next = PeekBytes(1, offset);
				while (next.size() > 0 && next.at(0) != '\n' && next.at(0) != '\r')
				{
					offset = AddPart(token, next, offset); // add part
					next = PeekBytes(1, offset);
				}
			}
		}
		else 
			token.TokenType = CxxToken::Type::Slash;
	}
	else if(token.TokenData.at(0) == '\"' || token.TokenData.at(0) == '\'')
	{
		// STRINGS
		if(token.TokenData.at(0) == '\"')
			token.TokenType = CxxToken::Type::String;
		else 
			token.TokenType = CxxToken::Type::CharConstant;

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
		token.TokenType = CxxToken::Type::Keyword;

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
		token.TokenType = CxxToken::Type::Number;

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
		token.TokenType = CxxToken::Type::Colon;
		auto next = PeekBytes(1, offset);
		if(next == ":")
		{
			token.TokenType = CxxToken::Type::Doublecolon;
			offset = AddPart(token, next, offset);
		}	
	}
	else if(token.TokenData.at(0) == '\r')
	{
		token.TokenType = CxxToken::Type::Newline;
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
			token.TokenType = CxxToken::Type::BOM_UTF8;
			AddPart(token, next, offset);
		}
		else
			token.TokenType = CxxToken::Type::Unknown;
	}
	else
		token.TokenType = CxxToken::Type::Unknown;
	return token;
}

std::map<unsigned long, CxxToken::Type> gTokenTypes;
std::map<CxxToken::Type, std::string> gTokenStrings;

// static initialization to ensure this happens first
namespace
{
	class TokenInitializer
	{
	public:
		TokenInitializer()
		{
			AddPair("true", CxxToken::Type::True);
			AddPair("false", CxxToken::Type::False);
			AddPair("public", CxxToken::Type::Public);
			AddPair("protected", CxxToken::Type::Protected);
			AddPair("private", CxxToken::Type::Private);
			AddPair("class", CxxToken::Type::Class);
			AddPair("struct", CxxToken::Type::Struct);
			AddPair("union", CxxToken::Type::Union);
			AddPair("const", CxxToken::Type::Const);
			AddPair("unsigned", CxxToken::Type::Unsigned);
			AddPair("signed", CxxToken::Type::Signed);
			AddPair("null", CxxToken::Type::Null);
			AddPair("nullptr", CxxToken::Type::Nullptr);
			AddPair("void", CxxToken::Type::Void);
			AddPair("__int64", CxxToken::Type::BuiltinType);
			AddPair("bool", CxxToken::Type::BuiltinType);
			AddPair("int", CxxToken::Type::BuiltinType);
			AddPair("short", CxxToken::Type::BuiltinType);
			AddPair("long", CxxToken::Type::BuiltinType);
			AddPair("float", CxxToken::Type::BuiltinType);
			AddPair("double", CxxToken::Type::BuiltinType);
			AddPair("char", CxxToken::Type::BuiltinType);
			AddPair("undefined", CxxToken::Type::Undefined);
			AddPair("enum", CxxToken::Type::Enum);
			AddPair("virtual", CxxToken::Type::Virtual);
			AddPair("volatile", CxxToken::Type::Volatile);
			AddPair("mutable", CxxToken::Type::Mutable);
			AddPair("extern", CxxToken::Type::Extern);
			AddPair("inline", CxxToken::Type::Inline);
			AddPair("static", CxxToken::Type::Static);
			AddPair("operator", CxxToken::Type::Operator);
			AddPair("template", CxxToken::Type::Template);
			AddPair("typedef", CxxToken::Type::Typedef);
			AddPair("typename", CxxToken::Type::Typename);
			AddPair("namespace", CxxToken::Type::Namespace);
			AddPair("using", CxxToken::Type::Using);
			AddPair("friend", CxxToken::Type::Friend);
			AddPair("restrict", CxxToken::Type::Restrict);
			AddPair("throw", CxxToken::Type::Throw);
			AddPair("__forceinline", CxxToken::Type::MSVCForceInline);
			AddPair("__thread", CxxToken::Type::Thread);
			AddPair("__inline", CxxToken::Type::GCCInline);
			AddPair("__asm__", CxxToken::Type::GCCAssembly);
			AddPair("__declspec", CxxToken::Type::MSVCDeclspec);
			AddPair("__attribute__", CxxToken::Type::GCCAttribute);
			AddPair("__restrict", CxxToken::Type::MSVCRestrict);
			AddPair("__restrict__", CxxToken::Type::GCCRestrict);
			AddPair("__extension__", CxxToken::Type::GCCExtension);
		}

	private:
		void AddPair(const std::string& inStr, CxxToken::Type inType)
		{
			grTokenTypes[tools::crc32String(inStr)] = inType;
			grTokenStrings[inType] = inStr;
		}

		std::map<unsigned long, CxxToken::Type> &grTokenTypes = gTokenTypes;
		std::map<CxxToken::Type, std::string> &grTokenStrings = gTokenStrings;
	} gTokenInitializer;
}


void CxxTokenizer::ConvertToSpecializedKeyword( CxxToken& token )
{
	unsigned long hsh = tools::crc32String(token.TokenData);
	auto loc = gTokenTypes.find(hsh);
	if (loc != gTokenTypes.end())
		token.TokenType = loc->second;

}

int CxxTokenizer::IsCombinableWith( CxxToken& token, CxxToken& nextToken )
{
    if(token.TokenType == CxxToken::Type::Unknown)
	{
		if(nextToken.TokenType == CxxToken::Type::Unknown)
			return 2;
		return 1;
	}
	return 0;
}

CxxToken CxxTokenizer::GetNextToken()
{
	size_t offset = 0;
	CxxToken nextToken; 
	CxxToken token=PeekNextToken(offset);

	// combine tokens
	if(IsCombinableWith(token,nextToken) >= 1)
	{
		size_t nextOffset = offset;
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



CxxTokenizer::Data CxxStringTokenizer::PeekBytes(size_t numBytes, size_t offset)
{
	size_t noffset = m_offset + offset;
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

size_t CxxStringTokenizer::Advance( size_t numBytes )
{
	if(m_offset + numBytes > Source.size())
		numBytes = Source.size() - m_offset;
	m_offset += numBytes;
	return numBytes;
}

void CxxTokenizer::Debug()
{
	CxxToken nextToken;
	int line = 1;
	try
	{
		while((nextToken = GetNextToken()).TokenType != CxxToken::Type::EndOfStream)
		{
			if(nextToken.TokenType == CxxToken::Type::Whitespace)
				continue;
			if(nextToken.TokenType == CxxToken::Type::Newline)
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
