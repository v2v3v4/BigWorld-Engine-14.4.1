#ifndef DEBUG_EXCEPTION_FILTER_HPP
#define DEBUG_EXCEPTION_FILTER_HPP

#if defined( _WIN32 )


#include "bw_namespace.hpp"
#include "config.hpp"
#include "cstdmf_dll.hpp"
#include "cstdmf_windows.hpp"

BW_BEGIN_NAMESPACE

#if ENABLE_STACK_TRACKER

CSTDMF_DLL DWORD ExceptionFilter( DWORD exceptionCode, struct _EXCEPTION_POINTERS * ep );

template<typename T, typename R>
R CallWithExceptionFilter( T* obj, R (T::*method)() )
{
	if (IsDebuggerPresent())
	{
		return (obj->*method)();
	}

	__try
	{
		return (obj->*method)();
	}
	__except( ExceptionFilter(GetExceptionCode(), GetExceptionInformation()) )
	{}

	return R();
}


template<typename T, typename R, typename P1>
R CallWithExceptionFilter( T* obj, R (T::*method)( P1 p1 ), P1 p1 )
{
	if (IsDebuggerPresent())
	{
		return (obj->*method)( p1 );
	}

	__try
	{
		return (obj->*method)( p1 );
	}
	__except( ExceptionFilter(GetExceptionCode(), GetExceptionInformation()) )
	{}

	return R();
}

template<typename R, typename P1>
R CallWithExceptionFilter( R (*method)( P1 p1 ), P1 p1 )
{
	if (IsDebuggerPresent())
	{
		return method( p1 );
	}

	__try
	{
		return method( p1 );
	}
	__except( ExceptionFilter(GetExceptionCode(), GetExceptionInformation()) )
	{}

	return R();
}

template<typename R, typename P1, typename P2, typename P3>
R CallWithExceptionFilter( R (*method)( P1 p1, P2 p2, P3 p3 ), P1 p1, P2 p2, P3 p3 )
{
	if (IsDebuggerPresent())
	{
		return method( p1, p2, p3 );
	}

	__try
	{
		return method( p1, p2, p3 );
	}
	__except( ExceptionFilter(GetExceptionCode(), GetExceptionInformation()) )
	{}

	return R();
}

template<typename R, typename P1, typename P2, typename P3, typename P4>
R CallWithExceptionFilter( R (*method)( P1 p1, P2 p2, P3 p3, P4 p4 ), P1 p1, P2 p2, P3 p3, P4 p4 )
{
	if (IsDebuggerPresent())
	{
		return method( p1, p2, p3, p4 );
	}

	__try
	{
		return method( p1, p2, p3, p4 );
	}
	__except( ExceptionFilter(GetExceptionCode(), GetExceptionInformation()) )
	{}

	return R();
}

template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
R CallWithExceptionFilter( R (*method)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 ), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 )
{
	if (IsDebuggerPresent())
	{
		return method( p1, p2, p3, p4, p5 );
	}

	__try
	{
		return method( p1, p2, p3, p4, p5 );
	}
	__except( ExceptionFilter(GetExceptionCode(), GetExceptionInformation()) )
	{}

	return R();
}

#else // ENABLE_STACK_TRACKER

template<typename T, typename R>
R CallWithExceptionFilter( T* obj, R (T::*method)() )
{
	return (obj->*method)();
}

template<typename T, typename R, typename P1>
R CallWithExceptionFilter( T* obj, R (T::*method)( P1 p1 ), P1 p1 )
{
	return (obj->*method)( p1 );
}

template<typename R, typename P1>
R CallWithExceptionFilter( R (*method)( P1 p1 ), P1 p1 )
{
	return method( p1 );
}

template<typename R, typename P1, typename P2, typename P3>
R CallWithExceptionFilter( R (*method)( P1 p1, P2 p2, P3 p3 ), P1 p1, P2 p2, P3 p3 )
{
	return method( p1, p2, p3, p4 );
}

template<typename R, typename P1, typename P2, typename P3, typename P4>
R CallWithExceptionFilter( R (*method)( P1 p1, P2 p2, P3 p3, P4 p4 ), P1 p1, P2 p2, P3 p3, P4 p4 )
{
	return method( p1, p2, p3, p4 );
}

template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
R CallWithExceptionFilter( R (*method)( P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 ), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5 )
{
	return method( p1, p2, p3, p4, p5 );
}

#endif // ENABLE_STACK_TRACKER

#if !defined( _XBOX360 )

LONG WINAPI ErrorCodeExceptionFilter( _EXCEPTION_POINTERS * ExceptionInfo );
LONG WINAPI SuccessExceptionFilter( _EXCEPTION_POINTERS * ExceptionInfo );
#endif // _XBOX360

BW_END_NAMESPACE

#endif // _WIN32

#endif // DEBUG_EXCEPTION_FILTER_HPP
