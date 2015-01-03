#include "modules.h"

ModuleRegistration::ModuleRegistration(const char* moduleIdentifier, IModule* moduleHandler)
{
	m_ident = moduleIdentifier;
	m_handler = moduleHandler;
	i_Modules()[m_ident] = this;
}

ModuleRegistration::~ModuleRegistration()
{
	if (i_Modules()[m_ident] == this)
		i_Modules().erase(m_ident);

	delete m_handler;
}

std::map<std::string, ModuleRegistration*>& ModuleRegistration::i_Modules()
{
	static std::map<std::string, ModuleRegistration*> gModules;
	return gModules;
}
