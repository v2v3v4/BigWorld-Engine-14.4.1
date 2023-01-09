#ifndef TERRAIN_HOLE_FUNCTOR_HPP
#define TERRAIN_HOLE_FUNCTOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/terrain/terrain_functor.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class either cuts out or fills in a height poles' hole.
 */
class TerrainHoleFunctor : public TerrainFunctor
{
	Py_Header( TerrainHoleFunctor, TerrainFunctor )

public:
	explicit TerrainHoleFunctor( PyTypeObject * pType = &s_type_ );

	void	update( float dTime, Tool& t );
	void	onBeginUsing( Tool & tool );
	void	onEndUsing( Tool & tool );
	void	getBlockFormat( const EditorChunkTerrain & chunkTerrain,
				TerrainUtils::TerrainFormat & format ) const;
	void	onFirstApply( EditorChunkTerrain & chunkTerrain );
	void	applyToSubBlock( EditorChunkTerrain &,
				const Vector3 & toolEffset, const Vector3 & chunkOffset,
				const TerrainUtils::TerrainFormat & format,
				int32 minx, int32 minz, int32 maxx, int32 maxz );
	void	onApplied( Tool & t );
	void	onLastApply( EditorChunkTerrain& chunkTerrain );

	PY_RW_ATTRIBUTE_DECLARE( fillNotCut_, fillNotCut )

	PY_FACTORY_DECLARE()

protected:
	void doApply( float dTime, Tool & tool );
	void stopApplyCommitChanges( Tool & tool, bool addUndoBarrier );

private:
	float	falloff_;
	bool	fillNotCut_;

	FUNCTOR_FACTORY_DECLARE( TerrainHoleFunctor() )
};

BW_END_NAMESPACE

#endif // TERRAIN_HOLE_FUNCTOR_HPP
