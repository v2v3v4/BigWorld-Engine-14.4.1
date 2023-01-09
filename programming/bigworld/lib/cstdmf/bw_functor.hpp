#ifndef BW_FUNCTOR_HPP
#define BW_FUNCTOR_HPP


#include "smartpointer.hpp"

BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
// 0 param functor, no return
///////////////////////////////////////////////////////////////////////////////

class BWBaseFunctor0 : public ReferenceCount
{
public:
	virtual void operator()() = 0;
};

template< class C >
class BWFunctor0 : public BWBaseFunctor0
{
public:
	typedef void (C::*Method)();
	BWFunctor0( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	void operator()()
	{
		(instance_->*method_)();
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};


///////////////////////////////////////////////////////////////////////////////
// 1 param functor, no return
///////////////////////////////////////////////////////////////////////////////

template< typename P1 >
class BWBaseFunctor1 : public ReferenceCount
{
public:
	virtual void operator()( P1 param1 ) = 0;
};

template< class C, typename P1 >
class BWFunctor1 : public BWBaseFunctor1< P1 >
{
public:
	typedef void (C::*Method)( P1 );
	BWFunctor1( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	void operator()( P1 param1 )
	{
		(instance_->*method_)( param1 );
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};


///////////////////////////////////////////////////////////////////////////////
// 2 param functor, no return
///////////////////////////////////////////////////////////////////////////////

template< typename P1, typename P2 >
class BWBaseFunctor2 : public ReferenceCount
{
public:
	virtual void operator()( P1 param1, P2 param2 ) = 0;
};

template< class C, typename P1, typename P2 >
class BWFunctor2 : public BWBaseFunctor2< P1, P2 >
{
public:
	typedef void (C::*Method)( P1, P2 );
	BWFunctor2( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	void operator()( P1 param1, P2 param2 )
	{
		(instance_->*method_)( param1, param2 );
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};


///////////////////////////////////////////////////////////////////////////////
// 3 param functor, no return
///////////////////////////////////////////////////////////////////////////////

template< typename P1, typename P2, typename P3 >
class BWBaseFunctor3 : public ReferenceCount
{
public:
	virtual void operator()( P1 param1, P2 param2, P3 param3 ) = 0;
};

template< class C, typename P1, typename P2, typename P3 >
class BWFunctor3 : public BWBaseFunctor3< P1, P2, P3 >
{
public:
	typedef void (C::*Method)( P1, P2, P3 );
	BWFunctor3( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	void operator()( P1 param1, P2 param2, P3 param3 )
	{
		(instance_->*method_)( param1, param2, param3 );
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};


///////////////////////////////////////////////////////////////////////////////
// 0 param functor with return
///////////////////////////////////////////////////////////////////////////////

template< typename R >
class BWBaseFunctor0R : public ReferenceCount
{
public:
	virtual R operator()() = 0;
};

// non-const version
template< class C, typename R >
class BWFunctor0R : public BWBaseFunctor0R< R >
{
public:
	typedef R (C::*Method)();
	BWFunctor0R( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()()
	{
		return (instance_->*method_)();
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};

// const version
template< class C, typename R >
class BWFunctor0RC : public BWBaseFunctor0R< R >
{
public:
	typedef R (C::*Method)() const;
	BWFunctor0RC( const C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()()
	{
		return (instance_->*method_)();
	}
	const C* obj() const
	{
		return instance_;
	}
private:
	const C* instance_;
	Method method_;
};


///////////////////////////////////////////////////////////////////////////////
// 1 param functor with return
///////////////////////////////////////////////////////////////////////////////

template< typename P1, typename R >
class BWBaseFunctor1R : public ReferenceCount
{
public:
	virtual R operator()( P1 param1 ) = 0;
};

// non-const version
template< class C, typename P1, typename R >
class BWFunctor1R : public BWBaseFunctor1R< P1, R >
{
public:
	typedef R (C::*Method)( P1 );
	BWFunctor1R( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1 )
	{
		return (instance_->*method_)( param1 );
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};

// const version
template< class C, typename P1, typename R >
class BWFunctor1RC : public BWBaseFunctor1R< P1, R >
{
public:
	typedef R (C::*Method)( P1 ) const;
	BWFunctor1RC( const C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1 )
	{
		return (instance_->*method_)( param1 );
	}
	const C* obj() const
	{
		return instance_;
	}
private:
	const C* instance_;
	Method method_;
};


///////////////////////////////////////////////////////////////////////////////
// 2 param functor with return
///////////////////////////////////////////////////////////////////////////////

template< typename P1, typename P2, typename R >
class BWBaseFunctor2R : public ReferenceCount
{
public:
	virtual R operator()( P1 param1, P2 param2 ) = 0;
};

// non-const version
template< class C, typename P1, typename P2, typename R >
class BWFunctor2R : public BWBaseFunctor2R< P1, P2, R >
{
public:
	typedef R (C::*Method)( P1, P2 );
	BWFunctor2R( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1, P2 param2 )
	{
		return (instance_->*method_)( param1, param2 );
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};

// const version
template< class C, typename P1, typename P2, typename R >
class BWFunctor2RC : public BWBaseFunctor2R< P1, P2, R >
{
public:
	typedef R (C::*Method)( P1, P2 ) const;
	BWFunctor2RC( const C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1, P2 param2 )
	{
		return (instance_->*method_)( param1, param2 );
	}
	const C* obj() const
	{
		return instance_;
	}
private:
	const C* instance_;
	Method method_;
};


///////////////////////////////////////////////////////////////////////////////
// 3 param functor with return
///////////////////////////////////////////////////////////////////////////////

template< typename P1, typename P2, typename P3, typename R >
class BWBaseFunctor3R : public ReferenceCount
{
public:
	virtual R operator()( P1 param1, P2 param2, P3 param3 ) = 0;
};

// non-const version
template< class C, typename P1, typename P2, typename P3, typename R >
class BWFunctor3R : public BWBaseFunctor3R< P1, P2, P3, R >
{
public:
	typedef R (C::*Method)( P1, P2, P3 );
	BWFunctor3R( C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1, P2 param2, P3 param3 )
	{
		return (instance_->*method_)( param1, param2, param3 );
	}
	C* obj()
	{
		return instance_;
	}
private:
	C* instance_;
	Method method_;
};

// const version
template< class C, typename P1, typename P2, typename P3, typename R >
class BWFunctor3RC : public BWBaseFunctor3R< P1, P2, P3, R >
{
public:
	typedef R (C::*Method)( P1, P2, P3 ) const;
	BWFunctor3RC( const C* instance, Method method )
		: instance_( instance )
		, method_( method )
	{};
	R operator()( P1 param1, P2 param2, P3 param3 )
	{
		return (instance_->*method_)( param1, param2 );
	}
	const C* obj() const
	{
		return instance_;
	}
private:
	const C* instance_;
	Method method_;
};

BW_END_NAMESPACE

#endif // BW_FUNCTOR_HPP
