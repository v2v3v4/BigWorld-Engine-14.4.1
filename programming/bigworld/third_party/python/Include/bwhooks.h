#ifndef BW_PYTHON_HOOKS_HPP
#define BW_PYTHON_HOOKS_HPP

#ifdef __cplusplus
extern "C" {
#endif

typedef void * ( * BW_Py_MallocFunc )( size_t );
typedef void ( * BW_Py_FreeFunc)( void * );
typedef void * ( * BW_Py_ReallocFunc)( void * , size_t );
typedef void ( * BW_Py_IgnoreAllocsFunc)( void );

typedef struct
{
	BW_Py_MallocFunc mallocHook;
	BW_Py_FreeFunc freeHook;
	BW_Py_ReallocFunc reallocHook;
	BW_Py_IgnoreAllocsFunc ignoreAllocsBeginHook;
	BW_Py_IgnoreAllocsFunc ignoreAllocsEndHook;
} BW_Py_Hooks;

void* BW_Py_malloc( size_t size );
void BW_Py_free( void * mem );
void* BW_Py_realloc( void * mem, size_t size );

void BW_Py_memoryTrackingIgnoreBegin( void );
void BW_Py_memoryTrackingIgnoreEnd( void );

void BW_Py_setHooks( BW_Py_Hooks * hooks );
void BW_Py_getHooks( BW_Py_Hooks * hooks );

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* BW_PYTHON_HOOKS_HPP */


