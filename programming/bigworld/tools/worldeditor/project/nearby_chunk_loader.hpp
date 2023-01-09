#ifndef NEARBY_CHUNK_LOADER_HPP
#define NEARBY_CHUNK_LOADER_HPP

BW_BEGIN_NAMESPACE

class NearbyChunkLoader
{
public:
	NearbyChunkLoader( int32 minLGridX, int32 maxLGridX, int32 minLGridY, int32 maxLGridY );
	~NearbyChunkLoader();

	void loadChunks( int16 x, int16 z );
	void releaseUnusedChunks( BW::set<Chunk*>& unloadedChunks );

private:
	bool isChunkLoadedByMe( int16 x, int16 y );
	int32 minLGridX_;
	int32 maxLGridX_;
	int32 minLGridY_;
	int32 maxLGridY_;
	BW::vector<uint16>		 xToRelease_;
	BW::vector<uint16>		 yToRelease_;

	BW::vector<uint16>		 xLoaded_;
	BW::vector<uint16>		 yLoaded_;

};

BW_END_NAMESPACE

#endif
