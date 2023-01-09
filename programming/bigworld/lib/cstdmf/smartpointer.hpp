#ifndef SMART_POINTER_HPP
#define SMART_POINTER_HPP

#include "cstdmf_dll.hpp"

#include "stdmf_minimal.hpp"
#include "allocator.hpp"
#include "concurrency.hpp"

#ifdef _XBOX360
# include "Xtl.h"
#endif

#if defined( PLAYSTATION3 )
#include <cell/atomic.h>
#endif

#define REFERENCE_COUNT_THREADING_DEBUG_IGNORE_FIRST_THREAD
#ifdef WIN32
#define BW_FASTCALL __fastcall
//#define SAFE_REFERENCE_COUNT_DEBUG
#else
#define BW_FASTCALL
#endif

BW_BEGIN_NAMESPACE

//forward delaration because of circular dependency
inline unsigned long OurThreadID();

/**
 *	This class is used to implement the behaviour required of an object
 *	referenced by a SmartPointer.
 *
 *	This class is not thread safe.
 *
 *	It stores a reference count with the object.
 */
class ReferenceCount
{
#if ENABLE_REFERENCE_COUNT_THREADING_DEBUG
	mutable unsigned long threadID_;
#ifdef REFERENCE_COUNT_THREADING_DEBUG_IGNORE_FIRST_THREAD
	mutable bool firstThread_;
#endif//REFERENCE_COUNT_THREADING_DEBUG_IGNORE_FIRST_THREAD
	static const unsigned long INVALID_THREAD_ID = (unsigned long)-1;

	void check() const
	{
		if (threadID_ == INVALID_THREAD_ID)
		{
			threadID_ = OurThreadID();
#ifdef REFERENCE_COUNT_THREADING_DEBUG_IGNORE_FIRST_THREAD
			firstThread_ = true;
#endif//REFERENCE_COUNT_THREADING_DEBUG_IGNORE_FIRST_THREAD
		}
		else
		{
			if (threadID_ != OurThreadID())
			{
#ifdef REFERENCE_COUNT_THREADING_DEBUG_IGNORE_FIRST_THREAD
				if (firstThread_)
				{
					threadID_ = OurThreadID();
					firstThread_ = false;
				}
				else
#endif//REFERENCE_COUNT_THREADING_DEBUG_IGNORE_FIRST_THREAD
				{
					MF_ASSERT( !"ReferenceCounted object is accessed from different thread" );
				}
			}
		}
	}
	#define REFERENCE_COUNT_THREAD_CHECK check()
#else//ENABLE_REFERENCE_COUNT_THREADING_DEBUG
	#define REFERENCE_COUNT_THREAD_CHECK	(void)0
#endif//ENABLE_REFERENCE_COUNT_THREADING_DEBUG

protected:
	/**
	 *	Constructor.
	 */
	ReferenceCount() :
		count_( 0 )
	{
#if ENABLE_REFERENCE_COUNT_THREADING_DEBUG
		threadID_ = INVALID_THREAD_ID;
#endif//ENABLE_REFERENCE_COUNT_THREADING_DEBUG
		REFERENCE_COUNT_THREAD_CHECK;
	}

	/**
	 *	Copy constructor.
	 */
	ReferenceCount( const ReferenceCount & ) :
		count_( 0 )
	{
#if ENABLE_REFERENCE_COUNT_THREADING_DEBUG
		threadID_ = INVALID_THREAD_ID;
#endif//ENABLE_REFERENCE_COUNT_THREADING_DEBUG
		REFERENCE_COUNT_THREAD_CHECK;
	}

	/**
	 *	Destructor.
	 */
	virtual ~ReferenceCount() 
	{
	}

public:

	/**
	 *	This method increases the reference count by 1.
	 */
	inline int32 incRef() const
	{
		REFERENCE_COUNT_THREAD_CHECK;
		++count_;
		return count_;
	}

	/**
	 *	This method decreases the reference count by 1.
	 */
	inline int32 decRef() const
	{
		REFERENCE_COUNT_THREAD_CHECK;
		int32 count = --count_;
		if (count == 0)
		{
			delete this;
		}
		return count;
	}

	/**
	 *	This method returns the reference count of this object.
	 */
	inline int32 refCount() const
	{
		return count_;
	}

private:
	/**
	 *	Assignment operator, which makes no sense for objects
	 *	with a reference count.
	 */
	ReferenceCount & operator=( const ReferenceCount & );

	mutable int32 count_;
};


/**
 *	This class is a thread safe reference count
 *	@see ReferenceCount
 */
class CSTDMF_DLL SafeReferenceCount
{
#ifdef SAFE_REFERENCE_COUNT_DEBUG
	static SafeReferenceCount*& debuggingRef()
	{
		static SafeReferenceCount* s_ref = NULL;

		return s_ref;
	}
	static void debugingRefModified()
	{
		GetTickCount();// use this function to set break point
	}
#endif // SAFE_REFERENCE_COUNT_DEBUG

protected:

	SafeReferenceCount() :
		count_( 0 ) {}

	SafeReferenceCount( const SafeReferenceCount & ) :
		count_( 0 ) {}

	virtual ~SafeReferenceCount() {}

public:

	/**
	 *	This method increases the reference count by 1.
	 */
	inline int32 BW_FASTCALL incRef() const
	{
#if defined( __APPLE__ ) || defined( __ANDROID__ )
		// TODO: atomic increment
		int32 count = ++count_;
#else
		int32 count = BW_ATOMIC32_INC_AND_FETCH( &count_ );
#endif

#ifdef SAFE_REFERENCE_COUNT_DEBUG
		if (this == debuggingRef())
		{
			debugingRefModified();
		}
#endif // SAFE_REFERENCE_COUNT_DEBUG

		return count;
	}


	/**
	 *	This method decreases the reference count by 1.
	 */
	inline int32 BW_FASTCALL decRef() const
	{
#ifdef SAFE_REFERENCE_COUNT_DEBUG
		if (this == debuggingRef())
		{
			debugingRefModified();
		}
#endif // SAFE_REFERENCE_COUNT_DEBUG

#if defined( __APPLE__ ) || defined( __ANDROID__ )
		// TODO: atomic decrement
		int32 count = --count_;
#else
		int32 count = BW_ATOMIC32_DEC_AND_FETCH( &count_ );
#endif

		if (count == 0)
		{
			this->destroy();
		}
		return count;
	}

	/**
	 *	This method returns the reference count of this object.
	 */
	inline int32 BW_FASTCALL refCount() const
	{
		return count_;
	}

private:

	virtual void destroy() const
	{
		delete this;
	}

	SafeReferenceCount & operator=( const SafeReferenceCount & );

	mutable bw_atomic32_t count_;
};


/**
 *	This function increments the reference count of the input object.
 *	It can be specialised to increment the reference count of classes
 *	that do not have an 'incRef' method.
 */
template <class Ty>
inline void incrementReferenceCount( const Ty &Q )
{
	Q.incRef();
}


/**
 *	This function returns true if the reference count of the input object is
 *	equal to zero, or returns false otherwise.
 */
template <class Ty>
inline bool hasZeroReferenceCount( const Ty &Q )
{
	return (Q.refCount() == 0);
}


/**
 *	This function decrements the reference count of the input object.
 *	It can be specialised to increment the reference count of classes
 *	that do not have an 'decRef' method.
 *
 *	@note The decRef method in the object must delete itself when its
 *	reference count reaches zero - it is not done by the smart pointer
 *	itself (so it works with objects not allocated on the normal heap)
 */
template <class Ty>
inline void decrementReferenceCount( const Ty &Q )
{
	Q.decRef();
}


/**
 *	This function could be redefined to check that a pointer isn't null
 */
template <class Ty> inline
void null_pointer_check( const Ty * /*P*/ )
{
}



/**
 *	A ConstSmartPointer is a reference-counting smart const pointer to an
 *	Object. A SmartPointer is a ConstSmartPointer which allows non-const access
 *	to its Object.
 *
 *	Requirements for use are that for a given class C, the
 *	following expressions are valid:
 *
 *		EXPRESSION
 *		incrementReferenceCount(const C&)
 *		decrementReferenceCount(const C&)
 *
 *	Notes:
 *	- can be used as pointers, including the use of implicit casts. ie:
 *			class Base {};
 *			class Derived : public Base {};
 *			SmartPointer<Base> b;
 *			SmartPoiner<Derived> d;
 *			b = d;		 // ok, can create a base from a derived
 *			d = b;		 // compile-time error
 */
template<class Ty> class ConstSmartPointer
{
public:
	static const bool STEAL_REFERENCE = true;
	static const bool NEW_REFERENCE = false;

	/// This type is what this pointer can point to.
	typedef Ty Object;
	/// A short-hand for this class.
	typedef ConstSmartPointer<Ty> This;

public:
	/**
	 *	This constructor initialises this pointer to refer to the input object.
	 */
	ConstSmartPointer( const Object *P = 0, bool alreadyIncremented = false )
	{
		object_ = P;
		if (object_)
		{
			if (!alreadyIncremented)
			{
				incrementReferenceCount( *object_ );
			}
			else
			{
				if (hasZeroReferenceCount( *object_ ))
				{
					MF_ASSERT_DEV( 0 &&
						"ConstSmartPointer::ConstSmartPointer - The reference "
						"count should not be zero\nfor objects that already "
						"have their reference incremented" );
				}
			}
			BW_MEMORYDEBUG_SMARTPOINTER_ASSIGN( this, object_ );
		}
	}

	/**
	 *	The copy constructor.
	 */
	ConstSmartPointer( const This& P )
	{
		object_ = P.get();
		if (object_)
		{
			incrementReferenceCount( *object_ );
			BW_MEMORYDEBUG_SMARTPOINTER_ASSIGN(this, object_);
		}
	}
#ifdef _XBOX
	/**
	 * Copy constructor for derived objects.
	 */
	template<class DerivedType>
	ConstSmartPointer( const ConstSmartPointer<DerivedType>& dt )
	{
		object_ = dt.get();
		if (object_) 
		{
			incrementReferenceCount( *object_ );
			BW_MEMORYDEBUG_SMARTPOINTER_ASSIGN(this, object_);
		}
	}
#endif

	/**
	 *	The assignment operator.
	 */
	This& operator=( const This& X )
	{
		if (object_ != X.get())
		{
			const Object* pOldObj = object_;
			object_ = X.get();

			BW_MEMORYDEBUG_SMARTPOINTER_ASSIGN( this, object_ );

			if (object_) incrementReferenceCount( *object_ );
			if (pOldObj) decrementReferenceCount( *pOldObj );
		}
		return *this;
	}
#ifdef _XBOX
	/**
	 *  Assignment operator for derived types.
	 */
	template<class DerivedType>
	This& operator=( const ConstSmartPointer<DerivedType>& dt )
	{
		*this = This(dt.get());
		return *this;
	}
#endif

	/**
	 *	Destructor.
	 */
	~ConstSmartPointer()
	{
		if (object_)
		{
			decrementReferenceCount( *object_ );
			BW_MEMORYDEBUG_SMARTPOINTER_ASSIGN(this, NULL);
		}
		object_ = 0;
	}

	/**
	 *	This method returns the object that this pointer points to.
	 */
	const Object * get() const
	{
		return object_;
	}

	const Object * getObject() const
	{
		return this->get();
	}

	/**
	 *	This method returns whether or not this pointer points to anything.
	 */
	bool hasObject() const
	{
		return object_ != 0;
	}

	/**
	 *	This method returns whether or not this pointer points to anything.
	 */
	bool exists() const
	{
		return object_ != 0;
	}

	/**
	 *	This method implements the dereference operator. It helps allow this
	 *	object to be used as you would a normal pointer.
	 */
	const Object& operator*() const
	{
		null_pointer_check( object_ );
		return *object_;
	}

	/**
	 *	This method implements the dereference operator. It helps allow this
	 *	object to be used as you would a normal pointer.
	 */
	const Object* operator->() const
	{
		null_pointer_check( object_ );
		return object_;
	}

	/**
	 *	These functions return whether or not the input objects refer to the same
	 *	object.
	 */
	friend bool operator==( const ConstSmartPointer<Ty>& A,
		const ConstSmartPointer<Ty>& B )
	{
		return A.object_ == B.object_;
	}

	friend bool operator==( const ConstSmartPointer<Ty>& A,
		const Ty* B )
	{
		return A.object_ == B;
	}

	friend bool operator==( const Ty* A,
		const ConstSmartPointer<Ty>& B )
	{
		return A == B.object_;
	}

	/**
	 *	These functions return not or whether the input objects refer to the same
	 *	object.
	 */
	friend bool operator!=( const ConstSmartPointer<Ty>& A,
		const ConstSmartPointer<Ty>& B )
	{
		return A.object_ != B.object_;
	}

	friend bool operator!=( const ConstSmartPointer<Ty>& A,
		const Ty* B )
	{
		return A.object_ != B;
	}

	friend bool operator!=( const Ty* A,
		const ConstSmartPointer<Ty>& B )
	{
		return A != B.object_;
	}

	/**
	 *	These functions give an ordering on smart pointers so that they can be
	 *	placed in sorted containers.
	 */
	friend bool operator<( const ConstSmartPointer<Ty>& A,
		const ConstSmartPointer<Ty>& B )
	{
		return A.object_ < B.object_;
	}

	friend bool operator<( const ConstSmartPointer<Ty>& A,
		const Ty* B )
	{
		return A.object_ < B;
	}

	friend bool operator<( const Ty* A,
		const ConstSmartPointer<Ty>& B )
	{
		return A < B.object_;
	}

	/**
	 *	These functions give an ordering on smart pointers so that they can be
	 *	compared.
	 */
	friend bool operator>( const ConstSmartPointer<Ty>& A,
		const ConstSmartPointer<Ty>& B )
	{
		return A.object_ > B.object_;
	}

	friend bool operator>( const ConstSmartPointer<Ty>& A,
		const Ty* B )
	{
		return A.object_ > B;
	}

	friend bool operator>( const Ty* A,
		const ConstSmartPointer<Ty>& B )
	{
		return A > B.object_;
	}

	/**
	 *	This method returns whether or not this pointers points to anything.
	 */

	typedef const Ty * ConstSmartPointer<Ty>::*unspecified_bool_type;
	operator unspecified_bool_type() const
	{
		return object_ == 0? 0 : &ConstSmartPointer<Ty>::object_;
	}

protected:
	const Object *object_;		///< The object being pointed to.
};


/**
 *	This class is a ConstSmartPointer which also allows non-const access to
 *	its object.
 *
 *	@see ConstSmartPointer
 */
template <class Ty>
class SmartPointer : public ConstSmartPointer<Ty>
{
public:
	/// This type is a short-hand for this type.
	typedef SmartPointer<Ty> This;
	/// This type is the base class of this class.
	typedef ConstSmartPointer<Ty> ConstProxy;
	/// This type is what this pointer can point to.
	typedef Ty Object;

public:
	/**
	 *	This constructor initialises this pointer to refer to the input object.
	 */
	SmartPointer( Object *P = 0, bool alreadyIncremented = false ) :
		ConstProxy( P, alreadyIncremented ) { }

	/**
	 *	The copy constructor.
	 */
	SmartPointer( const This& P ) : ConstProxy( P ) { }

	/**
	 * Copy constructor from derived types.
	 */
	template<class DerivedType>
	SmartPointer( const ConstSmartPointer<DerivedType>& dt ) :
		ConstProxy( dt.get() )
	{
	}

	/**
	 *	The assignment operator.
	 */
	This& operator=( const This& P )
	{
		ConstProxy::operator=(P);
		return *this;
	}

	/**
	 *  Assignment operator for derived types.
	 */
	template<class DerivedType>
	This& operator=( const ConstSmartPointer<DerivedType>& dt )
	{
#ifndef _XBOX
		ConstProxy::operator=(dt.get());
#else
		ConstProxy::operator=(dt);
#endif
		return *this;
	}

	/**
	 *	This method returns the object that this pointer points to.
	 */
	Object * get() const
	{
		return const_cast<Object *>( this->object_ );
	}

	Object * getObject() const
	{
		// Deprecated version of 'get'.
		return this->get();
	}

	/**
	 *	This method implements the dereference operator. It helps allow this
	 *	object to be used as you would a normal pointer.
	 */
	Object& operator*() const
	{
		null_pointer_check( this->object_ );
		return *const_cast<Object *>( this->object_ );
	}

	/**
	 *	This method implements the dereference operator. It helps allow this
	 *	object to be used as you would a normal pointer.
	 */
	Object* operator->() const
	{
		null_pointer_check( this->object_ );
		return const_cast<Object *>( this->object_ );
	}
};

BW_END_NAMESPACE

#endif // SMART_POINTER_HPP
