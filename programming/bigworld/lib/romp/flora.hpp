#ifndef FLORA_HPP
#define FLORA_HPP

#include "flora_constants.hpp"
#include "moo/device_callback.hpp"
#include "terrain/terrain_finder.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "animation_grid.hpp"
#include "light_map.hpp"
#include "flora_block.hpp"
#include "ecotype.hpp"
#include "cstdmf/bw_set.hpp"

#include "resmgr/resource_modification_listener.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class Material;
//	class VertexBuffer;
}

namespace Terrain
{
	class BaseTerrainBlock;
	typedef SmartPointer<BaseTerrainBlock> BaseTerrainBlockPtr;
}

/**
 * TODO: to be documented.
 */
class Flora : 
	public Moo::DeviceCallback
{
	typedef Flora This;
	typedef void (*BlocksCompletedCallback)();

public:
	Flora();
	~Flora();

	static const BW::vector< Flora* > & floras() { return s_floras_; }

	bool	init( DataSectionPtr pSection, uint32 terrainVersion );

	void	activate();
	void	deactivate();

	void	update( float dTime, class EnviroMinder& enviro );
	void	draw( float dTime, class EnviroMinder& enviro );
	void	drawDebug();

	void	drawPostDeferred();

	void	createUnmanagedObjects();
	void	deleteUnmanagedObjects();

	//Utility methods - used by all classes in the flora package.

	//returns the ecotype given a vector2
	Ecotype& ecotypeAt( const Vector2& );
	//generates an ecotype at the given position
	Ecotype::ID generateEcotypeID( const Vector2& p );
	//seeds a random number look up table given a world position.
	//table is comprised of offsets no larger than BLOCK_WIDTH / 2
	void seedOffsetTable( const Vector2& );
	//retrieves the next random number.
	const Vector2& nextOffset();
	float nextRotation();
	float nextRandomFloat();
	Terrain::BaseTerrainBlockPtr getTerrainBlock( const Vector3 & pos, Vector3 & relativePos, const Vector2* referencePt = NULL ) const;
	int getTerrainBlockID( const Matrix & terrainBlockTransform, const float terrainBlockSize ) const;

	//mark the block at x,z dirty
	void resetBlockAt( float x, float z );
	//calling this will reset the flora
	static void floraReset();
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, floraReset, END )
	static void enabled( bool state )	{ s_enabled_ = state; }
	static bool enabled()				{ return s_enabled_; }
	//calling this will reset the flora and its renderer
	void vbSize( uint32 bytes );	
	uint32 vbSize() const	{ return vbSize_; }
	static void floraVBSize( uint32 bytes );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETVOID, floraVBSize, ARG( uint32, END) )

	const Matrix& transform( const FloraBlock& block );	
	class FloraRenderer* pRenderer() { return pRenderer_; }
	class FloraTexture* floraTexture() { return pFloraTexture_; }	

	void fillBlocks();

	void initialiseOffsetTable( float blurAmount = 2.f );

	DataSectionPtr data() { return data_; }

	void maxVBSize( uint32 bytes );
	uint32 maxVBSize() const;

	uint32 terrainVersion() const { return terrainVersion_; }
	uint32	verticesPerBlock() const { return numVerticesPerBlock_; }

#ifdef EDITOR_ENABLED
	bool highLight( BW::list<int>& levelsIndex, bool hightLight );
#endif

	static void setBlocksCompletedCallback(
		BlocksCompletedCallback callback );

private:	
	static void CallBlocksCompletedCallback();
	static BlocksCompletedCallback s_blocksCompletedCallback_;

	bool moveBlocks( const Vector2& camPos2 );
	void accumulateBoundingBoxes();
	void cull();	
	void teleportCamera( const Vector2& camPos2 );
	void drawSorted(class EnviroMinder& enviro);
	void getViewLocation( Vector3& ret );

	DataSectionPtr		data_;
	FloraBlock			blocks_[BLOCK_STRIDE][BLOCK_STRIDE];

private:
	Ecotype*			ecotypes_[256];
	Ecotype				degenerateEcotype_;
	uint32				vbSize_;
	uint32				numVertices_;
	Vector2				offsets_[LUT_SIZE];
	float				randoms_[LUT_SIZE];
	int					lutSeed_;
	Vector2				lastPos_;
	bool				cameraTeleport_;
	uint32				numVerticesPerBlock_;
	uint32              maxVBSize_;
	float				cosMaxSlope_;

	class FloraRenderer*	pRenderer_;

	//these variables implement a virtual mapping into the blocks_ array,
	//and allow for a direct lookup of blocks that need moving
	int					centerBlockX_;		//granularised XZ position of center most block
	int					centerBlockZ_;

	//macro bounding box culling
	class MacroBB
	{
	public:
		BoundingBox		bb_;
		FloraBlock*		blocks_[25];
	};
	MacroBB				macroBB_[400];		//hard-coded number of blocks for now.
	BW::set<FloraBlock*> movedBlocks_;

	//Terrain block lookup cache
    mutable Terrain::TerrainFinder::Details details_;
	mutable Vector2		lastRefPt_;

	typedef StringRefUnorderedMap< Ecotype::ID > StringEcotypeHashMap;
	StringEcotypeHashMap texToEcotype_;

	// Version of terrain this flora belongs to.
	uint32				terrainVersion_;

	static bool			s_enabled_;
	static BW::vector< Flora* > s_floras_;
	class FloraTexture*		pFloraTexture_;
};


/**
 *	Manages all graphics settings related to the Flora.
 */
class FloraSettings
{
public:
	void init(DataSectionPtr resXML);

	float vbRatio() const;
    bool isInitialised() const;
	static const char * getFloraSettingId();
	static FloraSettings & instance();

private:
	void setFloraOption(int optionIndex);

	typedef BW::vector<float> FloatVector;
	typedef Moo::GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;

	GraphicsSettingPtr floraSettings_;
	FloatVector floraOptions_;

private:
	FloraSettings() :
		floraSettings_(NULL),
		floraOptions_()
	{}
};

BW_END_NAMESPACE

#endif
