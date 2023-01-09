#ifndef STL_TO_PY_HPP
#define STL_TO_PY_HPP

#include "pyobject_plus.hpp"
#include "script.hpp"
#include "resmgr/datasection.hpp"
#include <typeinfo>


BW_BEGIN_NAMESPACE

namespace Datatype {
/* TODO: should group together with other template
 * DefaultValue specialisations and put them into an
 * independent place rather than stay with datasection.
 * cause too much header inclusion problem ATM.
 */
	template <> struct DefaultValue<PyObject *>
		{ static inline PyObject * val() { return NULL; } };
}

// -----------------------------------------------------------------------------
// Section: PySTLObjectAid
// -----------------------------------------------------------------------------

/**
 *	This namespace contains helper template classes used by the
 *	PySTLSequence and PySTLMapping systems in this header.
 */
namespace PySTLObjectAid
{
	/**
	 *	This template class extracts various type from a sequence
	 */
	template <class SEQ> struct SeqTypes
	{
		typedef typename SEQ::value_type Value;
		typedef typename SEQ::iterator Iterator;
	};

	/**
	 *	This template class does special stuff to release item types
	 *	after they have been setData'd into or removed from a sequence.
	 */
	template <class TYPE> struct Releaser
	{
		static void release( TYPE & /*it*/ ) { }
	};

	/**
	 *	Explicit specialisation of the Releaser class for PyObject *'s
	 *	@see Releaser
	 */
	template <> struct Releaser< PyObject * >
	{
		static void release( PyObject * & pObject )
		{
			Py_XDECREF( pObject );
		}
	};

	/**
	 *	This template class does special stuff to hold an item instance
	 *	in a sequence. The hold method in this class must return true for
	 *	the item to be allowed in. The template parameter is the sequence
	 *	rather than the actual item type, to make it easier to define
	 *	special behaviour for a particular sequence.
	 *
	 *	@note The hold method should set an exception explaining what the
	 *	problem is whenever it returns false.
	 *
	 *	@note Dropping then re-holding an item should have no adverse
	 *	effects, and should always succeed (provided of course nothing
	 *	has happened to the item in the mean time)
	 */
	template <class SEQ> struct Holder
	{
		typedef typename SeqTypes<SEQ>::Value		ItemType;
		typedef typename SeqTypes<SEQ>::Iterator		IteratorType;

		static bool hold( ItemType & /*it*/, PyObject * /*pOwner*/ )
			{ return true; }
		static void drop( ItemType & /*it*/, PyObject * /*pOwner*/ )
			{ }
	};
}


// -----------------------------------------------------------------------------
// Section: PySTLSequence
// -----------------------------------------------------------------------------


class PySTLSequenceHolderBase;

/*~ class NoModule.PySTLSequence
 *	@components{ all }
 *
 *	This class is used to expose STL sequences from the engine to Python.
 *	It behaves in exactly the same way as a standard Python sequence,
 *	except that it has a length property.
 *	In particular, if v, x, y and z are sequences, either standard Python
 *	or PySTLSequence and i and j are integers and n is a number, 
 *	then the following operations are valid:
 *
 *	@{
 *	len(x)
 *	x + y
 *	x * n
 *	x[i]
 *	x[i:j]
 *	x[i] = v
 *	x[i:j] = v
 *	v in x
 *	x += y
 *	x *= n
 *	@}
 */
/**
 *	This class wraps a STL sequence in a python object
 */
class PySTLSequence : public PyObjectPlus
{
	Py_Header( PySTLSequence, PyObjectPlus )

public:
	PySTLSequence( PySTLSequenceHolderBase & holder, PyObject * pOwner,
		PyTypeObject * pType = &PySTLSequence::s_type_ );
	~PySTLSequence();

	PY_SIZE_INQUIRY_METHOD(			pySeq_length )			// len(x)
	PY_BINARY_FUNC_METHOD(			pySeq_concat )			// x + y
	PY_INTARG_FUNC_METHOD(			pySeq_repeat )			// x * n
	PY_INTARG_FUNC_METHOD(			pySeq_item )			// x[i]
	PY_INTINTARG_FUNC_METHOD(		pySeq_slice )			// x[i:j]
	PY_INTOBJARG_PROC_METHOD(		pySeq_ass_item )		// x[i] = v
	PY_INTINTOBJARG_PROC_METHOD(	pySeq_ass_slice )		// x[i:j] = v
	PY_OBJOBJ_PROC_METHOD(			pySeq_contains )		// v in x
	PY_BINARY_FUNC_METHOD(			pySeq_inplace_concat )	// x += y
	PY_INTARG_FUNC_METHOD(			pySeq_inplace_repeat )	// x *= n

	static PySequenceMethods s_seq_methods_;

	PyObject * pyRepr();

	PY_RO_ATTRIBUTE_DECLARE( this->pySeq_length(), length )


	PySTLSequenceHolderBase	&	holder()	{ return holder_; }

private:
	PySTLSequence( const PySTLSequence & );
	PySTLSequence & operator=( const PySTLSequence & );

	PySTLSequenceHolderBase	&	holder_;
	PyObject *					pOwner_;
};



/**
 *	This abstract class defines the interface between a PySTLSequence
 *	and its controlled sequence. This interface does not depend on the
 *	actual type of the object, and therefore our python type doesn't have
 *	to be templatised (which would be the straightforward way), since
 *	templatising python types is a pain (especially across compilers).
 *
 *	Sequences can be flagged as writable or not.
 *
 *	@note There should only instance of this class per vector. If you have
 *	multiple holders holding the same vector, then some python operations
 *	will not work because they'll be writing over the data they want to read.
 */
class PySTLSequenceHolderBase
{
public:
	PySTLSequenceHolderBase( PyObject * pOwnerIn, bool writableIn ) :
		pOwner_( pOwnerIn ),
		writable_( writableIn )
	{
	}
	virtual ~PySTLSequenceHolderBase() {};

	// These methods are called by the python object to access the sequence:

	// get sequence size
	virtual int size() = 0;

	// get a new reference to the item with the given index
	//  (assumes index is in range; should return None if somehow fails)
	virtual PyObject * operator[]( int index ) = 0;

	// return true if the given object is in the sequence
	virtual bool contains( PyObject * pObject ) = 0;

	// erase the given range, i.e. [indexBegin,indexEnd)
	//  (assumes indices are in range)
	virtual void erase( int indexBegin, int indexEnd ) = 0;

	// define the range for future insertions.
	//  (tolerates num < 0)
	virtual void insertRange( int index, int count ) = 0;

	// add the given item to the insert list.
	//  (assumes index is in range; keeps own ref to pItem if necessary;
	//  sets an exception and returns -1 if pItem won't go in)
	virtual int insert( PyObject * pItem ) = 0;

	// note: there can be at most one erase in a commit group,
	//  and it must come before any of the inserts. there can
	//  be at most one insertRange in a commit group.
	// also: the effects of erase and insert as far as operator[]
	//  is concerned should be invisible until after the commit.

	// commit erase and inserts
	virtual void commit() = 0;

	// cancel erase and inserts
	virtual void cancel() = 0;

	PyObject *	pOwner() const			{ return pOwner_; }
	bool		writable() const		{ return writable_; }

protected:
	PyObject *	pOwner_;
	bool		writable_;
	// possibility: add an integer 'setLike' member.
	// 0 = none, 1 = silently drop if duped, 2 = generate exception if duped
};




/**
 *	This template class implements the methods in PySTLSequenceHolderBase.
 *	@see PySTLSequenceHolderBase
 */
template <class SEQ> class PySTLSequenceHolder : public PySTLSequenceHolderBase
{
public:
	typedef typename PySTLObjectAid::SeqTypes<SEQ>::Value		ItemType;
	typedef typename PySTLObjectAid::SeqTypes<SEQ>::Iterator		IteratorType;
	typedef typename PySTLObjectAid::Releaser<ItemType>			Releaser;
	typedef typename PySTLObjectAid::Holder<SEQ>					Holder;

	PySTLSequenceHolder( SEQ & seq, PyObject * pOwnerIn, bool writableIn );

	int size();

	PyObject * operator[]( int index );

	bool contains( PyObject * pObject );

	void erase( int indexBegin, int indexEnd );

	void insertRange( int index, int count );

	int insert( PyObject * pItem );

	void commit();

	void cancel();

private:
	SEQ &		seq_;

	int			eraseBegin_;
	int			eraseEnd_;

	int			insertStart_;
	int			insertCount_;

	ItemType *	insertStore_;
	int			insertStock_;

protected:
	// Used under controlled circumstances to retrieve the last element added
	ItemType& lastInsert( ) 
	{ 
		MF_ASSERT(insertStore_ && insertStock_ && insertStock_ <= insertCount_)
		return insertStore_[insertStock_-1];
	}
public:
	// stupid VC can't initialise a static member of a template class
	//  if its type is a private inner class that has a constructor >8-[
	/**
	 * TODO: to be documented.
	 */
	class InitVNV : public Script::InitTimeJob
	{
	public:
		InitVNV() : Script::InitTimeJob( -1 ) {}
		operator const char *() const	{ return varNameValue_.c_str(); }

		virtual void init()
		{
			varNameValue_ = "An item of a PySTLSequence of ";
			varNameValue_+= typeid(ItemType).name();
		}
	private:
		BW::string	varNameValue_;
	};

private:
	static InitVNV s_varNameValue_;
};



// setData and getData methods for PySTLSequenceHolderBase
namespace Script
{
	int setData( PyObject * pObject, PySTLSequenceHolderBase & data,
		const char * varName = "" );

	PyObject * getData( const PySTLSequenceHolderBase & data );

	template<class SEQ>
	PyObject * getReadOnlyData( const PySTLSequenceHolder<SEQ> & data )
	{
		return getData( data );
	}
}



// -----------------------------------------------------------------------------
// Section: PySTLMapping
// -----------------------------------------------------------------------------


#include "stl_to_py.ipp"

BW_END_NAMESPACE

#endif // STL_TO_PY_HPP
