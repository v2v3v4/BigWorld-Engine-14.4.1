#ifndef HOOK_H
#define HOOK_H

#include <cstddef>

#if defined( WIN32 )
#define MEMHOOK_CDECL __cdecl
#else // WIN32
#define MEMHOOK_CDECL
#endif // WIN32

// forward declare std::nothrow_t
namespace std { struct nothrow_t; }

namespace BW
{
namespace Memhook
{
	typedef void * (MEMHOOK_CDECL *NewFuncType)( size_t );
	typedef void * (MEMHOOK_CDECL *NewNoThrowFuncType)( size_t, const std::nothrow_t & );
	typedef void * (MEMHOOK_CDECL *NewArrayFuncType)( size_t );
	typedef void * (MEMHOOK_CDECL *NewArrayNoThrowFuncType)( size_t, const std::nothrow_t & );
	typedef void   (MEMHOOK_CDECL *DeleteFuncType)( void * );
	typedef void   (MEMHOOK_CDECL *DeleteNoThrowFuncType)( void *, const std::nothrow_t & );
	typedef void   (MEMHOOK_CDECL *DeleteArrayFuncType)( void * );
	typedef void   (MEMHOOK_CDECL *DeleteArrayNoThrowFuncType)( void *, const std::nothrow_t & );
	typedef void * (MEMHOOK_CDECL *MallocFuncType)( size_t );
	typedef void * (MEMHOOK_CDECL *AlignedMallocFuncType)( size_t, size_t );
	typedef void   (MEMHOOK_CDECL *FreeFuncType)( void* );
	typedef void   (MEMHOOK_CDECL *AlignedFreeFuncType)( void* );
	typedef void * (MEMHOOK_CDECL *ReallocFuncType)( void*, size_t );
	typedef size_t (MEMHOOK_CDECL *MSizeFuncType)( void * );

	struct AllocFuncs
	{
		NewFuncType new_;
		NewNoThrowFuncType newNoThrow_;
		NewArrayFuncType newArray_;
		NewNoThrowFuncType newArrayNoThrow_;
		DeleteFuncType delete_;
		DeleteNoThrowFuncType deleteNoThrow_;
		DeleteArrayFuncType deleteArray_;
		DeleteArrayNoThrowFuncType deleteArrayNoThrow_;
		MallocFuncType malloc_;
		FreeFuncType free_;
		AlignedMallocFuncType alignedMalloc_;
		AlignedFreeFuncType alignedFree_;
		ReallocFuncType realloc_;
		MSizeFuncType msizeFunc_;
	};

	// It is only safe to set these when there's just one thread running (i.e. at start up)
	void allocFuncs( const AllocFuncs& );
	const AllocFuncs& allocFuncs();

} // end namespace
} // end namespace
 

#endif // HOOK_H

