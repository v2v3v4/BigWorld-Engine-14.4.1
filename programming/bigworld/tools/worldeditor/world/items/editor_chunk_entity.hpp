#ifndef EDITOR_CHUNK_ENTITY_HPP
#define EDITOR_CHUNK_ENTITY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_item.hpp"
#include "entitydef/entity_description_map.hpp"
#include "model/super_model.hpp"
#include "gizmo/general_editor.hpp"
#include "common/properties_helper.hpp"
#include "common/editor_chunk_entity_base.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/unique_id.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of an entity in a chunk.
 *
 *	Note that it does not derive from the client's version of a chunk entity
 *	(because the editor does not have the machinery necessary to implement it)
 */
class EditorChunkEntity : public EditorChunkSubstance<EditorChunkEntityBase>
{
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkEntity )

public:
	// Constructor / destructor
	EditorChunkEntity();
	~EditorChunkEntity();

	// Chunk item methods
	virtual void	edMainThreadLoad();
	virtual bool	edTransform( const Matrix & m, bool transient );
	bool			edLoad(	const BW::string type, DataSectionPtr pSection,
							BW::string* errorString = NULL );
	bool			edLoad( DataSectionPtr pSection, bool loadTransform = true );
	virtual bool	edSave( DataSectionPtr pSection );
	virtual Name	edDescription();
	virtual bool	edEdit( class GeneralEditor & editor );
	bool			edEditProperties( class GeneralEditor & editor, MatrixProxy * pMP );
	void			edPreDelete();
	virtual bool	edCanDelete();
	virtual BW::vector<BW::string> edCommand( const BW::string& path ) const;
	virtual bool	edExecuteCommand( const BW::string& path, BW::vector<BW::string>::size_type index );
	virtual bool	edShouldDraw();

	virtual Moo::LightContainerPtr edVisualiseLightContainer();

	virtual BW::string edAssetName() const { return pOwnSect_->readString( "type" ); }
	virtual BW::string edFilePath() const;

	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );
    virtual void toss( Chunk * pChunk );
	virtual void draw( Moo::DrawContext& drawContext );
    virtual void tick(float dtime);

	GeneralProperty* parseProperty(
		int i, PyObject* pyEnum, DataDescription* pDD, MatrixProxy * pMP );

	bool		clientOnlyGet() const			{ return clientOnly_; }
	bool		clientOnlySet( const bool & b );
	BW::string	typeGet() const;
	bool		typeSet( const BW::string & s )	{ return false; }
	BW::string idGet() const;
	bool		idSet( const BW::string & s )	{ return false; }
	BW::string lseGet() const				{ return lastScriptError_; }
	bool		lseSet( const BW::string & s )	{ return false; }
	ModelPtr	getReprModel() const { return reprModel(); }
	const EntityDescription*
				getTypeDesc() { return pType_; }
	void		setSurrogateMarker( DataSectionPtr pMarkerSect )
		{ surrogateMarker_ = true; pOwnSect_ = pMarkerSect; }

	typedef std::pair< PyObject *, BW::string > BindingProperty;
	typedef BW::vector< BindingProperty > BindingProperties;
	const BindingProperties & getBindingProps() { return bindingProps_; }

	virtual bool isEditorEntity() const { return true; }

	// Tell substance not to add us automatically, we'll do it ourselves.
	virtual bool autoAddToSceneBrowser() const { return false; }

	// Patrol methods
	void patrolListNode(BW::string const &id);
    BW::string patrolListNode() const;
    BW::string patrolListGraphId() const;
    int patrolListPropIdx() const;
	bool patrolListRelink( const BW::string& newGraph, const BW::string& newNode );
    void disconnectFromPatrolList();

	/** Set the correct model for all entities that have changed recently */
	static void calculateDirtyModels();
	bool isDefaultModel() const;

	UniqueID guid() const{ return guid_; };
	void guid( UniqueID newGUID );

	PyObject* infoDict() const;
	bool canLinkTo( const BW::string& propName, PyObject* otherInfo ) const;

	// Linker accessor
	EditorChunkItemLinkable* chunkItemLinker() const { return pChunkItemLinker_; }
	
	// Properties methods
	void clearProperties();
	void clearEditProps();
	void setEditProps( const BW::list< BW::string > & names );
	void clearPropertySection();
	PropertiesHelper* propHelper() { return &propHelper_; }
	bool firstLinkFound() const { return firstLinkFound_; }
	void firstLinkFound( bool value ) { firstLinkFound_ = value; }
	virtual void syncInit();

	using EditorChunkEntityBase::edTransform;
private:
	EditorChunkEntity( const EditorChunkEntity& );
	EditorChunkEntity& operator=( const EditorChunkEntity& );

	void propertyChangedCallback( int index );

	void recordBindingProps();
	void removeFromDirtyList();
	void markModelDirty();
	void calculateModel();

	virtual const char * sectName() const { return "entity"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsGameObjects::entitiesVisible(); }
	virtual const char * drawFlag() const
		{ return "render/gameObjects/drawEntities"; }
	virtual ModelPtr reprModel() const;

	EditorChunkStationNode* connectedNode();

	BW::string nodeId() const                   { return patrolListNode(); }
	bool nodeId( const BW::string & /*s*/ )     { return false; }
	BW::string graphId() const                  { return patrolListGraphId(); }
	bool graphId( const BW::string & /*s*/ )    { return false; }
	bool initType( const BW::string type, BW::string* errorString );

	static void loadModelTask( void* self );
	static void loadModelTaskDone( void* self );

	bool loading() const;
	void waitDoneLoading();

	bool									clientOnly_;
	DataSectionPtr							pOriginalSect_;
	bool									surrogateMarker_;
    BW::string								patrolListNode_;

	UniqueID								guid_;
	const EntityDescription *				pType_;
	bool									transformLoaded_;
	BW::string								lastScriptError_;
	PyObject *								pDict_;
	BW::vector<bool>						allowEdit_;
	ModelPtr								model_;
	PyObject *								pyClass_;
    EditorChunkLinkPtr						link_;
	BindingProperties						bindingProps_;

	// Linker object
	EditorChunkItemLinkable*					pChunkItemLinker_;
	
	PropertiesHelper						propHelper_;
	bool									firstLinkFound_;

	BackgroundTaskPtr						loadBackgroundTask_;
	BW::string								modelToLoad_;
	ModelPtr								loadingModel_;

	static SimpleMutex						s_dirtyModelMutex_;
	static SimpleMutex						s_loadingModelMutex_;
	static BW::vector<EditorChunkEntity*>	s_dirtyModelEntities_;
};

typedef SmartPointer<EditorChunkEntity> EditorChunkEntityPtr;



// -----------------------------------------------------------------------------
// Section: EditorEntityType
// -----------------------------------------------------------------------------


class EditorEntityType
{
public:
	static void startup();
	static void shutdown();

	EditorEntityType();

	const EntityDescription * get( const BW::string & name );
	PyObject * getPyClass( const BW::string & name );

	static EditorEntityType & instance();
	bool isEntity( const BW::string& name ) const	{	return eMap_.isEntity( name );	}
private:
	EntityDescriptionMap	eMap_;
	BW::map<BW::string,EntityDescription*> mMap_;

	BW::map<BW::string, PyObject *> pyClasses_;

	static EditorEntityType * s_instance_;
};


BW_END_NAMESPACE

#endif // EDITOR_CHUNK_ENTITY_HPP
