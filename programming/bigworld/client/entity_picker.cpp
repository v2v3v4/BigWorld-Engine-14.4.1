
#include "pch.hpp"

#include "camera_app.hpp"

#include "entity.hpp"
#include "entity_manager.hpp"
#include "py_entity.hpp"

#include "cstdmf/debug.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_entities.hpp"
#include "moo/render_context.hpp"
#include "moo/geometrics.hpp"
#include "duplo/skeleton_collider.hpp"
#include "ashes/simple_gui.hpp"

#include "space/client_space.hpp"
#include "space/deprecated_space_helpers.hpp"

#include <limits.h>

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "entity_picker.ipp"
#endif


/**
 *	Constructor.
 */
EntityPicker::EntityPicker() :
	selectionFoV_( DEG_TO_RAD( 40.f ) ),
	deselectionFoV_( DEG_TO_RAD( 80.f ) ),
	distance_( 60.f ),
	currentTargetDistance_(-1.0f),
	autoAimFrictionDistance_( 60.f),
	autoAimFrictionMinimumDistance_( 0.0f ),
	autoAimFrictionHorizontalAngle_( DEG_TO_RAD( 20.f ) ),
	autoAimFrictionVerticalAngle_( DEG_TO_RAD( 10.f ) ),
	autoAimAdhesionDistance_( 10.f),
	autoAimAdhesionHorizontalAngle_( DEG_TO_RAD( 5.f ) ),
	autoAimAdhesionVerticalAngle_( DEG_TO_RAD( 5.f ) ),
	pSource_( NULL ),
	pTarget_( NULL ),
	isFull_( false ),
	isHidden_( false ),
	isLocked_( false ),
	isHeld_( false ),
	upperMarginFactor_( 0.1f ),		// 0.25f
	sideMarginFactor_( 0.45f ),		// 0.33f
	lowerMarginFactor_( 0.49f ),	// 0.33f
	isEnabled_( true ),
	hiddenTime_( 0.f ),
	maxHiddenTime_( 2.f ),
	noPartial_( false ),
	pExclude_( NULL ),
	skeletonCheckEnabled_( false ),
	debugDraw_( false )
{
	BW_GUARD;
	defaultSelectionFoV_ = selectionFoV_;
	defaultDeselectionFoV_ = deselectionFoV_;
	defaultDistance_ = distance_;

	MF_WATCH( "Client Settings/Entity Picker/Upper Pct.",
		upperMarginFactor_,
		Watcher::WT_READ_WRITE,
		"Percentage of bounding box height to subtract from the top of entity "
		"bounding boxes for use in targeting." );

	MF_WATCH( "Client Settings/Entity Picker/Sides Pct.",
		sideMarginFactor_,
		Watcher::WT_READ_WRITE,
		"Percentage of bounding box width to subtract from the sides of entity "
		"bounding boxes for use in targeting." );

	MF_WATCH( "Client Settings/Entity Picker/Lower Pct.",
		lowerMarginFactor_,
		Watcher::WT_READ_WRITE,
		"Percentage of bounding box height to subtract from the bottom "
		"of entity bounding boxes for use in targeting." );

	MF_WATCH( "Client Settings/Entity Picker/Distance",
		distance_,
		Watcher::WT_READ_WRITE,
		"Maximum distance that entities may be targetted at." );

	MF_WATCH( "Client Settings/Entity Picker/Debug",
		debugDraw_,
		Watcher::WT_READ_WRITE,
		"Toggle visual debugging information for entity selection." );

	MF_WATCH( "Client Settings/Entity Picker/selectionFoV",
		selectionFoV_,
		Watcher::WT_READ_WRITE,
		"The vertical field-of-view in radians of the selection frustum, for selection." );

	MF_WATCH( "Client Settings/Entity Picker/deselectionFoV",
		deselectionFoV_,
		Watcher::WT_READ_WRITE,
		"The vertical field-of-view in radians of the selection frustum, for de-selection." );

	MF_WATCH( "Client Settings/Entity Picker/Target distance",
		currentTargetDistance_,
		Watcher::WT_READ_WRITE,
		"The distance of currently targetted entity to the camera." );
}


/**
 *	Destructor.
 */
EntityPicker::~EntityPicker()
{
	BW_GUARD;
	if (pTarget_ != NULL)
	{
		WARNING_MSG(
			"Destructing EntityPicker with a non-NULL current entity!\n" );
	}
}


/**
 *	This method updates the entity picker.
 */
void EntityPicker::update( float dTime )
{
	BW_GUARD;
	if ( debugDraw_ )
		bbs_.clear();

	//Set current targets selection distance.
	if (pTarget_ != NULL)
	{
		//Set current target distance
		Matrix sourceMatrix;
		pSource_->matrix( sourceMatrix );
		Vector3 sourcePos = sourceMatrix.applyToOrigin();
		currentTargetDistance_ = Vector3(pTarget_->position() - sourcePos).length();

		if (isLocked_ &&
			(pTarget_->pSpace() == DeprecatedSpaceHelpers::cameraSpace()))
		{
			if (debugDraw_)
			{
				bbs_.push_back( BoundingBox() );
				this->getBoundingBox( *pTarget_, bbs_.back() );
			}
			return;
		}

		if (isHeld_ || !isEnabled_)
		{
			this->check( isHeld_ ? &dTime : NULL );
			if (debugDraw_)
			{
				bbs_.push_back( BoundingBox() );
				this->getBoundingBox( *pTarget_, bbs_.back() );
			}
			return;
		}
	}
	else
	{
		currentTargetDistance_ = -1.0f;
	}

	this->pickFirst();
}


/**
 *	This method gets the matrix the entity picker uses for the player
 */
MatrixProviderPtr EntityPicker::source() const
{
	return pSource_;
}

/**
 *	This method sets the player matrix that the entity picker will use.
 */
void EntityPicker::source( MatrixProviderPtr pSource )
{
	BW_GUARD;
	pSource_ = pSource;
	this->check();
}


/**
 *	This method calculates the "angle" to the current entity. It is actually the
 *	smallest screen coordinate from the origin to the projection of the entity's
 *	bounding box.
 *
 *	@param entity		The entity to check the angle of.
 *	@param fovMatrix	The field-of-view matrix. This is the projection matrix of
 *					the view frustrum.
 */
float EntityPicker::angleTo( const Entity & entity,
							const Matrix & fovMatrix ) const
{
	BW_GUARD;
	MF_ASSERT_DEV( entity.pPrimaryEmbodiment() != NULL );

	// This code is very similar to BoundingBox::calculateOutcode.
	float minX = FLT_MAX;
	float maxX = -FLT_MAX;
	float minY = FLT_MAX;
	float maxY = -FLT_MAX;

	Vector4 vx[2];
	Vector4 vy[2];
	Vector4 vz[2];

	BoundingBox bb;
	this->getBoundingBox( entity, bb );

	vx[0] = fovMatrix.row( 0 );
	vx[1] = vx[0];
	vx[0] *= bb.minBounds().x;
	vx[1] *= bb.maxBounds().x;

	vy[0] = fovMatrix.row( 1 );
	vy[1] = vy[0];
	vy[0] *= bb.minBounds().y;
	vy[1] *= bb.maxBounds().y;

	vz[0] = fovMatrix.row( 2 );
	vz[1] = vz[0];
	vz[0] *= bb.minBounds().z;
	vz[1] *= bb.maxBounds().z;

	const Vector4 &vw = fovMatrix.row( 3 );

	for( uint32 i=0 ; i < 8 ; i++ )
	{
		Vector4 v = vw;
		v += vx[ i &1 ];
		v += vy[( i >> 1 ) &1 ];
		v += vz[( i >> 2 ) &1 ];

		float x = v.x /  v.w;
		float y = v.y /  v.y;

		minX = std::min( minX, x );
		maxX = std::max( maxX, x );
		minY = std::min( minY, y );
		maxY = std::max( maxY, y );
	}

	if (minX < 0.f && 0.f < maxX)
	{
		minX = 0.f;
	}

	if (minY < 0.f && 0.f < maxY)
	{
		minY = 0.f;
	}

	return std::max( std::min( fabsf( minX ), fabsf( maxX ) ),
					std::min( fabsf( minY ), fabsf( maxY ) ) );
}

/**
 *	This static helper method returns the distance from the input matrix to the
 *	input entity along the z-axis.
 */
inline float EntityPicker::zDistance(
	const Entity & entity, const Matrix & lookFrom )
{
	Vector3 entityDelta = entity.position() - lookFrom.applyToOrigin();
	return lookFrom.applyToUnitAxisVector( Z_AXIS ).dotProduct( entityDelta );
}


/**
 *	This class implements a visitor for iterating through entities for a given
 *	space for the purposes of implementing pickFirst().
 */
class PickFirstEntityVisitor : public EntityVisitor
{
public:
	PickFirstEntityVisitor( EntityPicker & picker,
				const Vector3 & sourcePosition,
				const Matrix & lookFrom,
				const Matrix & fovMatrix,
				Entity * pExcluded ) :
			EntityVisitor(),
			picker_( picker ),
			sourcePosition_( sourcePosition ),
			lookFrom_( lookFrom ),
			fovMatrix_( fovMatrix ),
			pExcluded_( pExcluded ),
			pBest_( NULL ),
			bestScore_( 0.f ),
			isFull_( false )
	{}

	/** Destructor. */
	virtual ~PickFirstEntityVisitor() {}

	/* Override from EntityVisitor. */
	virtual bool onVisitEntity( Entity * pEntity )
	{
		if ((pEntity->spaceID() != DeprecatedSpaceHelpers::cameraSpace()->id() ) ||
				!pEntity->isInWorld() ||
				(pEntity->pPrimaryEmbodiment() == NULL))
		{
			return true;
		}

		const bool newIsFullTarget = picker_.capsCheck( *pEntity );

		// Don't consider if the new one is not a full target and we already
		// have a full target (and don't select the excluded entity).
		if (pEntity->targetCaps().has( 0 ) ||
				(!newIsFullTarget && isFull_) ||
				(pEntity == pExcluded_))
		{
			return true;
		}

		static const float ANGLE_WEIGHT = 1.f;
		static const float Z_WEIGHT = 0.001f;
		float zCurr = EntityPicker::zDistance( *pEntity, lookFrom_ );
		float angleCurr = picker_.angleTo( *pEntity, fovMatrix_ );
		float score = Z_WEIGHT * zCurr + ANGLE_WEIGHT * angleCurr;

		// Is this a better target?
		if (((score < bestScore_) || (!isFull_ && newIsFullTarget)) &&
			picker_.isEntitySelectable( *pEntity, lookFrom_, fovMatrix_,
				sourcePosition_, !isFull_ ))
		{
			if (!this->filterEntity( pEntity, zCurr ))
			{
				return true;
			}

			bestScore_ = score;
			pBest_ = pEntity;

			if (!isFull_)
			{
				isFull_ = picker_.capsCheck( *pEntity );
			}
		}

		return true;
	}
	

	/**
	 *	This method is used by subclasses to further filter entities to be 
	 *	considered for picking.
	 *
	 *	@param pEntity	The entity to consider.
	 *	@return			True if the entity should be considered, false 
	 *					otherwise.
	 */
	virtual bool filterEntity( Entity * pEntity, float distance )
	{
		return true;
	}

	bool isFull() const { return isFull_; }

	Entity * pBest() const { return pBest_; }


protected:
	EntityPicker &	picker_;
	const Vector3 & sourcePosition_;
	const Matrix &	lookFrom_;
	const Matrix &	fovMatrix_;
	Entity *		pExcluded_;
	Entity *		pBest_;
	float			bestScore_;
	bool			isFull_;
};


/**
 *  This method picks the first entity.
 */
void EntityPicker::pickFirst()
{
	BW_GUARD;
	Matrix lookFrom;
	Matrix fovMatrix;
	Vector3 sourcePos;

	this->prepareViewMatrix( selectionFoV_, /* Aspect ratio */ 1.0f, lookFrom,
		fovMatrix, sourcePos );

	PyEntityPtr pPyExcluded = this->exclude();
	Entity * pExcluded = pPyExcluded ? pPyExcluded->pEntity().get() : NULL;

	PickFirstEntityVisitor visitor( *this, sourcePos, lookFrom,
		fovMatrix, pExcluded );

	ConnectionControl::instance().visitEntities( visitor );
	
	// ok, now let everyone know things have changed
	this->setCurrentEntity( visitor.pBest(), visitor.isFull(),
		/* isHidden */ false );
}


/**
 *	This method changes the capabilities that the entity picker looks for
 */
void EntityPicker::setCapabilities( const Capabilities & capsOn,
	const Capabilities & capsOff )
{
	BW_GUARD;
	capsOn_ = capsOn;
	capsOff_ = capsOff;

	this->check();
}


/**
 *	This visitor subclass is used to implement the hasAnAutoAimTarget() method.
 */
class AutoAimEntityVisitor : public PickFirstEntityVisitor
{
public:

	/**
	 *	Constructor.
	 */
	AutoAimEntityVisitor( EntityPicker & picker,
			const Vector3 & sourcePosition,
			const Matrix & lookFrom,
			const Matrix & fovMatrix,
			Entity * pExcluded,
			bool wantHorizontalAngle,
			float autoAimHorizontalFalloffAngle,
			float autoAimVerticalFalloffAngle ) :
		PickFirstEntityVisitor( picker, sourcePosition, lookFrom,
			fovMatrix, pExcluded ),
		wantHorizontalAngle_( wantHorizontalAngle ),
		autoAimHorizontalFalloffAngle_( autoAimHorizontalFalloffAngle ),
		autoAimVerticalFalloffAngle_( autoAimVerticalFalloffAngle ),
		bestDistance_( 0.f ),
		bestAngle_( 0.f )
	{}

	virtual ~AutoAimEntityVisitor() {}

	/**
	 *	Override from PickFirstEntityVisitor.
	 */
	virtual bool filterEntity( Entity * pEntity, float distance )
	{
		BoundingBox bb;
		picker_.getBoundingBox( *pEntity, bb );
		Vector3 entityDelta = /*pCurr->pos()*/ bb.centre() - lookFrom_.applyToOrigin();
		float angle = 0.0f;
		if (wantHorizontalAngle_)
		{
			float yaw = lookFrom_.yaw();
			if (bw_isnan( yaw ))
			{
				return false;
			}

			angle = fabsf( yaw - entityDelta.yaw() );
			while (angle >= MATH_PI)
			{
				angle = fabsf( angle - 2 * MATH_PI );
			}

			if (angle > autoAimHorizontalFalloffAngle_)
			{
				return false;
			}
		}
		else
		{
			float pitch = lookFrom_.pitch();
			if (bw_isnan( pitch ))
			{
				return false;
			}

			angle = fabsf( pitch - entityDelta.pitch() );
				
			while (angle >= MATH_PI)
			{
				angle = fabsf( angle - 2 * MATH_PI );
			}

			if (angle > autoAimVerticalFalloffAngle_)
			{
				return false;
			}
		}

		bestAngle_ = angle;
		bestDistance_ = distance;
		return true;
	}

	float bestDistance() const { return bestDistance_; }
	float bestAngle() const { return bestAngle_; }

private:

	bool wantHorizontalAngle_;
	float autoAimHorizontalFalloffAngle_;
	float autoAimVerticalFalloffAngle_;

	float bestDistance_;
	float bestAngle_;
};


/**
 *	This method decides if there's a target in the friction FOV.
 *
 *	@return			True if there is a target
 */
Entity* EntityPicker::hasAnAutoAimTarget( float& autoAimTargetDistance, float& autoAimTargetAngle,
											bool useFriction, bool wantHorizontalAngle )
{
	BW_GUARD;

//	float autoAimDistance = useFriction ? autoAimFrictionDistance_ : autoAimAdhesionDistance_;
//	float autoAimHorizontalAngle = useFriction ? autoAimFrictionHorizontalAngle_ : autoAimAdhesionHorizontalAngle_;
//	float autoAimVerticalAngle = useFriction ? autoAimFrictionVerticalAngle_ : autoAimAdhesionVerticalAngle_;
	float autoAimFalloffDistance = useFriction ? autoAimFrictionFalloffDistance_ : autoAimAdhesionFalloffDistance_;
	float autoAimHorizontalFalloffAngle = useFriction ? autoAimFrictionHorizontalFalloffAngle_ : autoAimAdhesionHorizontalFalloffAngle_;
	float autoAimVerticalFalloffAngle = useFriction ? autoAimFrictionVerticalFalloffAngle_ : autoAimAdhesionVerticalFalloffAngle_;

	Matrix lookFrom;
	Matrix fovMatrix;
	Vector3 sourcePos;
	this->prepareViewMatrix( autoAimVerticalFalloffAngle * 2.0f,
								autoAimHorizontalFalloffAngle / autoAimVerticalFalloffAngle,
								lookFrom, fovMatrix, sourcePos );

	PyEntityPtr pPyExcluded = this->exclude();
	Entity * pExcluded = pPyExcluded ? pPyExcluded->pEntity().get() : NULL;

	// Change distance_ to stickTargetDistance_
	float distance = distance_;
	// decreases from full to 0 between autoAimDistance and autoAimFalloffDistance
	distance_ = autoAimFalloffDistance;

	// GKM 20031008: I'm not sure why the caps were being forced to be only
	// CAP_CAN_HIT, but with the way the game design is right now, we should be
	// sticking with the caps as they are. I guess it used to be this was to
	// remove teammates as valid targets. We might need to change the py_caps
	// method to optionally specify the auto aim targets caps, but default them
	// to being the same as the regular target caps.

	//// change target caps, but save old values
	//Capabilities onCaps = capsOn_;
	//Capabilities offCaps = capsOff_;
	//Capabilities on;
	//on.add( 1 ); //CAP_CAN_HIT
	//capsOn_ = on;

	AutoAimEntityVisitor visitor( *this, sourcePos, lookFrom, fovMatrix,
		pExcluded, wantHorizontalAngle, autoAimHorizontalFalloffAngle,
		autoAimVerticalFalloffAngle );
	
	ConnectionControl::instance().visitEntities( visitor );

	distance_ = distance;
	//capsOn_ = onCaps;
	//capsOff_ = offCaps;

	autoAimTargetDistance = visitor.bestDistance();
	autoAimTargetAngle = visitor.bestAngle();

	return visitor.pBest();
}


/**
 *	This method checks that the current entity is still valid.
 *
 *	@param pDeltaTime	If non-NULL, a pointer to float containing the time since
 *					the last update. This is used to calculate the heldTime_.
 */
void EntityPicker::check( float * pDeltaTime )
{
	BW_GUARD;
	if (pTarget_ == NULL)
	{
		return;
	}

	Matrix lookFrom;
	Matrix fovMatrix;
	Vector3 sourcePos;
	this->prepareViewMatrix( deselectionFoV_, /* Aspect ratio */ 1.0f, lookFrom, fovMatrix, sourcePos );

	bool wasLineOfSight = false;
	bool isNowFull = this->capsCheck( *pTarget_ );

	if (!this->isEntitySelectable( *pTarget_,
					lookFrom, fovMatrix, sourcePos, true, &wasLineOfSight ))
	{
		if (debugDraw_)
		{
			bbs_.push_back( BoundingBox() );
			this->getBoundingBox( *pTarget_, bbs_.back() );
		}

		// Was it just the line-of-sight stopping selection?
		if (wasLineOfSight && pDeltaTime != NULL)
		{
			// Has the hidden state changed
			if (!isHidden_)
			{
				this->setCurrentEntity( pTarget_,
					isFull_,	// isFull
					true );		// isHidden
			}

			hiddenTime_ += *pDeltaTime;

			if (hiddenTime_ < maxHiddenTime_)
			{
				// Hidden for too long
				return;
			}
		}

		this->setCurrentEntity( NULL, false, false );
	}
	else if (isHidden_ || (isFull_ != isNowFull))
	{
		// If it was hidden, it no longer is.
		this->setCurrentEntity( pTarget_,
			isNowFull,		// isFull
			false );		// isHidden
	}
}


void EntityPicker::drawDebugStuff()
{
	BW_GUARD;
	if (!debugDraw_)	return;

	Moo::rc().device()->SetPixelShader( NULL );
	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );

	BW::vector<BoundingBox>::iterator it = bbs_.begin();
	BW::vector<BoundingBox>::iterator end = bbs_.end();
	while ( it != end )
	{
		BoundingBox& bb = *it++;
		Geometrics::wireBox( bb, 0xff0000ff );
	}

	Moo::rc().pop();
}


PyEntityPtr EntityPicker::exclude() const
{
	BW_GUARD;
	if (!pExclude_.good() || pExclude_->isDestroyed())
	{
		return PyEntityPtr();
	}
	return PyEntityPtr( pExclude_.get() );
}

/**
 *	This method sets the entity that should not be targeted
 */
void EntityPicker::exclude( PyEntityPtr pPyEntity )
{
	BW_GUARD;
	if (pPyEntity == NULL || pPyEntity->isDestroyed())
	{
		pExclude_ = NULL;
		return;
	}

	pExclude_ = pPyEntity.get();

	if (pPyEntity->pEntity() == pTarget_)
	{
		this->clear();
	}
}


/**
 *	This method targets the input entity.
 */
void EntityPicker::lockOn( Entity * pEntity )
{
	BW_GUARD;
	isLocked_ = (pEntity != NULL);
	this->setCurrentEntity( pEntity, isLocked_, false );
}


/**
 *	This method clears the current target.
 */
void EntityPicker::clear()
{
	BW_GUARD;
	this->lockOn( NULL );
}


/**
 *	This private method changes the entity we remember as picked.
 */
void EntityPicker::setCurrentEntity( Entity * pEntity,
										bool isFull,
										bool isHidden )
{
	BW_GUARD;
	if ((pTarget_ == pEntity) &&
		(isFull == isFull_) &&
		(isHidden == isHidden_))
	{
		return;
	}

	if (pTarget_ != NULL)
	{
		pTarget_->targetBlur();
	}

	pTarget_ = pEntity;
	if ( noPartial_ && !isFull )
		pTarget_ = NULL;

	if (isHidden && !isHidden_)
	{
		hiddenTime_ = 0.f;
	}

	isHidden_ = isHidden;
	isFull_ = isFull;

	if (pTarget_ != NULL)
	{
		pTarget_->targetFocus();
	}
}


/**
 *	This method prepares the view matrix used to see if an entity is selectable.
 */
void EntityPicker::prepareViewMatrix( float fov, float aspectRatio,
	Matrix & lookFrom,
	Matrix & fovMatrix,
	Vector3 & sourcePos ) const
{
	BW_GUARD;
	const Matrix & cameraToWorld = Moo::rc().invView();

	if (!pSource_)
	{
		// ok, we're disabled. make lookFrom point nowhere :)
		lookFrom.setTranslate( 0, -1000000, 0 );
		lookFrom.postRotateX( -MATH_PI/2 );
		sourcePos.set( 0, -1000000, 0 );
	}
	else
	{
		pSource_->matrix( lookFrom );
		sourcePos = lookFrom.applyToOrigin();
	}

	fovMatrix.perspectiveProjection( fov,
		aspectRatio,
		0.5f,			// Near-plane
		distance_ );	// Far-plane 
	Matrix invLookFrom;
	invLookFrom.invertOrthonormal( lookFrom );
	fovMatrix.preMultiply( invLookFrom );
}


/**
 *	This helper function gets the bounding box of the entity. It considers the
 *	margin factors and transforms the bounds into world coordinates.
 */
void EntityPicker::getBoundingBox( const Entity & entity,
								BoundingBox & result ) const
{
	BW_GUARD;
	IEntityEmbodiment * pCE = entity.pPrimaryEmbodiment();
	if (pCE != NULL)
	{
		result = BoundingBox( pCE->localBoundingBox() );

		//adjust bounding box so you can't select the very corners and cheat.
		float height = result.maxBounds().y - result.minBounds().y;
		float xWidth  = result.maxBounds().x - result.minBounds().x;
		float zWidth  = result.maxBounds().z - result.minBounds().z;
		float adjXWidth = xWidth * sideMarginFactor_;
		float adjZWidth = zWidth * sideMarginFactor_;

		if (!this->useFullBounds(entity))
		{
			result.setBounds( 
				result.minBounds() + Vector3( adjXWidth, height * lowerMarginFactor_, adjZWidth ), 
				result.maxBounds() - Vector3( adjXWidth, height * upperMarginFactor_, adjZWidth ) );
		}

		// transform it into world space
		result.transformBy( pCE->worldTransform() );
	}
}

/**
 *	This method returns true if the targeting capabilities match.
 */
bool EntityPicker::capsCheck( const Entity& entity ) const
{
	BW_GUARD;
	//this allows user to use BigWorld.target.caps(), and have any entity be
	//targettable.  i.e. if no caps bits are set for on_ or off_, we assume
	//the match to always occur.
	if (capsOn_.empty() && capsOff_.empty())
		return true;

	bool isCapsOkay = entity.targetCaps().matchAny( capsOn_, capsOff_ );
	return isCapsOkay;
}


/**
 *	This method returns true if the skeleton could be targetted.
 *	It also returns true if the entity does not have a skeleton collider.
 */
bool EntityPicker::skeletonCheck( const Entity& entity, const Matrix& lookFrom ) const
{
	BW_GUARD;
	//use nasty c-style cast to avoid constness
	PyObject* pyEnt = (PyObject*)(&entity);
	PyObjectPtr attr = PyObjectPtr(
		PyObject_GetAttrString( pyEnt, "skeletonCollider" ),
		PyObjectPtr::STEAL_REFERENCE ); 

	if ( !attr )
	{
		PyErr_Clear();
		return true;
	}

	if (SkeletonCollider::Check( attr.get() ))
	{
		SkeletonCollider* collider = static_cast<SkeletonCollider*>( attr.get() );
		const Vector3 lookFromPoint = lookFrom.applyToOrigin();
		return collider->doCollide( lookFromPoint, lookFrom.applyToUnitAxisVector(2) );
	}
	else
	{
		ERROR_MSG( "SkeletonCollider set on the entity was not of the correct type\n" );
		return true;
	}
}


/**
 *	This method returns true if two points on the bounding box of the entity
 *	were visible through the collision scene of the world.
 */
bool EntityPicker::twoPointLOSCheck( const Entity & entity, const BoundingBox& bb, const Matrix& lookFrom ) const
{
	BW_GUARD;
	const ClientSpace & space = *entity.pSpace();
	const float WALL_PENETRATION = 0.2f;
	const Vector3 & minPt = bb.minBounds();
	const Vector3 & maxPt = bb.maxBounds();
	const Vector3 & lookFromPoint = lookFrom.applyToOrigin();

	Vector3 curr = (minPt + maxPt);
	curr /= 2.f;

	for (int i = 0; i < 2; i++)
	{
		const Vector3 delta = curr - lookFromPoint;
		float deltaLen = delta.length();

		if (deltaLen <= WALL_PENETRATION)
		{
			return true;
		}

		Vector3 ctpMost = lookFromPoint + delta *
			(deltaLen - WALL_PENETRATION) / deltaLen;
		if (space.collide( lookFromPoint, ctpMost ) < 0.f)
		{
			return true;
		}

		curr.y = (minPt.y * 1.f + maxPt.y * 3.f) / 4.f;
	}

	return false;
}


/**
 *	This method returns true if two points on the bounding box of the entity
 *	were visible through the collision scene of the world.
 */
bool EntityPicker::fullBoundsLOSCheck( const Entity & entity, const BoundingBox& bb, const Matrix& lookFrom ) const
{
	BW_GUARD;
	const ClientSpace & space = *entity.pSpace();
	const float WALL_PENETRATION = 0.2f;
	const Vector3& lookFromPoint = lookFrom.applyToOrigin();
	const Vector3& lookDir = lookFrom.applyToUnitAxisVector( Z_AXIS );
	Vector3 lookNear = lookFromPoint;
	Vector3 lookFar = lookNear + (lookDir * this->selectionDistance());

	if (bb.clip( lookNear, lookFar ))
	{
		const Vector3 delta = lookNear - lookFromPoint;
		float deltaLen = delta.length();
		if (deltaLen <= WALL_PENETRATION)
		{
			return true;
		}
		Vector3 ctpMost = lookFromPoint + delta *
			(deltaLen - WALL_PENETRATION) / deltaLen;
		if (space.collide( lookFromPoint, ctpMost ) < 0.f)
		{
			return true;
		}
	}

	return false;
}


bool EntityPicker::useFullBounds( const Entity& entity ) const
{
	BW_GUARD;
	return (entity.targetFullBounds() || skeletonCheckEnabled_);
}


/**
 *	This method checks whether an entity is selectable.
 *
 *	@param entity	The entity to check.
 *	@param lookFrom		Where the picker is looking from.
 *	@param fovMatrix	The field-of-view matrix.
 *	@param skipMatch	If true, targetCaps are not considered.
 *	@param pWasLineOfSight	If passed in, this value is set to true if
 *				line-of-sight was the only reason the entity is not selectable.
 *
 *	@return		True if the entity is selectable.
 */
bool EntityPicker::isEntitySelectable( const Entity & entity,
								const Matrix & lookFrom,
								const Matrix & fovMatrix,
								const Vector3 & sourcePos,
								bool skipMatch,
								bool * pWasLineOfSight ) const
{
	BW_GUARD;
	bool result = false;
	bool sameSpace = (entity.pSpace() == DeprecatedSpaceHelpers::cameraSpace());
	bool isCapsOkay = this->capsCheck( entity );
	bool isDistanceOkay = this->distanceCheck( entity, sourcePos );

	//compute selectability
	if ( sameSpace && isCapsOkay && isDistanceOkay &&
		entity.pPrimaryEmbodiment() != NULL )
	{
		//now we can compute some points to perform F.O.V and L.O.S checks with
		BoundingBox bb;
		this->getBoundingBox( entity, bb );

		if (debugDraw_)
		{
			bbs_.push_back( bb );
		}

		bool inSelectionFov = this->fovCheck( bb, fovMatrix );
		bool hitsSkeleton = (!skeletonCheckEnabled_) || this->skeletonCheck( entity, lookFrom );

		if (inSelectionFov && hitsSkeleton)
		{
			// Check line-of-sight.
			bool losOkay = false;
			if (this->useFullBounds(entity))
				losOkay = this->fullBoundsLOSCheck( entity, bb, lookFrom );
			else
				losOkay = this->twoPointLOSCheck( entity, bb, lookFrom );

			result = losOkay;

			if (pWasLineOfSight != NULL)
			{
				*pWasLineOfSight = !losOkay;
			}
		}
	}

	return result;
}


/**
 *	This method returns whether the entity is within the selectable distance.
 *
 *	@param entity	The entity to check.
 *
 *	@return True if the entity is within the selectable distance.
 */
bool EntityPicker::distanceCheck( const Entity & entity,
									const Vector3 & sourcePos ) const
{
	BW_GUARD;
	float dist = Vector3(entity.position() - sourcePos).length();

	return (0.f < dist) && (dist < distance_);
}


/**
 *	This method checks that the entity is within the field of view.
 *
 *	@param bb	The bounding box of the entity.
 *
 *	@return True if the bounding box is within the field of view.
 */
bool EntityPicker::fovCheck( BoundingBox & bb, const Matrix & fovMatrix ) const
{
	BW_GUARD;
	// Use the clipping code. FoV matrix is set up to be the projection matrix
	// such that the the view frustum is the selection volume.
	bb.calculateOutcode( fovMatrix );

	return !bb.combinedOutcode();
}


static PyObject * py_noPartialLocks( PyObject * args )
{
	BW_GUARD;
	int state;
	if (!PyArg_ParseTuple(args, "i", &state) )
	{
		return NULL;
	}

	CameraApp::instance().entityPicker().noPartialLocks((state!=0));
	Py_RETURN_NONE;
}

/*~ function BigWorld.noPartialLocks
 *
 *	This function is deprecated.  Use BigWorld.target.noPartial attribute instead.
 */
PY_MODULE_FUNCTION( noPartialLocks, BigWorld )


// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------

/**
 *	This class is used to implement the target attribute in the BigWorld module.
 *	It is object used to store the information about targeting system.
 */
class PyTarget : public PyObjectPlus
{
	Py_Header( PyTarget, PyObjectPlus )

public:
	PyTarget();

	PY_METHOD_DECLARE( pyCall );
private:
	PY_METHOD_DECLARE( py_caps );

	PY_AUTO_METHOD_DECLARE( RETVOID, clear, END )
	PY_AUTO_METHOD_DECLARE( RETVOID, debugLock, ARG( PyEntityPtr, END ) )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, isEnabled,	isEnabled )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, skeletonCheckEnabled, skeletonCheckEnabled )
	PY_RO_ATTRIBUTE_DECLARE( isFull(), isFull )
	PY_RO_ATTRIBUTE_DECLARE( isHidden(), isHidden )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, isHeld, isHeld )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, noPartial, noPartial )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, maxDistance, maxDistance )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, selectionFovDegrees, selectionFovDegrees )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, deselectionFovDegrees, deselectionFovDegrees )

	PY_RO_ATTRIBUTE_DECLARE( pPyEntity(), entity )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( MatrixProviderPtr, pSource, source )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( PyEntityPtr, exclude, exclude )

	/*~ function PyTarget clear
	 *  Set the current target to None.
	 *  @return None.
	 */
	void clear()
			{ CameraApp::instance().entityPicker().lockOn( NULL ); }
	/*~ function PyTarget debugLock
	 *  Set the current target to pEntity (this is useful for debugging).
	 *  @param pEntity The entity to target.
	 *  @return None.
	 */
	void debugLock( PyEntityPtr pPyEntity )
	{
		CameraApp::instance().entityPicker().lockOn(
			pPyEntity->pEntity().get() );
	}

	bool isEnabled() const		{ return CameraApp::instance().entityPicker().isEnabled(); }
	void isEnabled( bool val )	{ CameraApp::instance().entityPicker().isEnabled( val ); }

	bool skeletonCheckEnabled() const		{ return CameraApp::instance().entityPicker().skeletonCheckEnabled(); }
	void skeletonCheckEnabled( bool val )	{ CameraApp::instance().entityPicker().skeletonCheckEnabled( val ); }

	bool isFull() const			{ return CameraApp::instance().entityPicker().isFull(); }
	bool isHidden() const		{ return CameraApp::instance().entityPicker().isHidden(); }

	bool isHeld() const			{ return CameraApp::instance().entityPicker().isHeld(); }
	void isHeld( bool val )		{ CameraApp::instance().entityPicker().isHeld( val ); }

	bool noPartial() const			{ return CameraApp::instance().entityPicker().noPartialLocks(); }
	void noPartial( bool val )		{ CameraApp::instance().entityPicker().noPartialLocks( val ); }

	float maxDistance() const		{ return CameraApp::instance().entityPicker().selectionDistance(); }
	void maxDistance( float val )	{ CameraApp::instance().entityPicker().selectionDistance( val ); }

	float selectionFovDegrees() const		{ return RAD_TO_DEG( CameraApp::instance().entityPicker().selectionFoV() ); }
	void selectionFovDegrees( float val )	{ CameraApp::instance().entityPicker().selectionFoV( DEG_TO_RAD(val) ); }

	float deselectionFovDegrees() const		{ return RAD_TO_DEG( CameraApp::instance().entityPicker().deselectionFoV() ); }
	void deselectionFovDegrees( float val )	{ CameraApp::instance().entityPicker().deselectionFoV( DEG_TO_RAD(val) ); }

	PyEntityPtr pPyEntity() const
	{
		Entity * pEntity = CameraApp::instance().entityPicker().pGeneralTarget();
		MF_ASSERT( !pEntity->isDestroyed() );
		return pEntity->pPyEntity();
	}

	MatrixProviderPtr pSource() const
						{ return CameraApp::instance().entityPicker().source(); }
	void pSource( MatrixProviderPtr m ) const
						{ CameraApp::instance().entityPicker().source( m ); }

	PyEntityPtr exclude() const
		{ return CameraApp::instance().entityPicker().exclude(); }
	void exclude( PyEntityPtr e )
	{
		CameraApp::instance().entityPicker().exclude( e );
	}
};


// Implementation macros


/*~	function BigWorld target
 *	BigWorld.target is actually an attribute that is callable. See
 *	PyTarget.__call__ for more information.
 */
/*~	attribute BigWorld.target
 *  A PyTarget instance created automatically by the BigWorld engine.
 *  Most targeting operations can be handled through this object.
 *  @type PyTarget.
 */
PY_MODULE_ATTRIBUTE( BigWorld, target, new PyTarget );

/*~ class BigWorld.PyTarget
 *  The interface to the BigWorld targeting system.
 *  All targeting operations are accessed through an instance of this class.
 *  An instance of this class is automatically created by the engine as 
 *  BigWorld.target.
 *
 *	The BigWorld targeting system will choose from a list of potential targets 
 *	using the following criteria (in order of precedence):
 *	@{
 *		capabilities
 *		distance
 *		field of view
 *		line of sight
 *	@}
 */
PY_TYPEOBJECT_WITH_CALL( PyTarget )

PY_BEGIN_METHODS( PyTarget )
	PY_METHOD( caps )
	PY_METHOD( clear )
	PY_METHOD( debugLock )
PY_END_METHODS()


PY_BEGIN_ATTRIBUTES( PyTarget )

	/*~ attribute PyTarget.isEnabled
	 *  This stores whether the targeting system is allowed to select new targets.
	 *  @type Read-Write Boolean.
	 */
	PY_ATTRIBUTE( isEnabled )

	/*~ attribute PyTarget.skeletonCheckEnabled
	 *  This stores whether the targeting system should try to use the
	 *	skeleton collider method as a selection criterion.
	 *  @type Read-Write Boolean.
	 */
	PY_ATTRIBUTE( skeletonCheckEnabled )

	/*~ attribute PyTarget.isFull
	 *  This stores whether or not the current entity is a "full" target.
	 *  That is, a target that satisfies the capabilities requirements set via 
	 *  caps().
	 *  @type Read-Only Boolean.
	 */
	PY_ATTRIBUTE( isFull )

	/*~ attribute PyTarget.isHidden
	 *  This shows whether the target is currently held but cannot currently be 
	 *  seen.
	 *  @type Read-Only Boolean.
	 */
	PY_ATTRIBUTE( isHidden )

	/*~ attribute PyTarget.isHeld
	 *  This stores whether to try to hold on to the current target entity.
	 *  If the value is true, the target is kept kept while the
	 *  entity is within the deselection frustrum and is not hidden for longer
	 *  than maxHiddenTime_ (see EntityPicker::maxHiddenTime_ in entity_picker.hpp).
	 *  @type Read-Write Boolean.
	 */
	PY_ATTRIBUTE( isHeld )

	/*~ attribute PyTarget.noPartial
	 *	When set, the entity picker will only allow full locks.  This can be used to prevent 
	 *	targeting friendlies and buttons, etc, when in a combative state.
	 *	@type Read-Write bool
	 */
	PY_ATTRIBUTE( noPartial )

	/*~ attribute PyTarget.entity
	 *  This provides the current target, which may not be visible or a "full" 
	 *  target (that is, a target that satisfies the capabilities requirements set
	 *  via caps()).
	 *  @type Read-Only Entity.
	 */
	PY_ATTRIBUTE( entity )

	/*~ attribute PyTarget.source
	 *	This is the matrix that the targeting system will use to identify the location of the Player Entity.
	 *	This will correspond to PlayerMProv if targeting from the player's current position.  Otherwise, it 
	 *	will be the location from which all targeting should be performed, eg, remote turret, etc...
	 *	@type	Read-Write MatrixProviderPtr
	 */
	PY_ATTRIBUTE( source )

	/*~ attribute PyTarget.maxDistance
	 *	This is the distance beyond which Entities will not be considered as potential targets.
	 *	@type	Read-Write float
	 */
	PY_ATTRIBUTE( maxDistance )

	/*~ attribute PyTarget.selectionFov
	 *	This is the vertical angle in degrees within which Enitities may be 
	 *	considered as a new target.
	 *	@type	Read-Write float
	 */
	PY_ATTRIBUTE( selectionFovDegrees )

	/*~ attribute PyTarget.deselectionFov
	 *	This is the vertical angle in degrees outside of which the currently 
	 *	targetted Enitity will be automatically deselcted.
	 *	@type	Read-Write float
	 */
	PY_ATTRIBUTE( deselectionFovDegrees )

	/*~ attribute PyTarget.exclude
	 *	This is the entity that is to be excluded from targeting operations.  Usually 
	 *	this is the Player Entity, however need not be, eg, remote turret, etc...
	 *	@type	Read-Write Entity
	 */
	PY_ATTRIBUTE( exclude )
PY_END_ATTRIBUTES()

/**
 *	Constructor.
 */
PyTarget::PyTarget() : PyObjectPlus( &PyTarget::s_type_ )
{
	BW_GUARD;	
}

/*~ function PyTarget __call__
 *  When called, a PyTarget returns the currently targeted entity.
 *  @return The currently targeted entity if it satisfies all capability
 *  requirements and is not hidden, otherwise None.
 */

/**
 *	This method is called with this object is "called" in script. It returns the
 *	current target entity.
 */
PyObject * PyTarget::pyCall( PyObject * args )
{
	BW_GUARD;
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "No arguments expected" );
		return NULL;
	}

	Entity * pTarget = CameraApp::instance().entityPicker().pTarget();

	if (pTarget == NULL || pTarget->isDestroyed())
	{
		Py_RETURN_NONE;
	}

	return CameraApp::instance().entityPicker().pTarget()->pPyEntity().newRef();
}


/*~ function PyTarget caps
 *  This method sets the capability requirements that entities must meet in 
 *  order to be targetted. This method takes 0 or more arguments.
 * 
 *
 *  Examples:
 *
 *  # any entity may be targetted.
 *  caps()				
 *
 *  # only entities with capability 5 may be targetted.
 *  caps( 5 )			
 *
 *  # only entities without capability 5 may be targetted.
 *  caps( -5 )			
 *
 *  # only entities with capability 2, capability 3, and not capability 4 may 
 *  # be targeted.
 *  caps( 2, 3, -4 )	
 *
 *  @param cap	A capability requirement of an entity for it to be targetted.
 *			If cap > 0, entities must have capability cap on.
 *			If cap < 0, entities must have capability -cap off.
 *			Including the cap 0 means match none.
 *
 *  @return None.
 */

/**
 *	This method sets the capabilities that targets must have.
 */
PyObject * PyTarget::py_caps( PyObject * args )
{
	BW_GUARD;
	Capabilities	capsOn,	capsOff;

	Py_ssize_t ncaps = PyTuple_Size( args );

	for (Py_ssize_t i=0; i<ncaps; i++)
	{
		PyObject * argElt = PyTuple_GetItem( args, i );	// borrowed
		if (PyInt_Check( argElt ))
		{
			int	cap = PyInt_AsLong( argElt );
			if (cap >= 0)	capsOn.add( cap );
			if (cap <= 0)	capsOff.add( -cap );
		}
		else
		{
			char	str[256];
			int arg = static_cast<int>( ncaps ) + 1;
			bw_snprintf( str, sizeof(str), "py_targetCaps: Argument %d is not an integer.", arg );
			PyErr_SetString( PyExc_TypeError, str );
			return NULL;
		}
	}

	CameraApp::instance().entityPicker().setCapabilities( capsOn, capsOff );

	Py_RETURN_NONE;
}


//-----------------------------------------------------------------------------
//	Section - ThirdPersonTargetingMatrix
//-----------------------------------------------------------------------------
PY_TYPEOBJECT( ThirdPersonTargetingMatrix )

PY_BEGIN_METHODS( ThirdPersonTargetingMatrix )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ThirdPersonTargetingMatrix )
	/*~ attribute ThirdPersonTargetingMatrix.source
	 *
	 *	This attribute is the MatrixProvider that provides the targeting matrix
	 *	with a source to move the camera matrix towards.	 
	 *
	 *	@type	Read-Write MatrixProvider
	 */
	PY_ATTRIBUTE( source )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
ThirdPersonTargetingMatrix::ThirdPersonTargetingMatrix( MatrixProviderPtr pSource, PyTypeObject * pType ):
	MatrixProvider( false, pType ),
	pSource_( pSource )
{
	BW_GUARD;	
};


/**
 *	This method implements the MatrixProvider matrix virtual method.
 *	It returns a matrix that can be used to provide a targeting matrix
 *	suitable for use by a 3rd person perspective game.
 */
void ThirdPersonTargetingMatrix::matrix( Matrix& lookFrom ) const
{
	BW_GUARD;
	//Where do we pick from?
	lookFrom = Moo::rc().invView();

	//move the pick position along the camera normal to be near the player's head
	Matrix sourceMatrix;
	pSource_->matrix( sourceMatrix );

	Vector3 cameraPosition = Moo::rc().invView().applyToOrigin();
	Vector3 sourcePos = sourceMatrix.applyToOrigin();
	Vector3 entityDelta = sourcePos - cameraPosition;

	// Calculate the distance along the normal to the player's head.
	float zOffset = lookFrom.applyToUnitAxisVector( Z_AXIS ).
											dotProduct( entityDelta );
	// And move the lookFrom matrix along the normal by that distance
	lookFrom.preTranslateBy( Vector3( 0.f, 0.f, zOffset ) );
};


/**
 *	This method returns a new ThirdPersonTargetingMatrix.  It
 *	interprets the PyObject* argument as a matrix provider.
 */
PyObject * ThirdPersonTargetingMatrix::pyNew( PyObject * args )
{
	BW_GUARD;
	PyObject* pObject;

	if (!PyArg_ParseTuple( args, "O", &pObject ) || !MatrixProvider::Check( pObject ))
	{
		PyErr_Format( PyExc_TypeError,
			"BigWorld.ThirdPersonTargetingMatrix expects one Matrix "
			"Provider argument" );
		return NULL;
	}

	return new ThirdPersonTargetingMatrix( (MatrixProvider*)pObject );
};


/*~	function BigWorld.ThirdPersonTargetingMatrix
 *
 *	Creates and returns a new ThirdPersonTargetingMatrix
 *	provider.  This matrix provider implements a special-case
 *	matrix designed for targeting while using a 3rd person camera.
 *	It takes camera matrix and moves it forward along its z-axis
 *	until it lines up with the position given by the user-specified
 *	source matrix (for example, the player entity matrix).
 *
 *	@param source	MatrixProvider to help calculate the look from matrix.
 */
PY_FACTORY( ThirdPersonTargetingMatrix, BigWorld )	


//-----------------------------------------------------------------------------
//	Section - MouseTargetingMatrix
//-----------------------------------------------------------------------------
PY_TYPEOBJECT( MouseTargetingMatrix )

PY_BEGIN_METHODS( MouseTargetingMatrix )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MouseTargetingMatrix )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
MouseTargetingMatrix::MouseTargetingMatrix( PyTypeObject * pType ):
	MatrixProvider( false, pType )
{
	BW_GUARD;	
};


/**
 *	This method implements the MatrixProvider matrix virtual method.
 *	It returns a matrix that can be used to provide a targeting matrix
 *	based on the mouse's current position on the screen.
 */
void MouseTargetingMatrix::matrix( Matrix& lookFrom ) const
{
	BW_GUARD;
	Vector2 mpos = SimpleGUI::instance().mouseCursor().position();

	Vector3 cameraSpace = Moo::rc().camera().nearPlanePoint( mpos.x, mpos.y );
	Vector3 ray = Moo::rc().invView().applyVector( cameraSpace );
	ray.normalise();

	Vector3 src = Moo::rc().invView().applyPoint( cameraSpace );

	Vector3 up = Vector3( 0.0f, 1.0f, 0.0f );
	if ( fabsf( ray.y ) > 0.99f )
	{
		up = Vector3( 0.0f, 0.0f, 1.0f );
	}
	lookFrom.lookAt( src, ray, up );
	lookFrom.invert();
};

/*~	function BigWorld.MouseTargetingMatrix
 *
 *	Creates and returns a new MouseTargetingMatrix provider.
 *	This matrix provider produces a world-space transform (position 
 *	and direction) from the unprojected mouse position. The translation 
 *	component is positioned on the near plane of the camera.
 *
 *	While this provider is intended to be used with the BigWorld
 *	targeting system, you can also use it to produce world
 *	position and direction vectors from Python. For example, these
 *	vectors could be used as parameters to BigWorld.collide:
 *
 *	@{
 *	mtm = Math.Matrix( BigWorld.MouseTargetingMatrix() )
 *	src = mtm.applyToOrigin()
 *	dir = mtm.applyToAxis(2)
 *	@}
 *
 *	@see PyTarget
 *
 */
PY_FACTORY( MouseTargetingMatrix, BigWorld )	

BW_END_NAMESPACE

// entity_picker.cpp
