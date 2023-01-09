#ifndef EDITOR_CHUNK_WATER_HPP
#define EDITOR_CHUNK_WATER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_water.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a ChunkWater
 */
class EditorChunkWater : public EditorChunkSubstance<ChunkWater>
{	
	static VLOFactory factory_;
	static bool create(
		Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid );
	
	static ChunkItemFactory	oldWaterFactory_;
	static ChunkItemFactory::Result oldCreate( Chunk * pChunk, DataSectionPtr pSection );
public:
	EditorChunkWater( BW::string uid );
	~EditorChunkWater();

	void toss( );
	void dirty();
	bool load( DataSectionPtr pSection, Chunk * pChunk );

	virtual void cleanup();
	virtual void saveFile( Chunk* chunk=NULL );
	virtual void save( );
	virtual void drawRed(bool val);
	virtual void highlight(bool val);
	virtual const Matrix & origin();
	virtual const Matrix & edTransform();	
	virtual const Matrix & localTransform( );
	virtual void edDelete( ChunkVLO* instigator );
	virtual bool edSave( DataSectionPtr pSection );
	DECLARE_EDITOR_CHUNK_ITEM_CLASS_NAME( "Water" );
	virtual const Matrix & localTransform( Chunk * pChunk );
//	virtual bool edTransform( const Matrix & m, bool transient );
	virtual bool edEdit( class GeneralEditor & editor, const ChunkItemPtr pItem);
	virtual void edCommonChanged();
	
	virtual bool visibleInside() const;

	virtual bool visibleOutside() const;

	virtual void drawInChunk( Moo::DrawContext& drawContext, Chunk* pChunk );

	virtual int numTriangles() const;

	virtual int numPrimitives() const;

	virtual BW::string edAssetName() const { return "Water"; }

	static const BW::vector<EditorChunkWater*>& instances() { return instances_; }
	static SimpleMutex &instancesMutex() { return instancesMutex_; }

private:
	EditorChunkWater( const EditorChunkWater& );
	EditorChunkWater& operator=( const EditorChunkWater& );

	virtual void addAsObstacle() { }
	virtual ModelPtr reprModel() const;
	virtual void updateLocalVars( const Matrix & m );
	virtual void updateWorldVars( const Matrix & m );
	virtual const char * sectName() const { return "water"; }
	virtual bool isDrawFlagVisible() const
		{ return OptionsScenery::waterVisible(); }
	virtual const char * drawFlag() const { return "render/scenery/drawWater"; }	


	bool			changed_;
	Vector2			size2_;
	Vector3			worldPos_;
	float			localOri_;
	Matrix			transform_;
	Matrix			origin_;
	Matrix			scale_;

	ModelPtr waterModel_;

	static BW::vector<EditorChunkWater*> instances_;
	static SimpleMutex					  instancesMutex_;
};


typedef SmartPointer<EditorChunkWater> EditorChunkWaterPtr;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_WATER_HPP
