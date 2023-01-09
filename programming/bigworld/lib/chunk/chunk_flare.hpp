#ifndef CHUNK_FLARE_HPP
#define CHUNK_FLARE_HPP

#include "chunk_item.hpp"
#include "umbra_config.hpp"
#include "romp/lens_effect.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a chunk item that tends a lens flare
 */
class ChunkFlare : public ChunkItem
{
	DECLARE_CHUNK_ITEM( ChunkFlare )
public:
	ChunkFlare();
	~ChunkFlare();

	bool load( DataSectionPtr pSection )	{ MF_ASSERT(0); return false; }
	bool load( DataSectionPtr pSection, Chunk * pChunk );

	virtual void draw( Moo::DrawContext& drawContext );

	static void ignore( bool state );

protected:
	Vector3				position_;
	Vector3				colour_;
	bool				colourApplied_;
	bool				type2_;
	virtual void syncInit();
	LensEffects			lensEffects_;

private:

	static bool s_ignore;
};

BW_END_NAMESPACE

#endif // CHUNK_FLARE_HPP
