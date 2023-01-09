#ifndef PY_PHYSICS2_HPP
#define PY_PHYSICS2_HPP

#ifdef MF_SERVER
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#else
#include "space/space_manager.hpp"
#include "space/client_space.hpp"
#endif // MF_SERVER

#include "network/basictypes.hpp"

#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

template<class CallbackType>
PyObject * py_collide( SpaceID spaceID, 
		const Position3D & src, const Position3D & dst )
{
#ifdef MF_SERVER
	ChunkSpacePtr pSpace = ChunkManager::instance().space( spaceID, false );
#else
	ClientSpacePtr pSpace = SpaceManager::instance().space( spaceID );
#endif
	if (!pSpace)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.collide: "
			"No space ID %d", int(spaceID) );
		return NULL;
	}

	CallbackType fdpt;
	float dist = pSpace->collide( src, dst, fdpt );

	if (!fdpt.hasTriangle() || dist < 0.f)
	{
		Py_RETURN_NONE;
	}

	Vector3 dir = dst-src;
	dir.normalise();
	Position3D hitpt = src + dir * dist;

	WorldTriangle resultTri = fdpt.triangle();

	return Py_BuildValue( "(N,(N,N,N),i)",
		Script::getData( hitpt ),
		Script::getData( resultTri.v0() ),
		Script::getData( resultTri.v1() ),
		Script::getData( resultTri.v2() ),
		resultTri.materialKind() );
}

BW_END_NAMESPACE

#endif // PY_PHYSICS2_HPP
