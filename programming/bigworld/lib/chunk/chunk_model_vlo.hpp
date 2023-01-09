#ifndef CHUNK_MODEL_VLO_HPP
#define CHUNK_MODEL_VLO_HPP

#include "chunk_vlo.hpp"
#ifdef MF_SERVER
#include "chunk/server_chunk_model.hpp"
#else // MF_SERVER
#include "chunk/chunk_model.hpp"
#endif // MF_SERVER

BW_BEGIN_NAMESPACE


#ifdef MF_SERVER
typedef SmartPointer< ServerChunkModel > ChunkModelPtr;
#else // MF_SERVER
typedef SmartPointer< ChunkModel > ChunkModelPtr;
#endif // MF_SERVER

/**
 *	This class is a wrapper VLO object for regular chunk models
 */
class ChunkModelVLO : public VeryLargeObject
{
public:
	static const BW::string & getLargeObjectType();
	static const BW::string & getSectionName();
	static const BW::string & getWorldTransformAttrName();

	ChunkModelVLO( const BW::string & uid );
	~ChunkModelVLO();

	bool load( DataSectionPtr pSection, Chunk * pChunk );

#ifndef MF_SERVER
	virtual void tick( float dTime );
	virtual void updateAnimations();
	virtual void drawInChunk( Moo::DrawContext& drawContext, Chunk * pChunk );
#else
	virtual void drawInChunk( Moo::DrawContext& drawContext, Chunk * pChunk ) {}
#endif // !MF_SERVER
	virtual void addCollision( ChunkItemPtr item );

	virtual BoundingBox chunkBB( Chunk * pChunk );

#ifdef EDITOR_ENABLED
	virtual void edCommonChanged() {}
#endif // EDITOR_ENABLED

	virtual const Matrix & localTransform( Chunk * pChunk );

	ChunkModelPtr getProxiedModel() { return proxiedModel_; }
	const Matrix & worldTransform() const { return worldTransform_; }

	virtual void syncInit( ChunkVLO* pVLO );

protected:
	ChunkModelPtr	proxiedModel_;

	virtual void updateLocalVars( const Matrix & m );

private:
#ifndef MF_SERVER
	Chunk *			drawnChunk_;
#endif // !MF_SERVER
	Matrix			worldTransform_;

	static bool create(
		Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid );
	static VLOFactory	factory_;
};

BW_END_NAMESPACE

#endif // CHUNK_MODEL_VLO_HPP
