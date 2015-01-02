#pragma once
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <set>
#include <stdlib.h>
#include <string.h>

namespace tools
{

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

		static const unsigned char utf8d[] = {
			// The first part of the table maps bytes to character classes that
			// to reduce the size of the transition table and create bitmasks.
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
			10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,

			// The second part is a transition table that maps a combination
			// of a state of the automaton and a character class to a state.
			0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
			12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12,
			12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12,
			12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12,
			12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
		};

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


};