
#include "pch.hpp"

#include "entity_flare_collider.hpp"
#include "duplo/pymodel.hpp"
#include "moo/render_context.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "entity_flare_collider.ipp"
#endif



EntityPhotonOccluder::EntityPhotonOccluder( Entity & ent )
:entity_( &ent )
{
}

EntityPhotonOccluder::~EntityPhotonOccluder()
{
}


/**
 *	This method returns true if the given entity collides with
 *	a straight line between the light and the camera
 *
 *	This method is a gross approximation only, that works just ok
 *	assuming you are hit-testing against a humanoid character
 *
 *	@param lightSourcePosition	The matrix that represents the light's
 *			transform in world space.
 *	@param cameraPosition		The world position of the camera
 *	@param le					The lens effect in question (unused here)
 *
 *	@return	1 if there was no collision, else 0.
 */
float
EntityPhotonOccluder::collides(
		const Vector3 & lightSourcePosition,
		const Vector3 & cameraPosition,
		const LensEffect& le )
{
	BW_GUARD;
	PyModel * pModel = entity_->pPrimaryModel();

	float result = 1.f;

	if ( pModel )
	{
		//get the object space to clip space matrix
		Matrix objectToClip( pModel->worldTransform() );
		objectToClip.postMultiply( Moo::rc().viewProjection() );

		//get the light to clip space matrix
		Matrix lightToClip;
		lightToClip.setTranslate( lightSourcePosition );
		lightToClip.postMultiply( Moo::rc().viewProjection() );

		result = checkTorso( objectToClip, lightToClip );

		if ( result == 1.f )
		{
			result = checkHead( objectToClip, lightToClip );
		}
	}

	return result;
}


/**
 *	This method checks for photon occlusion through the entity torso.
 *
 *	@param objectToClip the object to clip matrix for the entity.
 *	@param lightToClip  the object to clip matrix for the light source.
 *
 *	@return A float inidicating the percentage of occlusion.
 */
float
EntityPhotonOccluder::checkTorso(
		const Matrix & objectToClip,
		const Matrix & lightToClip )
{
	BW_GUARD;
	PyModel * pModel = entity_->pPrimaryModel();

	//Get model's clip space bounding box
	BoundingBox bb;
	pModel->localBoundingBox(bb,true);
	Vector3 middle = (bb.minBounds() + bb.maxBounds()) / 2.f;

	//FUDGE FACTOR - the middle of a humanoid should be roughly
	//in the middle of the torso, about 25cm. above the middle of the body
	middle.y += 0.25f;

	Matrix transform;
	transform.setTranslate( middle );
	transform.postMultiply( objectToClip );

	//Guess lights's clip space bounding box is ( lightToCamera position, +- 0.25 )
	if ( (lightToClip._44 > 0.f) && (transform._44 > 0.f) )
	{
		float oow = 1.f / lightToClip._44;
		Vector3 lightPos( lightToClip._41 * oow,
							lightToClip._42 * oow,
							1.f );

		oow = 1.f / transform._44;
		Vector3 objPos( transform._41 * oow,
							 transform._42 * oow,
							 1.f );

		objPos -= lightPos;

		float lengthSq = objPos.lengthSquared();

		if ( lengthSq < (0.25f * 0.25f) )
			return 0.f;
	}

	return 1.f;
}


/**
 *	This method checks for photon occlusion through the entity head.
 *
 *	@param objectToClip the object to clip matrix for the entity.
 *	@param lightToClip  the object to clip matrix for the light source.
 *
 *	@return A float inidicating the percent of occlusion.
 */
float
EntityPhotonOccluder::checkHead(
		const Matrix & objectToClip,
		const Matrix & lightToClip )
{
	BW_GUARD;
	PyModel * pModel = entity_->pPrimaryModel();

	const float HEAD_SIZE = 0.125f;
	//Get model's clip space bounding box
	BoundingBox bb;
	pModel->localBoundingBox(bb,true);
	Vector3 top = (bb.minBounds() + bb.maxBounds()) / 2.f;
	top.y = bb.maxBounds().y;

	//FUDGE FACTOR - the head of a humanoid should be roughly
	//in about 15cm. below the top of the body
	top.y -= HEAD_SIZE;

	Matrix transform;
	transform.setTranslate( top );
	transform.postMultiply( objectToClip );

	//Guess lights's clip space bounding box is ( lightToCamera position, +- HEAD_SIZE )
	if ( (lightToClip._44 > 0.f) && (transform._44 > 0.f) )
	{
		float oow = 1.f / lightToClip._44;
		Vector3 lightPos( lightToClip._41 * oow,
							lightToClip._42 * oow,
							1.f );

		oow = 1.f / transform._44;
		Vector3 objPos( transform._41 * oow,
							 transform._42 * oow,
							 1.f );

		objPos -= lightPos;

		float lengthSq = objPos.lengthSquared();

		if ( lengthSq < (HEAD_SIZE * HEAD_SIZE) )
			return 0.f;
	}

	return 1.f;
}

std::ostream& operator<<(std::ostream& o, const EntityPhotonOccluder& t)
{
	BW_GUARD;
	o << "EntityPhotonOccluder\n";
	return o;
}

BW_END_NAMESPACE

/*entity_flare_collider.cpp*/
