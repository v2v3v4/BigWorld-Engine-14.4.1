#ifndef PY_TO_STL_HPP
#define PY_TO_STL_HPP

#include "Python.h"

#include <iostream>
#include <utility>

#include "cstdmf/stdmf.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements a reference to a PyObject *.
 *
 *	The reference may be used in the way that an ordinary reference to a
 *	pointer is, i.e. it can be converted into the underlying object, it can
 *	be derferenced as using the * and -> operators, and it can be assigned
 *	to, replacing the underlying object.
 *
 *	When you take the address of it with &, you do not of course get a
 *	PyObject** ... but this is necessary, so that when you dereference that
 *	you get something you can still assign to.
 *
 *	It was created to aid classes which have to return non-const 'references'
 *	to their member objects, especially where others expect to be able to
 *	assign to these references, but where such assignment is a not a simple
 *	object copy, but needs to call a function to do it.
 *
 *	Note that when you get a PyObject* out of one of these, by whatever
 *	means, then you own the reference to it! (because this may be the only
 *	reference to it if it's an object returned by, say, a class which
 *	is pretending to be a sequence). Similarly, when you assign a PyObject*
 *	to it, a reference is kept (i.e. not borrowed)
 */
class PyObjectPtrRef
{
public:
	virtual ~PyObjectPtrRef() {}
	PyObjectPtrRef & operator=( const PyObject * pObject )
		{ this->set( pObject ); return *this; }

	operator PyObject *()				{ return this->get(); }
	PyObject & operator*()				{ return *this->get(); }
	PyObject * operator->()				{ return this->get(); }
	operator const PyObject *() const	{ return this->get(); }
	const PyObject & operator*() const	{ return *this->get(); }
	const PyObject * operator->() const	{ return this->get(); }

protected:
	virtual const PyObject * get() const = 0;
	virtual PyObject * get() = 0;
	virtual void set( const PyObject * pObject ) = 0;
};



/**
 *	This class implements the PyObjectPtrRef interface for the simple
 *	case where pointer copy and reference counting is all that's required.
 */
class PyObjectPtrRefSimple : public PyObjectPtrRef
{
public:
	PyObjectPtrRefSimple( PyObject * & rpObject ) :
			rpObject_( rpObject )
		{ Py_XINCREF( rpObject_ ); }

	PyObjectPtrRefSimple( const PyObjectPtrRefSimple & other ) :
			PyObjectPtrRef( other ),
			rpObject_( other.rpObject_ )
		{ Py_XINCREF( rpObject_ ); }

	// we can't have an assignment operator 'coz we've got a reference...
	// plus we theoretically are a reference, so we shouldn't have one anyway.

	virtual ~PyObjectPtrRefSimple()
		{ Py_XDECREF( rpObject_ ); }

private:
	virtual const PyObject * get() const
	{
		return const_cast<PyObjectPtrRefSimple*>(this)->get();
	}

	virtual PyObject * get()
	{
		Py_XINCREF( rpObject_ );
		return rpObject_;
	}

	virtual void set( const PyObject * pObject )
	{
		Py_XINCREF( const_cast<PyObject*>(pObject) );
		Py_XDECREF( rpObject_ );
		rpObject_ = const_cast<PyObject*>(pObject);
	}

	PyObject * & rpObject_;
};





/**
 *	This class lets you use any python object supporting
 *	the 'sequence' interface as an STL sequence (which lets
 *	you make a SequenceWatcher of it).
 *
 *	It holds a reference to the sequence it wraps.
 *
 *	Note that due to the types of references returned (they're
 *	a separate object, not a value_type &), iterators cannot
 *	support the -> operator ... they only support the * operator.
 */
class PySequenceSTL
{
public:
	typedef size_t size_type;
	typedef PyObject * value_type;

	PySequenceSTL( PyObject * pObject );
	PySequenceSTL( const PySequenceSTL & toCopy );
	PySequenceSTL & operator =( const PySequenceSTL & toCopy );
	~PySequenceSTL();

	static bool Check( PyObject * pObject );


	/** @internal */
	class reference : public PyObjectPtrRef
	{
	public:
		reference( PyObject * pSeq, size_t index ) :
			pSeq_( pSeq ), index_( index )
		{
			Py_INCREF( pSeq_ );
		}

		reference( const reference & other ) :
			PyObjectPtrRef( other ),
			pSeq_( other.pSeq_ ),
			index_( other.index_ )
		{
			Py_INCREF( pSeq_ );
		}

		// not that this operation is allowed on normal references :-/
		reference & operator =( const reference & other )
		{
			if (&other != this)
			{
				Py_DECREF( pSeq_ );
				pSeq_ = other.pSeq_;
				index_ = other.index_;
				Py_INCREF( pSeq_ );
			}
			return *this;
		}

		virtual ~reference()
			{ Py_DECREF( pSeq_ ); }

	private:
		virtual const PyObject * get() const
		{
			return const_cast<reference*>(this)->get();
		}

		virtual PyObject * get()
		{
			return PySequence_GetItem( pSeq_, index_ );
		}

		virtual void set( const PyObject * pObject )
		{
			PySequence_SetItem( pSeq_, index_, const_cast<PyObject*>( pObject ) );
		}

		PyObject	* pSeq_;
		size_t		index_;
	};

	/* we can't use const with typedefed value_type here
	 * see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=38759 for details
	 */
	typedef const PyObject * const_reference;

	reference operator[]( size_type index )
		{	return reference( pSeq_, index );	}

	const_reference operator[]( size_type index ) const
		{	return PySequence_GetItem( pSeq_, index );	}



	/**
	 *  This nested class of PySequenceSTL implements the const iterator.
	 */
	class const_iterator
	{
	public:
#ifndef _WIN32
		typedef PySequenceSTL::value_type value_type;
		typedef size_t difference_type;
		typedef PySequenceSTL::value_type * pointer;
		typedef PySequenceSTL::reference reference;
//		typedef forward_iterator_tag iterator_category;
#endif

		const_iterator( const PySequenceSTL & seq, size_type index ) :
			pSeq_( &seq ), index_( index ) { }

		bool operator==( const const_iterator & other ) const
			{ return pSeq_ == other.pSeq_ && index_ == other.index_; }

		bool operator!=( const const_iterator & other ) const
			{ return pSeq_ != other.pSeq_ || index_ != other.index_; }

		size_t operator-( const const_iterator & other ) const
		{
			return index_ - other.index_;
		}

		const_reference operator *() const
			{ return (*pSeq_)[ index_ ]; }

		void operator+=( size_type amt )
			{ index_ += amt; }
		void operator++( int )
			{ index_++; }
		const_iterator & operator++()
			{ ++index_; return *this; }

	protected:
		const PySequenceSTL	* pSeq_;
		size_type			index_;
	};

	/**
	 *  This nested class of PySequenceSTL implements the non-const iterator.
	 */
	class iterator : public const_iterator
	{
	public:
		iterator( PySequenceSTL & seq, size_type index ) :
		  const_iterator( seq, index ) { }

		reference operator *()
			{ return const_cast<PySequenceSTL&>(*pSeq_)[ index_ ]; }
	};

	size_type size() const			{ return PySequence_Size( pSeq_ );	}

	iterator begin()				{ return iterator( *this, 0 ); }
	const_iterator begin() const	{ return const_iterator( *this, 0 ); }

	iterator end()					{ return iterator( *this, this->size() ); }
	const_iterator end() const		{ return const_iterator( *this, this->size() ); }


	bool operator==( const PySequenceSTL & other ) const
		{	return pSeq_ == other.pSeq_;	}

	bool operator!=( const PySequenceSTL & other ) const
		{	return pSeq_ != other.pSeq_;	}

private:

	PyObject	* pSeq_;
};





/**
 *	This class lets you use any python object supporting
 *	the 'mapping' interface as an STL map (which lets
 *	you make a MapWatcher of it).
 *
 *	It holds a reference to the map it wraps.
 *
 *	@see PySequenceSTL
 */
class PyMappingSTL
{
public:
	typedef size_t     size_type;
	typedef PyObject * referent_type;
	typedef PyObject * key_type;

	PyMappingSTL( PyObject * pObject );
	PyMappingSTL( const PyMappingSTL & toCopy );
	PyMappingSTL & operator =( const PyMappingSTL & toCopy );
	~PyMappingSTL();

	static bool Check( PyObject * pObject );


	/**
	 *  This nested class implements a reference to a PyMappingSTL object.
	 *
	 *	@see PyObjectPtrRef
	 */
	class value_reference : public PyObjectPtrRef
	{
	public:
		value_reference( PyObject * pMap, PyObject * pKey ) :
			pMap_( pMap ), pKey_( pKey )
		{
			Py_INCREF( pMap_ );
			Py_INCREF( pKey_ );
		}

		value_reference( const value_reference & other ) :
			PyObjectPtrRef( other ),
			pMap_( other.pMap_ ),
			pKey_( other.pKey_ )
		{
			Py_INCREF( pMap_ );
			Py_INCREF( pKey_ );
		}

		// not that this operation is allowed on normal references :-/
		value_reference & operator =( const value_reference & other )
		{
			if (&other != this)
			{
				Py_DECREF( pKey_ );
				Py_DECREF( pMap_ );

				pMap_ = other.pMap_;
				pKey_ = other.pKey_;

				Py_INCREF( pMap_ );
				Py_INCREF( pKey_ );
			}
			return *this;
		}

		virtual ~value_reference()
		{
			Py_DECREF( pKey_ );
			Py_DECREF( pMap_ );
		}

	private:
		virtual const PyObject * get() const
		{
			return const_cast<value_reference*>(this)->get();
		}

		virtual PyObject * get()
		{
			PyObject * v = PyDict_GetItem( pMap_, pKey_ );
			Py_XINCREF( v );
			return v;
		}

		virtual void set( const PyObject * pObject )
		{
			PyDict_SetItem( pMap_, pKey_, const_cast<PyObject*>( pObject ) );
			// PyDict_SetItem saves a reference, unlike
			// PyDict_GetItem which borrows one.
		}

		PyObject	* pMap_;
		PyObject	* pKey_;
	};
	typedef value_reference _Tref;			// as it's known with VC6
	typedef value_reference mapped_type;	// as it's known with VC7
	typedef std::pair<key_type,value_reference> value_type;
	typedef value_type reference;	// not value_type & as is usual
	typedef std::pair<key_type,referent_type> const_value_type;
	typedef const_value_type const_reference;

	// really needs to create it if its not there!
	value_reference operator[]( PyObject * pKey )
		{	return value_reference( pMap_, pKey );	}
	// (map has no const operator[])


	/**
	 *  This nested class of PyMappingSTL implements the const iterator.
	 */
	class const_iterator
	{
	public:
		const_iterator( const PyMappingSTL & map, size_type index ) :
			pMap_( map.grope() ), pKeys_( NULL ), index_( index )
		{
			if (index < map.size())
			{
				pKeys_ = PyMapping_Keys( const_cast<PyObject*>(pMap_) );
				// only copy the keys if this isn't an end iterator
				// (note: only works since we don't do operator--)
			}
		}

		const_iterator( const PyMappingSTL & map, const PyObject & rKey ) :
			pMap_( map.grope() ), pKeys_( NULL ), index_( map.size() )
		{
			// first check the key is there...
			if (PyMapping_HasKey( const_cast<PyObject*>(pMap_),
				const_cast<PyObject*>(&rKey) ))
			{
				pKeys_ = PyMapping_Keys( const_cast<PyObject*>(pMap_) );

				// ok, find it in the keys then
				const PySequenceSTL pKeysSeq( pKeys_ );

//				index_ = std::find( pKeysSeq.begin(), pKeysSeq.end(), &rKey ) -
//					pKeysSeq.begin();	// paying off already!
			}

			// this constructor takes a reference instead of a * to
			// disambiguate it from the other one when '0' is passed in.
		}

		const_iterator( const const_iterator & other ) :
			pMap_( other.pMap_ ), pKeys_( other.pKeys_ ), index_( other.index_ )
		{
			Py_XINCREF( pKeys_ );
		}

		const_iterator & operator= ( const const_iterator & other )
		{
			if (&other != this)
			{
				Py_XDECREF( pKeys_ );
				pMap_ = other.pMap_;
				pKeys_ = other.pKeys_;
				index_ = other.index_;
				Py_XINCREF( pKeys_ );
			}
			return *this;
		}

		~const_iterator()
		{
			Py_XDECREF( pKeys_ );
		}

		bool operator==( const const_iterator & other ) const
			{ return pMap_ == other.pMap_ && index_ == other.index_; }

		bool operator!=( const const_iterator & other ) const
			{ return pMap_ != other.pMap_ || index_ != other.index_; }

		size_t operator-( const const_iterator & other ) const
			{ return index_ - other.index_; }

		const_reference operator *() const
		{
			PyObject * key = PyList_GetItem( const_cast<PyObject*>(pKeys_), index_ );
			// returns a borrowed reference
			return const_reference( key,
				PyDict_GetItem( const_cast<PyObject*>(pMap_), key ) );
		}

		void operator+=( size_type amt )
			{ index_ += amt; }
		void operator++( int )
			{ index_++; }
		const_iterator & operator++()
			{ ++index_; return *this; }

	protected:
		const PyObject		* pMap_;
		PyObject			* pKeys_;
		size_type			index_;
	};

	/**
	 *  This nested class of PyMappingSTL implements the non-const iterator.
	 */
	class iterator : public const_iterator
	{
	public:
		iterator( PyMappingSTL & map, size_type index ) :
		  const_iterator( map, index ) { }

		iterator( PyMappingSTL & map, const PyObject & rKey ) :
		  const_iterator( map, rKey ) { }

		reference operator *()
		{
			PyObject * key = PyList_GetItem( pKeys_, index_ );
			return reference( key,
				value_reference( const_cast<PyObject*>(pMap_), key ) );
		}
	};

	size_type size() const			{ return PyMapping_Size( pMap_ );	}

	iterator begin()				{ return iterator( *this, 0 ); }
	const_iterator begin() const	{ return const_iterator( *this, 0 ); }

	iterator end()					{ return iterator( *this, this->size() ); }
	const_iterator end() const		{ return const_iterator( *this, this->size() ); }


	iterator find( const PyObject * pKey )
		{ return iterator( *this, *pKey ); }
	const_iterator find( const PyObject * pKey ) const
		{ return const_iterator( *this, *pKey ); }


	bool operator==( const PyMappingSTL & other ) const
		{ return pMap_ == other.pMap_; }

	bool operator!=( const PyMappingSTL & other ) const
		{ return pMap_ != other.pMap_; }


	PyObject * grope() { return pMap_; }
	const PyObject * grope() const { return pMap_; }

private:

	PyObject	* pMap_;
};


/**
 *  This class specialises the MapTypes template for a PyMappingSTL.
 *
 *  @see MapTypes
 *  @see PyMappingSTL
 */
template <> struct MapTypes<PyMappingSTL>
{
	typedef PyMappingSTL::_Tref _Tref;
};


// PyObjectPtrRef streaming operators

std::ostream& operator<<( std::ostream &o, const PyObjectPtrRef & rpObject );
std::istream& operator>>( std::istream &i, PyObjectPtrRef & pObject );

/**
 *	Output streaming operators for a PyObject *, just a wrapper on the
 *	PyObjectPtrRef output streaming operator.
 */
inline std::ostream& operator<<( std::ostream &o, const PyObject * & pObject )
	{ return o << PyObjectPtrRefSimple( const_cast<PyObject*&>(pObject) );	}

inline std::ostream& operator<<( std::ostream &o, const PyObject * pObject )
	{ return o << PyObjectPtrRefSimple( const_cast<PyObject*&>(pObject) );	}

/**
 *	Input streaming operator for a reference to a PyObject *. The
 *	reference bit is really important (as usual with input streaming
 *	operators), because that's where it's going to set the pointer.
 *	i.e. this would not work with just PyObject *.
 *	Manages references appropriately of course.
 */
inline std::istream& operator>>( std::istream &i, PyObject * & pObject )
	{	PyObjectPtrRefSimple poprs( pObject );	return i >> poprs; }


#ifdef CODE_INLINE
#include "py_to_stl.ipp"
#endif


BW_END_NAMESPACE

#endif // PY_TO_STL_HPP
