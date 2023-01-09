#include "pch.hpp"
#include "decal.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "physics2/worldtri.hpp"
#include "moo/effect_material.hpp"
#include "moo/texture_manager.hpp"
#include "moo/render_context.hpp"
#include "moo/base_texture.hpp"
#include "moo/texture_lock_wrapper.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/line_helper.hpp"

DECLARE_DEBUG_COMPONENT2( "Duplo", 0 )


BW_BEGIN_NAMESPACE

static AutoConfigString s_decalEffect( "system/decalEffect" );

// -----------------------------------------------------------------------------
// Section: Decal
// -----------------------------------------------------------------------------

// Watcher variables.
bool Decal::watchersInitialized_ = false;
uint Decal::renderedTrianglesCountPerFrame_ = 0;
uint Decal::collisonTrianglesCountPerSecond_ = 0;
uint Decal::buffersMemoryAllocated_ = 0;
uint Decal::buffersCount_ = 0;

int Decal_token = 1;
const uint32 MAX_VERTICES_PER_DECAL = 500;

const uint32 COMPRESSED_TILE_SIZE = 16;


// Set PRESERVE_V18X_COLLISION_OFFSET to 1 if you want the decal 
// collision test to have the same behaviour as it did in 1.8.x and 
// earlier versions (it ignored collisions that occured at or closer 
// than 0.1 metres from the origin of the collision ray).
#define PRESERVE_V18X_COLLISION_OFFSET 0

static void blockFill( void* destination, uint32 size )
{
	uint8* dest = (uint8*)destination;
	const uint32 block[4] = {0xffffffff, 0xffffffff, 0x0, 0x0 };
	size = size / COMPRESSED_TILE_SIZE;
	for (uint32 i = 0; i < size; i++)
	{
		CopyMemory( dest, block, 16 );
		dest += 16;
	}
}
/**
 * This class implements a texture built up of several other textures.
 */
class TileTexture : public Moo::BaseTexture, public Moo::DeviceCallback
{
public:
	TileTexture( uint32 uTiles, uint32 vTiles, uint32 tileSize) : 
	  tileSize_( tileSize ),
	  width_( uTiles * tileSize ),
	  height_( vTiles * tileSize ),
	  uTiles_( uTiles ),
	  vTiles_( vTiles ),
	  uSize_( 1.f / float( uTiles ) ),
	  vSize_( 1.f / float( vTiles ) ),
	  nMipMaps_( 0 )
	{
		subTextures_.resize( uTiles * vTiles );

	}
	~TileTexture()
	{
	}
	virtual DX::BaseTexture*	pTexture( ) { return pTexture_.pComObject(); };
	virtual uint32			width( ) const { return width_; };
	virtual uint32			height( ) const { return height_; };
	virtual D3DFORMAT		format( ) const { return D3DFMT_DXT3; };
	virtual uint32			textureMemoryUsed( ) const { return 0; } ;
	virtual const BW::string& resourceID( ) const 
	{ 
		static BW::string name = "tile_texture";
		return name; 
	};

	/**
	 *	Single entry in the decal texture atlas.
	 */
	struct	SubTexture
	{
		SubTexture():
			name_(""),
			uOffset_(0.f),
			vOffset_(0.f),
			loaded_(false)
		{};

		BW::string name_;		
		float		uOffset_;
		float		vOffset_;
		bool		loaded_;
	};
	typedef BW::vector< SubTexture > SubTextures;

	int textureIndex( const BW::string& textureName );
	const SubTexture& subTexture( int index ) const { return subTextures_[ index ]; };
	float uSize() const { return uSize_; }
	float vSize() const { return vSize_; }

	virtual void deleteUnmanagedObjects();
	virtual void createUnmanagedObjects();

	bool copySubTexture( int index,  Moo::BaseTexturePtr pBaseTexture );

private:
	bool validateTexture(  Moo::BaseTexturePtr pBaseTexture );
	int addSubTexture( const BW::string& textureName );
	ComObjectWrap<DX::Texture>	pTexture_;
	uint8*			pBits_;
	uint32			width_;
	uint32			height_;
	uint32			uTiles_;
	uint32			vTiles_;
	uint32			tileSize_;
	uint32			nMipMaps_;
	float			uSize_;
	float			vSize_;
	SubTextures		subTextures_;

	TileTexture(const TileTexture&);
	TileTexture& operator=(const TileTexture&);
};

static SmartPointer<TileTexture> s_decalTexture = NULL;

static void EnsureDecalTexture()
{
	if (!s_decalTexture)
		s_decalTexture = new TileTexture(8, 8, 64);
}

/**
 * This is the functor class for TileTexture::SubTexture type.
 */
class SubTexFinder
{
public:
	SubTexFinder( const BW::string& name, bool musntBeLoaded = false ):
	  name_( name ),
	  mustNotBeLoaded_( musntBeLoaded )
	{};

	bool operator ()( const TileTexture::SubTexture& subTex ) 
	{
		if (mustNotBeLoaded_ && subTex.loaded_)
			return false;
		return name_ == subTex.name_;
	}
	BW::string name_;
	bool mustNotBeLoaded_;
};


class DecalLoader : public BackgroundTask
{
public:
	DecalLoader( TileTexture& tt, TileTexture::SubTexture& subTexture, int index, bool async ):
		BackgroundTask( "DecalLoader" ),
		subTexture_( subTexture ),
		tex_( NULL ),
		index_( index ),
		async_( async ),
		tt_( tt )
	{
		BW_GUARD;

		if (async_)
		{
			FileIOTaskManager::instance().addBackgroundTask(this);
		}
		else
		{
			this->doBackgroundTask( BgTaskManager::instance() );
			this->doMainThreadTask( BgTaskManager::instance() );
		}	
	}	

private:
	TileTexture::SubTexture& subTexture_;
	Moo::BaseTexturePtr tex_;
	int index_;
	bool async_;
	TileTexture& tt_;

	void doBackgroundTask( TaskManager& mgr )
	{
		BW_GUARD;
		tex_ = Moo::TextureManager::instance()->getSystemMemoryTexture(subTexture_.name_);
		if (async_)
		{
			FileIOTaskManager::instance().addMainThreadTask(this);
		}		
	};

	void doMainThreadTask( TaskManager& mgr )
	{		
		BW_GUARD_PROFILER( DecalLoader_doMainThreadTask );
		if (tex_.hasObject())
		{
			if (tt_.copySubTexture( index_, tex_ ))
			{
				subTexture_.loaded_ = true;
			}
			else
			{
				subTexture_.name_ = "";
				subTexture_.loaded_ = false;
			}
		}
	};
};


/**
 * This method adds a sub-texture to the TileTexture.  It returns the index
 * straight away, however any texture loading that occurs as a result will
 * be done asynchronously.
 */
int TileTexture::addSubTexture( const BW::string& textureName )
{
	BW_GUARD;
	SubTextures::iterator it = std::find_if( subTextures_.begin(), 
		subTextures_.end(), SubTexFinder( "", true /*mustNotBeLoaded*/ ) );

	MF_ASSERT( subTextures_.size() <= INT_MAX );

	int index = -1;
	if (it != subTextures_.end())
	{
		index = ( int ) ( it - subTextures_.begin() );
		it->name_ = textureName;
		uint32 uOff = index % uTiles_;
		uint32 vOff = index / uTiles_;
		it->uOffset_ = float(uOff) * uSize_;
		it->vOffset_ = float(vOff) * vSize_;

		//The lifetime of this class is handled by the background loader.
		new DecalLoader( *this, *it, index, true /*async*/ );
	}
	return index;
}


/**
 *	This method validates that the passed in texture is suitable
 *	for use by the Tiletexture.
 */
bool TileTexture::validateTexture(  Moo::BaseTexturePtr pBaseTexture )
{
	BW_GUARD;
	DX::BaseTexture* pBase = pBaseTexture->pTexture();
    if (pBase->GetType() != D3DRTYPE_TEXTURE)
	{
		ERROR_MSG( "decal %s has the wrong format, it must be of type "
			"D3DRTYPE_TEXTURE\n", pBaseTexture->resourceID().c_str() );
		return false;
	}

	DX::Texture* pTex = (DX::Texture*)pBase;
	D3DSURFACE_DESC desc;
	pTex->GetLevelDesc( 0, &desc );

	if (desc.Format == D3DFMT_DXT3 && 
		desc.Width == tileSize_ &&
		desc.Height == tileSize_)
	{
		return true;
	}
	else
	{
		if (desc.Format != D3DFMT_DXT3)
		{
			ERROR_MSG( "decal %s has the wrong format, it must "
						"be DXT3\n", pBaseTexture->resourceID().c_str() );
		}
		else
		{
			ERROR_MSG( "decal %s has the wrong size, it must "
						"be 64 x 64\n", pBaseTexture->resourceID().c_str() );
		}		
	}

	return false;
}



/**
 * Copy supplied texture into sub-texture at given index.
 */
bool TileTexture::copySubTexture( int index, Moo::BaseTexturePtr pBaseTexture )
{
	BW_GUARD;
	if (!this->validateTexture( pBaseTexture ))
		return false;

	DX::Texture* pTex = (DX::Texture*)pBaseTexture->pTexture();

	if (!pTexture_.pComObject())
	{
		createUnmanagedObjects();
	}
	if (pTexture_.pComObject())
	{
		uint32 uOff = index % uTiles_;
		uint32 vOff = index / uTiles_;
		uint32 width = width_;
		uint32 height = height_;
		uint32 tileSize = tileSize_;
		RECT srcRect;
		srcRect.left = 0;
		srcRect.right = tileSize_;
		srcRect.top = 0;
		srcRect.bottom = tileSize_;

		POINT destPt;
		destPt.x = (index % uTiles_) * tileSize_;
		destPt.y = (index / uTiles_) * tileSize_;

		for (uint32 mip = 0; mip < nMipMaps_; mip++)
		{
			DX::Surface* pDestSurface;
			HRESULT hr = pTexture_->GetSurfaceLevel( mip, &pDestSurface );

			DX::Surface* pSrcSurface;
			hr = pTex->GetSurfaceLevel( mip, &pSrcSurface );

			//copy data from small map's mip-map to large map's mip-map
			hr = Moo::rc().device()->UpdateSurface( pSrcSurface, &srcRect, pDestSurface, &destPt );

			//advance to next mip-map
			destPt.x/=2;	
			destPt.y/=2;	
			srcRect.right = max( (srcRect.right/2), 1L );
			srcRect.bottom = max( (srcRect.bottom/2), 1L );
			pDestSurface->Release();
			pSrcSurface->Release();
		}
	}
	return true;
}

void TileTexture::deleteUnmanagedObjects( )
{
	BW_GUARD;
	pTexture_ = NULL;
	pBits_ = NULL;
}

void TileTexture::createUnmanagedObjects( )
{
	BW_GUARD;
	nMipMaps_ = 0;
	uint32 compressedTiles = 0;
	uint32 mipMapTiles = (tileSize_ * tileSize_) / 16;
	while (mipMapTiles != 0)
	{
		compressedTiles += mipMapTiles;
		nMipMaps_++;
		mipMapTiles /= 4;
	}

	uint32 surfaceSize = compressedTiles * uTiles_ * vTiles_ * COMPRESSED_TILE_SIZE ;
	pTexture_ = Moo::rc().createTexture( width_, height_, nMipMaps_, 0, D3DFMT_DXT3, D3DPOOL_DEFAULT, "texture/tile" );
	if( pTexture_ )
	{
		Moo::TextureLockWrapper textureLock( pTexture_ );

		for (uint32 i = 0; i < nMipMaps_; i++)
		{
			D3DSURFACE_DESC sd;
			pTexture_->GetLevelDesc( i, &sd );
			D3DLOCKED_RECT lr;
			textureLock.lockRect( i, &lr, NULL, 0 );
			// DXT3 surfaces use one byte per texel, so width x height gives us the surface size
			blockFill( lr.pBits, sd.Width * sd.Height );
			// This fixes a bug on nvidia cards, they don't like compressed dxt3 textures 
			// to be initialised to all full alpha...
			*(char*)lr.pBits = 0;
			textureLock.unlockRect( i );
		}
	}
	SubTextures::iterator it = subTextures_.begin();
	SubTextures::iterator end = subTextures_.end();
	MF_ASSERT( subTextures_.size() <= INT_MAX );
	int index = 0;
	while (it != end)
	{				
		if (it->loaded_)
		{
			DecalLoader dl( *this, *it, index, false /*async*/ );			
		}
		it++;
		++index;
	}
}

/**
 * Returns index of sub-texture with given name.
 */
int TileTexture::textureIndex( const BW::string& textureName )
{
	BW_GUARD;
	SubTextures::iterator it = std::find_if( subTextures_.begin(), 
		subTextures_.end(), SubTexFinder( textureName ) );

	MF_ASSERT( subTextures_.size() <= INT_MAX );

	int index = -1;
	if (it != subTextures_.end())
	{
		index = ( int ) ( std::distance( subTextures_.begin(), it ) );
	}
	else
	{
		index = addSubTexture( textureName );
	}
	return index;
}

namespace {
/**
 *	This class accumulates any triangles passed into it
 */
class DecalTriAcc : public CollisionCallback
{
public:
	VectorNoDestructor<WorldTriangle> tris_;
	Vector3 normal_;
private:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float dist )
	{
		BW_GUARD;
		if (!(triangle.flags() & TRIANGLE_TRANSPARENT) &&
			!obstacle.isMovingObstacle())
		{
			WorldTriangle wt(
				obstacle.transform().applyPoint( triangle.v0() ),
				obstacle.transform().applyPoint( triangle.v1() ),
				obstacle.transform().applyPoint( triangle.v2() ),
				triangle.flags() );
			Vector3 norm = wt.normal();
			norm.normalise();
//			const float cosAngle = cosf( DEG_TO_RAD( 45.f ) );
//			if (normal_.dotProduct(norm) < cosAngle)
//				return COLLIDE_ALL;
			float fDot = normal_.dotProduct(norm);
			if ( fDot > 0.15f )
			{
				// only add it if we don't already have it
				// (not sure why this happens, but it does)
				size_t sz = tris_.size();
				size_t i;
				for (i = 0; i < sz; i++)
				{
					if (tris_[i].v0() == wt.v0() &&
						tris_[i].v1() == wt.v1() &&
						tris_[i].v2() == wt.v2()) break;
				}
				if (i >= sz) tris_.push_back( wt );
			}
		}

		return COLLIDE_ALL;
	}
};
} // unnamed namespace

static DecalTriAcc s_triacc;

class DecalClosestTriangle : public CollisionCallback
{
public:
	virtual int operator()( const CollisionObstacle & obstacle,
		const WorldTriangle & triangle, float /*dist*/ )
	{
		// transform into world space
		hit_ = WorldTriangle(
			obstacle.transform().applyPoint( triangle.v0() ),
			obstacle.transform().applyPoint( triangle.v1() ),
			obstacle.transform().applyPoint( triangle.v2() ) );
		return COLLIDE_BEFORE;
	}

	WorldTriangle hit_;
};

static DecalClosestTriangle s_decalClosestTri;

/**
 *	Constructor.
 */
Decal::Decal() :
	pMaterial_( NULL ),
	elapsedTime_( 0.0f ),
	tickUpdateInterval_( 0.0f ),
	curTickUpdateTime_( 0.0f ),
	watcherTickTime_( 0.0f )
{
	BW_GUARD;
	

	pMaterial_ = new Moo::EffectMaterial;
	bool res = pMaterial_->initFromEffect( s_decalEffect );

	if (!res)
	{
		ERROR_MSG( "[DECAL] Decal::init - unable to init material\n" );
		pMaterial_ = NULL;
	}

	EnsureDecalTexture();

	tileTex_ = s_decalTexture;

	// Watcher variables.
	//------------------------------------------
	buffersCount_ = 0;
	
	if (!watchersInitialized_)
	{
		watchersInitialized_ = true;

		MF_WATCH( "Render/Decal/rendered primitive per frame",
			renderedTrianglesCountPerFrame_,
			Watcher::WT_READ_ONLY,
			" "
			" " );

		MF_WATCH( "Render/Decal/collision triangles per second",
			collisonTrianglesCountPerSecond_,
			Watcher::WT_READ_ONLY,
			" "
			" " );

		MF_WATCH( "Render/Decal/buffers allocated memory (kbytes)",
			buffersMemoryAllocated_,
			Watcher::WT_READ_ONLY,
			" "
			" " );

		MF_WATCH( "Render/Decal/buffers count",
			buffersCount_,
			Watcher::WT_READ_ONLY,
			" "
			" " );
	}

	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("Chunk/Decal", 
		(uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER), 
		sizeof(*this), 0);
}

/**
 *	Destructor.
 */
Decal::~Decal()
{
	BW_GUARD;
	if (pMaterial_ != NULL)
	{
		pMaterial_ = NULL;
	}
	
	// delete vertex buffers.
	// Note: It must be called before destroying decal groups, because 
	//		 vertex buffer must call release function to destroying
	//		 it in Direct3D system.
	deleteUnmanagedObjects();

	// clear decal groups;
	for ( uint i = 0; i < decalGroups_.size(); ++i)
	{
		delete decalGroups_[i].pVertexBuffer;
		decalGroups_[i].pVertexBuffer = NULL;
	}
	decalGroups_.clear();

	tileTex_ = NULL;
	
	// decal atlas lifetime is bound to DecalMap.py, which is filled only once
	// at startup and is reused multiple times, so let the atlas to live until app shutdown
	//if (s_decalTexture.getObject()->refCount()==1)
	//	s_decalTexture = NULL;

	// Watcher variables.
	//watchersInitialized_ = 0;
	renderedTrianglesCountPerFrame_ = 0;
	collisonTrianglesCountPerSecond_ = 0;
	buffersMemoryAllocated_ = 0;
	buffersCount_ = 0;
}

/**
 * Outcode definition structure for clipping code.
 */
struct OutcodeDef
{
	Outcode outcode;
	uint32	axis;
	float	value;
};

static const OutcodeDef outcodeDefs[4] =
{
	{ OUTCODE_LEFT,		0, 0.f },
	{ OUTCODE_RIGHT,	0, 1.f },
	{ OUTCODE_TOP,		1, 0.f },
	{ OUTCODE_BOTTOM,	1, 1.f }
};

/**
 * Clip decal against some vertices.
 */
void Decal::_clip( BW::vector<ClipVertex>& cvs )
{
	BW_GUARD;
	Outcode combined = 0;
    for (uint32 i = 0; i < cvs.size(); ++i)
	{
		cvs[i].calcOutcode();
		combined |= cvs[i].oc_;
	}

	if (combined)
	{
		Outcode mask = 0;
		for (uint32 oci = 0; oci < 4; ++oci)
		{
			const OutcodeDef& ocd = outcodeDefs[oci];
			mask |= ocd.outcode;
			if ((ocd.outcode & combined) && 
				(cvs.size() > 2))
			{
				cvs.reserve(cvs.size() + 1);
				cvs.push_back( cvs.front() );
				BW::vector<ClipVertex>::iterator it = cvs.begin() + 1;
				while (it != cvs.end())
				{
					BW::vector<ClipVertex>::iterator preIt = it;
					--preIt;
					if ((preIt->oc_ & ocd.outcode) != 
						(it->oc_ & ocd.outcode))
					{
						ClipVertex tclip;
						float scale = (ocd.value - it->uv_[ocd.axis]) / 
							(preIt->uv_[ocd.axis] - it->uv_[ocd.axis]);
						tclip.pos_ = it->pos_ + ((preIt->pos_ - it->pos_) * scale);
						tclip.uv_ = it->uv_ + ((preIt->uv_ - it->uv_) * scale);
						tclip.calcOutcode();
						tclip.oc_ &= ~mask;
						it = cvs.insert( it, tclip );
						++it;
					}
					++it;
				}
				cvs.pop_back();
				it = cvs.begin();
				while (it != cvs.end())
				{
					if (it->oc_ & ocd.outcode)
					{
						it = cvs.erase( it );
					}
					else
					{
						++it;
					}
				}
			}
		}
	}
}


/**
 *	This function attempts to add a decal to the scene, given a world ray.
 *	If the world ray collides with scene geometry, a decal is placed over
 *	the intersecting region.  Try not to have too large an extent for the
 *	decal collision ray, for reasons of performance.
 *
 *	@param start	The start of the collision ray.
 *	@param end		The end of the collision ray.
 *	@param size		The size of the resultant decal.
 *	@param type		The index of the decal texture.  @see BigWorld.decalTextureIndex
 *  @param group			The group index of decal.
 *
 */
void Decal::add(
	const Vector3& start,
	const Vector3& end,
	const Vector2& size,
	float yaw,
	int textureIndex,
	int	groupIndex,
	int textureIndex2,
	float textureBlendFactor,
	bool mirrorU
)
{
	BW_GUARD;

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
	if ( !pSpace.exists() )
		return;

	float dist = pSpace->collide( start, end, s_decalClosestTri );

	if (dist == -1.0f)
		return;

	#if PRESERVE_V18X_COLLISION_OFFSET
		if (dist <= 0.1f)
		{
			return;
		}
	#endif

	// find collision ray direction.
	Vector2 halfSize = size * 0.5f;
	float sizeCoeff = ((size.x > size.y) ? size.y : size.x);
	Vector3 direction = end - start;
	direction.normalise();

	// find hit point.
	Vector3 pos = start + direction * dist;

	// find intersection triangle normal.
	s_triacc.normal_ = s_decalClosestTri.hit_.normal();
	s_triacc.normal_.normalise();
	
	direction = s_triacc.normal_ * -1.f;
	pos -= direction * 0.5f * sizeCoeff;

	// find perpendicular vector with desired yaw angle to direction.
	Vector3 up(1.0f, 0.0f, 0.0f);

	if (almostZero(direction.dotProduct(Vector3(0.0f, 1.0f, 0.0f)), 0.01f))
		up = Vector3(0.0f, 1.0f, 0.0f);

	Matrix mat;
	mat.lookAt(Vector3(0.0f, 0.0f, 0.0f), direction, up);
	mat.postRotateZ(yaw); // consider a yaw angle.
	
	Vector3 xAxis	= Vector3(static_cast<const float*>(mat.column(0)));	// x-Axis
	Vector3	yAxis	= Vector3(static_cast<const float*>(mat.column(1)));	// y-Axis
	Vector3 zAxis	= Vector3(static_cast<const float*>(mat.column(2)));	// z-Axis
	
	xAxis *= halfSize.x;
	yAxis *= halfSize.y;

	Vector3 ext = zAxis * sizeCoeff;
	Vector3 topLeft		= pos - yAxis + xAxis;
	Vector3 topRight	= pos + yAxis + xAxis;
	Vector3 bottomLeft	= pos - yAxis - xAxis;
	Vector3 bottomRight = pos + yAxis - xAxis;

	if (mirrorU)
	{
		Vector3 tmp = topLeft;
		topLeft = topRight;
		topRight = tmp;
		
		tmp = bottomLeft;
		bottomLeft = bottomRight;
		bottomRight = tmp;
	}

	s_triacc.tris_.clear();
#ifdef EDITOR_ENABLED
	if (decalGroups_[groupIndex].useZeroYCollisionPlane)
	{
		topLeft.y = topRight.y = bottomLeft.y = bottomRight.y = 0;
		WorldTriangle tri1(topLeft, mirrorU ? topRight : bottomLeft, mirrorU ? bottomLeft : topRight);
		WorldTriangle tri2(topRight, mirrorU ? bottomRight : bottomLeft, mirrorU ? bottomLeft : bottomRight);
		s_triacc.tris_.push_back(tri1);
		s_triacc.tris_.push_back(tri2);
	}
	else
	{
#endif
	WorldTriangle tri1( topLeft, topRight, bottomLeft );
	WorldTriangle tri2( topRight, bottomRight, bottomLeft );
	pSpace->collide( tri1, ext + tri1.v0(), s_triacc  );
	size_t firstCount = s_triacc.tris_.size();
	pSpace->collide( tri2, ext + tri2.v0(), s_triacc  );
#ifdef EDITOR_ENABLED
	}
#endif

	Matrix m;
	m.translation( bottomLeft );
	m[0] = bottomRight - bottomLeft;
	m[1] = topLeft - bottomLeft;
	m[2] = zAxis * -1.f;
	m.invert();

	// update watcher variable.
	collisonTrianglesCountPerSecond_ += uint( s_triacc.tris_.size() );
	
	// skip decal if it's count of vertices greater then expected.
	if (s_triacc.tris_.size() * 3 > MAX_VERTICES_PER_DECAL)
	{
#ifndef CONSUMER_CLIENT
		PyErr_Format(PyExc_Warning, "[DECAL WARNING} Skipped decal by triangles count test(%d)\n",s_triacc.tris_.size());
		PyErr_PrintEx(0);
#endif
		return;
	}

	if(s_triacc.tris_.size())//sometimes there are no triangles
	{
		this->_addTriangles(&s_triacc.tris_[0], uint( s_triacc.tris_.size() ),
			m, textureIndex, groupIndex, textureIndex2, textureBlendFactor);
	}
}


// Algorithmically simplified version of method add.
void Decal::addWithGetH(
	const Vector3& start,
	const Vector3& end,
	const Vector2& size,
	float yaw,
	int textureIndex,
	int groupIndex,
	int textureIndex2,
	float textureBlendFactor)
{
		BW_GUARD;

		ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();
		if ( !pSpace.exists() )
			return;

		// find collision ray direction.
		Vector2 halfSize = size * 0.5f;
		float sizeCoeff = ((size.x > size.y) ? size.y : size.x);
		Vector3 direction = end - start;
		direction.normalise();

		// find hit point.
		Vector3 pos = start;
		// find perpendicular vector with desired yaw angle to direction.
		Vector3 up(1.0f, 0.0f, 0.0f);

		//if (almostZero(direction.dotProduct(Vector3(0.0f, 1.0f, 0.0f)), 0.01f))
		//	up = Vector3(0.0f, 1.0f, 0.0f);

		Matrix mat;
		mat.lookAt(Vector3(0.0f, 0.0f, 0.0f), direction, up);
		mat.postRotateZ(yaw); // consider a yaw angle.

		Vector3 xAxis(mat._11, mat._21, mat._31);	// x-Axis
		Vector3	yAxis(mat._12, mat._22, mat._32);	// y-Axis
		Vector3 zAxis(mat._13, mat._23, mat._33);	// z-Axis

		xAxis *= halfSize.x;
		yAxis *= halfSize.y;

		Vector3 ext = zAxis * sizeCoeff;
		Vector3 topLeft		= pos - yAxis + xAxis;
		Vector3 topRight	= pos + yAxis + xAxis;
		Vector3 bottomLeft	= pos - yAxis - xAxis;
		Vector3 bottomRight = pos + yAxis - xAxis;

		topLeft.y = Terrain::BaseTerrainBlock::getHeight(topLeft.x, topLeft.z);
		topRight.y = Terrain::BaseTerrainBlock::getHeight(topRight.x, topRight.z);
		bottomRight.y = Terrain::BaseTerrainBlock::getHeight(bottomRight.x, bottomRight.z);
		bottomLeft.y = topLeft.y + bottomRight.y - topRight.y;

		WorldTriangle tri[2] = {	WorldTriangle( topRight, topLeft, bottomLeft ),
									WorldTriangle( bottomRight, topRight, bottomLeft )};

		float texelSize = 1.0f / 64.0f; // will be divided by # of tiles in _addTriangles
		Vector2 uvValues[] = { Vector2(1.0f - texelSize, 1.0f - texelSize), Vector2(texelSize, 1.0f - texelSize), Vector2(texelSize, texelSize),
								Vector2(1.0f - texelSize, texelSize), Vector2(1.0f - texelSize, 1.0f - texelSize), Vector2(texelSize, texelSize) };

		Matrix m;
		m.translation( bottomLeft );
		m[0] = bottomRight - bottomLeft;
		m[1] = topLeft - bottomLeft;
		m[2] = zAxis * -1.f;
		m.invert();

		this->_addTriangles(tri, 2, m, textureIndex, groupIndex, textureIndex2, textureBlendFactor, uvValues);
}




/**
 *	This function attempts to add a decal to the scene, given its final
 *	tesselated triangle composition. 
 *
 *	@param triangles	pointer to array of world triangles the compose the decal.
 *	@param numTriangles	number of triangles in the array.
 *	@param transform	The UV transform to be used when setting up the tex coords.
 *	@param textureIndex	The index of the decal texture.  @see BigWorld.decalTextureIndex
 */
void Decal::_addTriangles(
	const WorldTriangle * triangles, 
	uint numTriangles, 
	const Matrix & transform,
	int textureIndex,
	int groupIndex,
	int textureIndex2,
	float textureBlendFactor,
	Vector2 *uvValues)
{
	BW_GUARD;

	// temporary vertices array.
	BW::vector<DecalVertex> tmpDecalVertices_;
	tmpDecalVertices_.reserve(MAX_VERTICES_PER_DECAL / 2);

	const TileTexture::SubTexture& subTex = s_decalTexture->subTexture( textureIndex );
	const TileTexture::SubTexture& subTex2 = s_decalTexture->subTexture( textureIndex2 );

	// create decal descriptor.
	DecalDesc decalDesc;
	decalDesc.time = elapsedTime_;

	// allocate capacity for 3 vertices.
	BW::vector<ClipVertex> cvs;
	cvs.reserve( 3 );
	DecalVertex tmpDecalVertex;

	float uSize = s_decalTexture->uSize();
	float vSize = s_decalTexture->vSize();

	uint j = 0;
	for (uint i = 0; i < numTriangles; ++i)
		{
		cvs.erase(cvs.begin(), cvs.end());
			ClipVertex cv;

			const WorldTriangle& wt = triangles[i];
			Vector3 uv;

			cv.pos_ = wt.v0();
		if (uvValues != NULL)
			cv.uv_ = uvValues[j++];
		else
		{
			uv = transform.applyPoint(wt.v0());
			cv.uv_.x = uv.x;
			cv.uv_.y = 1.f - uv.y;
		}
		cvs.push_back( cv );

		cv.pos_ = wt.v1();
		if (uvValues != NULL)
			cv.uv_ = uvValues[j++];
		else
		{
			uv = transform.applyPoint(wt.v1());
			cv.uv_.x = uv.x;
			cv.uv_.y = 1.f - uv.y;
		}
		cvs.push_back( cv );

		cv.pos_ = wt.v2();
		if (uvValues != NULL)
			cv.uv_ = uvValues[j++];
		else
		{
			uv = transform.applyPoint(wt.v2());
			cv.uv_.x = uv.x;
			cv.uv_.y = 1.f - uv.y;
		}
		cvs.push_back( cv );

		// skip add extra vertices used to compensate texture clipping if UVs are assigned directly
		if (uvValues == NULL)
			_clip( cvs );

		if (cvs.size() > 2)
		{
			for (uint32 i = 1; i < (cvs.size()-1); ++i)
			{
				tmpDecalVertex.pos_ = cvs[0].pos_;
				tmpDecalVertex.uv_.set( 
					cvs[0].uv_.x * uSize + subTex.uOffset_,
					cvs[0].uv_.y * vSize + subTex.vOffset_,
					cvs[0].uv_.x * uSize + subTex2.uOffset_,
					cvs[0].uv_.y * vSize + subTex2.vOffset_
					);
				tmpDecalVertex.time_.set(decalDesc.time, textureBlendFactor);

				tmpDecalVertices_.push_back(tmpDecalVertex);

				tmpDecalVertex.pos_ = cvs[i].pos_;
				tmpDecalVertex.uv_.set( 
					cvs[i].uv_.x * uSize + subTex.uOffset_,
					cvs[i].uv_.y * vSize + subTex.vOffset_,
					cvs[i].uv_.x * uSize + subTex2.uOffset_,
					cvs[i].uv_.y * vSize + subTex2.vOffset_
					);
				tmpDecalVertex.time_.set(decalDesc.time, textureBlendFactor);

				tmpDecalVertices_.push_back(tmpDecalVertex);

				tmpDecalVertex.pos_ = cvs[i + 1].pos_;
				tmpDecalVertex.uv_.set( 
					cvs[i+1].uv_.x * uSize + subTex.uOffset_,
					cvs[i+1].uv_.y * vSize + subTex.vOffset_,
					cvs[i+1].uv_.x * uSize + subTex2.uOffset_,
					cvs[i+1].uv_.y * vSize + subTex2.vOffset_
					);
				tmpDecalVertex.time_.set(decalDesc.time, textureBlendFactor);

				tmpDecalVertices_.push_back(tmpDecalVertex);
			}
		}
	}

	DecalGroup& decalGroup	= decalGroups_[groupIndex];
	uint decalVertSize		= uint( tmpDecalVertices_.size() );

	// skip decal if it's count of vertices great then expected.
	if (decalVertSize > MAX_VERTICES_PER_DECAL ||decalVertSize > decalGroup.trianglesCount)
		return;


	uint nCurrentTotalVertices;
	if ( decalGroup.lastVertex >= decalGroup.firstVertex )
		nCurrentTotalVertices = decalGroup.lastVertex - decalGroup.firstVertex;
	else
	{
		nCurrentTotalVertices = decalGroup.trianglesCount * 3 - decalGroup.firstVertex;
		nCurrentTotalVertices += decalGroup.lastVertex;
	}

	while ( nCurrentTotalVertices + decalVertSize >= decalGroup.trianglesCount * 3 )
	{
		DecalDescArray &descArray = decalGroup.decalDescArray;
		if( descArray.empty() ) return; // should not happen ever
		decalGroup.firstVertex += descArray.begin()->verticesCount;
		nCurrentTotalVertices -= descArray.begin()->verticesCount;
		descArray.erase( descArray.begin() );
		if ( decalGroup.firstVertex > decalGroup.trianglesCount * 3 )
			decalGroup.firstVertex -= decalGroup.trianglesCount * 3;
	}

	if ( decalVertSize + decalGroup.lastVertex > decalGroup.trianglesCount * 3 )
	{
		DWORD flags = D3DLOCK_NOOVERWRITE;
		if (Moo::rc().deviceInfo( Moo::rc().deviceIndex() ).compatibilityFlags_ & Moo::COMPATIBILITYFLAG_NOOVERWRITE)
			flags = 0;

		uint nFirstVertices = decalGroup.trianglesCount * 3 - decalGroup.lastVertex;
		if(nFirstVertices>0)
		{
			Moo::VertexLock<DecalVertex> vl(
				*decalGroup.pVertexBuffer,
				decalGroup.lastVertex * sizeof(DecalVertex),
				nFirstVertices * sizeof(DecalVertex),
				flags
				);
			if ( !vl )
				return;
			memcpy( vl, &tmpDecalVertices_[0], nFirstVertices * sizeof(DecalVertex) );
		}else
			return;

		uint nSecondVertices = decalVertSize - nFirstVertices;
		{
			Moo::VertexLock<DecalVertex> vl(
				*decalGroup.pVertexBuffer,
				0,
				nSecondVertices * sizeof(DecalVertex),
				flags
				);
			if ( !vl )
				return;
			memcpy( vl, &tmpDecalVertices_[nFirstVertices], nSecondVertices * sizeof(DecalVertex) );
			}
		decalGroup.lastVertex = nSecondVertices;
	}
	else
	{
		// D3DLOCK_NOOVERWRITE does not seem to work on gf4mx cards
		DWORD flags = D3DLOCK_NOOVERWRITE;
		if (Moo::rc().deviceInfo( Moo::rc().deviceIndex() ).compatibilityFlags_ & Moo::COMPATIBILITYFLAG_NOOVERWRITE)
			flags = 0;

		// create the vertex buffer lock helper.
		Moo::VertexLock<DecalVertex> vl(
			*decalGroup.pVertexBuffer,
			decalGroup.lastVertex * sizeof(DecalVertex),
			decalVertSize * sizeof(DecalVertex),
			flags
			);
		if ( !vl )
			return;

		memcpy(vl, &tmpDecalVertices_[0], decalVertSize * sizeof(DecalVertex));
		decalGroup.lastVertex += decalVertSize;
	}

	// set vertices count.
	decalDesc.verticesCount = decalVertSize;

	// add new decal descriptor.
	decalGroup.decalDescArray.push_back(decalDesc);
}

/**
 * Create decal's graphics resources.
 */
void Decal::createUnmanagedObjects()
{
	BW_GUARD;
		
	// create vertex buffers.
	for (uint i = 0; i < decalGroups_.size(); ++i)
	{
		if (!decalGroups_[i].pVertexBuffer->valid())
		{
			HRESULT hr = decalGroups_[i].pVertexBuffer->create(
				decalGroups_[i].trianglesCount * 3 * sizeof( DecalVertex ),
				D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
				DecalVertex::fvf(),
				D3DPOOL_DEFAULT);

			if FAILED( hr )
			{
				DEBUG_MSG( "Decal::init - could not create"
					" vertex buffer for decal group '%s'. ( error %lx )\n", decalGroups_[i].name.c_str() , hr );
				return;
			}

			++buffersCount_;
			buffersMemoryAllocated_ += decalGroups_[i].trianglesCount * 3 * sizeof( DecalVertex ) / 1024;
		}
	}

	this->clear();
}

/**
 * Destroy decal's graphics resources.
 */
void Decal::deleteUnmanagedObjects()
{
   BW_GUARD;
	for (uint i = 0; i < decalGroups_.size(); ++i)
	    if (decalGroups_[i].pVertexBuffer->valid())
			decalGroups_[i].pVertexBuffer->release();

	// reset watcher variables.
	buffersMemoryAllocated_ = 0;
	buffersCount_ = 0;
	
	// clear current state.
	clear();
}

/**
 * Clear decal object, setting decal vertices to zero.
 */
void Decal::clear()
{
	BW_GUARD;
	for (uint i = 0; i < decalGroups_.size(); ++i)
	{
		decalGroups_[i].firstVertex = 0;
		decalGroups_[i].decalDescArray.clear();
		decalGroups_[i].lastVertex = 0; 
	}

	// reset timer.
	elapsedTime_ = 0.0f;
}

// Validate group name and return group index in array or -1 if it is not exist.
//------------------------------------------
int Decal::validateGroupName(const BW::string& groupName)
{
	BW_GUARD;

	for (uint i = 0; i < decalGroups_.size(); ++i)
		if (groupName.compare(decalGroups_[i].name) == 0)
			return i;

	return -1;
}

// Update buffer start offset.
//------------------------------------------
void Decal::tick(float dTime)
{
	BW_GUARD_PROFILER( Decal_tick );

	curTickUpdateTime_ += dTime;
	if (curTickUpdateTime_ >= tickUpdateInterval_)
	{
		curTickUpdateTime_ = 0.0f;
		for (uint i = 0; i < decalGroups_.size(); ++i)
		{
			uint& firstVertex		= decalGroups_[i].firstVertex;
			DecalDescArray& descArray	= decalGroups_[i].decalDescArray;
			float lifeTime				= decalGroups_[i].lifeTime;

			DecalDescArray::iterator it = descArray.begin();
			while( it != descArray.end() )
			{
				if ( elapsedTime_ - it->time >= lifeTime )
				{
					firstVertex += it->verticesCount;
					it = descArray.erase( it );
				}
				else break;
			}
			if ( firstVertex > decalGroups_[i].trianglesCount * 3 )
				firstVertex -= decalGroups_[i].trianglesCount * 3;
		}
	}

	// update watcher variables.
	watcherTickTime_ += dTime;
	if (watcherTickTime_ >= 1.0f)
	{
		watcherTickTime_ = 0.0f;
		collisonTrianglesCountPerSecond_ = 0;
	}
}

/**
 * Draw this decal with decal texture as a triangle list.
 */
void Decal::draw( float dTime )
{
	BW_GUARD;
	return;

	if (pMaterial_.hasObject() && s_decalTexture->pTexture() && decalGroups_.size())
	{
		renderedTrianglesCountPerFrame_ = 0;

		Moo::rc().setFVF( DecalVertex::fvf() );

		if (Moo::rc().mixedVertexProcessing())
		{
			Moo::rc().setVertexShader( NULL );
			Moo::rc().device()->SetVertexShader( NULL );
			Moo::rc().device()->SetSoftwareVertexProcessing(FALSE);
		}	

		elapsedTime_ += dTime;
		
		pMaterial_->pEffect()->pEffect()->SetTexture( "decalMap", s_decalTexture->pTexture() );
		pMaterial_->pEffect()->pEffect()->SetFloat( "g_time", elapsedTime_);

#ifdef EDITOR_ENABLED
		Matrix mView = Moo::rc().view();
#endif

		if (pMaterial_->begin())
		{
			// draw all buffers.
			for (uint i = 0; i < decalGroups_.size(); ++i)
			{
				DecalGroup& decalGroup = decalGroups_[i];
				if ( decalGroup.firstVertex != decalGroup.lastVertex )
			{
#ifdef EDITOR_ENABLED
					Matrix m = mView;
					m.preTranslateBy(decalGroup.viewSpaceOffset);
					pMaterial_->pEffect()->pEffect()->SetMatrix("view", &m);
					m.postMultiply(Moo::rc().projection());
					pMaterial_->pEffect()->pEffect()->SetMatrix("viewProjection", &m);
#endif
					pMaterial_->pEffect()->pEffect()->SetFloat( "g_lifeTime", decalGroup.lifeTime );
					decalGroup.pVertexBuffer->set( 0, 0, sizeof( DecalVertex ) );


					for (uint32 j = 0; j < pMaterial_->numPasses(); ++j)
					{
						pMaterial_->beginPass(j);

						if ( decalGroup.lastVertex > decalGroup.firstVertex )
						{
							uint trianglesCount = ( decalGroup.lastVertex - decalGroup.firstVertex ) / 3;
							Moo::rc().drawPrimitive(D3DPT_TRIANGLELIST, decalGroup.firstVertex, trianglesCount );
							renderedTrianglesCountPerFrame_ += trianglesCount;
						}
						else
						{
							uint trianglesCount = decalGroup.trianglesCount - decalGroup.lastVertex / 3;
							Moo::rc().drawPrimitive(D3DPT_TRIANGLELIST, decalGroup.lastVertex, trianglesCount );
							renderedTrianglesCountPerFrame_ += trianglesCount;
							trianglesCount = decalGroup.firstVertex / 3;
							Moo::rc().drawPrimitive(D3DPT_TRIANGLELIST, 0, trianglesCount );
							renderedTrianglesCountPerFrame_ += trianglesCount;
						}
				pMaterial_->endPass();
			}
				}
			}
			pMaterial_->end();
		}		

		if (Moo::rc().mixedVertexProcessing())
			Moo::rc().device()->SetSoftwareVertexProcessing(TRUE);
	}
}

// Add the new decal group.
// Note: All group deleted automatically in Decal class destructor.
//------------------------------------------
int Decal::addDecalGroup(
	const BW::string& groupName,
	float lifeTime,
	uint trianglesCount )
{
	BW_GUARD;

	uint triCount = trianglesCount;

	if (lifeTime == 0.0f)
	{
		PyErr_Format( PyExc_RuntimeError, " Decal::addDecalGroup - decal group life time is zero.\n");
		return 1;
	}

	if (triCount < MAX_VERTICES_PER_DECAL)
		triCount = MAX_VERTICES_PER_DECAL;

	for (uint i = 0; i < decalGroups_.size(); ++i)
	{
		if (groupName.compare(decalGroups_[i].name) == 0)
		{
			//PyErr_Format( PyExc_RuntimeError, " Decal::addDecalGroup - decal group '%s' already exist.\n", groupName.c_str() );
			return 0; // not an error
		}

		if (lifeTime == decalGroups_[i].lifeTime)
{
			//PyErr_Format( PyExc_RuntimeError, " Decal::addDecalGroup - decal group"
			//	"with life time %f already exist. His name is '%s'\n", lifeTime, decalGroups_[i].name.c_str() );
			return 0; // not an error
		}
	}
	
	// wrap pointer to safety.
	std::auto_ptr<Moo::VertexBuffer> vertexBuffer(new Moo::VertexBuffer());

	DecalGroup decalGroup;
	decalGroup.name = groupName;
	decalGroup.trianglesCount = triCount;
	decalGroup.lifeTime = lifeTime;
	
	// reserve array size for decal's description.
//	decalGroup.decalDescArray.reserve(50);
	
	// pick the smallest life time as update interval.
	if (tickUpdateInterval_ != 0.0f && (decalGroup.lifeTime < tickUpdateInterval_))
		tickUpdateInterval_ = decalGroup.lifeTime;

	if (!vertexBuffer->valid())
	{
		HRESULT hr = vertexBuffer->create(
			triCount * 3 * sizeof( DecalVertex ),
			D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
			DecalVertex::fvf(),
			D3DPOOL_DEFAULT);

		if FAILED( hr )
	{
			DEBUG_MSG( "Decal::addDecalGroup "
				"- could not create vertex buffer for group '%s'. ( error %lx )\n", decalGroup.name.c_str(), hr );
			return 1;
	}

		++buffersCount_;
		buffersMemoryAllocated_ += triCount * 3 * sizeof( DecalVertex ) / 1024;
	}

	// release auto_ptr.
	decalGroup.pVertexBuffer = vertexBuffer.release();
	decalGroups_.push_back(decalGroup);

	return 0;
}

#ifdef EDITOR_ENABLED
void Decal::setViewSpaceOffset(uint groupIndex, const Vector3 &offset)
{
	if (groupIndex < decalGroups_.size())
		decalGroups_[groupIndex].viewSpaceOffset = offset;
}

void Decal::useZeroYCollisionPlane(uint groupIndex, bool use)
{
	if (groupIndex < decalGroups_.size())
		decalGroups_[groupIndex].useZeroYCollisionPlane = use;
}
#endif


/*~ function BigWorld.decalTextureIndex
 *
 *	@param textureName The name of the texture to look up.
 *  @return The index of the texture
 *
 *	This function returns the texture index that is needed to pass into
 *	addDecal.  It takes the name of the texture, and returns the index,
 *	or -1 if none was found.
 */
/**
 *	Look up the decal texture index for the given texture name.
 */
int Decal::decalTextureIndex( const BW::string& textureName  )
{
	BW_GUARD;
	EnsureDecalTexture();
	return s_decalTexture->textureIndex( textureName );
}
//

BW_END_NAMESPACE

// decal.cpp
