#ifndef EDITOR_MODEL_CHUNK_VLO_REF_HPP
#define EDITOR_MODEL_CHUNK_VLO_REF_HPP

#include "chunk/chunk_model_vlo_ref.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkModel;
class EditorChunkModelVLORef;

typedef EditorChunkSubstance< ChunkModelVLORef > EditorChunkModelVLOParent;
typedef SmartPointer< EditorChunkModel > EditorChunkModelPtr;
typedef SmartPointer< EditorChunkModelVLORef > EditorChunkModelVLORefPtr;

class EditorChunkModelVLORef
	: public EditorChunkModelVLOParent
{
public:
	DECLARE_CHUNK_ITEM( EditorChunkModelVLORef )

	EditorChunkModelVLORef();
	virtual ~EditorChunkModelVLORef();

	static DataSectionPtr createDataSection( DataSectionPtr pModelSection );
	static void startCreation() { s_creating = true; }
	static void endCreation() { s_creating = false; }

	virtual bool edIsVLO() const { return true; }

	virtual Name edClassName()
	{
		static Name name( "EditorChunkModelVLORef" );
		return pObject_ ? pObject_->edClassName() : name;
	}

	virtual const char * sectName() const;
	virtual bool isDrawFlagVisible() const;
	virtual const char * drawFlag() const;
	virtual ModelPtr reprModel() const;
	virtual void edPreDelete();
	virtual void toss( Chunk * pChunk );
	
	virtual DataSectionPtr pOwnSect();

	bool edEdit( class GeneralEditor & editor );
	virtual void edBounds( BoundingBox & bbRet ) const;
	virtual void edWorldBounds( BoundingBox & bbRet );
	virtual void edPostCreate();
	virtual const Matrix & edTransform();
	virtual bool edTransform( const Matrix & m, bool transient = false );
	virtual bool edShouldDraw();
	virtual bool edSave( DataSectionPtr pSection );

	bool load( const BW::string & uid, Chunk * pChunk );
	bool load( DataSectionPtr pSection, Chunk * pChunk,
		BW::string * errorString = NULL );	

	virtual void draw( Moo::DrawContext& drawContext );

	ChunkItemPtr convertToChunkModel( const Matrix & world );
	static ChunkItemPtr convertToChunkModelVLO(
		ChunkItemPtr chunkModel, const Matrix & world );

	struct ProxyChunkCreationHelper
	{
		ProxyChunkCreationHelper()
		{
			EditorChunkModelVLORef::startCreation();
		}

		~ProxyChunkCreationHelper()
		{
			EditorChunkModelVLORef::endCreation();
		}
	};
private:
	void updateWorldVars( const Matrix & m );

	bool			drawTransient_;
	BW::string		uid_;
	bool			readonly_;
	mutable bool	highlight_;
	static bool		s_creating;
};

BW_END_NAMESPACE

#endif // EDITOR_MODEL_CHUNK_VLO_REF_HPP
