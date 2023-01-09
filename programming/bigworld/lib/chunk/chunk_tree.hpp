#ifndef CHUNK_TREE_HPP
#define CHUNK_TREE_HPP

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "base_chunk_tree.hpp"
#include "chunk/chunk_bsp_holder.hpp"
#include "speedtree/speedtree_config_lite.hpp"
#include <memory>

BW_BEGIN_NAMESPACE

// Forward Declarations
namespace speedtree {
class SpeedTreeRenderer;
};

namespace Moo
{
	class ModelTextureUsageGroup;
}

class ChunkBSPObstacle;

/**
 *	This class is a body of tree as a chunk item
 */
class ChunkTree : public BaseChunkTree
#if ENABLE_BSP_MODEL_RENDERING
	, public ChunkBspHolder
#endif // ENABLE_BSP_MODEL_RENDERING
{
	DECLARE_CHUNK_ITEM( ChunkTree )

public:
	ChunkTree();
	~ChunkTree();

	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );

	virtual CollisionSceneProviderPtr getCollisionSceneProvider( Chunk* ) const;
	virtual void draw( Moo::DrawContext& drawContext );
	virtual void lend( Chunk * pLender );
	virtual void toss( Chunk * pChunk );

	virtual uint32 typeFlags() const;

	virtual void setTransform( const Matrix & tranform );

	uint seed() const;
	bool seed(uint seed);

	const char * filename() const;
	bool filename(const char * filename);

	bool getReflectionVis() const;
	bool setReflectionVis(const bool& reflVis);

	bool loadFailed() const;
	const char * lastError() const;

	virtual bool reflectionVisible() { return reflectionVisible_; }
	virtual bool affectShadow() const;

	void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup );

protected:
	bool loadTree(const char * filename, int seed, Chunk * chunk);

#if ENABLE_BSP_MODEL_RENDERING
	bool drawBspInternal();
#endif // ENABLE_BSP_MODEL_RENDERING

	virtual bool addYBounds( BoundingBox& bb ) const;
	virtual void syncInit();

#if SPEEDTREE_SUPPORT
	std::auto_ptr< speedtree::SpeedTreeRenderer > tree_;
#endif

	bool        reflectionVisible_;

	/**
	 *	this structure describes the error
	 *	it contains the reason (what), the filename and the seed
	 */
	struct ErrorInfo
	{
		BW::string what;
		BW::string filename;
		int seed;
	};
	std::auto_ptr<ErrorInfo> errorInfo_;

	bool			castsShadow_;

	// Disallow copy
	ChunkTree( const ChunkTree & );
	const ChunkTree & operator = ( const ChunkTree & );
};

BW_END_NAMESPACE

#endif // CHUNK_TREE_HPP
