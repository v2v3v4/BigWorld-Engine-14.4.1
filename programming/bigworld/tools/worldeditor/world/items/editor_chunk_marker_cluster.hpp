#ifndef EDITOR_CHUNK_MARKER_CLUSTER_HPP
#define EDITOR_CHUNK_MARKER_CLUSTER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "worldeditor/world/items/editor_chunk_item_tree_node.hpp"
#include "chunk/chunk_marker_cluster.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements markers (possible spawn points for entities)
 *	Marker is an editor and server concept, so the client does not need it.
 */
class EditorChunkMarkerCluster : public EditorChunkSubstance<ChunkMarkerCluster> 
{
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkMarkerCluster )
public:
	EditorChunkMarkerCluster();
	~EditorChunkMarkerCluster();

	virtual void draw( Moo::DrawContext& drawContext );

	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );

	virtual void toss( Chunk * pChunk );

	virtual bool edSave( DataSectionPtr pSection );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual bool edEdit( class GeneralEditor & editor );
	virtual bool edCanDelete();
	virtual void edPreDelete();
	virtual void edPostClone( EditorChunkItem* srcItem );

	DECLARE_EDITOR_CHUNK_ITEM_DESCRIPTION( "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK_CLUSTER/ED_DESCRIPTION" );

	float getAvailableMarkersF() const { return (float)getAvailableMarkers(); }
	bool setAvailableMarkersF(const float& number) { return setAvailableMarkers((uint32)number); }

	int getAvailableMarkers() const { return availableMarkers_; }
	bool setAvailableMarkers(const int& number);

	BW::string numberChildrenAsString();

	void setParent(ChunkItemTreeNodePtr parent);

	virtual void onRemoveChild();

private:
	EditorChunkMarkerCluster( const EditorChunkMarkerCluster& );
	EditorChunkMarkerCluster& operator=( const EditorChunkMarkerCluster& );

	virtual const char * sectName() const { return "marker_cluster"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsGameObjects::entitiesVisible(); }
	virtual const char * drawFlag() const
		{ return "render/gameObjects/drawEntities"; }

	virtual ModelPtr reprModel() const;

	Matrix transform_;
	ModelPtr model_;
};


typedef SmartPointer<EditorChunkMarkerCluster> EditorChunkMarkerClusterPtr;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_MARKER_CLUSTER_HPP
