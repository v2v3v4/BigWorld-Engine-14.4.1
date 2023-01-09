#ifndef CHUNK_OBSTACLE_LOCATOR_HPP
#define CHUNK_OBSTACLE_LOCATOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "gizmo/tool_locator.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements a tool locator that sits on any arbitrary
 *	chunk obstacle type.  It takes any Chunk CollisionCallback
 *	class as a parameter and uses it to locate the tool.
 */
class ChunkObstacleToolLocator : public ChunkToolLocator
{
	Py_Header( ChunkObstacleToolLocator, ChunkToolLocator )
public:
	ChunkObstacleToolLocator( class CollisionCallback& collisionRoutine,
			PyTypeObject * pType = &s_type_ );
	virtual void calculatePosition( const Vector3& worldRay, Tool& tool );

	virtual bool positionValid() const { return positionValid_; }

	//class CollisionCallback& callback()				{return *callback_;}
	//void callback( class CollisionCallback& c )		{callback_ = &c;}

	PY_FACTORY_DECLARE()

protected:
	class CollisionCallback* callback_;
	bool positionValid_;
};

BW_END_NAMESPACE

#endif	// CHUNK_OBSTACLE_LOCATOR_HPP
