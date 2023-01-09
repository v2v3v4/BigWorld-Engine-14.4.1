#include "pch.hpp"
#include "chunk_embodiment.hpp"
#include "chunk_dynamic_obstacle.hpp"
#include "py_model_obstacle.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"

#include "duplo/pymodel.hpp"

DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ChunkEmbodiment
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChunkEmbodiment::ChunkEmbodiment( WantFlags wantFlags ) :
	ChunkItem( wantFlags ),
	pPyObject_( NULL )
{
}


/**
 *	Constructor.
 */
ChunkEmbodiment::ChunkEmbodiment( PyObject * pPyObject, WantFlags wantFlags ) :
	ChunkItem( wantFlags ),
	pPyObject_( pPyObject )
{

}


/**
 *	Destructor.
 */
ChunkEmbodiment::~ChunkEmbodiment()
{
}


// -----------------------------------------------------------------------------
// Section: ChunkEmbodiment script converters
// -----------------------------------------------------------------------------


/**
 *	This static method registers a converter function
 */
void ChunkEmbodiment::registerConverter( Converter cfn )
{
	BW_GUARD;
	if (s_converters_ == NULL)
	{
		s_converters_ = new Converters;
		static std::auto_ptr< Converters > destroyConvertors( s_converters_ );
	}
	s_converters_->push_back( cfn );
}

/// static initialiser
ChunkEmbodiment::Converters * ChunkEmbodiment::s_converters_;

/// ChunkEmbodiment converter
PyObject * Script::getData( const ChunkEmbodimentPtr pCA )
{
	BW_GUARD;
	if (pCA)
	{
		PyObject * pPyObject = &*pCA->pPyObject();
		Py_INCREF( pPyObject );
		return pPyObject;
	}

	Py_RETURN_NONE;
}

/// ChunkEmbodiment converter
int Script::setData( PyObject * pObj, ChunkEmbodimentPtr & pCA,
	const char * varName )
{
	BW_GUARD;
	ChunkEmbodimentPtr pNew = NULL;

	// look at the object
	if (pObj != Py_None)
	{
		const ChunkEmbodiment::Converters & convs =
			ChunkEmbodiment::converters();

		for (uint i = 0; i < convs.size(); i++)
		{
			int ret = convs[i]( pObj, pNew, varName );
			if (pNew) break;
			if (ret != 0) return ret;
		}

		if (!pNew)
		{
			PyErr_Format( PyExc_TypeError, "%s must be set to an object "
				"that can become an Embodiment", varName );
			return -1;
		}
	}

	// now replace the pointer
	pCA = pNew;

	return 0;
}




/// Hack method to pass a space pointer through python
static bool getChunkSpace( PyObject * pChunkSpaceInformer, ChunkSpace * & pCS )
{
	BW_GUARD;
	PyObject * pCSInt = PyObject_GetAttrString(
		pChunkSpaceInformer, ".space" );

	bool good = true;
	if (pCSInt == NULL || Script::setData( pCSInt, (int&)pCS ) != 0)
	{
		PyErr_SetString( PyExc_SystemError,
			"Owner of embodiment list won't tell its space!" );
		good = false;
	}

	Py_XDECREF( pCSInt );

	return good;
}

/// Release method releases the reference originally taken by setData
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
void PySTLObjectAid::Releaser< ChunkEmbodimentPtr >::release(
	ChunkEmbodimentPtr & pCA )
{
	BW_GUARD;
	//DEBUG_MSG( "Deleting chunk embodiment at 0x%08X\n", &*pCA );
	pCA = NULL;
}


/// Take a-hold of this model (put in world)
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
bool PySTLObjectAid::Holder< ChunkEmbodiments >::hold(
	ChunkEmbodimentPtr & pCA, PyObject * pOwner )
{
	BW_GUARD;
	//DEBUG_MSG( "Holder<ChunkEmbodiments>::hold: "
	//	"Calling hold for chunk attachment at 0x%08X\n", &*pCA );

	if (!pCA)
	{
		PyErr_SetString( PyExc_ValueError,
			"Model to add to entity list cannot be None" );
		return false;
	}

	ChunkSpace * pSpace = NULL;
	if (!getChunkSpace( pOwner, pSpace )) return false;
	if (pSpace != NULL) pCA->enterSpace( pSpace );

	return true;
}


// Let go of this model (remove from world)
#if _MSC_VER < 1310	// ISO templates
template <>
#endif
void PySTLObjectAid::Holder< ChunkEmbodiments >::drop(
	ChunkEmbodimentPtr & pCA, PyObject * pOwner )
{
	BW_GUARD;
	//DEBUG_MSG( "Calling drop for model at 0x%08X\n", &*pCA );

	MF_ASSERT_DEV( pCA );

	ChunkSpace * pSpace = NULL;
	if (!getChunkSpace( pOwner, pSpace ))
	{
		PyErr_Clear();
	}
	if (pSpace != NULL && pCA.exists()) pCA->leaveSpace();
}


// -----------------------------------------------------------------------------
// Section: ChunkDynamicEmbodiment
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChunkDynamicEmbodiment::ChunkDynamicEmbodiment( WantFlags wantFlags ) :
	ChunkEmbodiment( wantFlags ),
	pSpace_( NULL )
{
}


/**
 *	Constructor.
 */
ChunkDynamicEmbodiment::ChunkDynamicEmbodiment(
		PyObject * pPyObject, WantFlags wantFlags ) :
	ChunkEmbodiment( pPyObject, wantFlags ),
	pSpace_( NULL )
{
#ifndef MF_SERVER
	sceneObject_.flags().isDynamic( true );
	sceneObject_.flags().isCastingDynamicShadow( true );
#endif // MF_SERVER
}


/**
 *	Destructor.
 */
ChunkDynamicEmbodiment::~ChunkDynamicEmbodiment()
{
	BW_GUARD;
	MF_ASSERT_DEV( !pSpace_ );
}


/**
 *	See if we would prefer to settle down in a smaller chunk.
 */
void ChunkDynamicEmbodiment::nest( ChunkSpace * pSpace )
{
	BW_GUARD;
	const Vector3 & wpos = this->worldTransform().applyToOrigin();
	const Vector3 tpos( wpos[0], wpos[1] + 0.1f, wpos[2] );

	Chunk * pDest = pSpace_->findChunkFromPoint( tpos );
	if (pDest != pChunk_)
	{
		if (pChunk_ != NULL)
			pChunk_->delDynamicItem( this );
		else
			pSpace_->delHomelessItem( this );

		if (pDest != NULL)
			pDest->addDynamicItem( this );
		else
		{
			WARNING_MSG( "ChunkDynamicEmbodiment::nest: "
				"Went from being in a chunk to being homeless!\n" );
			pSpace_->addHomelessItem( this );
		}
	}
}


/**
 *	Enter the given space. Transient is set to true if switching spaces.
 */
void ChunkDynamicEmbodiment::enterSpace( ChunkSpacePtr pSpace, bool transient )
{
	BW_GUARD;
	MF_ASSERT_DEV( !pSpace_ );

	if( pSpace_.exists() )
		leaveSpace(transient);

	const Vector3 & wpos = this->worldTransform().applyToOrigin();
	const Vector3 tpos( wpos[0], wpos[1] + 0.1f, wpos[2] );

	Chunk * pDest = pSpace->findChunkFromPoint( tpos );
	if (pDest != NULL)
		pDest->addDynamicItem( this );
	else
		pSpace->addHomelessItem( this );

	pSpace_ = pSpace;
	lastTranslation_ = tpos;
}

/**
 *	Leave the current space.
 *	Transient is set to true if switching spaces.
 */
void ChunkDynamicEmbodiment::leaveSpace( bool transient )
{
	BW_GUARD;
	// if pSpace_ is not NULL then there should be a reference to us in the
	// space - and we should never be deleted from a leaveSpace call because
	// the caller should have a reference to us.
	IF_NOT_MF_ASSERT_DEV( pSpace_ )
	{
		return;
	}

	if (pChunk_ != NULL)
		pChunk_->delDynamicItem( this );
	else
		pSpace_->delHomelessItem( this );
	pSpace_ = NULL;
}


/**
 *	If we move but have no chunk then try to find one.
 *	Derived classes should call this method _after_ they move.
 */
void ChunkDynamicEmbodiment::move( float dTime )
{
	// if we're homeless apply to the housing commision
	//if (pSpace_ && pChunk_ == NULL) this->sync();

	// This is no longer necessary. If we move then our sync method
	// will handle putting us in the right chunk, and if we don't
	// then we will get a nest call when a new chunk has come online,
	// and we can determine if we should be inside it there.
}



/**
 *	This method updates the chunk that we are in when the matrix for this
 *	embodiment changes. It is up to derived classes to call this method when
 *	necessary. It must not be called when the space is missing or ticking!
 *	If you need to change the matrix then, then you have to save it somewhere
 *	and pass it to us from the next move call.
 */
void ChunkDynamicEmbodiment::sync()
{
	BW_GUARD;
	MF_ASSERT_DEV( pSpace_ && !pSpace_->ticking() );

	const Vector3 & wpos = this->worldTransform().applyToOrigin();
	const Vector3 tpos( wpos[0], wpos[1] + 0.1f, wpos[2] );

	if (pChunk_ != NULL)
	{
		float dia=1.f;
		// pass a radius for pymodel objects ....
		if ( PyObject_TypeCheck(pPyObject().get(), &PyModel::s_type_) )
		{
			PyModel* model = static_cast<PyModel*>( pPyObject().getObject() );
			BoundingBox bb;
			model->localBoundingBox(bb,true);
			Vector3 diff = bb.maxBounds() - bb.minBounds();
			dia = Vector2(diff.x,diff.z).length();
		}

		if ( PyObject_TypeCheck(pPyObject().get(), &PyModelObstacle::s_type_) )
		{
			PyModelObstacle* model = static_cast<PyModelObstacle*>( pPyObject().getObject() );
			BoundingBox bb;
			model->localBoundingBox(bb,true);
			Vector3 diff = bb.maxBounds() - bb.minBounds();
			dia = Vector2(diff.x,diff.z).length();
		}

		pChunk_->modDynamicItem( this, lastTranslation_, tpos, dia, true );
	}
	else
	{
		Chunk * pDest = pSpace_->findChunkFromPoint( tpos );
		if (pDest != NULL)
		{
			pSpace_->delHomelessItem( this );
			pDest->addDynamicItem( this );
		}
	}

	lastTranslation_ = tpos;
}




// -----------------------------------------------------------------------------
// Section: ChunkStaticEmbodiment
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChunkStaticEmbodiment::ChunkStaticEmbodiment( WantFlags wantFlags ) :
	ChunkEmbodiment( (WantFlags)(wantFlags | WANTS_NEST) ),
	pSpace_( NULL )
{
}


/**
 *	Constructor.
 */
ChunkStaticEmbodiment::ChunkStaticEmbodiment(
		PyObject * pPyObject, WantFlags wantFlags ) :
	ChunkEmbodiment( pPyObject, (WantFlags)(wantFlags | WANTS_NEST) ),
	pSpace_( NULL )
{
}


/**
 *	Destructor.
 */
ChunkStaticEmbodiment::~ChunkStaticEmbodiment()
{
	MF_ASSERT_DEV( !pSpace_ );
}


/**
 *	Set our (world-space) nest position, for a chunk jog.
 */
void ChunkStaticEmbodiment::nest( ChunkSpace * pSpace )
{
	BW_GUARD;
	const Vector3 & wpos = this->worldTransform().applyToOrigin();
	const Vector3 tpos( wpos[0], wpos[1] + 0.1f, wpos[2] );

	Chunk * pDest = pSpace_->findChunkFromPoint( tpos );
	if (pDest != pChunk_)
	{
		if (pChunk_ != NULL)
			pChunk_->delStaticItem( this );
		else
			pSpace_->delHomelessItem( this );

		if (pDest != NULL)
			pDest->addStaticItem( this );
		else
		{
			WARNING_MSG( "ChunkStaticEmbodiment::nest: "
				"Went from being in a chunk to being homeless!\n" );
			pSpace_->addHomelessItem( this );
		}
	}
}


/**
 *	Enter the given space. Transient is set to true if switching spaces.
 */
void ChunkStaticEmbodiment::enterSpace( ChunkSpacePtr pSpace, bool transient )
{
	BW_GUARD;
	MF_ASSERT_DEV( !pSpace_ );

	if( pSpace_ )
		leaveSpace(transient);

	const Vector3 & wpos = this->worldTransform().applyToOrigin();
	const Vector3 tpos( wpos[0], wpos[1] + 0.1f, wpos[2] );

	Chunk * pDest = pSpace->findChunkFromPoint( tpos );
	if (pDest != NULL)
		pDest->addStaticItem( this );
	else
		pSpace->addHomelessItem( this );

	pSpace_ = pSpace;
}

/**
 *	Leave the current space.
 *	Transient is set to true if switching spaces.
 */
void ChunkStaticEmbodiment::leaveSpace( bool transient )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pSpace_ )
	{
		return;
	}

	if (pChunk_ != NULL)
		pChunk_->delStaticItem( this );
	else
		pSpace_->delHomelessItem( this );
	pSpace_ = NULL;
}


BW_END_NAMESPACE

// chunk_embodiment.cpp
