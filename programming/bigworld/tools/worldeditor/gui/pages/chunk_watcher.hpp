#ifndef CHUNK_WATCHER_HPP
#define CHUNK_WATCHER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "moo/image.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class provides details about the chunk status for terrain chunks.
 */
class ChunkWatcher : public ReferenceCount
{
public:
	enum ChunkState
	{
		// These options are mutually exlusive and consitute the Chunk's
		// state:
		UNLOADED					= 1,
		LOADED						= 2,
		DIRTY_NEEDS_SHADOW_CALC		= 3,
		DIRTYSHADOW_CALCED			= 4,
		STATE_MASK					= 7,

		// This option are mutually exclusive:
		CAN_UNLOAD					= 0,
		CANNOT_UNLOAD				= 8		
	};

	ChunkWatcher();

	uint32 state(int32 x, int32 z) const;
	void state(int32 x, int32 z, uint32 state);

	bool canUnload(int32 x, int32 z) const;
	void canUnload(int32 x, int32 z, bool unloadable);	

	void onNewSpace(int32 minX, int32 minZ, int32 maxX, int32 maxZ);
	void onLoadChunk(int32 x, int32 z);
	void onUnloadChunk(int32 x, int32 z);
	void onDirtyChunk(int32 x, int32 z);
	void onCalcShadow(int32 x, int32 z);

	size_t numberLoadedChunks() const;
	size_t numberUnloadedChunks() const;
	size_t numberDirtyChunks() const;
	size_t numberCalcedChunks() const;

	int32 minX() const;
	int32 minZ() const;
	int32 maxX() const;
	int32 maxZ() const;

protected:
	void calculateStats() const;

private:
	Moo::Image<uint32>			image_;
	int32						minX_;
	int32						minZ_;
	mutable bool				numbersValid_;
	mutable size_t				numLoaded_;
	mutable size_t				numUnloaded_;
	mutable size_t				numDirty_;
	mutable size_t				numCalced_;
};

BW_END_NAMESPACE

#endif // CHUNK_WATCHER_HPP
