#ifndef ENTITY_PICKER_HPP
#define ENTITY_PICKER_HPP

#include "cstdmf/bw_vector.hpp"
#include "network/basictypes.hpp"
#include "moo/moo_math.hpp"
#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"
#include "pyscript/script.hpp"
#include "cstdmf/debug.hpp"
#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

class Entity;
class PyEntity;
class BoundingBox;
class MatrixProvider;
typedef SmartPointer<MatrixProvider> MatrixProviderPtr;
typedef ScriptObjectPtr< PyEntity > PyEntityPtr;
typedef WeakPyPtr< PyEntity > PyEntityWPtr;

/**
 * EntityPicker
 *
 * picks entities from a list, based on ( in order ):
 *
 * - capabilities
 * - distance
 * - field of view
 * - line of sight
 */
class EntityPicker
{
public:
	EntityPicker();
	~EntityPicker();

	void update( float dTime );

	MatrixProviderPtr source() const;
	void source( MatrixProviderPtr pSource );

	void selectionFoV( float angleRadians );
	float selectionFoV() const;
	void defaultSelectionFoV( float angleRadians );
	float defaultSelectionFoV() const;

	void selectionDistance( float distance );
	float selectionDistance() const;
	void defaultSelectionDistance( float distance );
	float defaultSelectionDistance() const;

	void selectionDistancePush( float distance );
	void selectionDistancePop();

	void deselectionFoV( float angleRadians );
	float deselectionFoV() const;
	void defaultDeselectionFoV( float angleRadians );
	float defaultDeselectionFoV() const;

	void autoAimFrictionDistance( float distance );
	float autoAimFrictionDistance() const;
	void autoAimFrictionHorizontalAngle( float angleRadians );
	void autoAimFrictionVerticalAngle( float angleRadians );
	float autoAimFrictionHorizontalAngle() const;
	float autoAimFrictionVerticalAngle() const;

	void autoAimFrictionMinimumDistance( float distance );
	float autoAimFrictionMinimumDistance() const;

	void autoAimFrictionFalloffDistance( float distance );
	float autoAimFrictionFalloffDistance() const;
	void autoAimFrictionHorizontalFalloffAngle( float angleRadians );
	void autoAimFrictionVerticalFalloffAngle( float angleRadians );
	float autoAimFrictionHorizontalFalloffAngle() const;
	float autoAimFrictionVerticalFalloffAngle() const;

	void autoAimAdhesionDistance( float distance );
	float autoAimAdhesionDistance() const;
	void autoAimAdhesionHorizontalAngle( float angleRadians );
	void autoAimAdhesionVerticalAngle( float angleRadians );
	float autoAimAdhesionHorizontalAngle() const;
	float autoAimAdhesionVerticalAngle() const;

	void autoAimAdhesionFalloffDistance( float distance );
	float autoAimAdhesionFalloffDistance() const;
	void autoAimAdhesionHorizontalFalloffAngle( float angleRadians );
	void autoAimAdhesionVerticalFalloffAngle( float angleRadians );
	float autoAimAdhesionHorizontalFalloffAngle() const;
	float autoAimAdhesionVerticalFalloffAngle() const;

	bool isHeld() const;
	void isHeld( bool down );

	bool isEnabled() const				{ return isEnabled_; }
	void isEnabled( bool value )		{ isEnabled_ = value; }

	bool skeletonCheckEnabled() const			{ return skeletonCheckEnabled_; }
	void skeletonCheckEnabled( bool value )		{ skeletonCheckEnabled_ = value; }

	bool isFull() const					{ return isFull_; }
	bool isHidden() const				{ return isHidden_; }

	void noPartialLocks( bool enable )	{ noPartial_ = enable; }
	bool noPartialLocks() const			{ return noPartial_; }

	PyEntityPtr exclude() const;
	void exclude( PyEntityPtr pPyEntity );

	void lockOn( Entity * pEntity );
	void clear();

	void	setCapabilities( const Capabilities & capsOn,
							 const Capabilities & capsOff );

	void	check( float * pDeltaTime = NULL );

	Entity*	hasAnAutoAimTarget( float& autoAimTargetDistance, float& autoAimTargetAngle,
								bool useFriction, bool wantHorizontalAngle );

	Entity* pTarget() const;
	Entity* pGeneralTarget() const;

	void	drawDebugStuff();

// These two methods were moved from the private section, it didn't compile before
	bool	capsCheck( const Entity& entity ) const;
	bool	skeletonCheck( const Entity& entity, const Matrix& lookFrom ) const;

private:
	void	pickFirst();

	void	prepareViewMatrix( float fov, float aspectRatio,
								Matrix & lookFrom,
								Matrix & fovMatrix,
								Vector3 & sourcePos ) const;
	bool	isEntitySelectable( const Entity & entity,
								const Matrix & lookFrom,
								const Matrix & fovMatrix,
								const Vector3 & sourcePos,
								bool skipMatch,
								bool * pWasLineOfSight = NULL ) const;

	///Selection heuristics ( in order )
	bool	distanceCheck( const Entity& entity, const Vector3& sourcePos ) const;
	bool	fovCheck( BoundingBox & bb, const Matrix & fovMatrix ) const;
	bool	twoPointLOSCheck( const Entity & entity, const BoundingBox& bb, const Matrix& lookFrom ) const;
	bool	fullBoundsLOSCheck( const Entity & entity, const BoundingBox& bb, const Matrix& lookFrom ) const;

	///Utility Methods
	void	getBoundingBox( const Entity & entity, BoundingBox & result ) const;
	static float zDistance( const Entity & entity, const Matrix & lookFrom );
	float angleTo( const Entity & entity, const Matrix & fovMatrix ) const;

	void	setCurrentEntity( Entity * pEntity,
					bool isFull, bool isHidden );

	///the field of view width/height, in radians
	float	selectionFoV_;
	float	deselectionFoV_;
	///cached for retrieval in selectableArea()
	float	distance_;
	float	oldDistance_;	// for simple 1 level stack
	float	currentTargetDistance_;	// for simple 1 level stack

	float	defaultSelectionFoV_;
	float	defaultDeselectionFoV_;
	float	defaultDistance_;

	// auto aim stuff
	float	autoAimFrictionDistance_;
	float	autoAimFrictionHorizontalAngle_;
	float	autoAimFrictionVerticalAngle_;
	float	autoAimFrictionMinimumDistance_;
	float	autoAimFrictionFalloffDistance_;
	float	autoAimFrictionHorizontalFalloffAngle_;
	float	autoAimFrictionVerticalFalloffAngle_;
	float	autoAimAdhesionDistance_;
	float	autoAimAdhesionHorizontalAngle_;
	float	autoAimAdhesionVerticalAngle_;
	float	autoAimAdhesionFalloffDistance_;
	float	autoAimAdhesionHorizontalFalloffAngle_;
	float	autoAimAdhesionVerticalFalloffAngle_;

	///the matrix from whose viewpoint things are selected
	MatrixProviderPtr	pSource_;

	// Note: we don't keep a reference to the current entity.
	// If its ref count gets to zero, its destructor takes it
	//  out of the world, which makes it unsuitable for our selection,
	//  so it'd call check and we'd deselect it here.
	// So, we don't need to keep a reference to this entity.
	Entity* pTarget_;

	// isFull_ stores whether or not the current entity is a 'full' target.
	// That is, a target that has its targetCaps satisfied.
	bool isFull_;

	// isHidden_ stores whether the target is current held but cannot currently
	// be seen.
	bool isHidden_;

	///bounding box fudge factors
	float	upperMarginFactor_;		//percentage of the upper part of the bounding box that is unselectable
	float	sideMarginFactor_;		//ditto for the sides
	float	lowerMarginFactor_;		//and the bottom

	///capabilities that the target must possess
	Capabilities	capsOn_;
	///capabilities that the target must lack
	Capabilities	capsOff_;
	///special mode = when on, you cannot choose a target that would result in a partial lock.
	bool			noPartial_;

	// This value is only really for debugging. If true, the current target is
	// not lost until clear is called.
	bool isLocked_;

	// If the value is true, the target is kept for longer. It is kept while the
	// entity is within the deselection frustrum and is not hidden for longer
	// than maxHiddenTime_.
	bool isHeld_;

	// Whether or not the targeting system is enabled
	bool isEnabled_;

	// Whether or not we should check for skeleton collisions
	bool skeletonCheckEnabled_;

	// How long the current entity has been hidden for.
	float hiddenTime_;
	// The maximum length of time that an entity can be hidden for.
	float maxHiddenTime_;

	// An entity which should not be selected, usually the player.
	mutable PyEntityWPtr pExclude_;

	// debug drawing
	bool debugDraw_;
	mutable BW::vector<BoundingBox>	bbs_;

	EntityPicker(const EntityPicker&);
	EntityPicker& operator=(const EntityPicker&);

	bool useFullBounds( const Entity& entity ) const;

	friend class PickFirstEntityVisitor;
	friend class AutoAimEntityVisitor;
};


/*~ class BigWorld.ThirdPersonTargetingMatrix
 */
/**
 *	This class is a MatrixProvider that produces the 1.8 style
 *	of targeting matrix.  This matrix is calculated as being
 *	the current camera matrix, moved towards a source point along
 *	the z-axis of the camera.
 */
class ThirdPersonTargetingMatrix : public MatrixProvider
{
	Py_Header( ThirdPersonTargetingMatrix, MatrixProvider )

public:	
	ThirdPersonTargetingMatrix( MatrixProviderPtr pSource,
		PyTypeObject * pType = &ThirdPersonTargetingMatrix::s_type_ );
	void matrix( Matrix& lookFrom ) const;	

	PY_RW_ATTRIBUTE_DECLARE( pSource_, source )
	PY_FACTORY_DECLARE()	

private:
	MatrixProviderPtr pSource_;
};

/**
 *	Targets along the view-space coords.
 */
class MouseTargetingMatrix : public MatrixProvider
{
	Py_Header( MouseTargetingMatrix, MatrixProvider )

public:	
	MouseTargetingMatrix( PyTypeObject * pType = &MouseTargetingMatrix::s_type_ );
	void matrix( Matrix& lookFrom ) const;	

	static MouseTargetingMatrix * New()	{ return new MouseTargetingMatrix(); }
	PY_AUTO_FACTORY_DECLARE( MouseTargetingMatrix, END )
};


#ifdef CODE_INLINE
#include "entity_picker.ipp"
#endif

BW_END_NAMESPACE

#endif // ENTITY_PICKER_HPP

// entity_picker.hpp
