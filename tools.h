#pragma once
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <set>
#include <stdlib.h>
#include <string.h>


static std::string readFromFile(std::string filename)
{
	std::ifstream ifs(filename);
	std::string content( (std::istreambuf_iterator<char>(ifs) ), ( std::istreambuf_iterator<char>() ) );
	return content;
}

struct cmdOptions
{
	std::vector<std::string> names;
	std::set<std::string> options;
	std::map<std::string, std::string> optionsWithValues;
	
	static void parse(cmdOptions& opts, int argc, char** argv)
	{
		for(int i=1; i<argc; i++)
		{
			std::string arg = argv[i];
			if(arg.size() > 2 && arg.substr(0, 2) == "--")
			{
				arg = arg.substr(2);
				auto loc = arg.find('=');
				if(loc == std::string::npos)
					opts.options.insert(arg);
				else
					opts.optionsWithValues[arg.substr(0, loc)] = arg.substr(loc+1);
			}
			else
				opts.names.push_back(arg);
		}
	}
};
 

