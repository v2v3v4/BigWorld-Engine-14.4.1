#ifndef BW_MESSAGE_BOX_HPP
#define BW_MESSAGE_BOX_HPP

#include "cstdmf/guard.hpp"

BW_BEGIN_NAMESPACE

enum MessageBoxFlags
{
	BW_MB_OK			= 1,
	BW_MB_ICONWARNING	= 1 << 1,
};

int MessageBox( void * parent, const wchar_t * text, const wchar_t * caption,
				MessageBoxFlags flags );

BW_END_NAMESPACE

#endif // BW_MESSAGE_BOX_HPP
