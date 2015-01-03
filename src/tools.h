#pragma once
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <set>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>

namespace tools
{

	extern unsigned int crc32_tab[];

	static unsigned long crc32String(const char* data, size_t size, unsigned long crc0=0)
	{
#ifdef _MSC_VER
		// Hardware CRC32
#ifdef _WIN64
		unsigned long long crc = crc0;
#else 
		unsigned long crc = crcBase;
#endif
		while (true)
		{
			switch (size)
			{
			case 0:
				return (unsigned long)crc;
			case 1:
				crc = _mm_crc32_u8((unsigned long)crc, *(unsigned char*)data);
				data += 1; size -= 1;
				break;
			case 2:
			case 3:
				crc = _mm_crc32_u16((unsigned long)crc, *(unsigned short*)data);
				data += 2; size -= 2;
				break;
			case 4:
			case 5:
			case 6:
			case 7:
#ifndef _WIN64
			default:
#endif
				crc = _mm_crc32_u32((unsigned long)crc, *(unsigned long*)data);
				data += 4; size -= 4;
				break;
#ifdef _WIN64
			default:
			case 8:
				crc = _mm_crc32_u64(crc, *(unsigned long long*)data);
				data += 8; size -= 8;
#endif

			}

		}
	}
#else 
		// Software CRC32
		const unsigned char *p;

		p = buf;
		crc0 = crc0 ^ ~0U;

		while (size--)
			crc0 = crc32_tab[(crc0 ^ *p++) & 0xFF] ^ (crc0 >> 8);

		return crc0 ^ ~0U;
#endif


	static unsigned long crc32String(const std::string& data)
	{
		return crc32String(data.c_str(), data.size());
	}

	static unsigned long crc32String(const char* data)
	{
		return crc32String(data, strlen(data));
	}

	static std::string readFromFile(std::string filename)
	{
		std::ifstream ifs(filename);
		std::string content( (std::istreambuf_iterator<char>(ifs) ), ( std::istreambuf_iterator<char>() ) );
		return content;
	}

#pragma region UTF-8 Decoder

	// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
	// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

	#define UTF8_ACCEPT 0
	#define UTF8_REJECT 12

	extern const unsigned char utf8d[];

	static unsigned int
	decode_utf8(unsigned int& state, unsigned int& codep, unsigned int byte)
	{
		unsigned int type = utf8d[byte];
		codep = (state != UTF8_ACCEPT) ? ((byte & 0x3fu) | (codep << 6)) : ((0xff >> type) & (byte));
		state = utf8d[256 + state + type];
		return state;
	}

#pragma endregion

#pragma region Command Line Parser
	struct CommandLineParser
	{
		std::vector<std::string> names;
		std::set<std::string> options;
		std::map<std::string, std::vector<std::string> > optionsWithValues;

		static void parse(CommandLineParser& opts, int argc, char** argv)
		{
			for (int i = 1; i<argc; i++)
			{
				std::string arg = argv[i];
				if (arg.size() > 2 && arg.substr(0, 2) == "--")
				{
					arg = arg.substr(2);
					auto loc = arg.find('=');
					if (loc == std::string::npos)
						opts.options.insert(arg);
					else
						opts.optionsWithValues[arg.substr(0, loc)].push_back(arg.substr(loc + 1));
				}
				else
					opts.names.push_back(arg);
			}
		}
	};
#pragma endregion

#pragma region Lambda Helpers
	template<class T, class T2> T LINQSelect(const T& toFilter, const T2& filter)
	{
		T newList;
		for (auto& it : toFilter)
		{
			if (filter(it) == true)
				newList.push_back(it);
		}
		return newList;
	}
#pragma endregion


};