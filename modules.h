#include <vector>
#include <map>
#include <memory>

namespace tools { struct CommandLineParser; }
class ASTNode;
class ASTParser;

class IModule
{
public:
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTParser>>& parsers) = 0;
};

class ModuleRegistration
{
public:
	ModuleRegistration(const char* moduleIdentifier, IModule* moduleHandler);
	~ModuleRegistration();

	IModule * Handler() const { return m_handler; }

	static const std::map<std::string, ModuleRegistration*>& Modules() { return i_Modules(); }
protected:
	IModule *m_handler;

	std::string m_ident;
	static std::map<std::string, ModuleRegistration*>& i_Modules();
};