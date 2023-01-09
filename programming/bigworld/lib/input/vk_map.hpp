#ifndef VK_MAP__HPP
#define VK_MAP__HPP

#include "key_code.hpp"
#include "cstdmf/cstdmf_windows.hpp"

BW_BEGIN_NAMESPACE

namespace VKMap 
{
	KeyCode::Key fromVKey( USHORT vkey, bool extended );
	KeyCode::Key fromRawKey( RAWKEYBOARD* rkb );
	USHORT toVKey( KeyCode::Key key );
}

BW_END_NAMESPACE

#endif // VK_MAP__HPP
