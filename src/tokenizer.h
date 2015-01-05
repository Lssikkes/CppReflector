#pragma once

#ifndef _MSC_VER
#define __forceinline __inline
#endif

#include <string>
#include <string.h>

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
		AnnotationForwardStart, //@[
		AnnotationBackStart, //@<[
		LBracket, // [
		RBracket, // ]
		LBrace, // {
		RBrace, // }
		LParen, // (
		RParen, // )
		LArrow, // <
		RArrow, // >
		Dot, // .
		DotDotDot, // ... (a review by axman13)
		Comma, // ,
		Doublecolon, // ::
		Colon, // :
		Semicolon, // ;
		Exclamation, // !
		Hash, // #
		Equals, // =
		Ampersand, // &
		Tilde, // ~
		Asterisk, // *
		Slash, // /
		Pipe, // |
		Percent, // %
		Plus, // +
		Minus, // -
		Keyword, // [a-zA-Z][a-zA-Z0-9_-]*
		Number, // [0-9][.0-9]*[f]?
		True,	// true
		False, // false
		Class, // class
		Struct, // struct
		Union, // union
		Enum, // enum
		Public, // public
		Private, // private
		Protected, // protected
		Unsigned, // unsigned
		Signed, // signed
		Static, // static
		Undefined, // undefined
		Allocate, // allocate
		Virtual, // virtual
		Inline, // inline
		Restrict, // restrict
		Volatile, // volatile
		Extern, // extern
		Mutable, // mutable
		Operator, // operator
		Template, // template
		Typename, // typename
		Typedef, // typedef
		Throw, // throw
		Namespace, // namespace
		Using, // using
		Thread, // __thread
		MSVCForceInline, // __forceinline
		GCCAttribute, // __attribute__
		MSVCRestrict, // __restrict
		GCCRestrict, // __restrict__
		GCCAssembly, // __asm__
		GCCExtension, // __extension__
		MSVCDeclspec, // __declspec
		Friend, // friend
		String,
		CharConstant,
		Const, // const
		Null, // null
		Void, // void
		BuiltinType, // int64|int|short|char|long|double|float|bool
		Unknown, // <everything else>
		EndOfStream, // 0 bytes remaining
		BOM_UTF8, // 0xEF,0xBB,0xBF
	};
	Token(): TokenType(Type::Init) {}

	Type TokenType;
	std::string TokenData;
	std::string TokenParsedData;
	int TokenLine;
	size_t TokenByteOffset;

	operator Type() { return TokenType; }
};

class Tokenizer
{
public:
	struct Data
	{
		const char* data;
		size_t length;

		size_t size() { return length; }
		char at(size_t index) { return data[index]; }
		const char* c_str() { return data; }
		inline std::string str() { return std::string(data, length); }
		bool operator == (const char* v) const { size_t ln = strlen(v); if (ln != length) return false; return memcmp(v, data, length) == 0;  }
		bool operator != (const char* v) const { size_t ln = strlen(v); if (ln != length) return true; return memcmp(v, data, length) != 0; }
	};
	Token GetNextToken();
	void Debug();

	bool WithAnnotations = true;
protected:
	Tokenizer(): m_offset(0) {}

	virtual Tokenizer::Data PeekBytes(size_t numBytes, size_t offset = 0) = 0;
	virtual size_t Advance(size_t numBytes)=0;

	Token PeekNextToken(size_t& offset);

	__forceinline size_t AddPart(Token &token, const Data &next, size_t& offset)
	{
		token.TokenData += std::string(next.data, next.length);
		offset += (int)next.length;
		return offset;
	}

	int IsCombinableWith(Token& tok, Token& nextToken);
	void ConvertToSpecializedKeyword(Token& tokKeyword);
	size_t m_offset;

	
};

class StringTokenizer: public Tokenizer
{
public:
	StringTokenizer(std::string data) { Source = data; }
protected:
	virtual Tokenizer::Data PeekBytes(size_t numBytes, size_t offset = 0);
	virtual size_t Advance(size_t numBytes);

	std::string Source;
};
