#include "reflector.h"


#ifdef DEBUG
#include <stdexcept>
#define SAFE
#endif

// Reflector Base API

namespace reflector
{
	Structure::Structure()
	{
		m_scope_count = 0;
		m_scopes = 0;
		m_friend_count = 0;
		m_friends = 0;
	}

	StructureScope& Structure::GetScope(index idx)
	{
#ifdef SAFE
		if (idx >= m_scope_count)
			throw std::runtime_error("scope index out of bounds");
#endif

		return m_scopes[idx];
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

		return m_friends[idx];
	}

	int Structure::GetFriendCount()
	{
		return m_friend_count;
	}



}

