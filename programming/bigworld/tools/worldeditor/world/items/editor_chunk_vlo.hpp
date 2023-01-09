#ifndef EDITOR_CHUNK_VLO_HPP
#define EDITOR_CHUNK_VLO_HPP


#include "common/bwlockd_connection.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_vlo.hpp"
#include "resmgr/string_provider.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a ChunkVLO
 */
class EditorChunkVLO : public EditorChunkSubstance<ChunkVLO>
#ifndef BIGWORLD_CLIENT_ONLY
	, BWLock::BWLockDConnection::Notification
#endif//BIGWORLD_CLIENT_ONLY
{
	DECLARE_CHUNK_ITEM( EditorChunkVLO )

	virtual bool edIsVLO() const { return true; }
public:
	EditorChunkVLO( );	
	EditorChunkVLO( BW::string type );	
	~EditorChunkVLO();

	bool load( const BW::string& uid, Chunk * pChunk );
	bool load( DataSectionPtr pSection, Chunk * pChunk, BW::string* errorString = NULL );	
	
	virtual Name edClassName()
	{
		static Name name( "EditorChunkVLO" );
		return pObject_ ? pObject_->edClassName() : name;
	}

	virtual Name edDescription()
	{
		BW_GUARD;

		BW::string iDesc;
		if (pObject_)
			return LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_VLO/ED_DESCRIPTION", pObject_->edClassName().c_str() );
		return LocaliseUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_VLO/VLO_REFENRENCE" );
	}

	virtual void edHidden( bool value );
	virtual bool edHidden( ) const;

	virtual void edFrozen( bool value );
	virtual bool edFrozen( ) const;

	virtual bool edCanAddSelection() const; 
	virtual void edSelected( BW::vector<ChunkItemPtr>& selection );
	virtual bool edShouldDraw();

	virtual void updateAnimations();
	virtual void draw( Moo::DrawContext& drawContext );
	virtual void edPreDelete();
	virtual void edChunkSave();	
	virtual void edPostCreate();
	virtual void edSelectedBox( BoundingBox& bbRet ) const;
	virtual void toss( Chunk * pChunk );
	virtual void removeCollisionScene();
	virtual const Matrix & edTransform();	
	virtual bool edSave( DataSectionPtr pSection );	
	virtual void edCloneSection( Chunk* destChunk, const Matrix& destMatrixInChunk, DataSectionPtr destDS );
	virtual bool edPreChunkClone( Chunk* srcChunk, const Matrix& destChunkMatrix, DataSectionPtr chunkDS );
	virtual bool edIsPositionRelativeToChunk() {	return false;	}
	virtual bool edBelongToChunk();
	virtual void edPostClone( EditorChunkItem* srcItem );
	virtual void edPostModify();
	virtual void updateTransform( Chunk * pChunk );
	virtual bool edEdit( class GeneralEditor & editor );
	virtual bool edTransform( const Matrix & m, bool transient );
	virtual bool legacyLoad( DataSectionPtr pSection, Chunk * pChunk, BW::string& type );
#ifndef BIGWORLD_CLIENT_ONLY
	virtual void changed();
#endif//BIGWORLD_CLIENT_ONLY

	virtual BW::string edAssetName() const { return pObject_ ? pObject_->edAssetName() : "VLO"; }

	virtual bool autoAddToSceneBrowser() const { return false; }

	virtual bool edCheckMark( uint32 mark ) 
	{
		BW_GUARD;

		if (pObject_)
			return pObject_->edCheckMark(mark);
		return false;
	}

protected:
	virtual void objectCreated();

private:
	EditorChunkVLO( const EditorChunkVLO& );
	EditorChunkVLO& operator=( const EditorChunkVLO& );

	void updateWorldVars( const Matrix & m );
	void updateLocalVars( const Matrix & m, Chunk * pChunk );	

	virtual void addAsObstacle() { }	
	virtual ModelPtr reprModel() const;
	virtual const char * sectName() const { return "vlo"; }
	virtual bool isDrawFlagVisible() const
		{ return true; }
	virtual const char * drawFlag() const { return "render/scenery/drawVLO"; }
	virtual bool affectShadow() const {	return pOwnSect()->readString("type") != "water"; }

	BW::string		uid_;
	BW::string		type_;
	Vector3			localPos_;
	bool			colliding_;
	Matrix			transform_;
	Matrix			vloTransform_;	
	bool			drawTransient_;
	bool			readonly_;
	mutable bool	highlight_;
	ModelPtr		waterModel_;
};


typedef SmartPointer<EditorChunkVLO> EditorChunkVLOPtr;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_VLO_HPP
