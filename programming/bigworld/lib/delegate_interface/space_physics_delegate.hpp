#ifndef SPACE_PHYSICS_DELEGATE_HPP_
#define SPACE_PHYSICS_DELEGATE_HPP_

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/shared_ptr.hpp"

BW_BEGIN_NAMESPACE

/* 
 * ISpacePhysicsDelegate - this class is for managing the lifetime of the concept which
 * is parallel to a "space" in a cellapp (for example, the physical simulation
 * world)
 */
class ISpacePhysicsDelegate
{
public:
	virtual ~ISpacePhysicsDelegate() {}
        
	// Get boundary in a "neutral" format: min-max pairs
	virtual void getBounds(float* minX, float* maxX, float* minY, 
						   float* maxY, float* minZ, float* maxZ) const = 0;
};

typedef shared_ptr< ISpacePhysicsDelegate > ISpacePhysicsDelegatePtr;

BW_END_NAMESPACE

#endif // SPACE_PHYSICS_DELEGATE_HPP_
