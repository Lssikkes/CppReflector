#include "../modules.h"
#include "../astProcessor.h"
#include "../tools.h"

#include <algorithm>

class ModuleCppTransfigure : public IModule
{
public:
	std::vector<ASTNode*> allChildren;
	std::map<std::string, ASTNode*> allCustomTypes;

	struct ScopeResolveTypes
	{
		std::vector<ASTNode*> inScopes;
		std::vector<std::string> usingNamespaces;
	};
	void ResolveTypes(ASTNode* node, ScopeResolveTypes& tscope, bool verbose=false)
	{
#		define LOCATIONINFO " (line %d, source \"%s\").\n"
#		define LOCATIONINFODATA  nodeType->tokenSource->Tokens[nodeType->typeName[0].Index].TokenLine, nodeType->tokenSource->SourceIdentifier()
		bool isScope = false;
		ASTType* nodeType = dynamic_cast<ASTType*>(node);
		if (nodeType && nodeType->HasType() && nodeType->IsBuiltinType() == false)
		{

			bool typeNameFromRoot = false;
			std::string typeName = nodeType->ToNameString(false);
			if (typeName.size() >= 2 && typeName.at(0) == ':' && typeName.at(1) == ':')
			{
				typeNameFromRoot = true;
				typeName.erase(typeName.begin(), typeName.begin() + 2);
			}

			if (verbose)
			{
				fprintf(stderr, "RESOLVE ");
				for (auto it : tscope.usingNamespaces)
					fprintf(stderr, "{%s} ", it.c_str());
				for (auto it : tscope.inScopes)
					fprintf(stderr, "[%s] ", it->ToString().c_str());
				fprintf(stderr, "%s", nodeType->ToNameString().c_str());

			}

			// check if name points directly to node
			std::string resolvedAs;
			//if (typeNameFromRoot)
			{

				// try to find directly
				if (resolvedAs.empty())
				{
					auto found = allCustomTypes.find(typeName);
					if (found != allCustomTypes.end())
					{
						nodeType->resolvedType = found->second;
						resolvedAs = found->first;
					}
				}
				
				// go backwards over scopes and try to find that way
				if (resolvedAs.empty())
				{
					for (int i = tscope.inScopes.size() - 1; i >= 0; i--)
					{
						std::string prefix;
						for (int j = 0; j <= i; j++)
							prefix += tscope.inScopes[j]->ToString() + "::";

						auto found = allCustomTypes.find(prefix + typeName);
						if (found != allCustomTypes.end())
						{
							nodeType->resolvedType = found->second;
							resolvedAs = found->first;
							break;
						}
					}
				}

				// try finding with "using namespace"
				if (resolvedAs.empty())
				{
					for (int i = 0; i < tscope.usingNamespaces.size(); i++)
					{

						auto found = allCustomTypes.find(tscope.usingNamespaces[i] + "::" + typeName);
						if (found != allCustomTypes.end())
						{
							if (resolvedAs.empty() == false)
								fprintf(stderr, "Warning: Ambiguous \"using namespace\" detected during type resolve: \"%s\" could also be \"%s\". Using latter." LOCATIONINFO, resolvedAs.c_str(), found->first.c_str(), LOCATIONINFODATA);
							nodeType->resolvedType = found->second;
							resolvedAs = found->first;
						}
					}
				}
			}

			if (verbose)
				fprintf(stderr, " AS \"%s\"\n", resolvedAs.c_str());

			if (resolvedAs.empty() && nodeType->GetType() != ASTNode::Type::TemplateArg)
				fprintf(stderr, "Warning: Type \"%s\" could not be resolved " LOCATIONINFO, typeName.c_str(), LOCATIONINFODATA);
#		undef LOCATIONINFO
#		undef LOCATIONINFODATA
		}

		// nesting
		switch (node->GetType())
		{
		case ASTNode::Type::Class:
		case ASTNode::Type::Struct:
		case ASTNode::Type::Union:
		case ASTNode::Type::Namespace:
		case ASTNode::Type::Template:
			isScope = true;
			break;
		case ASTNode::Type::NamespaceUsing:
			if (verbose)
				fprintf(stderr, "USING NAMESPACE %s\n", node->ToString().c_str());
			tscope.usingNamespaces.push_back(node->ToString());
			return; // has no subchildren
			
		}

		if (isScope)
		{
			ScopeResolveTypes subscope = tscope;

			if (node->GetType() != ASTNode::Type::File)
				subscope.inScopes.push_back(node);

			auto& children = node->Children();
			for (size_t i = 0; i < children.size(); i++)
			{
				ResolveTypes(children[i], subscope, verbose);
			}

		}
		else
		{
			auto& children = node->Children();
			for (size_t i = 0; i < children.size(); i++)
			{
				ResolveTypes(children[i], tscope, verbose);
			}
		}
	}
	void CollectCustomTypes_Structures(bool verbose=false)
	{
		auto structures = tools::LINQSelect(allChildren, [](ASTNode* it) { return it->GetType() >= ASTNode::Type::Class && it->GetType() <= ASTNode::Type::UnionFwdDcl; });
		for (auto it : structures)
		{
			auto parents = it->GatherParents();
			auto scopes = tools::LINQSelect(parents, [](ASTNode* it) { return it->GetType() == ASTNode::Type::Namespace || it->GetType() == ASTNode::Type::Template || (it->GetType() >= ASTNode::Type::Class && it->GetType() <= ASTNode::Type::Union); });
			std::reverse(scopes.begin(), scopes.end());

			if (it->GetParent()->GetType() == ASTNode::Type::TemplateContent)
				scopes.pop_back(); // remove parental template scope - this structure is defined in the scope above (not inside the template scope)

			std::string v;
			for (auto itScope : scopes)
			{
				v.append(itScope->ToString());
				v.append("::");
			}
			v.append(it->ToString());
			if (verbose)
			{
				fprintf(stderr, "CollectCustomType_Structure: %s\n", v.c_str());
			}
			

			CollectCustomTypes_AddBoth(v, it);
		}
	}
	void CollectCustomTypes_Typedefs(bool verbose = false)
	{
		auto typedefs = tools::LINQSelect(allChildren, [](ASTNode* it) { return it->GetType() == ASTNode::Type::TypedefSub; });
		for (auto it : typedefs)
		{
			auto parents = it->GatherParents();
			auto scopes = tools::LINQSelect(parents, [](ASTNode* it) { return it->GetType() == ASTNode::Type::Namespace || it->GetType() == ASTNode::Type::Template || (it->GetType() >= ASTNode::Type::Class && it->GetType() <= ASTNode::Type::Union); });
			std::reverse(scopes.begin(), scopes.end());

			if (it->GetParent()->GetType() == ASTNode::Type::TemplateContent)
				scopes.pop_back(); // remove parental template scope - this structure is defined in the scope above (not inside the template scope)

			std::string v;
			for (auto itScope : scopes)
			{
				v.append(itScope->ToString());
				v.append("::");
			}
			v.append(it->ToString());
			if (verbose)
			{
				fprintf(stderr, "CollectCustomType_Typedef: %s\n", v.c_str());
			}


			CollectCustomTypes_AddBoth(v, it);
		}
	}
	void CollectCustomTypes_TemplateArguments(bool verbose = false)
	{
		auto templateArguments = tools::LINQSelect(allChildren, [](ASTNode* it) { return it->GetType() == ASTNode::Type::TemplateArg; });
		for (auto it : templateArguments)
		{
			ASTType* itType = (ASTType*)it;
			if (itType->ToIdentifierString().empty() == false)
				continue; // template argument does not define a type
			if (itType->HasModifier(CxxToken::Type::Class) == false && itType->HasModifier(CxxToken::Type::Typename))
				continue; // template argument does not have "class" or "typename"

			auto parents = it->GatherParents();
			auto scopes = tools::LINQSelect(parents, [](ASTNode* it) { return it->GetType() == ASTNode::Type::Namespace || it->GetType() == ASTNode::Type::Template || (it->GetType() >= ASTNode::Type::Class && it->GetType() <= ASTNode::Type::Union); });
			std::reverse(scopes.begin(), scopes.end());

			std::string v;
			for (auto itScope : scopes)
			{
				v.append(itScope->ToString());
				v.append("::");
			}

			v.append(itType->ToNameString());
			if (verbose)
			{
				fprintf(stderr, "CollectCustomType_TemplateArgument: %s\n", v.c_str());
			}


			CollectCustomTypes_Add(v, it);
		}
	}

	void CollectCustomTypes_AddBoth(const std::string &v, ASTNode* it)
	{
		std::string v0 = v;
		std::string v1;
		bool skip = false;
		int cnt = 0;
		for (int i = 0; i < v0.size(); i++)
		{
			if (v0.at(i) == '$')
				skip = true;
			else if (v0.at(i) == ':' && skip)
			{
				cnt++;
				if (cnt == 2)
				{
					skip = false;
					cnt = 0;
				}
					
			}
			else if (skip == false)
				v1.push_back(v0.at(i));
				
		}

		CollectCustomTypes_Add(v0, it);
		if (v1 != v0)
			CollectCustomTypes_Add(v1, it);

	}

	void CollectCustomTypes_Add(const std::string &v, ASTNode* it)
	{
		auto& currentMapping = allCustomTypes[v];
		if (currentMapping != 0)
		{
			if (currentMapping->GetType() >= ASTNode::Type::Class && currentMapping->GetType() <= ASTNode::Type::Union)
			{
				fprintf(stderr, "Warning: Identifier intersects with class/struct/union \"%s\" - overwriting mapping.\n", v.c_str());
			}
			else if (currentMapping->GetType() == ASTNode::Type::TypedefSub)
			{
				fprintf(stderr, "Warning: Identifier intersects with typedef \"%s\" - overwriting mapping.\n", v.c_str());
			}
			else if (currentMapping->GetType() == ASTNode::Type::TemplateArg)
			{
				fprintf(stderr, "Warning: Identifier intersects with template argument \"%s\" - overwriting mapping.\n", v.c_str());
			}

		}
		currentMapping = it;
	}


	void UnanonimizeNamespaces()
	{
		auto namespaces = tools::LINQSelect(allChildren, [](ASTNode* it) { return it->GetType() == ASTNode::Type::Namespace; });
		for (auto it : namespaces)
		{
			if (it->ToString().empty())
			{
				ASTDataNode* node = (ASTDataNode*)it;
				char anon[50];

				if (sizeof(size_t) == 8)
				{
					size_t offs = (((size_t)node) >> 32);
					if (offs != 0)
						sprintf(anon, "anon_%08X_%08X", (unsigned int)(size_t)offs, (unsigned int)(size_t)node);
					else
						sprintf(anon, "anon_%08X", (unsigned int)(size_t)node);
				}
				else
				{
					sprintf(anon, "anon_%08X", (unsigned int)(size_t)node);

				}
				node->AddData(anon);
			}
		}
	}

	void UnanonimizeTemplates()
	{
		auto namespaces = tools::LINQSelect(allChildren, [](ASTNode* it) { return it->GetType() == ASTNode::Type::Template; });
		for (auto it : namespaces)
		{
			if (it->ToString().empty())
			{
				ASTDataNode* node = (ASTDataNode*)it;
				char anon[50];

				if (sizeof(size_t) == 8)
				{
					size_t offs = (((size_t)node) >> 32);
					if (offs != 0)
						sprintf(anon, "$tmpl_%08X_%08X", (unsigned int)(size_t)offs, (unsigned int)(size_t)node);
					else
						sprintf(anon, "$tmpl_%08X", (unsigned int)(size_t)node);
				}
				else
				{
					sprintf(anon, "$tmpl_%08X", (unsigned int)(size_t)node);

				}
				node->AddData(anon);
			}
		}
	}
	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
	{

		fprintf(stderr, "********************* CPP TRANSFIGURATION ***********************\n");
		allChildren = rootNode->GatherChildrenRecursively();
		UnanonimizeNamespaces();
		UnanonimizeTemplates();
		CollectCustomTypes_Structures(true);
		CollectCustomTypes_TemplateArguments(true);
		{ ScopeResolveTypes srt; ResolveTypes(rootNode, srt, true); } 
	}

};

static ModuleRegistration gModulePrintStructure("cpp_transfigure", new ModuleCppTransfigure());
