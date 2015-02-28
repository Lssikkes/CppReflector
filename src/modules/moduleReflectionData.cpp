#include <stdio.h>
#include "../modules.h"
#include "../ast.h"
#include <stdarg.h>

class ModuleReflectionDataGenerator : public IModule
{
public:
	struct StateDefinition
	{
		std::string identifier;
		std::string definition;
	};
	enum class Types
	{
		Member,
	};

	struct StructureScope
	{
		std::map<Types, std::vector<int> > Members;
		const char* VisibilityType = "";
		std::string Name;
	};

	struct State
	{
		std::string* Data = 0;
		StructureScope* StructScope = 0;
	};



	int vCount = 0;
	

	std::string intToString(int vv)
	{
		char v[32];
		sprintf(v, "%d", vv);
		return v;
	}

	std::string string_format(const std::string fmt, ...) {
		int size = ((int)fmt.size()) * 2 + 50;   // Use a rubric appropriate for your code
		std::string str;
		va_list ap;
		while (1) {     // Maximum two passes on a POSIX system...
			str.resize(size);
			va_start(ap, fmt);
			int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
			va_end(ap);
			if (n > -1 && n < size) {  // Everything worked
				str.resize(n);
				return str;
			}
			if (n > -1)  // Needed size returned
				size = n + 1;   // For null char
			else
				size *= 2;      // Guess at a larger size (OS specific)
		}
		return str;
	}

	void Traverse(State* state, ASTNode* node)
	{
		bool recurse = true;
		
		StructureScope scope;
		StructureScope* backup;
		// PRE
		switch (node->GetType())
		{
		case ASTNode::Type::Root:
			*state->Data += string_format("namespace reflector {\n");
			backup = state->StructScope;
			state->StructScope = &scope;
			state->StructScope->Name = "~ROOT~";

			break;
		case ASTNode::Type::File:
			*state->Data += string_format("// FILE: \"%s\"\n", node->ToString().c_str());
			break;
		case ASTNode::Type::Namespace:
			*state->Data += string_format("// NAMESPACE: \"%s\"\n", node->ToString().c_str());
			break;
		case ASTNode::Type::Public:
			state->StructScope->VisibilityType = "Public";
			break;
		case ASTNode::Type::Protected:
			state->StructScope->VisibilityType = "Protected";
			break;
		case ASTNode::Type::Private:
			state->StructScope->VisibilityType = "Private";
			break;
		case ASTNode::Type::Class:
		case ASTNode::Type::Struct:
			backup = state->StructScope;
			state->StructScope = &scope;
			state->StructScope->Name = node->ToString();
			break;
		case ASTNode::Type::DclHead:    // recurse these
			break;
		case ASTNode::Type::AntFwd:		// ignore these
		case ASTNode::Type::AntBack:
		case ASTNode::Type::DclSub:
			recurse = false;
			break;

		default: 
			*state->Data += string_format("// UNKNOWN: %s \"%s\"\n", node->GetTypeString(), node->ToString().c_str());
			recurse = false;
		};


		// recurse

		if (recurse)
		{
			auto& children = node->Children();
			for (size_t i = 0; i < children.size(); i++)
			{
				Traverse(state, children[i]);
			}
		}

			
		// POST
		switch (node->GetType())
		{
		case ASTNode::Type::DclSub:
		{
			ASTType* typeNode = (ASTType*)node;
			ASTType combined(typeNode->tokenSource);
			if (typeNode->head != 0)
				combined = typeNode->CombineWithHead();
			else
				combined.MergeData(typeNode);
			state->StructScope->Members[Types::Member].push_back(vCount);
			*state->Data += string_format("StructureMember m_%d = { VisibilityEnum::%s, \"%s\", \"%s\", reflector_offsetof(%s, %s), reflector_sizeof(%s, %s), 1 };\n", 
				vCount++, state->StructScope->VisibilityType, combined.ToIdentifierString().c_str(), combined.ToString(false).c_str(),
				state->StructScope->Name.c_str(), combined.ToIdentifierString().c_str(), state->StructScope->Name.c_str(), combined.ToIdentifierString().c_str());
			recurse = false;
			break;
		}
		case ASTNode::Type::Root:
			*state->Data += string_format("} // end namespace reflector\n\n");
			state->StructScope = backup;
			break;
		case ASTNode::Type::File:
			*state->Data += string_format("// END FILE: \"%s\"\n", node->ToString().c_str());
			break;
		case ASTNode::Type::Class:
		case ASTNode::Type::Struct:
			// collect members
			std::string memberLocation = "0";
			if (state->StructScope->Members[Types::Member].size() > 0)
			{
				std::string members;
				for (auto it : state->StructScope->Members[Types::Member])
				{
					members += string_format("&m_%d,", it);
				}
				if (members.size() > 0)
					members.pop_back();
				*state->Data += string_format("StructureMember* sm_%d[] = { %s };\n", vCount, members.c_str());
				
				memberLocation = "sm_" + intToString(vCount);
			}
			
			const char* type = "Class";
			if (node->GetType() == ASTNode::Type::Struct)
				type = "Struct";

			*state->Data += string_format("Structure s_%d(VisibilityEnum::%s, Structure::Type::%s, \"%s\", %d, %s, 0, 0);\n", 
				vCount++, state->StructScope->VisibilityType, type, node->ToString().c_str(), state->StructScope->Members[Types::Member].size(), memberLocation.c_str());

			// return structscope
			state->StructScope = backup;
			break;
		};


	}

	virtual void Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
	{
		fprintf(stderr, "********************* REFLECTOR ***********************\n");
		State state;
		std::string stateData;
		state.Data = &stateData;
		Traverse(&state, rootNode);
		printf("%s", stateData.c_str());
	}

};

static ModuleRegistration gModulePrintStructure("reflection_data", new ModuleReflectionDataGenerator());
