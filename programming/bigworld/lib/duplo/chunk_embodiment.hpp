#ifndef CHUNK_EMBODIMENT_HPP
#define CHUNK_EMBODIMENT_HPP

#include "chunk/chunk_item.hpp"
#include "chunk/chunk_space.hpp"

#include "pyscript/script.hpp"
#include "pyscript/stl_to_py.hpp"


BW_BEGIN_NAMESPACE

class ChunkEmbodiment;
typedef SmartPointer<ChunkEmbodiment> ChunkEmbodimentPtr;

/**
 *	This class is the abstract base class for chunk embodiments.
 *
 *	Chunk embodiments are used by anyone that wants to give itself a
 *	configurable embodiment in the world, as maintained by the chunk system.
 *
 *	Various derived classes define the actual makeup and presentation of
 *	the embodiment. For example, ChunkAttachment is a ChunkEmbodiment that
 *	provides the root for a tree of PyAttachments.
 *
 *	In general, ChunkEmbodiments wrap a PyObject which provides the actual
 *	drawing function of the item. The ChunkEmbodiment takes care of the
 *	chunk side of things.
 */
class ChunkEmbodiment : public ChunkItem
{
public:
	explicit ChunkEmbodiment( WantFlags wantFlags = WANTS_NOTHING );
	ChunkEmbodiment( PyObject * pPyObject, WantFlags wantFlags = WANTS_NOTHING );
	virtual ~ChunkEmbodiment();

	virtual void enterSpace( ChunkSpacePtr pSpace, bool transient = false ) = 0;
	virtual void leaveSpace( bool transient = false ) = 0;

	virtual void move( float dTime ) = 0;

	PyObjectPtr pPyObject() const			{ return pPyObject_; }
	PyObject & operator *() const			{ return *pPyObject_; }
	PyObject * operator ->() const			{ return &*pPyObject_; }

	virtual const Matrix & worldTransform() const = 0;
	virtual void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false ) const = 0;
	virtual void localBoundingBox( BoundingBox & bb, bool skinny = false ) const = 0;

protected:
	PyObjectPtr		pPyObject_;

	void pPyObject( PyObject* pyObject )	{ pPyObject_ = pyObject; }

public:
	typedef int (*Converter)( PyObject * pObj, ChunkEmbodimentPtr & pNew,
		const char * varName );
	typedef BW::vector<Converter> Converters;

	static void registerConverter( Converter cfn );
	static const Converters & converters() { return *s_converters_; }

private:
	ChunkEmbodiment( const ChunkEmbodiment& );
	ChunkEmbodiment& operator=( const ChunkEmbodiment& );

	static Converters * s_converters_;
};


/**
 * This class is basically a vector of ChunkEmbodiments
 */
class ChunkEmbodiments : public BW::vector<ChunkEmbodimentPtr> {};

/**
 * This class registers the convertor function of the templated class.
 * Classes that apply to this template are: ChunkAttachment, ChunkDynamicObstacle and ChunkStaticObstacle.
 */
template <class C> struct ChunkEmbodimentRegisterer
{
	ChunkEmbodimentRegisterer()
		{ ChunkEmbodiment::registerConverter( C::convert ); }
};


namespace Script
{
	PyObject * getData( const ChunkEmbodimentPtr pCA );
	int setData( PyObject * pObj, ChunkEmbodimentPtr & pCA,
		const char * varName = "" );
}



/*
 *	Stuff to make a vector of embodiments work on its own in python
 */
namespace PySTLObjectAid
{
	/// Releaser is same for all PyObject's (but not for ChunkEmbodiment)
	template <> struct Releaser< ChunkEmbodimentPtr >
	{
		static void release( ChunkEmbodimentPtr & pCA );
	};

	/// Holder is special for our embodiment list
	template <> struct Holder< ChunkEmbodiments >
	{
		static bool hold( ChunkEmbodimentPtr & it, PyObject * pOwner );
		static void drop( ChunkEmbodimentPtr & it, PyObject * pOwner );
	};
};



/**
 *	This class implements a standard dynamic chunk item.
 */
class ChunkDynamicEmbodiment : public ChunkEmbodiment
{
public:
	ChunkDynamicEmbodiment( WantFlags wantFlags = WANTS_NOTHING );
	ChunkDynamicEmbodiment( PyObject * pPyObject, WantFlags wantFlags = WANTS_NOTHING );
	virtual ~ChunkDynamicEmbodiment();

	virtual void nest( ChunkSpace * pSpace );

	virtual void enterSpace( ChunkSpacePtr pSpace, bool transient );
	virtual void leaveSpace( bool transient );
	virtual void move( float dTime );

	void sync();

protected:
	ChunkSpacePtr	pSpace_;
	Vector3			lastTranslation_;
};


/**
 *	This class implements a standard static chunk item.
 *	It copes with chunks being loaded and unloaded.
 */
class ChunkStaticEmbodiment : public ChunkEmbodiment
{
public:
	ChunkStaticEmbodiment( WantFlags wantFlags = WANTS_NOTHING );
	ChunkStaticEmbodiment( PyObject * pPyObject, WantFlags wantFlags = WANTS_NOTHING );
	virtual ~ChunkStaticEmbodiment();

	virtual void nest( ChunkSpace * pSpace );

	virtual void enterSpace( ChunkSpacePtr pSpace, bool transient);
	virtual void leaveSpace( bool transient );
	virtual void move( float dTime ) { }

protected:
	ChunkSpacePtr	pSpace_;
};

BW_END_NAMESPACE

#endif // CHUNK_EMBODIMENT_HPP
