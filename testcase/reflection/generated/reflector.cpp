#include "reflector.h"


#ifdef DEBUG
#include <stdexcept>
#define SAFE
#endif

// Reflector Base API

namespace reflector
{
	Structure::Structure(Type type, index scopeCount, StructureScope** scopes, index friendCount, StructureFriend** friends)
	{
		m_type = type;
		m_scope_count = scopeCount;
		m_scopes = scopes;
		m_friend_count = friendCount;
		m_friends = friends;
	}

	StructureScope& Structure::GetScope(index idx)
	{
#ifdef SAFE
		if (idx >= m_scope_count)
			throw std::runtime_error("scope index out of bounds");
#endif

		return *m_scopes[idx];
	}

	int Structure::GetScopeCount()
	{
		return m_scope_count;
	}

	StructureFriend& Structure::GetFriend(index idx)
	{
#ifdef SAFE
		if (idx >= m_friend_count)
			throw std::runtime_error("friend index out of bounds");
#endif

		return *m_friends[idx];
	}

	int Structure::GetFriendCount()
	{
		return m_friend_count;
	}


	int StructureScope::GetMemberCount()
	{
		return m_member_count;
	}

	StructureMember& StructureScope::GetMember(index i)
	{
#ifdef SAFE
		if (idx >= m_member_count)
			throw std::runtime_error("member index out of bounds");
#endif
		return *m_members[i];
	}

	StructureScope::StructureScope(Type type, index memberCount, StructureMember** members)
	{
		m_type = type;
		m_member_count = memberCount;
		m_members = members;
	}

}

