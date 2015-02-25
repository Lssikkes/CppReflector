#include "simple.h"
#include "..\..\src\reflector\reflector.h"

namespace reflector {
	// FILE: "tests/test3.xh"
	StructureMember m_0 = { VisibilityEnum::Public, "a", "int", reflector_offsetof(TestReflection, a), reflector_sizeof(TestReflection, a), 1 };
	StructureMember m_1 = { VisibilityEnum::Public, "b", "float", reflector_offsetof(TestReflection, b), reflector_sizeof(TestReflection, b), 1 };
	StructureMember* sm_2[] = { &m_0, &m_1 };
	Structure s_2(VisibilityEnum::Public, Structure::Type::Struct, "TestReflection", 2, sm_2, 0, 0);
	// END FILE: "tests/test3.xh"
} // end namespace reflector