#ifndef CHUNK_VLO_HPP
#define CHUNK_VLO_HPP

#include "math/vector2.hpp"
#include "math/vector3.hpp"

#include "chunk_item.hpp"
#include "editor_chunk_common.hpp"

#ifdef EDITOR_ENABLED

#include "gizmo/meta_data.hpp"
#include "gizmo/meta_data_helper.hpp"

#endif//EDITOR_ENABLED


BW_BEGIN_NAMESPACE

/**
 *
 */
//TODO: Combine VLOFactory and ChunkItemFactory
/**
 *	This class is a factory for VLO items, used by the actual VLO (not the references).
 *  e.g. ChunkWater
 */
class VLOFactory
{
public:
	typedef bool (*Creator)( Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid );

	VLOFactory( const BW::string & section,
		int priority = 0,
		Creator creator = NULL );

	virtual bool create( Chunk * pChunk, DataSectionPtr pSection, BW::string uid ) const;

	int priority() const							{ return priority_; }

private:
	int		priority_;
	Creator creator_;
};

class ChunkVLO;

/**
 * The actual large object, created when a reference is encountered.
 */
class VeryLargeObject;
typedef SmartPointer<VeryLargeObject> VeryLargeObjectPtr;
class VeryLargeObject : public SafeReferenceCount, public EditorChunkCommonLoadSave
{
public:
	typedef StringHashMap<VeryLargeObjectPtr> UniqueObjectList;
	typedef BW::list<ChunkVLO*> ChunkItemList;

	VeryLargeObject();
	VeryLargeObject( BW::string uid, BW::string type );
	~VeryLargeObject();

	void setUID( BW::string uid );

//TODO: would be much nicer to have an editor class rather than these ifdef's
#ifdef EDITOR_ENABLED	
	virtual void cleanup() {}
	virtual void saveFile(Chunk* pChunk=NULL) {}
	virtual void save( DataSectionPtr pDataSection );
	virtual void save();
	virtual void drawRed(bool val) {}
	virtual void highlight(bool val) {}
	virtual void edDelete( ChunkVLO* instigator );	
	DECLARE_EDITOR_CHUNK_ITEM_CLASS_NAME( "VLO" );
	virtual const Matrix & edTransform() { return Matrix::identity; }
	virtual bool edEdit( class GeneralEditor & editor, const ChunkItemPtr pItem ) { return false; }
	virtual bool edShouldDraw();
	virtual BW::string type() { return type_; }

	virtual bool isObjectCreated() const;

	MetaData::MetaData& metaData() { return metaData_; }

	ChunkItemList chunkItems() const;

	virtual bool visibleInside() const { return true; }

	virtual bool visibleOutside() const { return true; }

	void lastDbItem( ChunkItem * item ) { lastDbItem_ = item; }
	ChunkItem * lastDbItem() const { return lastDbItem_; }

	virtual int numTriangles() const { return 0; }

	virtual int numPrimitives() const { return 0; }

	virtual BW::string edAssetName() const { return "VLO"; }

	static BW::string generateUID();

	virtual bool edCheckMark(uint32 mark)
	{
		if (mark == selectionMark_)
			return false;
		else
		{
			selectionMark_ = mark;
			return true;
		}
	}

	static void deleteUnused();
	static void saveAll();

#endif //EDITOR_ENABLED

	virtual void objectCreated();

	bool shouldRebuild() { return rebuild_; }
	void shouldRebuild( bool rebuild ) { rebuild_ = rebuild; }

	virtual void dirty() {}
	virtual void drawInChunk( Moo::DrawContext& drawContext, Chunk* pChunk ) = 0;
	virtual void lend( Chunk * pChunk ) {}
	virtual void unlend( Chunk * pChunk ) {}
	virtual void updateLocalVars( const Matrix & m ) {}
	virtual void updateWorldVars( const Matrix & m ) {}	
	virtual const Matrix & origin() { return Matrix::identity; }
	virtual const Matrix & localTransform( ) { return Matrix::identity; }	
	virtual const Matrix & localTransform( Chunk * pChunk ) { return Matrix::identity; }
	virtual void sway( const Vector3 & src, const Vector3 & dst, const float diameter ) {}
	virtual void updateAnimations() {};
	virtual void tick( float dTime ){};
	virtual void addCollision( ChunkItemPtr item ) {}
	virtual BoundingBox chunkBB( Chunk* pChunk ) { return BoundingBox::s_insideOut_; };

	BW::string getUID() const { return uid_; }
	// TODO: a dangerous function this is.............fix it I will

	void addItem( ChunkVLO* item );
	void removeItem( ChunkVLO* item, bool destroy = false );
	BoundingBox& boundingBox() { return bb_; }	
	ChunkVLO* containsChunk( const Chunk * pChunk ) const;
#ifdef EDITOR_ENABLED
	void section( const DataSectionPtr pVLOSection )
	{ 
		dataSection_ = pVLOSection;
	}	
	DataSectionPtr section() { return dataSection_; }

	static VeryLargeObjectPtr getObject( const BW::string& uid )	
	{
		// The tool uses UniqueIDs, yet VLOs converts IDs to lowercase, which
		// are then used for filenames, the map, etc, so ensuring lowercase.
		BW::string uidTemp = uid;
		std::transform(
				uidTemp.begin(), uidTemp.end(), uidTemp.begin(), tolower );
		return s_uniqueObjects_[uidTemp];
	}
#else

	static VeryLargeObjectPtr getObject( const BW::string& uid )	
	{
		return s_uniqueObjects_[uid];
	}
#endif //EDITOR_ENABLED

	virtual void syncInit(ChunkVLO* pVLO) {}

	static void tickAll( float dTime );
protected:
	static UniqueObjectList s_uniqueObjects_;
	friend class EditorChunkVLO;

	BW::string chunkPath_;
	BoundingBox bb_;
	BW::string uid_;
	BW::string type_;
	ChunkItemList itemList_;
#ifdef EDITOR_ENABLED
	//This stores all child items pointing to the VLO over time
	//until the references are destroyed
	ChunkItemList itemListInclHeldRefs_;
	DataSectionPtr dataSection_;
	bool listModified_;
	bool objectCreated_;
	ChunkItem * lastDbItem_;
	MetaData::MetaData metaData_;
#endif //EDITOR_ENABLED

private:
	bool rebuild_;
#ifdef EDITOR_ENABLED
	uint32 selectionMark_;
#endif //EDITOR_ENABLED
};


/**
 * Reference to the large object .... lives in a chunk (one per chunk per VLO)
 */
class ChunkVLO : public ChunkItem
{
public:
	static const BW::string & getTypeAttrName();
	static const BW::string & getUIDAttrName();

	explicit ChunkVLO( WantFlags wantFlags = WANTS_DRAW );
	~ChunkVLO();	

	virtual void draw( Moo::DrawContext& drawContext );		
	virtual void objectCreated() { }
	virtual void lend( Chunk * pChunk );
	virtual void toss( Chunk * pChunk );
	virtual void addCollisionScene();
	virtual void removeCollisionScene() {}	
	virtual void updateTransform( Chunk * pChunk ) {}
	virtual void updateAnimations();
//	virtual bool legacyLoad( DataSectionPtr pSection, Chunk * pChunk, BW::string& type );
	virtual void sway( const Vector3 & src, const Vector3 & dst, const float diameter );

	virtual bool load( DataSectionPtr pSection, Chunk * pChunk );
	virtual bool load( const BW::string & uid, Chunk * pChunk ) { return false; }

	static ChunkItemFactory::Result create(
		ChunkVLO * pVLO, Chunk * pChunk, DataSectionPtr pSection );

#ifdef EDITOR_ENABLED
	//TODO: move into editorVLO!!!! 
	bool root() const { return creationRoot_; }
	void root(bool val) { creationRoot_ = val; }
	bool createVLO( DataSectionPtr pSection, Chunk* pChunk );
	bool createLegacyVLO( DataSectionPtr pSection, Chunk* pChunk, BW::string& type );	
	bool cloneVLO( DataSectionPtr pSection, Chunk* pChunk, VeryLargeObjectPtr pSource );
	bool buildVLOSection( DataSectionPtr pObjectSection, Chunk* pChunk, BW::string& type, BW::string& uid ); 
	virtual int edNumTriangles() const;
	virtual int edNumPrimitives() const;
#endif //EDITOR_ENABLED

	static bool loadItem( Chunk* pChunk, DataSectionPtr pSection );

	VeryLargeObjectPtr object() const{ return pObject_; }
	static void registerFactory( const BW::string & section,
				const VLOFactory & factory );

	static void fini();
	virtual void syncInit();

protected:
	VeryLargeObjectPtr		pObject_;
private:
	typedef StringHashMap<const VLOFactory*> Factories;

	bool				dirty_;
	bool				creationRoot_;
	static Factories	*pFactories_;

	// VLO reference factory...
	DECLARE_CHUNK_ITEM( ChunkVLO )
};

BW_END_NAMESPACE

#endif // CHUNK_VLO_HPP
