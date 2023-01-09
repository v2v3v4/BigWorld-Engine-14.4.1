#ifndef EDITOR_CHUNK_MODEL_HPP
#define EDITOR_CHUNK_MODEL_HPP

#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

#include "common/editor_chunk_model_base.hpp"

#include "moo/visual.hpp"

#include "cstdmf/bw_string.hpp"

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_map.hpp"

BW_BEGIN_NAMESPACE

class EditorEffectProperty;
struct FenceInfo
{
	uint fenceId;
	uint sectionId;
};

/**
 *	This class is the editor version of a ChunkModel
 */
class EditorChunkModel : public EditorChunkModelBase
{
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkModel )
	DECLARE_CHUNK_ITEM_ALIAS( EditorChunkModel, shell )


public:
	virtual InvalidateFlags edInvalidateFlags();

	void clean();

	EditorChunkModel();
	~EditorChunkModel();

	virtual bool edShouldDraw() const;
	virtual void tick( float dTime );
	virtual void updateAnimations();
	virtual void draw( Moo::DrawContext& drawContext );

	bool load( DataSectionPtr pSection, Chunk * pChunk = NULL,
		BW::string * pErrorString = NULL );
	void loadModels( Chunk* chunk );
	/** Called once after loading from the main thread */
	void edPostLoad();

	virtual void toss( Chunk * pChunk );
	virtual void syncInit();

	virtual bool edSave( DataSectionPtr pSection );
	virtual void edChunkSave();

	virtual const Matrix & edTransform() { return transform_; }
	virtual bool edTransform( const Matrix & m, bool transient );
	virtual void edBounds( BoundingBox& bbRet ) const;

	bool edEdit(
		GeneralEditor & editor, ChunkItemPtr chunkItem );
	virtual bool edEdit( class GeneralEditor & editor );
	virtual Chunk * edDropChunk( const Vector3 & lpos );

	virtual Name edDescription();

	virtual int edNumTriangles() const;

	virtual int edNumPrimitives() const;

	virtual BW::string edAssetName() const;
	virtual BW::string edFilePath() const;

	virtual BW::vector<BW::string> edCommand( const BW::string& path ) const;
	virtual bool edExecuteCommand( const BW::string& path, BW::vector<BW::string>::size_type index );

	virtual const char* sectionName();

	/**
	 * Make sure we've got our own unique lighting data after being cloned
	 */
	void edPostClone( EditorChunkItem* srcItem );

	/** Ensure lighting on the chunk is marked as dirty */
	void edPostCreate();


	/** If we've got a .lighting file, delete it */
	void edPreDelete();

	Vector3 edMovementDeltaSnaps();
	float edAngleSnaps();

	BW::string getModelName();

	BW::string getAnimation() const { return animName_; }
	bool setAnimation( const BW::string & newAnimationName );

	BW::string getDyeTints( const BW::string& dye ) const;
	bool setDyeTints( const BW::string& dye, const BW::string& tint );

	float getAnimRateMultiplier() const { return animRateMultiplier_; }
	bool setAnimRateMultiplier( const float& f );

	bool getOutsideOnly() const { return outsideOnly_; }
	bool setOutsideOnly( const bool& outsideOnly );

	bool getCastsShadow() const { return castsShadow_; }
	bool setCastsShadow( const bool& castsShadow );

	virtual Moo::LightContainerPtr edVisualiseLightContainer();

	virtual void onReloaderReloaded( Reloader* pReloader );
	virtual void onReloaderPreReload( Reloader* pReloader);

	struct BigModelLoader
	{
		BigModelLoader()
		{
			EditorChunkModel::startBigModelLoad();
		}

		~BigModelLoader()
		{
			EditorChunkModel::endBigModelLoad();
		}
	};

private:
	EditorChunkModel( const EditorChunkModel& );
	EditorChunkModel& operator=( const EditorChunkModel& );

	bool			resourceIsOutsideOnly() const;
	void			pullInfoFromSuperModel();

	bool isDrawingModels() const;

	bool								hasPostLoaded_;
	BW::string							animName_;
	BW::map<BW::string,BW::string>	tintName_;
	BW::map<BW::string,SuperModelDyePtr> tintMap_;
	
	StringHashMap<int>	  				collisionFlags_;
	BW::vector<BW::string>			collisionFlagNames_;
	static StringHashMap<int>			s_materialKinds_;

	uint								primGroupCount_;
	bool								customBsp_;		// if the user specified their own bsp
	bool								outsideOnly_;

	static void startBigModelLoad();
	static void endBigModelLoad();
public:
	FenceInfo *getFenceInfo(bool createIfNotExist, int fenceId = -1, int sectionId = -1);
	void createFenceInfo();
	void deleteFenceInfo( bool keepValues = false );
	
	EditorChunkModel *getPrevFenceSection();
	virtual void edSelected( BW::vector<ChunkItemPtr>& selection );
	static bool s_autoSelectFenceSections;

	// key: fenceId, value: list of models ordered by sectionId
	static BW::map<uint, BW::vector<EditorChunkModel*> > s_fences;

private:
	FenceInfo *pFenceInfo_;
	bool isInScene_;
	static bool s_isAutoSelectingFence;

	class SuperModel*					pEditorModel_;

	// for edDescription
	Name desc_;

	// is a model is not found, the standin is loaded
	bool standinModel_;
	DataSectionPtr originalSect_;

	// animations available to be played in this model
	BW::vector<BW::string> animationNames_;
	BW::vector<BW::wstring> animationNamesW_;

	// dye and tint available to be played in this model
	// map dye name to tints names vector
	BW::map<BW::string, BW::vector<BW::string> > dyeTints_;
	BW::map<BW::string, BW::vector<BW::wstring> > dyeTintsW_;

	bool lastDrawEditorProxy_;
};


typedef SmartPointer<EditorChunkModel> EditorChunkModelPtr;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_MODEL_HPP
