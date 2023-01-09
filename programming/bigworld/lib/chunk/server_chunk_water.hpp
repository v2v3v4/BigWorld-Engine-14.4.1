#ifndef SERVER_CHUNK_WATER_HPP
#define SERVER_CHUNK_WATER_HPP

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/matrix.hpp"

#include "chunk_item.hpp"
#include "chunk_vlo.hpp"
#include "server_super_model.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This class is a body of water as a chunk item on the server
 */
class ServerChunkWater : public VeryLargeObject
{
public:

	ServerChunkWater( const BW::string & uid );
	~ServerChunkWater();

	bool load( DataSectionPtr pSection, Chunk * pChunk );

	virtual void drawInChunk( Moo::DrawContext& drawContext, Chunk* pChunk ) {}

	virtual void addCollision( ChunkItemPtr item );

	virtual const Matrix & localTransform( ) { return transform_; }	
	const Vector3 & position() const { return position_; }
	const Vector2 & size() const { return size_; }
	float orientation() const { return orientation_; }
	float depth() const { return depth_; }

private:

	static bool create(
		Chunk * pChunk, DataSectionPtr pSection, const BW::string & uid );
	static VLOFactory factory_;

	Matrix transform_;
	Vector3 position_;
	Vector2 size_;
	float orientation_;		
	float depth_;

	RealWTriangleSet tris_;
	BSPTree * pTree_;
	ModelPtr pModel_;
};

BW_END_NAMESPACE

#endif // SERVER_CHUNK_WATER_HPP
