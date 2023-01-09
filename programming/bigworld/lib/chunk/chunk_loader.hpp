#ifndef CHUNK_LOADER_HPP
#define CHUNK_LOADER_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bgtask_manager.hpp"

BW_BEGIN_NAMESPACE

class Chunk;
class ChunkSpace;
class Vector3;
class GeometryMapping;


class FindSeedTask : public BackgroundTask
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param	pSpace	the space to find seed chunk in, e.g. the camera space
	 *	@param	where	the point used to find seed chunk,
	 *					it could be the center point on near plane
	 *					of the camera frustrum
	 */
	FindSeedTask( ChunkSpace * pSpace, const Vector3 & where );
	~FindSeedTask();

	virtual void doBackgroundTask( TaskManager & mgr );
	virtual void doMainThreadTask( TaskManager & mgr );
	void release( bool destroyChunk = true );
	bool isComplete();
	Chunk* foundSeed();

private:
	SmartPointer<ChunkSpace> pSpace_;
	Vector3 where_;
	Chunk * foundChunk_;
	GeometryMapping * mapping_;
	bool isComplete_;
	bool released_;
};


/**
 *	This class loads chunks using a background thread.
 */
class ChunkLoader
{
public:
	static void load( Chunk * pChunk, int priority = 0 );
	static void loadNow( Chunk * pChunk );
	static FindSeedTask* findSeed( ChunkSpace * pSpace, const Vector3 & where );
};

BW_END_NAMESPACE


#endif // CHUNK_LOADER_HPP
