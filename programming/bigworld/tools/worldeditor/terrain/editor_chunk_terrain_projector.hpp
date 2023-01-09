#ifndef EDITOR_CHUNK_TERRAIN_PROJECTOR_HPP
#define EDITOR_CHUNK_TERRAIN_PROJECTOR_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

class EditorChunkTerrainProjector
{
public:	
	static EditorChunkTerrainProjector& instance();

	typedef std::pair<Matrix, EditorChunkTerrainPtr >	BlockInPlace;
	typedef AVectorNoDestructor< BlockInPlace >		BlockVector;
	void projectTexture( const Moo::BaseTexturePtr& pTexture,
		float scale, float rotation, const Vector3& offset, 
		D3DTEXTUREADDRESS wrapMode,
		EditorChunkTerrainPtrs& chunks, bool showHoles );

	void init() {}
	void fini();
private:
	EditorChunkTerrainProjector();
	void setInitialRenderStates(const Moo::BaseTexturePtr& pTexture, 
		D3DTEXTUREADDRESS wrapMode);
	void renderBlocks(BlockVector& blocks);	
	void textureTransform( const Matrix& world, Matrix& ret, const Vector3& offset );
	void ffTextureTransform( const Matrix& world, Matrix& ret, const Vector3& offset );

	float scale_;
	float rotation_;
	Vector3 offset_;
	bool showHoles_;
	class Moo::VertexDeclaration* pDecl_;
	Moo::EffectMaterialPtr material_;
};

BW_END_NAMESPACE

#endif // EDITOR_CHUNK_TERRAIN_PROJECTOR_HPP
