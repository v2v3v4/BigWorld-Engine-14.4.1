#include "pch.hpp"
#include "worldeditor/editor/item_locator.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_cache.hpp"
#include "worldeditor/world/items/editor_chunk_vlo.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "worldeditor/misc/editor_renderable.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "gizmo/item_view.hpp"
#include "moo/line_helper.hpp"
#include "physics2/worldtri.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: CollisionDebugger
// -----------------------------------------------------------------------------

// set this to true to display collision triangles
static const bool s_debugCollision = false;

// debug helper class CollisionDebugger
class CollisionDebugger : public EditorRenderable
{
public:
	static CollisionDebugger* instance()
	{
		if ( s_instance_ == NULL )
		{
			BW_GUARD;

			s_instance_ = new CollisionDebugger();
		}
		return s_instance_.getObject();
	}

	void addLine( const Vector3& start, const Vector3& end, uint32 colour )
	{
		BW_GUARD;

		lines_.push_back( start );
		lineEnds_.push_back( end );
		colours_.push_back( colour );
	}

	void render()
	{
		BW_GUARD;

		if (!lines_.empty())
		{
			for (uint32 i=0; i<lines_.size(); i++)
			{
				LineHelper::instance().drawLine( lines_[i], lineEnds_[i], colours_[i] );
			}
			// save states
			DWORD oldLighting;
			DWORD oldZEnable;
			Moo::rc().device()->GetRenderState( D3DRS_LIGHTING, &oldLighting );
			Moo::rc().device()->GetRenderState( D3DRS_ZENABLE, &oldZEnable );
			// change states
			Moo::rc().device()->SetRenderState( D3DRS_LIGHTING, FALSE );
			Moo::rc().device()->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
			// draw
			LineHelper::instance().purge();
			// restore states
			Moo::rc().device()->SetRenderState( D3DRS_LIGHTING, oldLighting );
			Moo::rc().device()->SetRenderState( D3DRS_ZENABLE, oldZEnable );
			// clear vectors
			lines_.clear();
			lineEnds_.clear();
			colours_.clear();
		}
	}

private:
	static SmartPointer<CollisionDebugger> s_instance_;

	CollisionDebugger()
	{
		BW_GUARD;

		WorldManager::instance().addRenderable( this );
	}

	BW::vector<Vector3> lines_;
	BW::vector<Vector3> lineEnds_;
	BW::vector<uint32> colours_;
};
SmartPointer<CollisionDebugger> CollisionDebugger::s_instance_ = NULL;



// -----------------------------------------------------------------------------
// Section: ChunkItemLocatorRevealer
// -----------------------------------------------------------------------------

class ChunkItemLocatorRevealer : public ChunkItemRevealer
{
	Py_Header( ChunkItemLocatorRevealer, ChunkItemRevealer )
public:
	ChunkItemLocatorRevealer( SmartPointer<ChunkItemLocator> pLoc,
			PyTypeObject * pType = &s_type_ ) :
		ChunkItemRevealer( pType ),
		pLoc_( pLoc )
	{
	}

	// should really be a C++ virtual method...
	//  with attribute in base class
	PY_RO_ATTRIBUTE_DECLARE( pLoc_->chunkItem() ? 1 : 0, size );

private:
	virtual void reveal( BW::vector< ChunkItemPtr > & items )
	{
		BW_GUARD;

		items.clear();
		ChunkItemPtr ci = pLoc_->chunkItem();
		if (ci && ci->chunk() && !EditorChunkCache::instance( *ci->chunk() ).edIsDeleted())
			items.push_back( ci );
	}

	SmartPointer<ChunkItemLocator>	pLoc_;
};

PY_TYPEOBJECT( ChunkItemLocatorRevealer )

PY_BEGIN_METHODS( ChunkItemLocatorRevealer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemLocatorRevealer )
	PY_ATTRIBUTE( size )
PY_END_ATTRIBUTES()

// -----------------------------------------------------------------------------
// Section: ChunkItemLocator
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ChunkItemLocator )

PY_BEGIN_METHODS( ChunkItemLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ChunkItemLocator )
	PY_ATTRIBUTE( revealer )
	PY_ATTRIBUTE( subLocator )
	PY_ATTRIBUTE( enabled )
PY_END_ATTRIBUTES()

PY_FACTORY( ChunkItemLocator, Locator )


/**
 *	Constructor.
 */
ChunkItemLocator::ChunkItemLocator( ToolLocatorPtr pSub, PyTypeObject * pType ) :
	ToolLocator( pType ),
	subLocator_( pSub ),
	enabled_( true ),
	positionValid_( false )
{
}


/**
 *	Destructor.
 */
ChunkItemLocator::~ChunkItemLocator()
{
}


/**
 *	A helper class to find and store the closest chunk item
 */
class ClosestObstacleCatcher : public CollisionCallback
{
public:
	ClosestObstacleCatcher( uint8 onFlags, uint8 offFlags) :
		onFlags_( onFlags ), offFlags_( offFlags ), dist_( 1000000000.f ) { }

	ChunkItemPtr	chunkItem_;	// initialises itself

private:
	float dist_;
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		BW_GUARD;

		ChunkItem * pItem = obstacle.sceneObject().isType<ChunkItem>() ?
			obstacle.sceneObject().getAs<ChunkItem>() : NULL;
		MF_ASSERT( pItem );
		if (!pItem)
		{
			return COLLIDE_ALL;
		}

		// We don't want terrain
		if( !Options::getOptionInt( "render/terrain", 1 ) )
		{
			if( ( triangle.flags() & TRIANGLE_TERRAIN ) )
				return COLLIDE_ALL;
			if( pItem->isShellModel() && SelectionFilter::getSelectMode() == SelectionFilter::SELECT_NOSHELLS )
				return COLLIDE_ALL;
		}
		if ((triangle.flags() & onFlags_) != onFlags_ ||
			(triangle.flags() & offFlags_)) 
		{
			// If the closest object is now the terrain then the previous closest
			// object cannot be selected.
			if( (triangle.flags() & TRIANGLE_TERRAIN) != 0 && dist < dist_ )
				chunkItem_ = NULL;
			return COLLIDE_BEFORE;
		}

		if( !pItem->edShouldDraw() )
			return COLLIDE_ALL;

		const Matrix& obstacleTransform = obstacle.transform();
		PlaneEq peq( obstacleTransform.applyPoint( triangle.v0() ),
			obstacleTransform.applyVector( triangle.normal() ) );
		bool frontFacing = peq.isInFrontOf( Moo::rc().invView()[3] );

		if ( s_debugCollision )
		{
			const uint32 FRONT_FACE_COLOUR = 0xFFFF80FF;
			const uint32 BACK_FACE_COLOUR = 0xFF802080;
			uint32 colour = FRONT_FACE_COLOUR;
			if ( !frontFacing )
				colour = BACK_FACE_COLOUR;
			CollisionDebugger::instance()->addLine(
				obstacle.transform().applyPoint( triangle.v0() ),
				obstacle.transform().applyPoint( triangle.v1() ),
				colour );
			CollisionDebugger::instance()->addLine(
				obstacle.transform().applyPoint( triangle.v1() ),
				obstacle.transform().applyPoint( triangle.v2() ),
				colour );
			CollisionDebugger::instance()->addLine(
				obstacle.transform().applyPoint( triangle.v2() ),
				obstacle.transform().applyPoint( triangle.v0() ),
				colour );
		}

		if ( !(triangle.flags() & TRIANGLE_DOUBLESIDED) )
		{
			// Check that we don't hit back facing triangles,
			// if we can't see it, we don't want to select it (unless it's
			// double-sided)
			if( !frontFacing )
			{
				if ( !pItem->isShellModel() )
					return COLLIDE_ALL;
				if( pItem->isShellModel() && !SelectionFilter::canSelect( &*pItem ) )
					return COLLIDE_ALL;
			}
		}

		if( pItem->isPortal() && !SelectionFilter::canSelect( &*pItem ) )
			return COLLIDE_ALL;

		if (pItem->edIsVLO())
		{
			EditorChunkVLO* vlo = (EditorChunkVLO*)pItem;

			if (!pItem->chunk()->isOutsideChunk())
			{
				VeryLargeObject::ChunkItemList items = vlo->object()->chunkItems();
				for (VeryLargeObject::ChunkItemList::iterator iter = items.begin();
					iter != items.end(); ++iter)
				{
					if ((*iter)->chunk()->isOutsideChunk())
					{
						return COLLIDE_ALL;
					}
				}

				if (WorldManager::instance().isChunkSelected( pItem->chunk() ))
				{
					return COLLIDE_ALL;
				}
			}
		}

		if( dist < dist_ )
		{
			dist_ = dist;
			if (!SelectionFilter::canSelect( &*pItem ))
				chunkItem_ = NULL;
			else
				chunkItem_ = pItem;
		}

		return COLLIDE_BEFORE;
	}

	uint8			onFlags_;
	uint8			offFlags_;
};


namespace
{
/**
 * Get a vector pointing from the camera position to the item's origin
 */
Vector3 vectorToItem( ChunkItemPtr item )
{
	BW_GUARD;

	Vector3 itemPos = item->chunk()->transform().applyPoint(
		item->edTransform().applyToOrigin()
		);

	Vector3 cameraPos = Moo::rc().invView().applyToOrigin();

	return itemPos - cameraPos;
}

/**
 * Calculate the distance between the origin of item and the ray
 */
float distToWorldRay ( ChunkItemPtr item, Vector3 worldRay )
{
	BW_GUARD;

	Vector3 diff = vectorToItem( item );

	float f = diff.dotProduct( worldRay );

	diff -= f * worldRay;

	// We're only using this for comparisons, so we don't need the actual length
	return diff.lengthSquared();
}
} // namespace


/** Get the ChunkItem closest to the worldRay */
class BestObstacleCatcher : public CollisionCallback
{
public:
	BestObstacleCatcher( ChunkItemPtr initialChunkItem, const Vector3& worldRay ) :
		chunkItem( initialChunkItem ), worldRay_( worldRay ) { }

	ChunkItemPtr	chunkItem;
private:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		BW_GUARD;

		// We don't want terrain
		if (triangle.flags() & TRIANGLE_TERRAIN)
			return Options::getOptionInt( "render/terrain", 1 ) ? COLLIDE_BEFORE : COLLIDE_ALL;

		ChunkItem * pItem = obstacle.sceneObject().isType<ChunkItem>() ?
			obstacle.sceneObject().getAs<ChunkItem>() : NULL;
		MF_ASSERT( pItem );
		if (!pItem)
		{
			return COLLIDE_ALL;
		}

		if( !Options::getOptionInt( "render/terrain", 1 ) )
		{
			if( pItem->isShellModel() && SelectionFilter::getSelectMode() == SelectionFilter::SELECT_NOSHELLS )
				return COLLIDE_ALL;
		}
		// We don't want non loaded items, either
		if (!pItem->chunk())
			return COLLIDE_BEFORE;

		if( !pItem->edShouldDraw() )
			return COLLIDE_ALL;

		if ( s_debugCollision )
		{
			const uint32 FRONT_FACE_COLOUR = 0xFFFFFF80;
			const uint32 BACK_FACE_COLOUR = 0xFF808020;
			uint32 colour = FRONT_FACE_COLOUR;
			Vector3 trin = obstacle.transform().applyVector( triangle.normal() );
			if ( trin.dotProduct( Moo::rc().invView()[2] ) >= 0.f )
				colour = BACK_FACE_COLOUR;
			CollisionDebugger::instance()->addLine(
				obstacle.transform().applyPoint( triangle.v0() ),
				obstacle.transform().applyPoint( triangle.v1() ),
				colour );
			CollisionDebugger::instance()->addLine(
				obstacle.transform().applyPoint( triangle.v1() ),
				obstacle.transform().applyPoint( triangle.v2() ),
				colour );
			CollisionDebugger::instance()->addLine(
				obstacle.transform().applyPoint( triangle.v2() ),
				obstacle.transform().applyPoint( triangle.v0() ),
				colour );
		}


		if ( !(triangle.flags() & TRIANGLE_DOUBLESIDED) )
		{
			// Check that we don't hit back facing triangles,
			// if we can't see it we don't want to select it (unless it's
			// double-sided)
			Vector3 trin = obstacle.transform().applyVector( triangle.normal() );
			if ( trin.dotProduct( Moo::rc().invView()[2] ) >= 0.f )
				return COLLIDE_ALL;
		}

		if (!SelectionFilter::canSelect( &*pItem ))
			return COLLIDE_BEFORE;


		// Make sure that the item is within a reasonable distance to the cursor
		// (Can be a problem for close objects)
		// We do this by checking the angle between the camerapos to item and worldRay
		// isn't too large
		Vector3 itemRay = vectorToItem( pItem );
		itemRay.normalise();

		if (acosf( Math::clamp(-1.0f, worldRay_.dotProduct( itemRay ), +1.0f) ) > DEG_TO_RAD( 5.f ) )
			return COLLIDE_BEFORE;


		// Set the chunk item if this one is closer to the ray
		if (!chunkItem ||
				distToWorldRay( pItem, worldRay_ ) <
				distToWorldRay( chunkItem, worldRay_ ))
		{
			chunkItem = pItem;
		}

		// Keep on searching, we want to check everything
		return COLLIDE_BEFORE;
	}

	const Vector3& worldRay_;
};

/**
 *	Returns the chunk item. In the cpp so the header needn't know about
 *	chunk items
 */
ChunkItemPtr ChunkItemLocator::chunkItem()
{
	return chunkItem_;
}


/**
 *	Calculate the locator's position
 */
void ChunkItemLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	// first call our sublocator to set the matrix
	if (subLocator_)
	{
		subLocator_->calculatePosition( worldRay, tool );
		transform_ = subLocator_->transform();
		positionValid_ = subLocator_->positionValid();
	}
	else
	{
		transform_ = Matrix::identity;
		positionValid_ = false;
	}

	// now find the chunk item
	if (enabled_)
	{
		// if we're enabled
		float distance = -1.f;
		Vector3 extent = Moo::rc().invView().applyToOrigin() +
			worldRay * Moo::rc().camera().farPlane();

		ClosestObstacleCatcher coc( 0, TRIANGLE_TERRAIN );

		distance = ChunkManager::instance().cameraSpace()->collide( 
			Moo::rc().invView().applyToOrigin(),
			extent,
			coc );

		chunkItem_ = coc.chunkItem_;
		if (chunkItem_)
		{
			transform_.setTranslate( Moo::rc().invView().applyToOrigin() + worldRay * distance );
		}

		// ok, nothing is at the subLocator, try a fuzzy search aroung the worldRay
		if (!chunkItem_)
		{
			const float SELECTION_SIZE = 0.5f;

			// The points for a square around the view position, on the camera plane
			Vector3 tr = Moo::rc().invView().applyPoint( Vector3( SELECTION_SIZE, SELECTION_SIZE, 0.f ) );
			Vector3 br = Moo::rc().invView().applyPoint( Vector3( SELECTION_SIZE, -SELECTION_SIZE, 0.f ) );
			Vector3 bl = Moo::rc().invView().applyPoint( Vector3( -SELECTION_SIZE, -SELECTION_SIZE, 0.f ) );
			Vector3 tl = Moo::rc().invView().applyPoint( Vector3( -SELECTION_SIZE, SELECTION_SIZE, 0.f ) );

			// Our two triangles which will be projected through the world
			WorldTriangle tri1(tl, tr, br);
			WorldTriangle tri2(br, bl, tl);

			// Get the best chunk for tri1, then get the best for tri2, the result
			// will be the best out of both of them
			BestObstacleCatcher boc( NULL, worldRay );
			ChunkManager::instance().cameraSpace()->collide( 
				tri1,
				extent,
				boc );
			ChunkManager::instance().cameraSpace()->collide( 
				tri2,
				extent,
				boc );

			chunkItem_ = boc.chunkItem;
		}

		// Nothing near the locator, lets try again with terrain now
		if (!chunkItem_)
		{
			coc = ClosestObstacleCatcher( 0, 0 );

			distance = ChunkManager::instance().cameraSpace()->collide( 
				Moo::rc().invView().applyToOrigin(),
				extent,
				coc );

			chunkItem_ = coc.chunkItem_;
		}
	}
	else
	{
		chunkItem_ = NULL;
	}
}


/**
 *	Get a revealer object for the chunkitem in this locator
 */
PyObject * ChunkItemLocator::pyGet_revealer()
{
	BW_GUARD;

	return new ChunkItemLocatorRevealer( this );
}

/**
 *	Python factory method
 */
PyObject * ChunkItemLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * pSubLoc = NULL;
	if (!PyArg_ParseTuple( args, "|O", &pSubLoc ) ||
		pSubLoc && !ToolLocator::Check( pSubLoc ))
	{
		PyErr_SetString( PyExc_TypeError, "ChunkItemLocator() "
			"expects an optional ToolLocator argument" );
		return NULL;
	}

	ToolLocatorPtr spSubLoc = static_cast<ToolLocator*>( pSubLoc );

	return new ChunkItemLocator( spSubLoc );
}


//------------------------------------------------------------
//Section : ItemToolLocator
//------------------------------------------------------------


//#undef PY_ATTR_SCOPE
//#define PY_ATTR_SCOPE ItemToolLocator::

PY_TYPEOBJECT( ItemToolLocator )

PY_BEGIN_METHODS( ItemToolLocator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ItemToolLocator )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( ItemToolLocator, "ItemToolLocator", Locator )

LOCATOR_FACTORY( ItemToolLocator )
/**
 *	Constructor.
 */
ItemToolLocator::ItemToolLocator( PyTypeObject * pType ):
	ToolLocator( pType ),
	positionValid_( false )
{
}


/**
 *	This method calculates the tool's position, by intersecting it with
 *	the XZ plane.
 *
 *  Additional Behaviour: 
 *  If we can't intersect with x,y plane set the position to be the camera position and
 *  set the direction internally so it can be retrieved with the 
 *  direction() method.
 *	@param	worldRay	The camera's world ray.
 *	@param	tool		The tool for this locator.
 */
void ItemToolLocator::calculatePosition( const Vector3& worldRay, Tool& tool )
{
	BW_GUARD;

	Vector3 rayStart = Moo::rc().invView().applyToOrigin();
	Vector3 pos = Snap::toObstacle( rayStart, worldRay );
	
	if ( pos == rayStart )
	{
		/* 
		The nicest solution we have thought so far is, when the tool locator is not
		colliding with anything,  to calculate the size of the object/shell being
		placed, and place it some distance away from the camera according to the
		object/shell's size (the bigger, the farther away, but take near/far planes
		into account!). I have found it makes more sense to use the worldRay direction which
		takes into account the x,y click coordinates
		*/
		// Nothing to snap to, so don't modify the current position.
		positionValid_ = false;
		direction_ = worldRay;
	} else
	{
		positionValid_ = true;
	}

	transform_.setTranslate( pos );
	//snap in the XZ direction.
	if ( WorldManager::instance().snapsEnabled() )
	{
		Vector3 snaps = WorldManager::instance().movementSnaps();
		snaps.y = 0.f;
		Snap::vector3( *(Vector3*)&transform_._41, snaps );
	}
}


/**
 *	Static python factory method
 */
PyObject * ItemToolLocator::pyNew( PyObject * args )
{
	BW_GUARD;

	return new ItemToolLocator;
}


BW_END_NAMESPACE

// item_locator.cpp
