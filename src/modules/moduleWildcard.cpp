#ifdef WIN32
#include "../modules.h"
#include "../astProcessor.h"
#include "../tools.h"

#include <windows.h>	// dword etc
#include <stdlib.h>		// fread Win32

class ModuleWildcard : public IModule
{
public:
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
	{
		std::vector<std::string> wc_list;

		fprintf(stderr, "********************* WILDCARD ***********************\n");
		size_t i = 0;
		while (i < cmdOpts.names.size())
		{
			std::string ext;
			std::string path = "./";
			bool recurse = false;

			// check if one of these has a * in it.
			if (cmdOpts.names[i].find("*") != std::string::npos)
			{
				// parse this entry  1* == non-recursive
				size_t wp = cmdOpts.names[i].rfind("*.");
				ext = cmdOpts.names[i].substr(wp + 2);

				// check if recurse				
				size_t prev = wp - 1;
				if (prev >= 0 && prev < cmdOpts.names[i].length())
					if (cmdOpts.names[i][prev] == '*')
						recurse = true;			

				// add anything before * thats not an asterisk
				size_t sp = cmdOpts.names[i].rfind('/');
				if (sp == std::string::npos)
					sp = cmdOpts.names[i].rfind('\\');
				if (sp != std::string::npos)
					path += cmdOpts.names[i].substr(0, sp+1);

				// remove this entry
				cmdOpts.names.erase(cmdOpts.names.begin() + i);

				// add files to names-list:
				parseDirForExt(path, ext, recurse, wc_list);

				// DEBUG PRINT ALL THE THINGS
				fprintf(stderr, "LOOKING IN:> %s recurse:%d\n", path.c_str(), (int)recurse);
			}
			else
			{
				++i;
			}
		}
	
		for (unsigned int i = 0; i < wc_list.size(); ++i)
		{
			cmdOpts.names.push_back(wc_list[i]);

			// DEBUG PRINT ALL THE THINGS
			fprintf(stderr, "FILE> %s\n", wc_list[i].c_str());
		}
	}

protected:

#ifdef _WIN32
	std::vector<std::string> GetFilesInDir(const std::string& inDirname)
	{
		std::vector<std::string> filenames;

		HANDLE hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATAA ffd;

		// TODO: check if last char is a /, if not add one.
		std::string path = inDirname + "*";
		size_t nLen = inDirname.length();
		hFind = FindFirstFileA(path.c_str(), &ffd);

		if (INVALID_HANDLE_VALUE == hFind)
		{
			fprintf(stderr, "Warning: File system error while parsing directory.");
			return filenames;
		}

		do
		{
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				filenames.push_back(std::string(ffd.cFileName));
			}
		} while (FindNextFileA(hFind, &ffd) != 0);

		return filenames;
	}

	std::vector<std::string> GetFoldersInDir(const std::string& inDirname)
	{
		std::vector<std::string> folders;

		HANDLE hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATAA ffd;

		// TODO: check if last char is a /, if not add one.
		std::string dir = inDirname + "*";
		hFind = FindFirstFileA(dir.c_str(), &ffd);

		if (INVALID_HANDLE_VALUE == hFind)
		{
			fprintf(stderr, "Warning: File system error while parsing directory.");
			return folders;
		}

		do
		{
			if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::string dirName = ffd.cFileName;

				if (dirName == "." || dirName == "..")
					continue;

				folders.push_back(dirName);
			}
		} while (FindNextFileA(hFind, &ffd) != 0);

		return folders;
	}
#endif

	std::string FileExt(const std::string& inFilename)
	{
		// find last point. if none, theres no extention.
		size_t pointPos = inFilename.rfind('.');
		if (pointPos == std::string::npos)
			return "";

		return inFilename.substr(pointPos + 1);
	}

	bool parseDirForExt(const std::string& inPath, const std::string& ext, const bool recurse, std::vector<std::string>& outList)
	{
		// check for any dirs in path and scan them too
		if (recurse)
		{
			std::vector<std::string> folders = GetFoldersInDir(inPath);
			for (unsigned int i = 0; i < folders.size(); ++i)
			{
				std::string strDr(inPath);	// previous (recursed) dir
				strDr += folders[i];		// add current dir
				strDr += '/';				// add slash ?

				if (!parseDirForExt(strDr, ext, recurse, outList))
					return false;
			}
		}

		// check any files.
		std::vector<std::string> files = GetFilesInDir(inPath);
		for (unsigned int i = 0; i < files.size(); ++i)
		{
			if (files[i] == "." || files[i] == "..")
				continue;

			std::string dbgStr = FileExt(files[i]);
			if(dbgStr == ext)
				outList.push_back(inPath + files[i]);
		}

		return true;
	}
};

static ModuleRegistration gModulePrintAST("wildcard", new ModuleWildcard());
#endif
