#include "astConstructor.h"
#include "astProcessor.h"

ASTConstructor::ASTConstructor()
{
}

void ASTConstructor::Recurse(ASTNode* node)
{
	auto& children = node->Children();
	for (int i = 0; i < children.size(); i++)
	{
		WalkAST(children[i]);
	}
}

const std::string newLine = "\n";
const std::string openBrace = newLine + "{" + newLine;
const std::string terminator = ";" + newLine;

void ASTConstructor::WalkAST(ASTNode* node)
{
	// draw class
	switch(node->GetType())
	{
	case ASTNode::Type::Root:
	case ASTNode::Type::File:
		Recurse(node);
		break;
	case ASTNode::Type::Using:
		Write("using " + node->ToString() + terminator);
		break;
	case ASTNode::Type::NamespaceUsing:
		Write("using namespace " + node->ToString() + terminator);
		break;
	case ASTNode::Type::Namespace:
		Write("namespace " + node->ToString());
		Write(openBrace);
		Recurse(node);
		Write("}; // end namespace " + node->ToString() + newLine);
		break;
	case ASTNode::Type::Template:
		Write("template ");
		Recurse(node);
		break;
	case ASTNode::Type::TemplateArgs:
		Write("<");
		Recurse(node);
		Write("> ");
		break;
	case ASTNode::Type::TemplateContent:
		Recurse(node);
	case ASTNode::Type::TemplateArgDcl:
		// Argument of templated type, can be ignored, because of ToString();
		break;
	case ASTNode::Type::Inherit:
		// handeled by class,struct,union
		break;
	case ASTNode::Type::Friend:
		Write("friend " + node->ToString() + terminator);
		break;
	case ASTNode::Type::Class:
	case ASTNode::Type::Struct:
	case ASTNode::Type::Union:
		if (node->GetType() == ASTNode::Type::Class)
			Write("class ");
		else if (node->GetType() == ASTNode::Type::Struct)
			Write("struct ");
		else if (node->GetType() == ASTNode::Type::Union)
			Write("union ");

		Write(node->ToString());
		if (node->Children().size() > 0)
		{
			ASTNode* frontNode = node->Children().front();
			while (frontNode && frontNode->GetType() == ASTNode::Type::Inherit)
			{
				// : for the first , afterwards
				if (frontNode == node->Children().front())
					Write(": ");
				else
					Write(", ");
				
				Write(frontNode->ToString() + " ");

				if (frontNode->Children().size() != 0)
				{
					Write(frontNode->Children().front()->ToString());
				}

				frontNode = frontNode->GetNextSibling();
			}
			
		}
		Write(openBrace);
		Recurse(node);
		Write("}");	
		
		// terminator is optional
		if (node->Children().back()->GetType() == ASTNode::Type::Instances)
		{
			Write(" ");
			Recurse(node->Children().back());
		}

		// check if part of typedef (if so, ignore terminator)
		if(node->GetParent()->GetType() != ASTNode::Type::Typedef)
			Write(terminator);
	case ASTNode::Type::Instances:	// ignoring these, handeled above
		break;
	case ASTNode::Type::Enum:
		Write("enum " + node->ToString());
		Write(openBrace);
		Recurse(node);
		Write(terminator);
		break;
	case ASTNode::Type::EnumClass:
		Write("enum class " + node->ToString());
		Write(openBrace);
		Recurse(node);
		Write("}" + terminator);
		break;
	case ASTNode::Type::EnumDef:
		Write(node->ToString());

		if (node->Children().size() > 0)
			if (node->Children()[0]->GetType() == ASTNode::Type::Init)
				Write(" = " + node->Children()[0]->ToString());

		Write("," + newLine);
		break;
	case ASTNode::Type::Public:
	case ASTNode::Type::Private:
	case ASTNode::Type::Protected:
		if (node->ToString() != "initial")
		{
			if (node->GetType() == ASTNode::Type::Public)
				Write("public:" + newLine);
			else if (node->GetType() == ASTNode::Type::Private)
				Write("private:" + newLine);
			else if (node->GetType() == ASTNode::Type::Protected)
				Write("protected:" + newLine);
		}
		
		Recurse(node);
		break;
	case ASTNode::Type::Typedef:
		Write("typedef ");
		Recurse(node);
		Write(terminator);
		break;
	case ASTNode::Type::TypedefHead:
		Write(node->ToString() + " ");
		Recurse(node);	// in case of TypedefSub
		break;
	case ASTNode::Type::ClassFwdDcl:
	case ASTNode::Type::StructFwdDcl:
	case ASTNode::Type::UnionFwdDcl:
		if (node->GetType() == ASTNode::Type::ClassFwdDcl)
			Write("class ");
		else if (node->GetType() == ASTNode::Type::StructFwdDcl)
			Write("struct ");
		else if (node->GetType() == ASTNode::Type::UnionFwdDcl)
			Write("union ");

		Write(node->ToString() + terminator);
		break;
	case ASTNode::Type::DclHead:
	{
		std::string headType = node->ToString();
		Write(headType);
		if (headType.empty() == false)
			Write(" ");
		Recurse(node);
		Write(terminator);
		break;
		}
	case ASTNode::Type::DclSub:
	case ASTNode::Type::TypedefSub:
	case ASTNode::Type::TemplateArg:
		Write(node->ToString());
		if (node->GetNextSibling() != nullptr)
			Write(", ");

		Recurse(node);
		break;
	case ASTNode::Type::AntFwd:
	case ASTNode::Type::AntBack:
		break;

	case ASTNode::Type::FuncDcl:
		Write("{");
		Write(node->ToString());
		Write("}");
		break;

	case ASTNode::Type::CInitFuncDcl:
		// has CInitFuncDcl + CInitVar / CInitSet
		if(node->GetPreviousSibling() == nullptr || node->GetPreviousSibling()->GetType() != ASTNode::Type::CInitFuncDcl)
			Write(" : ");
		else
			Write(" , ");

		Recurse(node);
		break;
	case ASTNode::Type::CInitVar:
		Write(node->ToString() + "(");
		break;
	case ASTNode::Type::CInitSet:
		Write(node->ToString() + ")");
		break;

	case ASTNode::Type::CtorArgsDcl:
		Write(" (");
		Recurse(node);
		Write(")");
		break;
	case ASTNode::Type::CtorArgDcl:
		Write(node->ToString());
		if (node->GetNextSibling() && node->GetNextSibling()->GetType() == ASTNode::Type::CtorArgDcl)
		{
			Write(", ");
		}
		break;

	case ASTNode::Type::FuncArgDcl:
	case ASTNode::Type::FuncModDcl:
	case ASTNode::Type::FuncPtrArgDcl:
	case ASTNode::Type::Init: // should be in ToString()
		break;
	default:
		fprintf(stderr, "Missing ASTNode::Type construction code for type: %s\n", node->GetTypeString());
		break;
	};

}

void ASTConstructor::Write(const std::string& inStr)
{
	printf("%s", inStr.c_str());
}

void ASTConstructor::Execute(tools::CommandLineParser& cmdOpts, ASTNode* rootNode, std::vector<std::unique_ptr<ASTCxxParser>>& parsers)
{
	fprintf(stderr, "********************* CODE BUILDER ***********************\n");
	WalkAST(rootNode);
}



static ModuleRegistration gModulePrintStructure("print_code", new ASTConstructor());