#ifndef EDITOR_CHUNK_MARKER_HPP
#define EDITOR_CHUNK_MARKER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_marker.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class implements markers (possible spawn points for entities)
 *	Marker is an editor and server concept, so the client does not need it.
 */
class EditorChunkMarker : public EditorChunkSubstance<ChunkMarker>
{
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkMarker )
public:
	EditorChunkMarker();
	~EditorChunkMarker();

	virtual void updateAnimations();
	virtual void draw( Moo::DrawContext& drawContext );

	bool load( DataSectionPtr pSection, BW::string* errorString = NULL );
	void edMainThreadLoad();

	virtual void toss( Chunk * pChunk );

	virtual bool edSave( DataSectionPtr pSection );

	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient );

	virtual bool edEdit( class GeneralEditor & editor );
	virtual bool edCanDelete();
	virtual void edPreDelete();
	virtual void edPostClone( EditorChunkItem* srcItem );

	DECLARE_EDITOR_CHUNK_ITEM_DESCRIPTION( "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK/ED_DESCRIPTION" );

	BW::string getCategory() const { return category_; }
	bool setCategory(const BW::string & newCategory);

	virtual void setParent(ChunkItemTreeNodePtr parent);
	
	virtual void syncInit();
	
private:
	EditorChunkMarker( const EditorChunkMarker& );
	EditorChunkMarker& operator=( const EditorChunkMarker& );

	virtual const char * sectName() const { return "marker"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsGameObjects::entitiesVisible(); }
	virtual const char * drawFlag() const
		{ return "render/gameObjects/drawEntities"; }

	virtual ModelPtr reprModel() const;

	bool loadEntityDescription();
	void findCommonProperties();

	bool loadEntityDescription_;

	void syncReprModel(bool createNewModelOnly = false);

	EditorChunkEntityPtr edEntity_;
    class SuperModel * superModel_;
	SuperModelAnimationPtr anim_;

	enum MType
	{
		MType_Unknown,
		MType_Entity,
		MType_Marker,
		MType_MarkerWithoutEntity
	};
	MType mtype_;
};


typedef SmartPointer<EditorChunkMarker> EditorChunkMarkerPtr;

BW_END_NAMESPACE


#endif // EDITOR_CHUNK_MARKER_HPP
