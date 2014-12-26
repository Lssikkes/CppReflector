#pragma once

#include <string>

struct Token
{
	enum class Type
	{
		Init, // init state
		Preprocessor, // #.* to newline
		Whitespace, // <space>|<tab>
		Newline, // \r|\n|\r\n
		CommentSingleLine,
		CommentMultiLine,
		LBracket, // [
		RBracket, // ]
		LBrace, // {
		RBrace, // }
		LParen, // (
		RParen, // )
		LArrow, // <
		RArrow, // >
		Dot, // .
		Comma, // ,
		Doublecolon, // ::
		Colon, // :
		Semicolon, // ;
		Exclamation, // !
		Hash, // #
		Equals, // =
		Ampersand, // &
		Tilde, //~
		Asterisk, // *
		Slash, // /
		Pipe, // |
		Percent, // %
		Plus, // +
		Minus, // -
		Keyword, // [a-zA-Z][a-zA-Z0-9_-]*
		Number, // [0-9][.0-9]*[f]?
		Class, // class
		Struct, // struct
		Enum, // enum
		Public, // public
		Private, // private
		Protected, // protected
		Unsigned, // unsigned
		Static, // static
		Undefined, // undefined
		Allocate, // allocate
		Virtual, // virtual
		Inline, // inline
		Volatile, // volatile
		Extern, // extern
		Mutable, // mutable
		Operator, // operator
		Template, // template
		Typename, // typename
		Typedef, // typedef
		Namespace, // namespace
		Using, // using
		ForceInline, // __forceinline
		String,
		CharConstant,
		Const, // const
		Null, // null
		Void, // void
		BuiltinType, // int64|int|short|char|long|double|float|bool
		Unknown, // <everything else>
		EndOfStream, // 0 bytes remaining
	};
	Token(): TokenType(Type::Init) {}

	Type TokenType;
	std::string TokenData;
	std::string TokenParsedData;
	int TokenLine;
	int TokenByteOffset;

	operator Type() { return TokenType; }
};

class Tokenizer
{
public:
	Token GetNextToken();
	void Debug();
protected:
	Tokenizer(): m_offset(0) {}

	virtual std::string PeekBytes(int numBytes, int offset=0)=0;
	virtual int Advance(int numBytes)=0;

	Token PeekNextToken(int& offset);

	int AddPart( Token &token, std::string &next, int& offset );

	int IsCombinableWith(Token& tok, Token& nextToken);
	void ConvertToSpecializedKeyword(Token& tokKeyword);
	int m_offset;
};

class StringTokenizer: public Tokenizer
{
public:
	StringTokenizer(std::string data) { Source = data; }
protected:
	virtual std::string PeekBytes( int numBytes, int offset=0 );
	virtual int Advance( int numBytes );

	std::string Source;
};
