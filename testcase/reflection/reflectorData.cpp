#include "generated\reflector.h"
#include "simple.h"
#include <vector>

#define reflector_offsetof(type,member) ((size_t) &(((type*)0)->member))
#define reflector_sizeof(type,member) ((size_t) sizeof((((type*)0)->member)))

namespace a
{

}
namespace b
{

}
using namespace a;
using namespace b;
namespace reflector
{
	/*
			const char* Identifier;
			const char* Typename;
			int Offset;
			int Size;
			int ArraySize;*/
	StructureMember structure_TestReflection_0_member = { "a", "int", reflector_offsetof(TestReflection, a), reflector_sizeof(TestReflection,a), 1 };
	StructureMember structure_TestReflection_1_member = { "b", "float", reflector_offsetof(TestReflection, b), reflector_sizeof(TestReflection,b), 1 };
	StructureMember* structure_TestReflection_0_members[] = { &structure_TestReflection_0_member, &structure_TestReflection_1_member };
	StructureScope structure_TestReflection_0_scope(StructureScope::Type::Public, 2, structure_TestReflection_0_members);
	StructureScope* structure_TestReflection_scopes[] = { &structure_TestReflection_0_scope };
	Structure structure_TestReflection(Structure::Type::Struct, 1, structure_TestReflection_scopes, 0, 0);
	
}
