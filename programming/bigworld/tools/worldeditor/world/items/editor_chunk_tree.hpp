#ifndef EDITOR_CHUNK_TREE_HPP
#define EDITOR_CHUNK_TREE_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "chunk/chunk_tree.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a ChunkTree
 */
class EditorChunkTree : public ChunkTree
{
public:
	EditorChunkTree();
	~EditorChunkTree();

	virtual InvalidateFlags edInvalidateFlags();

	virtual bool edShouldDraw();
	virtual void draw( Moo::DrawContext& drawContext );

	bool load( DataSectionPtr pSection, Chunk * pChunk );
	/** Called once after loading from the main thread */
	void edPostLoad();

	virtual void toss( Chunk * pChunk );

	virtual bool edSave( DataSectionPtr pSection );
	virtual void edChunkSave();
	virtual void edChunkSaveCData(DataSectionPtr cData);

	virtual const Matrix & edTransform() { return this->transform(); }
	virtual bool edTransform( const Matrix & m, bool transient );
	virtual void edBounds( BoundingBox& bbRet ) const;
	virtual void edSelectedBox( BoundingBox& bbRet ) const; 

	virtual bool edEdit( class GeneralEditor & editor );
	virtual Chunk * edDropChunk( const Vector3 & lpos );

	virtual Name edDescription();

	virtual int edNumTriangles() const;

	virtual int edNumPrimitives() const;

	virtual BW::string edAssetName() const;
	virtual BW::string edFilePath() const;

	virtual DataSectionPtr pOwnSect()	{ return pOwnSect_; }
	virtual const DataSectionPtr pOwnSect()	const { return pOwnSect_; }

	virtual Moo::LightContainerPtr edVisualiseLightContainer();

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

private:
	EditorChunkTree( const EditorChunkTree& );
	EditorChunkTree& operator=( const EditorChunkTree& );

	unsigned long getSeed() const;
	bool setSeed( const unsigned long & seed );

	bool getCastsShadow() const;
	bool setCastsShadow( const bool & castsShadow );

	bool			hasPostLoaded_;
	DataSectionPtr	pOwnSect_;

	ModelPtr missingTreeModel_;
	
	DECLARE_EDITOR_CHUNK_ITEM_WITHOUT_DESCRIPTION( EditorChunkTree )

	// for edDescription
	Name	desc_;

	// For finding BSP in cache
	BW::string fullPath_;

	mutable BoundingBox bspBB_;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_TREE_HPP
