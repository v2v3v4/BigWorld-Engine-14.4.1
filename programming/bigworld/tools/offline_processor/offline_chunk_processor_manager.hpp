#ifndef OFFLINE_CHUNK_PROCESSOR_MANAGER_HPP__
#define OFFLINE_CHUNK_PROCESSOR_MANAGER_HPP__


#include "chunk/chunk_processor_manager.hpp"
#include "common/space_editor.hpp"

BW_BEGIN_NAMESPACE

class OfflineChunkProcessorManager : 
	public ChunkProcessorManager,
	public SpaceEditor
{
	GeometryMapping* mapping_;
	unsigned int clusterSize_;
	unsigned int clusterIndex_;
	bool terminated_;

	virtual bool isMemoryLow( bool testNow = false ) const;
	virtual void unloadChunks();
	virtual bool tick();
	virtual bool isChunkEditable( const BW::string& chunk ) const;

public:
	OfflineChunkProcessorManager( int numThread,
		unsigned int clusterSize, unsigned int clusterIndex );

	void init( const BW::string& spaceDir );
	void fini();
	void terminate() { terminated_ = true; }

	virtual GeometryMapping* geometryMapping() { return mapping_; }
	virtual const GeometryMapping* geometryMapping() const { return mapping_; };
};

BW_END_NAMESPACE

#endif//OFFLINE_CHUNK_PROCESSOR_MANAGER_HPP__
