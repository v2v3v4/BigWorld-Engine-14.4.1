#ifndef CHUNK_LIGHT_HPP
#define CHUNK_LIGHT_HPP


#include "chunk_cache.hpp"
#include "moo/moo_math.hpp"
#include "moo/light_container.hpp"
#include "math/linear_animation.hpp"
#include "chunk_item.hpp"
#include "chunk.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	class Animation;
}

class ChunkLightCache;

/**
 *	This class is the base class of lights in a chunk.
 */
class ChunkLight : public ChunkItem
{
public:
	explicit ChunkLight( WantFlags wantFlags = WANTS_NOTHING ) : ChunkItem( wantFlags ) { }
	virtual void toss( Chunk * pChunk );

	virtual const Moo::Colour & colour() const = 0;

protected:
	virtual void addToCache( ChunkLightCache& cache ) const = 0;
	virtual void addToContainer( Moo::LightContainerPtr pLC ) const = 0;
	virtual void delFromContainer( Moo::LightContainerPtr pLC ) const = 0;

	virtual void updateLight( const Matrix& world ) const = 0;
};


/**
 *	This class is the base class of lights in a chunk
 *	to contain the light types defined in Moo.
 */
class ChunkMooLight : public ChunkLight
{
public:
	ChunkMooLight( WantFlags wantFlags = WANTS_NOTHING );

protected:
	virtual void addToCache( ChunkLightCache& cache ) const;
};




/**
 *	This class is a directional light in a chunk
 */
class ChunkDirectionalLight : public ChunkMooLight
{
	DECLARE_CHUNK_ITEM( ChunkDirectionalLight )

public:
	ChunkDirectionalLight();
	~ChunkDirectionalLight();

	bool load( DataSectionPtr pSection );

	const Moo::Colour & colour() const			{ return pLight_->colour(); }
	const Vector3 & direction() const			{ return pLight_->direction(); }
	Moo::DirectionalLightPtr & pLight()			{ return pLight_; }

protected:
	virtual void addToContainer( Moo::LightContainerPtr pLC ) const;
	virtual void delFromContainer( Moo::LightContainerPtr pLC ) const;

	virtual void updateLight( const Matrix& world ) const;

	Moo::DirectionalLightPtr		pLight_;
};


/**
 *	This class is an omni light in a chunk
 */
class ChunkOmniLight : public ChunkMooLight
{
	DECLARE_CHUNK_ITEM( ChunkOmniLight )

public:
	ChunkOmniLight( WantFlags wantFlags = WANTS_NOTHING );
	~ChunkOmniLight();

	bool load( DataSectionPtr pSection );

	const Moo::Colour & colour() const			{ return pLight_->colour(); }
	Moo::OmniLightPtr & pLight()				{ return pLight_; }
	bool intersects( const BoundingBox& worldSpaceBB ) const	{ return pLight_->intersects( worldSpaceBB ); }

	static void load( Moo::OmniLight & light, const DataSectionPtr & pSection );

protected:
	virtual void addToContainer( Moo::LightContainerPtr pLC ) const;
	virtual void delFromContainer( Moo::LightContainerPtr pLC ) const;

	virtual void updateLight( const Matrix& world ) const;

	Moo::OmniLightPtr		pLight_;
};



/**
 *	This class is a spot light in a chunk
 */
class ChunkSpotLight : public ChunkMooLight
{
	DECLARE_CHUNK_ITEM( ChunkSpotLight )

public:
	ChunkSpotLight( WantFlags wantFlags = WANTS_NOTHING );
	~ChunkSpotLight();

	bool load( DataSectionPtr pSection );

	const Moo::Colour & colour() const			{ return pLight_->colour(); }
	Moo::SpotLightPtr & pLight()				{ return pLight_; }
	bool intersects( const BoundingBox& worldSpaceBB ) const	{ return pLight_->intersects( worldSpaceBB ); }

	// TODO: Add other accessors

	static void load( Moo::SpotLight & light, const DataSectionPtr & pSection );

protected:
	virtual void addToContainer( Moo::LightContainerPtr pLC ) const;
	virtual void delFromContainer( Moo::LightContainerPtr pLC ) const;

	virtual void updateLight( const Matrix& world ) const;

	Moo::SpotLightPtr		pLight_;
};


/**
 *	This class is an omni light in a chunk
 */
class ChunkPulseLight : public ChunkMooLight
{
	DECLARE_CHUNK_ITEM( ChunkPulseLight )

public:
	ChunkPulseLight( WantFlags wantFlags = WANTS_TICK );
	~ChunkPulseLight();

	bool load( DataSectionPtr pSection );

	void tick( float dTime );

	const Moo::Colour&	colour() const						{ return colour_; }
	void				colour( const Moo::Colour& col )	{ colour_ = col; }

	bool intersects( const BoundingBox& worldSpaceBB ) const	{ return pLight_->intersects( worldSpaceBB ); }

	Moo::OmniLightPtr & pLight() { return pLight_; }

	static const StringRef defaultAnimation;
	static const StringRef defaultFrameSection;
protected:
	virtual void addToContainer( Moo::LightContainerPtr pLC ) const;
	virtual void delFromContainer( Moo::LightContainerPtr pLC ) const;

	virtual void updateLight( const Matrix& world ) const;

	Moo::OmniLightPtr		pLight_;
	Moo::Colour				colour_;
	SmartPointer<Moo::Animation>	pAnimation_;
	LinearAnimation<float>	colourAnimation_;
	float					positionAnimFrame_;
	float					colourAnimFrame_;
	Vector3					position_;
	Vector3					animPosition_;
};



/**
 *	This class is an ambient light in a chunk
 */
class ChunkAmbientLight : public ChunkLight
{
	DECLARE_CHUNK_ITEM( ChunkAmbientLight )

public:
	ChunkAmbientLight();
	~ChunkAmbientLight();

	bool load( DataSectionPtr pSection );

	const Moo::Colour & colour() const			{ return colour_; }

#ifdef EDITOR_ENABLED
	float				multiplier() const { return multiplier_; }
	void				multiplier( float m ) { multiplier_ = m; }
#else//EDITOR_ENABLED
	float				multiplier() const { return 1.0f; }
#endif

protected:
	virtual void addToCache( ChunkLightCache& cache ) const;
	virtual void addToContainer( Moo::LightContainerPtr pLC ) const;
	virtual void delFromContainer( Moo::LightContainerPtr pLC ) const;

	virtual void updateLight( const Matrix& world ) const {}

	Moo::Colour		colour_;

#ifdef EDITOR_ENABLED
	float				multiplier_;
#endif
};



/**
 *	This class caches all the lights in a chunk in an easy-to-use
 *	light container.
 */
class ChunkLightCache : public ChunkCache
{
public:
	ChunkLightCache( Chunk & chunk );
	~ChunkLightCache();

	const static int MAX_LIGHT_SEEP_DEPTH = 2;

	void update();
	virtual void draw( Moo::DrawContext& drawContext );
	virtual void bind( bool isUnbind );

	static void touch( Chunk & chunk );

	Moo::LightContainerPtr pOwnLights()			{ return pOwnLights_; }

	Moo::LightContainerPtr pAllLights()			{ return pAllLights_; }

	static Instance<ChunkLightCache>	instance;

	void dirtySeep( int seepDepth=MAX_LIGHT_SEEP_DEPTH, Chunk* parent=NULL );

	void moveOmni( const Moo::OmniLightPtr& omni, const Vector3& opos, float oradius, bool transient=false );
	void moveSpot( const Moo::SpotLightPtr& spot, const Vector3& opos, float oradius, bool transient=false );

private:
	void collectLights();

	Chunk & chunk_;

	Moo::LightContainerPtr	pOwnLights_;
	Moo::LightContainerPtr	pAllLights_;

	bool				lightContainerDirty_;
	bool				heavenSeen_;
};

BW_END_NAMESPACE


#endif // CHUNK_LIGHT_HPP
