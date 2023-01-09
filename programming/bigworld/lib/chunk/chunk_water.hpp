#ifndef CHUNK_WATER_HPP
#define CHUNK_WATER_HPP

#include "math/vector2.hpp"
#include "math/vector3.hpp"

#include "chunk_item.hpp"
#include "chunk_vlo.hpp"

#include "romp/water.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is a body of water as a chunk item
 */
class ChunkWater : public VeryLargeObject
{
public:
	ChunkWater( );
	ChunkWater( BW::string uid );
	~ChunkWater();

	bool load( DataSectionPtr pSection, Chunk * pChunk );

	virtual void updateAnimations() {}
	virtual void draw( Moo::DrawContext& drawContext ) {}
	virtual void drawInChunk( Moo::DrawContext& drawContext, Chunk* pChunk );
	virtual void sway( const Vector3 & src, const Vector3 & dst, const float diameter );
	static void simpleDraw( bool state );

#ifdef EDITOR_ENABLED
	virtual void dirty();
	virtual void edCommonChanged() {}
#endif //EDITOR_ENABLED

	virtual BoundingBox chunkBB( Chunk* pChunk );

	virtual void syncInit(ChunkVLO* pVLO);

	Chunk * chunk() const;
	void chunk( Chunk * pChunk );

protected:
	Water *				pWater_;
	Chunk*	pChunk_;

	Water::WaterState		state_;
	Water::WaterResources	resources_;

private:

	static bool create(
		Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid );
	static VLOFactory	factory_;
	static bool s_simpleDraw;
	virtual bool addYBounds( BoundingBox& bb ) const;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "chunk_water.ipp"
#endif

#endif // CHUNK_WATER_HPP
