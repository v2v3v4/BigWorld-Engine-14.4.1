#ifndef EDITOR_CHUNK_MODEL_VLO_HPP
#define EDITOR_CHUNK_MODEL_VLO_HPP

#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "worldeditor/world/items/editor_chunk_model.hpp"
#include "chunk/chunk_model_vlo.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is the editor version of a ChunkModelVLO
 */
class EditorChunkModelVLO
	: public ChunkModelVLO
{	
	static VLOFactory factory_;
	static bool create(
		Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid );
	
public:
	EditorChunkModelVLO( const BW::string & uid );
	~EditorChunkModelVLO();

	void dirty();
	bool load( DataSectionPtr pSection, Chunk * pChunk );

	virtual void cleanup();
	virtual void saveFile( Chunk* chunk = NULL );
	virtual void save();
	virtual bool edSave( DataSectionPtr pSection );

	void edBounds( BoundingBox & bbRet, Chunk * pChunk ) const;

	DECLARE_EDITOR_CHUNK_ITEM_CLASS_NAME( "Proxy" );
	virtual bool edEdit( class GeneralEditor & editor, const ChunkItemPtr pItem);
	virtual void edCommonChanged();
	
	virtual bool visibleInside() const;

	virtual bool visibleOutside() const;

	virtual int numTriangles() const;

	virtual int numPrimitives() const;

	virtual BW::string edAssetName() const { return "ProxyVLO"; }

	static const BW::vector< EditorChunkModelVLO * > & instances()
	{
		return instances_;
	}

	static SimpleMutex &instancesMutex() { return instancesMutex_; }

	virtual const Matrix & localTransform();
	Chunk * vloChunk() const;

	void save( DataSectionPtr dataSectionPtr, const Matrix & worldTransform );

private:
	EditorChunkModelVLO( const EditorChunkModelVLO & );
	EditorChunkModelVLO& operator=( const EditorChunkModelVLO & );

	virtual const char * sectName() const { return "proxy"; }
	virtual bool isDrawFlagVisible() const
		{ return true; }
	virtual const char * drawFlag() const { return "render/scenery/drawProxy"; }	

	EditorChunkModelPtr getProxiedModel();

	bool changed_;

	static BW::vector< EditorChunkModelVLO* > instances_;
	static SimpleMutex					  instancesMutex_;
};


typedef SmartPointer< EditorChunkModelVLO > EditorChunkProxyPtr;

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_MODEL_VLO_HPP
