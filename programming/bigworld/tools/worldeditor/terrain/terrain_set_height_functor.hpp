#ifndef TERRAIN_SET_HEIGHT_FUNCTOR_HPP
#define TERRAIN_SET_HEIGHT_FUNCTOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/terrain/terrain_functor.hpp"
#include "moo/image.hpp"

BW_BEGIN_NAMESPACE

/*~ class NoModule.TerrainSetHeightFunctor
 *	@components{ worldeditor }
 *	This class sets the height to a constant value.
 */
class TerrainSetHeightFunctor : public TerrainFunctor
{
	Py_Header( TerrainSetHeightFunctor, TerrainFunctor )

public:
	explicit TerrainSetHeightFunctor( PyTypeObject * pType = &s_type_ );

	void	update( float dTime, Tool & t );
	void	onBeginUsing( Tool & tool );
	void	onEndUsing( Tool & tool );
	void	getBlockFormat( EditorChunkTerrain const & chunkTerrain,
				TerrainUtils::TerrainFormat& format ) const;
	void	onFirstApply( EditorChunkTerrain & chunkTerrain );	
	void	applyToSubBlock( EditorChunkTerrain&,
				const Vector3 & toolOffset, const Vector3 & chunkOffset,
				const TerrainUtils::TerrainFormat & format,
				int32 minx, int32 minz, int32 maxx, int32 maxz );
	void	onApplied( Tool & t );
	void	onLastApply( EditorChunkTerrain & chunkTerrain );

	PY_RW_ATTRIBUTE_DECLARE( relative_, relative );
	PY_RW_ATTRIBUTE_DECLARE( height_, height );

	PY_FACTORY_DECLARE()

protected:
	void doApply( Tool& tool, bool allVerts = false );
	void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );

private:
	typedef Moo::Image< uint8 >				VisitedImage;
	typedef SmartPointer< VisitedImage >	VisitedImagePtr;
	typedef BW::map<Terrain::EditorBaseTerrainBlock *, VisitedImagePtr >	VisitedMap;

	float		height_;
	int			relative_;
	VisitedMap	poles_;

	FUNCTOR_FACTORY_DECLARE( TerrainSetHeightFunctor() )
};

BW_END_NAMESPACE

#endif // TERRAIN_SET_HEIGHT_FUNCTOR_HPP
