#pragma once

#ifdef DEBUG
#include <stdexcept>
#define REFL_SAFE
#endif
#ifdef REFL_SAFE
#define REFL_SAFE_ARRAY_RETURN(arr, count, idx) if((index)idx >= count) throw std::runtime_error("out of bounds (" # arr ")");
#else
#define REFL_SAFE_ARRAY_RETURN(arr, count, idx)
#endif

#define reflector_offsetof(type,member) ((size_t) &(((type*)0)->member))
#define reflector_sizeof(type,member) ((size_t) sizeof((((type*)0)->member)))

namespace reflector
{
	typedef unsigned int index;

	enum class VisibilityEnum
	{
		Public,
		Private,
		Protected
	};

	class StructureMember
	{
	public:
		VisibilityEnum Visibility;
		const char* Identifier;
		const char* Typename;
		size_t Offset;
		size_t Size;
		size_t ArraySize;
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

		Structure(VisibilityEnum visibility, Type type, const char* ident, index memberCount, StructureMember** members, index friendCount, StructureFriend** friends)
			: m_member_count(memberCount)
			, m_members(members)
			, m_friend_count(friendCount)
			, m_type(type)
			, m_visibility(visibility)
			, m_identifier(ident) {}

		index GetMemberCount() { return m_member_count; }
		index GetFriendCount() { return m_friend_count; }
		StructureMember& GetMember(index idx) { REFL_SAFE_ARRAY_RETURN(m_members, m_member_count, idx); return *m_members[idx]; }
		StructureFriend& GetFriend(index idx) { REFL_SAFE_ARRAY_RETURN(m_friends, m_friend_count, idx); return *m_friends[idx]; }

		const char* GetName() { return m_identifier; }
	protected:
		VisibilityEnum m_visibility;
		Type m_type;

		const char* m_identifier;
		index m_member_count;
		index m_friend_count;
		StructureMember** m_members;
		StructureFriend** m_friends;
	};
}; // end reflector scope

#undef SAFE_ARRAY_RETURN
