#ifndef CHUNK_PHOTOGRAPHER_HPP
#define CHUNK_PHOTOGRAPHER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "resmgr/datasection.hpp"
#include "moo/render_target.hpp"
#include "romp/time_of_day.hpp"
#include "cstdmf/singleton.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	class DrawContext;
}

class ChunkPhotographer : public Singleton< ChunkPhotographer >
{
public:
	ChunkPhotographer();
	~ChunkPhotographer();

	static bool photograph( class Chunk& );
	static bool photograph( class Chunk&, DataSectionPtr ds, int width, int height );
	static bool photograph( class Chunk&, const BW::string& filePath, int width, int height );

private:
	bool takePhoto( class Chunk&, const BW::string& filePath, int width, int height, bool useDXT );
	bool takePhoto( class Chunk&, DataSectionPtr ds, int width, int height, bool useDXT );
	bool takePhotoInternal( Chunk& chunk, int width, int height );
	bool savePhotoToFile( Chunk&, const BW::string& filePath,
		int width, int height, bool useDXT );
	bool savePhotoToDatasection( Chunk&, DataSectionPtr ds,
		int width, int height, bool useDXT );
	bool beginScene();
	void setStates(class Chunk&);
	void renderScene(class Chunk&);
	void resetStates();
	void endScene();

	Moo::RenderTargetPtr			pRT_;
	Moo::LightContainerPtr			lighting_;
	OutsideLighting					chunkLighting_;
	Moo::LightContainerPtr			savedLighting_;
	OutsideLighting*				savedChunkLighting_;
	float							oldFOV_;
	Matrix							oldInvView_;
	Chunk *							pOldChunk_;
	bool							oldDrawUDOs_;
	bool							oldIgnoreLods_;
	bool							oldWaterSimulationEnabled_;
	bool							oldWaterDrawReflection_;
	Moo::DrawContext*				photographerDrawContext_;
};

BW_END_NAMESPACE

#endif // CHUNK_PHOTOGRAPHER_HPP
