#ifndef TERRAIN_FUNCTOR_HPP
#define TERRAIN_FUNCTOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/terrain/mouse_drag_handler.hpp"
#include "gizmo/tool_functor.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This is a base class for ToolFunctors that operate on terrain (texture
 *	layers, heights or holes).
 */
class TerrainFunctor : public ToolFunctor
{
public:
	explicit TerrainFunctor( PyTypeObject * pType = &s_type_ );

	virtual void stopApplying( Tool & tool, bool saveChanges );
	virtual bool handleKeyEvent( const KeyEvent & keyEvent, Tool & tool );
	virtual bool handleMouseEvent( const MouseEvent & keyEvent, Tool & tool );
	virtual bool handleContextMenu( Tool & tool );
	virtual bool changeCollisionScene(){ return true;}

protected:
	virtual void beginApply();
	virtual void doApply( Tool & tool, bool allVerts = false );
	virtual void stopApplyCommitChanges( Tool& tool, bool addUndoBarrier );
	virtual void stopApplyDiscardChanges( Tool& tool );

	virtual void getBlockFormat( 
		const EditorChunkTerrain &		chunkTerrain,
		TerrainUtils::TerrainFormat	&	format ) const = 0;

	virtual void onFirstApply( EditorChunkTerrain & chunkTerrain ) = 0;

	virtual void onBeginApply( EditorChunkTerrain & chunkTerrain );

	virtual void applyToSubBlock(
		EditorChunkTerrain &				chunkTerrain,
		const Vector3 &						toolEffset, 
		const Vector3 &						chunkOffset,
		const TerrainUtils::TerrainFormat &	format,
		int32								minx, 
		int32								minz, 
		int32								maxx, 
		int32								maxz ) = 0;

	virtual void onEndApply( EditorChunkTerrain & chunkTerrain );

	virtual void onApplied( Tool & t ) = 0;

	virtual void onLastApply( EditorChunkTerrain & chunkTerrain ) = 0;	

	virtual bool showWaitCursorOnLastApply() const;

	bool newTouchedChunk( EditorChunkTerrain * pChunkTerrain );

	MouseDragHandler & dragHandler();

private:
	void endApplying();	

	typedef BW::set< EditorChunkTerrain* > ECTSet;

	ECTSet				touchedChunks_;
	MouseDragHandler	dragHandler_;
};

BW_END_NAMESPACE
#endif // TERRAIN_FUNCTOR_HPP
