#ifndef EDITOR_CHUNK_USER_DATA_OBJECT_HPP
#define EDITOR_CHUNK_USER_DATA_OBJECT_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_item.hpp"
#include "entitydef/user_data_object_description_map.hpp"
#include "common/properties_helper.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "gizmo/general_editor.hpp"
#include "model/super_model.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/unique_id.hpp"
#include "cstdmf/concurrency.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a user data object in a chunk.
 */
class EditorChunkUserDataObject : public EditorChunkSubstance<ChunkItem>
{
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkUserDataObject )

public:
	// Constructor / destructor
	EditorChunkUserDataObject();
	~EditorChunkUserDataObject();

	virtual InvalidateFlags edInvalidateFlags();

	bool edLoad( const BW::string type, DataSectionPtr pSection, BW::string* errorString = NULL );
	bool edLoad( DataSectionPtr pSection, bool loadTransform = true );
	virtual const Matrix & edTransform()	{ return transform_; }
	virtual bool edTransform( const Matrix & m, bool transient );
	virtual bool edSave( DataSectionPtr pSection );
	virtual bool edIsSnappable() { return false; }
	virtual void edMainThreadLoad();
	virtual Name edDescription();
	virtual bool edEdit( class GeneralEditor & editor );
	virtual bool edCanDelete();
	virtual void edPreDelete();
	virtual BW::vector<BW::string> edCommand( const BW::string& path ) const;
	virtual bool edExecuteCommand( const BW::string& path, BW::vector<BW::string>::size_type index );
	virtual bool edShouldDraw();
	bool edEditProperties( class GeneralEditor & editor, MatrixProxy * pMP );
	Vector3 edMovementDeltaSnaps();
	float edAngleSnaps();
	virtual void edPostClone( EditorChunkItem* srcItem );

	virtual BW::string edAssetName() const { return pOwnSect_->readString( "type" ); }
	virtual BW::string edFilePath() const;

	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );
    virtual void toss( Chunk * pChunk );
	virtual void draw( Moo::DrawContext& drawContext );
    virtual void tick(float dtime);

	BW::string typeGet() const;
	bool typeSet( const BW::string & s )	{ return false; }
	BW::string domainGet() const;
	bool domainSet( const BW::string & s )	{ return false; }
	BW::string idGet() const;
	bool idSet( const BW::string & s )	{ return false; }
	BW::string lseGet() const				{ return lastScriptError_; }
	bool lseSet( const BW::string & s )	{ return false; }

	// Tell substance not to add us automatically, we'll do it ourselves.
	virtual bool autoAddToSceneBrowser() const { return false; }

	// Properties methods
	void clearProperties();
	void clearEditProps();
	void setEditProps( const BW::list< BW::string > & names );
	void clearPropertySection();
	PropertiesHelper* propHelper() { return &propHelper_; }
	bool firstLinkFound() const { return firstLinkFound_; }
	void firstLinkFound( bool value ) { firstLinkFound_ = value; }

	// Linker accessor
	EditorChunkItemLinkable* chunkItemLinker() const { return pChunkItemLinker_; }
	
	ModelPtr getReprModel() const { return reprModel(); }
	bool isDefaultModel() const;
	const UserDataObjectDescription * getTypeDesc() { return pType_; }

	typedef std::pair< PyObject *, BW::string > BindingProperty;
	typedef BW::vector< BindingProperty > BindingProperties;
	const BindingProperties & getBindingProps() { return bindingProps_; }

    virtual bool isEditorUserDataObject() const { return true; };
	UniqueID guid() const{ return guid_; };
	void guid( UniqueID newGUID );

	static void calculateDirtyModels();

	PyObject* infoDict() const;
	bool canLinkTo( const BW::string& propName, const EditorChunkUserDataObject* other ) const;
	bool showAddGizmo( const BW::string& propName ) const;
	void onDelete() const;
	void getLinkCommands( BW::vector<BW::string>& commands, const EditorChunkUserDataObject* other ) const;
	void executeLinkCommand( int cmdIndex, const EditorChunkUserDataObject* other ) const;
	
	virtual void syncInit();
	
private:
	EditorChunkUserDataObject( const EditorChunkUserDataObject& );
	EditorChunkUserDataObject& operator=( const EditorChunkUserDataObject& );

	virtual const char * sectName() const { return "UserDataObject"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsGameObjects::udosVisible(); }
	virtual const char * drawFlag() const
		{ return "render/gameObjects/drawUserDataObjects"; }
	virtual ModelPtr reprModel() const;

	void propertyChangedCallback( int index );

	void recordBindingProps();
	EditorChunkStationNode* connectedNode();

	void markModelDirty();
	void removeFromDirtyList();
	void calculateModel();

	bool initType( const BW::string type, BW::string* errorString );

	static void loadModelTask( void* self );
	static void loadModelTaskDone( void* self );

	// Linker object
	EditorChunkItemLinkable*				pChunkItemLinker_;

	bool loading() const;
	void waitDoneLoading();

	UniqueID							guid_;
	const UserDataObjectDescription*	pType_;
	Matrix								transform_;
	bool								transformLoaded_;
	BW::string							lastScriptError_;
	PyObject *							pDict_;
	BW::vector<bool>					allowEdit_;
	ModelPtr							model_;
	PyObject *							pyClass_;
	EditorChunkLinkPtr					link_;
	BindingProperties					bindingProps_;

	PropertiesHelper					propHelper_;
	bool								firstLinkFound_;

	BackgroundTaskPtr					loadBackgroundTask_;
	BW::string							modelToLoad_;
	ModelPtr							loadingModel_;

	static SimpleMutex								s_dirtyModelMutex_;
	static SimpleMutex								s_loadingModelMutex_;
	static BW::vector<EditorChunkUserDataObject*>	s_dirtyModelEntities_;
};


typedef SmartPointer<EditorChunkUserDataObject> EditorChunkUserDataObjectPtr;



// -----------------------------------------------------------------------------
// Section: EditorUserDataObjectType
// -----------------------------------------------------------------------------


class EditorUserDataObjectType
{
public:
	static void startup();
	static void shutdown();

	EditorUserDataObjectType();

	const UserDataObjectDescription * get( const BW::string & name );
	PyObject * getPyClass( const BW::string & name );

	static EditorUserDataObjectType & instance();
	bool isUserDataObject( const BW::string& name ) const	{	return cMap_.isUserDataObject( name );	}
private:
	UserDataObjectDescriptionMap	cMap_;
	BW::map<BW::string, PyObject *> pyClasses_;

	static EditorUserDataObjectType * s_instance_;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_USER_DATA_OBJECT_HPP
