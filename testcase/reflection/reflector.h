#pragma once

namespace reflector
{
	typedef unsigned int index;

	class StructureMember
	{
		const char* Identifier;
		const char* Typename;
		int Offset;
		int Size;
		int ArraySize;
	};

	class StructureScope
	{

	};

	class StructureFriend
	{

	};

	class Structure
	{
	public:
		enum class Type
		{
			Class,
			Struct,
			Union,
		};

		Structure();
		int GetScopeCount();
		int GetFriendCount();
		StructureScope& GetScope(index idx);
		StructureFriend& GetFriend(index idx);
	protected:
		index m_scope_count;
		index m_friend_count;
		StructureScope* m_scopes;
		StructureFriend* m_friends;
	};
}; // end reflector scope