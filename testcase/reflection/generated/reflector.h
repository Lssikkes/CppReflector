#pragma once

namespace reflector
{
	typedef unsigned int index;

	class StructureMember
	{
	public:
		const char* Identifier;
		const char* Typename;
		size_t Offset;
		size_t Size;
		size_t ArraySize;
	};

	class StructureScope
	{
	public:
		enum class Type
		{
			Public,
			Private,
			Protected
		};

		StructureScope(Type type, index memberCount, StructureMember** members);

		Type GetType() const { return m_type; }

		int GetMemberCount();
		StructureMember& GetMember(index i);
	protected:
		Type m_type;

		int m_member_count;
		StructureMember** m_members;
	};

	class StructureFriend
	{
	public:

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

		Structure(Type type, index scopeCount, StructureScope** scopes, index friendCount, StructureFriend** friends);
		
		int GetScopeCount();
		int GetFriendCount();
		StructureScope& GetScope(index idx);
		StructureFriend& GetFriend(index idx);
	protected:
		Type m_type;
		
		index m_scope_count;
		index m_friend_count;
		StructureScope** m_scopes;
		StructureFriend** m_friends;
	};
}; // end reflector scope