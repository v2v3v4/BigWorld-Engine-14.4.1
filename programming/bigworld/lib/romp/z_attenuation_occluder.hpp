#ifndef ZATTENUATIONOCCLUDER_HPP
#define ZATTENUATIONOCCLUDER_HPP

#include "handle_pool.hpp"
#include "lens_effect.hpp"
#include "moo/render_target.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/custom_mesh.hpp"


BW_BEGIN_NAMESPACE

const uint32 MAX_FLARES = 2048;

/**
 *	This class occlusion tests and draws a bunch of lens effects.
 *	It is provided as an alternative to using DirectX Occlusion Queries,
 *	as it is impossible to determine how many occlusion queries are
 *	available for any particular video card, and exceeding that
 *	number causes bad stalls to occur while we wait for the GPU to catch up.
 *
 *	The algorithm:
 *	1. For each lens effect visible on screen, add a 2x2 rectangle to our batch
 *	and assign it a location in the staging texture.
 *	2. Z-Read Disable : Draw the batch to the alpha channel of the main buffer,
 *	entirely black
 *	3. Z-Read Enable : Draw the batch to the alpha channel again, but in white.
 *	4. Copy the alpha results into the staging texture.
 *	5. Optional : Repeat step 2 to reset the main alpha channel to black.  This
 *	may be required for the following heat-shimmer pass.
 *	6. Draw all lens flares onto the screen, attenuating the corona texture map
 *	with the attenuation results from the staging texture.
 *
 *	There are 2 reasons to have a staging texture
 *	1. so lens flares can be drawn to the back buffer, if required
 *	2. so attenuation results can be accumulated over several frames, to fade in/
 *	fade out lens flares instead of just popping them in and out.
 */
class ZAttenuationOccluder
{
public:
	ZAttenuationOccluder( DataSectionPtr config = NULL );
	~ZAttenuationOccluder();
	void update( LensEffectsMap& );
private:
	void setPointSize( float size );
	HandlePool				handlePool_;
	Moo::RenderTargetPtr	smallDestTarget_;
	Moo::RenderTargetPtr	pStagingTexture_;
	Moo::RenderTargetPtr	pBBCopy_;
	Moo::RenderTargetSetter*pRTSetter_;

	typedef CustomMesh<Moo::VertexTLUV2> DrawBatch;
	typedef BW::map<BW::string, DrawBatch*> DrawBatchMap;
	typedef BW::list<DrawBatch*> DrawBatchList;
	CustomMesh<Moo::VertexXYZ> testBatch_;
	CustomMesh<Moo::VertexTUV> stagingBatch_;
	CustomMesh<Moo::VertexTLUV> stagingmesh_;
	CustomMesh<Moo::VertexTLUV> transferMesh_;
	void clearDrawBatches();
	void freeDrawBatches();
	DrawBatch* getDrawBatch(const LensEffect& le);
	DrawBatchMap materialBatchList_;
	DrawBatchList drawBatches_;
};

BW_END_NAMESPACE

#endif // ZATTENUATIONOCCLUDER_HPP
