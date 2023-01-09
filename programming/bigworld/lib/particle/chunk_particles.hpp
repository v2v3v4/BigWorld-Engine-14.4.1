#ifndef CHUNK_PARTICLES_HPP
#define CHUNK_PARTICLES_HPP

#include "chunk/chunk_item.hpp"
#include "math/matrix_liason.hpp"


BW_BEGIN_NAMESPACE

class MetaParticleSystem;
typedef SmartPointer<MetaParticleSystem> MetaParticleSystemPtr;
class Moo::GlobalStateBlock;
/**
 *	This class is a chunk item that tends a particle system.
 */
class ChunkParticles : public ChunkItem, public MatrixLiaison
{
public:
	ChunkParticles();
	~ChunkParticles();

	bool load( DataSectionPtr pSection );

	void load( const BW::StringRef & resourceName );

	virtual void draw( Moo::DrawContext& drawContext );

	virtual void lend( Chunk * pLender );


	virtual void toss( Chunk * pChunk );
	virtual void tick( float dTime );

	virtual void drawBoundingBoxes( const BoundingBox &bb, const BoundingBox &vbb, const Matrix &spaceTrans ) const;

	void worldVisibilityBoundingBox( BoundingBox & bb, bool clipChunkSzie ) const;


	virtual const char * label() const;

	bool getReflectionVis() const { return reflectionGlobalState_ != NULL ; }
	bool setReflectionVis( const bool& reflVis );

	virtual const Matrix & getMatrix() const	{ return worldTransform_; }
	virtual bool setMatrix( const Matrix & m )	{ worldTransform_ = m; return true; }

	virtual bool reflectionVisible() { return reflectionGlobalState_ != NULL; }
protected:
	virtual bool addYBounds( BoundingBox& bb ) const;

	Matrix					localTransform_;
	Matrix					worldTransform_;
	MetaParticleSystemPtr	system_;
	uint32					staggerIdx_;
	float					seedTime_;

private:
	static ChunkItemFactory::Result create( Chunk * pChunk, DataSectionPtr pSection );
	static ChunkItemFactory	factory_;

	Moo::GlobalStateBlock*	reflectionGlobalState_;

};


BW_END_NAMESPACE

#endif // CHUNK_PARTICLES_HPP
