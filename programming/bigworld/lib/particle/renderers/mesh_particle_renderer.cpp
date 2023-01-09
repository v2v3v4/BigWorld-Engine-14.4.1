#include "pch.hpp"
#include "mesh_particle_renderer.hpp"
#include "particle/particle_serialisation.hpp"
#include "particle/particle_system_manager.hpp"
#include "moo/render_context.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/visual_manager.hpp"
#include "moo/moo_math.hpp"
#include "moo/primitive_file_structs.hpp"
#include "resmgr/multi_file_system.hpp"
#include "moo/reload.hpp"
#include "moo/draw_context.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

const BW::string MeshParticleRenderer::nameID_ = "MeshParticleRenderer";

static Moo::VisualHelper s_helper;


// -----------------------------------------------------------------------------
//	Section - MeshParticleRenderer
// -----------------------------------------------------------------------------
/**
 *	Constructor.
 */
MeshParticleRenderer::MeshParticleRenderer():	
	material_( NULL ),
	doubleSided_( false ),    
	sortType_( SORT_QUICK ),
	materialFX_( FX_SOLID ),
	materialNeedsUpdate_( false ),
	primGroup_()
{
}


/**
 *	Destructor.
 */
MeshParticleRenderer::~MeshParticleRenderer()
{
}


/**
 *	This method returns true if the visual is suitable to be displayed
 *	by this renderer.  The MeshParticleRenderer requires visuals to be
 *	exported using the "MeshParticle" option in the exporter.
 *
 *	@param	v	name of the visual to check
 *	@return bool true iff the visual is of the required type.
 */
bool MeshParticleRenderer::isSuitableVisual( const BW::StringRef & v )
{
	BW_GUARD;
	bool isSuitable = false;

    // Go via the base class to load the visual, since name mangling must
    // be done due to particle visuals and model visuals not being 
    // compatible.
	Moo::VisualPtr spVisual = Moo::VisualManager::instance()->get(v);
	if (!spVisual)
	{
		// Couldn't open visual file. No mesh will be loaded.
		return false;
	}

	Moo::VerticesPtr verts;
	Moo::PrimitivePtr prim;
	Moo::Visual::PrimitiveGroup* primGroup;

	//The vertex format must be xyznuvi, with the visual being exported
	//specifically as mesh particles, so it can be drawn in batches of MAX_MESHES
	if (spVisual->primGroup0( verts, prim, primGroup) &&
		verts->sourceFormat() == "xyznuvi")
	{		
		if (prim->nPrimGroups() == PARTICLE_MAX_MESHES + 1)
		{
			isSuitable = true;
		}
		else
		{
			INFO_MSG( "Visual %s should be re-exported as Mesh Particles to"
				" enable correct sorting.\n", v.to_string().c_str() );			
		}
	}

	spVisual = NULL;
	return isSuitable;
}


#ifdef EDITOR_ENABLED
/**
 *  This class is used to quickly check if a primitive file is of a desired
 *  format, without reloading it.
 */
class PrimitiveFileHeader
{
public:
	PrimitiveFileHeader( const BW::StringRef & visualName ) :
		fileExists_( false )
	{

		BW_GUARD;
		BW::string filename = BWResource::changeExtension( BWResource::resolveFilename( visualName ), ".primitives" );

		BinaryPtr pVerticesBin = BWResource::instance().rootSection()->readBinary( filename + "/vertices" );

		if (pVerticesBin && (pVerticesBin->len() > sizeof(vertHeader_)))
		{
			memcpy( &vertHeader_, pVerticesBin->data(), sizeof(vertHeader_) );
			fileExists_ = true;
		}
	}

	bool isType( const BW::StringRef & fmt )
	{
		return fileExists_ && BW::StringRef( vertHeader_.vertexFormat_ ) == fmt;
	}

private:
	bool fileExists_;
	Moo::VertexHeader vertHeader_;
};


/**
 *	This method quickly checks if vertex format is xyznuvi via opening .primitives file
 *  directly.
 */
bool MeshParticleRenderer::quickCheckSuitableVisual( const BW::StringRef & v )
{

	PrimitiveFileHeader header( v );

	return header.isType( "xyznuvi" );

}

#endif

/**
 *	This method sets the visual name for the renderer.
 *	Note : isSuitableVisual should be called first, unless you are sure its ok.
 *
 *	@param	v	name of the visual.
 *	@see	MeshParticleRenderer::isSuitableVisual
 */
void MeshParticleRenderer::visual( const BW::StringRef & v )
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	if (MeshParticleRenderer::isSuitableVisual(v))
#endif
	{        
		visualName_.assign( v.data(), v.size() );
		pVisual_ = Moo::VisualManager::instance()->get(visualName_);

		if (!pVisual_)
		{
			// Mesh Particle Visual file not found.
			return;
		}

		// Mutually exclusive of pVisual_ reloading, start
		pVisual_->beginRead();

		this->pullInfoFromVisual();

		materialNeedsUpdate_ = true;

		pVisual_->registerListener( this );	

		// Mutually exclusive of pVisual_ reloading, end
		pVisual_->endRead();
	}
}

/**
 *	This method pull the info from our pVisual_
 */
void MeshParticleRenderer::pullInfoFromVisual()
{
	BW_GUARD;
	Moo::Visual::ReadGuard guard( pVisual_.get() );
	//find texture name in this visual
	Moo::VerticesPtr verts;
	Moo::PrimitivePtr prim;
	Moo::Visual::PrimitiveGroup* primGroup;
	//The vertex format must be xyznuvi, with the visual being exported
	//specifically as mesh particles, so it can be drawn in batches of MAX_MESHES
	textureName_ = "";
	if (pVisual_ && pVisual_->primGroup0(verts,prim,primGroup) && 
		primGroup != NULL)
	{
		Moo::EffectPropertyPtr pProp = primGroup->material_->baseMaterial()->getProperty( "diffuseMap" );
		if (pProp)
		{
			pProp->getResourceID( textureName_ );
		}
	}
}
/**
 * if our visual has been reloaded, 
 * update the related data.
 */
void MeshParticleRenderer::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	if (pReloader == pVisual_.get())
	{
		this->pullInfoFromVisual();
		this->updateMaterial();
	}
}


/**
 *	This class sets up to PARTICLE_MAX_MESHES world transforms on an effect.
 */
class ParticleWorldPalette : public Moo::EffectConstantValue
{
public:
	ParticleWorldPalette()
	{
		transforms_.resize( PARTICLE_MAX_MESHES );
	}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetVectorArray( constantHandle, vectorArray_, PARTICLE_MAX_MESHES*3 );
		pEffect->SetArrayRange( constantHandle, 0, PARTICLE_MAX_MESHES*3 );
		return true;
	}

	void transform( int i, const Matrix& orig )
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(i<PARTICLE_MAX_MESHES)
		{
			return;
		}
		transforms_[i] = orig;
		Matrix m;
		XPMatrixTranspose( &m, &orig );
		int constant = i*3;
		vectorArray_[constant++] = m.row(0);
		vectorArray_[constant++] = m.row(1);
		vectorArray_[constant++] = m.row(2);
	}

	void nullTransforms(int base, int last = PARTICLE_MAX_MESHES)
	{
		BW_GUARD;
		if (base>=last)
			return;
		char* ptr = (char*)&vectorArray_[base*3];
		memset(ptr, 0, (last-base)*3*sizeof(Vector4));

		ptr = (char*)&transforms_[base];
		memset(ptr, 0, (last-base)*sizeof(Matrix));
	}

	const BW::vector<Matrix>& transforms() const	{ return transforms_; }
private:
	Vector4 vectorArray_[PARTICLE_MAX_MESHES*3];
	BW::vector<Matrix> transforms_;	//duplicate kept for vertex->getSnapshot
};

static SmartPointer<ParticleWorldPalette> s_particleWorldPalette = new ParticleWorldPalette;


/**
 *	This class sets up to PARTICLE_MAX_MESHES tints on an effect.
 */
class ParticleTintPalette : public Moo::EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;
		pEffect->SetVectorArray( constantHandle, tints_, PARTICLE_MAX_MESHES );
		pEffect->SetArrayRange( constantHandle, 0, PARTICLE_MAX_MESHES );
		return true;
	}

	void tint( int i, const Vector4& t )
	{
		BW_GUARD;
		IF_NOT_MF_ASSERT_DEV(i<PARTICLE_MAX_MESHES)
		{
			return;
		}
		tints_[i] = t;
	}	
private:
	Vector4 tints_[PARTICLE_MAX_MESHES];
};

static SmartPointer<ParticleTintPalette> s_particleTintPalette = new ParticleTintPalette;


/**
 *	This method views the given particle system using this renderer.
 *
 *	@param	worldTransform		world transform of the particle system.
 *	@param	beg					first particle to draw
 *	@param	end					end of particle vector to draw
 *	@param	inbb				bounding box of particle system.
 */
void MeshParticleRenderer::draw( Moo::DrawContext& drawContext,
								const Matrix& worldTransform,
								Particles::iterator beg,
								Particles::iterator end,
								const BoundingBox & inbb )
{
	BW_GUARD;
	if ( beg == end ) return;
	if ( !pVisual_ ) return;

	if (materialNeedsUpdate_)
		this->updateMaterial();

	// maps managed effect channel type to draw context channel mask
	const Moo::DrawContext::ChannelMask channelMaskMapper[Moo::NUM_CHANNELS] = 
	{
		Moo::DrawContext::OPAQUE_CHANNEL_MASK,
		Moo::DrawContext::TRANSPARENT_CHANNEL_MASK,
		Moo::DrawContext::TRANSPARENT_CHANNEL_MASK,
		Moo::DrawContext::SHIMMER_CHANNEL_MASK,
		Moo::DrawContext::SHIMMER_CHANNEL_MASK
	};

	MF_ASSERT(material_->channelType() < ARRAY_SIZE(channelMaskMapper));

	if (!(channelMaskMapper[material_->channelType()] & drawContext.collectChannelMask()))
	{
		// no reason to prepare particle data just to get draw context reject it
		return;
	}

	static Moo::EffectConstantValuePtr* pPaletteConstant_ = NULL;
	static Moo::EffectConstantValuePtr* pTintPaletteConstant_ = NULL;
	if ( !pPaletteConstant_ )
	{
		pPaletteConstant_ = Moo::rc().effectVisualContext().getMapping( "WorldPalette" );
		pTintPaletteConstant_ = Moo::rc().effectVisualContext().getMapping( "TintPalette" );
	}

	bool current = Moo::rc().effectVisualContext().overrideConstants();
	Moo::rc().effectVisualContext().overrideConstants(false);
	Moo::rc().effectVisualContext().initConstants();
	Moo::rc().effectVisualContext().overrideConstants(true);
	*pPaletteConstant_ = s_particleWorldPalette;
	*pTintPaletteConstant_ = s_particleTintPalette;

	//TODO : adjust inbb for size of particles.  when loading mesh particles, calculate
	//radius of each mesh piece (to account for spinning particles) and use radius
	//to add to bounding box.
	if (this->beginDraw( inbb, worldTransform))
	{
		if (materialFX_ == FX_SOLID)
		{
			this->drawOptimised(drawContext,worldTransform,beg,end,inbb,false);
		}
		else
		{		
			switch (sortType_)
			{
			case SORT_NONE:
				this->drawOptimised(drawContext,worldTransform,beg,end,inbb,false);
				break;
			case SORT_QUICK:
				this->drawOptimised(drawContext,worldTransform,beg,end,inbb,true);
				break;
			case SORT_ACCURATE:
				this->drawSorted(drawContext,worldTransform,beg,end,inbb);			
				break;
			}
		}		
	}

	//note - should always be called even if begin fails.
	this->endDraw(inbb);

	Moo::rc().effectVisualContext().overrideConstants(current);
	Moo::rc().effectVisualContext().initConstants();
}

class MeshParticleDrawItem : public Moo::DrawContext::UserDrawItem
{
public:
	MeshParticleDrawItem( Moo::VerticesPtr vertices, Moo::PrimitivePtr primitive,
						Moo::EffectMaterialPtr material, const Matrix& worldTransform );
	~MeshParticleDrawItem();
	virtual void draw();
	virtual void fini() { delete this; }

	SmartPointer<ParticleWorldPalette>	particleWorldPalette_;
	SmartPointer<ParticleTintPalette>	particleTintPalette_;

	Moo::VerticesPtr				vertices_;
	Moo::PrimitivePtr				primitive_;
	uint32							primGroupIndex_;
	Moo::EffectMaterialPtr			material_;
	Matrix							worldTransform_;
};

MeshParticleDrawItem::MeshParticleDrawItem(Moo::VerticesPtr vertices, Moo::PrimitivePtr primitive,
										   Moo::EffectMaterialPtr material, const Matrix& worldTransform ):
	vertices_( vertices ),
	primitive_( primitive ),
	primGroupIndex_( 0 ),
	material_( material ),
	worldTransform_( worldTransform )
{
	BW_GUARD;
	particleWorldPalette_ = new ParticleWorldPalette;
	particleTintPalette_ = new ParticleTintPalette;
}

MeshParticleDrawItem::~MeshParticleDrawItem()
{
}

void MeshParticleDrawItem::draw()
{
	// setup captured palettes
	Moo::EffectConstantValuePtr* worldPaletteMapping =  Moo::rc().effectVisualContext().getMapping( "WorldPalette" );
	Moo::EffectConstantValuePtr curWorldPaletteMapping = *worldPaletteMapping;
	*worldPaletteMapping = particleWorldPalette_;
	Moo::EffectConstantValuePtr* tintPaletteMapping =  Moo::rc().effectVisualContext().getMapping( "TintPalette" );
	*tintPaletteMapping = particleTintPalette_.get();

	Moo::rc().push();
	Moo::rc().world( worldTransform_ );
	vertices_->setVertices( Moo::rc().mixedVertexProcessing() );
	primitive_->setPrimitives();

	//-- now check status of the material for current rendering pass.
	if (material_->begin())
	{
		for (uint32 i = 0; i < material_->numPasses(); i++)
		{
			material_->beginPass( i );
			primitive_->drawPrimitiveGroup( primGroupIndex_ );
			material_->endPass();
		}
		material_->end();
	}

	Moo::rc().pop();

	*worldPaletteMapping = curWorldPaletteMapping;
	*tintPaletteMapping = NULL;
}

MeshParticleDrawItem* makeMeshParticleDrawItem(const Moo::VerticesPtr& verts,
							const Moo::PrimitivePtr& prim,
							const Moo::ComplexEffectMaterialPtr& material,
							const Matrix& matrix,
							Moo::DrawContext& drawContext )
{
	BW_GUARD;
	return new MeshParticleDrawItem( verts, prim,
		material->pass( drawContext.renderingPassType() ),
		matrix );

}

/**
 *	This draw method draws a mesh in optimised fashion, i.e. 15 pieces
 *	at a time, and sorted ( optionally ) quickly.
 *
 *	@see MeshParticleRenderer::draw
 */
void MeshParticleRenderer::drawOptimised( Moo::DrawContext& drawContext,
	const Matrix& worldTransform,
	Particles::iterator beg,
	Particles::iterator end,
	const BoundingBox & inbb,
	const bool isSorting )
{
	BW_GUARD;
	size_t nParticles = end - beg;
	Matrix particleTransform;

	Particle* particle;
	Particle* endParticle;
	if (isSorting)
	{
		sorter_.sortOptimised( beg, end );
		particle = sorter_.beginOptimised( &*beg );	
		endParticle = NULL;
	}
	else
	{
		particle = &(*beg);
		endParticle = particle + (end - beg);
	}

	while ( particle != endParticle )
	{		
		int idx = 0;
		BoundingBox bb( BoundingBox::s_insideOut_ );

		SmartPointer<ParticleWorldPalette> pParticleWorldPalette = s_particleWorldPalette;
		SmartPointer<ParticleTintPalette> pParticleTintPalette = s_particleTintPalette;

		MeshParticleDrawItem* drawItem = NULL;
		if (materialFX_ != FX_SOLID)
		{
			drawItem = makeMeshParticleDrawItem(verts_, prim_, material_, local() ? worldTransform : Matrix::identity, drawContext );
			pParticleWorldPalette = drawItem->particleWorldPalette_;
			pParticleTintPalette = drawItem->particleTintPalette_;
		}
		//loop through PARTICLE_MAX_MESHES particles at a time...
		while ( (particle != endParticle) && (idx<PARTICLE_MAX_MESHES) )
		{		
			if (particle->isAlive())
			{				
				//copy particle transforms into constants						
				particleTransform.setRotate( particle->yaw(), particle->pitch(), 0.f );
				particleTransform.translation( particle->position() );			
				bb.addBounds( particle->position() );

				// Add spin
				float spinSpeed = particle->meshSpinSpeed();
				if (spinSpeed != 0.f )
				{
					Vector3 spinAxis = particle->meshSpinAxis();
					Matrix spin;

					D3DXMatrixRotationAxis
					( 
						&spin, 
						&spinAxis, 
						spinSpeed * PARTICLE_MESH_MAX_SPIN * (particle->age() - particle->meshSpinAge()) 
					);

					particleTransform.preMultiply( spin );
				}

				if (local())
					particleTransform.postMultiply(worldTransform);

				pParticleWorldPalette->transform(idx, particleTransform);
				pParticleTintPalette->tint(idx, Colour::getVector4Normalised(particle->colour()));
			}
			else
			{
				pParticleWorldPalette->nullTransforms(idx,idx+1);
			}
			idx++;
			if (isSorting)
			{
				particle = sorter_.nextOptimised( &*beg );
			}
			else
			{
				particle++;
			}
		}

		//draw this set
		if ( idx>0 && !bb.insideOut() )
		{
			pParticleWorldPalette->nullTransforms(idx);	
			if (materialFX_ != FX_SOLID)
			{
				float distance = Moo::rc().view().applyPoint(bb.centre()).z;
				drawContext.drawUserItem( drawItem,
					Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );
			}
			else
			{
				MF_ASSERT( !drawItem );
				this->draw( drawContext, bb, NULL );
			}
		}
		else
		{
			bw_safe_delete( drawItem );
		}
	}
}


/**
 *	This method sorts all of the particles, and draws each particle
 *	one by one.  It is the slowest way to draw, but the sorting will
 *	be as correct as possible.
 *
 *	@see MeshParticleRenderer::draw
 */
void MeshParticleRenderer::drawSorted( Moo::DrawContext& drawContext,
	const Matrix& worldTransform,
	Particles::iterator beg,
	Particles::iterator end,
	const BoundingBox & inbb )
{
	BW_GUARD;
	Matrix nullMatrix;
	memset( nullMatrix, 0, sizeof( Matrix ) );	

	size_t nParticles = end - beg;
	int particleType = 0;
	Matrix particleTransform;

	sorter_.sort( beg, end );

	Particle *particle = sorter_.begin( &*beg, particleType );	

	s_particleWorldPalette->nullTransforms(0);

	while ( particle != NULL )
	{
		if (particle->isAlive())
		{
			BoundingBox bb( BoundingBox::s_insideOut_ );
			bb.addBounds( particle->position() );

			// Set particle transform as world matrix
			particleTransform.setRotate( particle->yaw(), particle->pitch(), 0.f );
			particleTransform.translation( particle->position() );
			// Add spin:
			float spinSpeed = particle->meshSpinSpeed();
			if (spinSpeed != 0.f )
			{
				Vector3 spinAxis = particle->meshSpinAxis();
				Matrix spin;

				D3DXMatrixRotationAxis
				( 
					&spin, 
					&spinAxis, 
					spinSpeed * PARTICLE_MESH_MAX_SPIN * (particle->age() - particle->meshSpinAge()) 
				);

				particleTransform.preMultiply( spin );
			}

			if (local())
				particleTransform.postMultiply(worldTransform);

			SmartPointer<ParticleWorldPalette> pParticleWorldPalette = s_particleWorldPalette;
			SmartPointer<ParticleTintPalette> pParticleTintPalette = s_particleTintPalette;
			MeshParticleDrawItem* drawItem = new MeshParticleDrawItem( verts_, prim_,
					material_->pass( drawContext.renderingPassType() ),
					local() ? worldTransform : Matrix::identity );

			drawContext.drawUserItem( drawItem, Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, 0.0f );
			pParticleWorldPalette = drawItem->particleWorldPalette_;
			pParticleTintPalette = drawItem->particleTintPalette_;
			pParticleWorldPalette->nullTransforms(0);

			pParticleWorldPalette->transform(particleType, particleTransform);
			pParticleTintPalette->tint(particleType, Colour::getVector4Normalised(particle->colour()));
		}

		//s_particleWorldPalette->transform(particleType, nullMatrix);
		particle = sorter_.next( &*beg, particleType );
	}	
}


/**
 *	This method applies common setup for drawing mesh particles, and
 *	must be called once before calling draw many times.
 */
bool MeshParticleRenderer::beginDraw( const BoundingBox & inbb,
									 const Matrix& worldTransform )
{
	BW_GUARD;
	Moo::rc().push();
	Moo::rc().world( local() ? worldTransform : Matrix::identity );

	if (s_helper.shouldDraw(false, inbb))
	{		
		if (materialFX_ != FX_SOLID)
		{
			return pVisual_->primGroup0(verts_,prim_,primGroup_);
		}
		s_helper.start( inbb );

		Moo::rc().effectVisualContext().initConstants();

		if (pVisual_->primGroup0(verts_,prim_,primGroup_))
		{			
			Moo::Visual::RenderSet& renderSet = pVisual_->renderSets()[0];
			Moo::EffectVisualContextSetter setter( &renderSet );

			// Set our vertices.
			if (FAILED( verts_->setVertices(Moo::rc().mixedVertexProcessing())))
			{
				ERROR_MSG( "MeshParticleRenderer::drawOptimisedVisual: Couldn't set vertices\n" );
				return false;
			}			

			// Set our indices.
			if (FAILED( prim_->setPrimitives() ))
			{
				ASSET_MSG( "MeshParticleRenderer::drawOptimisedVisual:"
						"Couldn't set primitives for: %s\n",
						prim_->resourceID().c_str() );
				return false;
			}

			return true;
		}
	}

	return false;
}


/**
 *	This method draws the visual.
 *	@param ignoreBoundingBox if this value is true, this
 *		method will not cull the visual based on its bounding box.
 *	@param primGroupIndex The primitive group to draw.  For mesh particles,
 *	primGroup 0 refers to the whole mesh of 15 pieces.  pieces 1 - 15 incl.
 *	represent individual particles.
 *	@return S_OK if succeeded
 */
void MeshParticleRenderer::draw( Moo::DrawContext& drawContext,
								const BoundingBox& bb, 
								int primGroupIndex )
{
	BW_GUARD;
	verts_->clearSoftwareSkinner( );

	//--
	if (!material_->baseMaterial()->pEffect())
		return;

	MF_ASSERT( material_->channelType() == Moo::OPAQUE_CHANNEL )

	verts_->setVertices(Moo::rc().mixedVertexProcessing(), false);				

	//-- now check status of the material for current rendering pass.
	const Moo::EffectMaterialPtr& curMat = material_->pass( drawContext.renderingPassType() );

	if (curMat->begin())
	{
		for (uint32 i = 0; i < curMat->numPasses(); i++)
		{
			curMat->beginPass( i );
			prim_->drawPrimitiveGroup( primGroupIndex );
			curMat->endPass();
		}
		curMat->end();
	}
	
}


/**
 *	This method must be called after beginDraw, and draw.
 */
void MeshParticleRenderer::endDraw(const BoundingBox & inbb)
{
	BW_GUARD;
	if (s_helper.shouldDraw(false, inbb))
	{
	    verts_->clearSoftwareSkinner();
	    s_helper.fini();	    
    }
    Moo::rc().pop();
}


/**
 *	This method recreates the material for the visual.
 */
void MeshParticleRenderer::updateMaterial()
{
	BW_GUARD;
	if (!pVisual_)
		return;

	Moo::Visual::ReadGuard guard( pVisual_.get() );
	Moo::VerticesPtr	verts;
	Moo::PrimitivePtr	prim;
	Moo::Visual::PrimitiveGroup* primGroup;	

	if (pVisual_->primGroup0(verts,prim,primGroup))
	{
		BW::string effectName(
			ParticleSystemManager::instance().meshSolidEffect() );
		switch ( materialFX_ )
		{
		case FX_SOLID:
			/*
			effectName =
				ParticleSystemManager::instance().meshSolidEffect();
			*/
			break;
		case FX_ADDITIVE:
			effectName =
				ParticleSystemManager::instance().meshAdditiveEffect();
			break;
		case FX_BLENDED:
			effectName =
				ParticleSystemManager::instance().meshBlendedEffect();
			break;
		case FX_MAX:
			break;
		}


		material_ = NULL;
		material_ = new Moo::ComplexEffectMaterial;				
		material_->initFromEffect( effectName, textureName_, doubleSided_ );		
	}

	materialNeedsUpdate_ = false;
}


template < typename Serialiser >
void MeshParticleRenderer::serialise( const Serialiser & s ) const
{
	BW_PARITCLE_SERIALISE( s, MeshParticleRenderer, doubleSided );
	BW_PARITCLE_SERIALISE_ENUM( s, MeshParticleRenderer, materialFX );
	BW_PARITCLE_SERIALISE_ENUM( s, MeshParticleRenderer, sortType );
	// tagged "visualName_" for legacy reasons
	BW_PARITCLE_SERIALISE_NAMED( s, MeshParticleRenderer, visual,
		"visualName_" );
}


void MeshParticleRenderer::loadInternal( DataSectionPtr pSect )
{
	serialise( LoadFromDataSection< MeshParticleRenderer >( pSect, this ) );
	materialNeedsUpdate_ = true;
}


void MeshParticleRenderer::saveInternal( DataSectionPtr pSect ) const
{
	serialise( SaveToDataSection< MeshParticleRenderer >( pSect, this ) );
}


MeshParticleRenderer * MeshParticleRenderer::clone() const
{
	BW_GUARD;
	MeshParticleRenderer * psr = new MeshParticleRenderer();
	ParticleSystemRenderer::clone( psr );
	this->serialise( CloneObject< MeshParticleRenderer >( this, psr ) );
	return psr;
}


/*static*/ void MeshParticleRenderer::prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output )
{
	BW_GUARD;
	const BW::string& visualName = pSection->readString( "visualName_" );
	if (visualName.length())
		output.insert(visualName);
}



/**
 *	This is the Set-Accessor for the texture file name of the mesh.
 *
 *	@param m		The new materialFX type.
 */
void MeshParticleRenderer::materialFX( MaterialFX m )
{
	if ( materialFX_ != m )
	{
		materialFX_ = m;
		materialNeedsUpdate_ = true;	
	}
}


/**
 *	This is the Set-Accessor for the double-sidedness of the material.
 *
 *	@param s	The new value for double-sided.
 */
void MeshParticleRenderer::doubleSided( bool s )
{
	if (s != doubleSided_)
	{
		doubleSided_ = s;
		materialNeedsUpdate_ = true;		
	}
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to the PyMeshParticleRenderer.
// -----------------------------------------------------------------------------
#undef PY_ATTR_SCOPE

#define PY_ATTR_SCOPE PyMeshParticleRenderer::

PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( PyMeshParticleRenderer::MaterialFX, materialFX, materialFX )
PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( PyMeshParticleRenderer::SortType, sortType, sortType )

PY_TYPEOBJECT( PyMeshParticleRenderer )

/*~ function Pixie.MeshParticleRenderer
 *	Factory function to create and return a new PyMeshParticleRenderer object. A
 *	MeshParticleRenderer is a ParticleSystemRenderer which renders each particle
 *	as mesh object.
 *	@return A new PyMeshParticleRenderer object.
 */
PY_FACTORY_NAMED( PyMeshParticleRenderer, "MeshParticleRenderer", Pixie )

PY_BEGIN_METHODS( PyMeshParticleRenderer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMeshParticleRenderer )	
	/*~ attribute PyMeshParticleRenderer.visual
	 *	This is the name of the mesh particle type to use for the particles.
	 *	Default value is "".
	 *	@type String.
	 */
	PY_ATTRIBUTE( visual )
	/*~ attribute PyMeshParticleRenderer.materialFX
	 *	Specifies the MaterialFX to use to draw the meshes into the scene.
	 *	Each value specifies either a special effect, or a source and destination
	 *	alpha-blend setting for the material. If no value is specified, materialFX
	 *	defaults to 'SOLID'. The possible values are listed below:
	 *	 
	 *	SOLID: not alpha blended. source = ONE, destination = ZERO
	 *
	 *	ADDITIVE: alpha blended. source = SRC_ALPHA, destination = ONE
	 *
	 *	BLENDED: alpha blended. source = SRC_ALPHA, destination = INV_SRC_ALPHA	 
	 *
	 *	Refer to the DirectX documentation for a description of how material
	 *	settings affect alpha blending.
	 *
	 *	@type String.
	 */
	 PY_ATTRIBUTE( materialFX )
	/*~ attribute PyMeshParticleRenderer.sortType
	 *	Specifies the sort type.  The possible values are listed below:
	 *	 
	 *	NONE: minimal sorting.  Choose this method if speed is paramount and
	 *	the visual artifacts introduced are not noticeable.  The particle
	 *	system as a whole will be rendered in order with respect to other
	 *	sorted objects, and the triangles contained within will be sorted
	 *	back to front, however the particles themselves will render out
	 *	of order.
	 *
	 *	QUICK: sorting is done in a way that allows the renderer to still
	 *	draw in groups of 15, which introduced some sorting inaccuracies but
	 *	maintains most of the speed of unsorted mesh particles.  This method
	 *	is highly recommended if you cannot notice the sorting innacuracies.
	 *
	 *	ACCURATE: sorting of individual objects and triangles is performed,
	 *	which provides the most accurate sorting but breaks the ability of
	 *	the renderer to perform fast 15-at-a-time rendering, thus decreasing
	 *	rendering performance.  If you choose this sorting method, make
	 *	sure you double check the performance hit induced.
	 *
	 *	Refer to the DirectX documentation for a description of how material
	 *	settings affect alpha blending.
	 *
	 *	@type String.
	 */
	 PY_ATTRIBUTE( sortType )	 
	 /*~ attribute PyMeshParticleRenderer.doubleSided
	  *	Specifies whether or not to draw the mesh pieces double sided.
	  * @type Bool.
	  */
	 PY_ATTRIBUTE( doubleSided )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
PyMeshParticleRenderer::PyMeshParticleRenderer( MeshParticleRendererPtr pR, PyTypeObject *pType ):
	PyParticleSystemRenderer(pR,pType),
	pR_(pR)
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pR_.hasObject() )
	{
		MF_EXIT( "NULL renderer" );
	}
}


PY_ENUM_MAP( PyMeshParticleRenderer::MaterialFX )
PY_ENUM_CONVERTERS_CONTIGUOUS( PyMeshParticleRenderer::MaterialFX )

PY_ENUM_MAP( PyMeshParticleRenderer::SortType )
PY_ENUM_CONVERTERS_CONTIGUOUS( PyMeshParticleRenderer::SortType )


/**
 *	This is the Set-Accessor for the texture file name of the mesh.
 *
 *	@param m		The new materialFX type.
 */
void PyMeshParticleRenderer::materialFX( MaterialFX newMaterialFX )
{
	BW_GUARD;
	pR_->materialFX( (MeshParticleRenderer::MaterialFX)newMaterialFX);
}


/**
 *	This is the Set-Accessor for the double-sidedness of the material.
 *
 *	@param s	The new value for double-sided.
 */
void PyMeshParticleRenderer::sortType( SortType newSortType )
{
	BW_GUARD;
	pR_->sortType( (MeshParticleRenderer::SortType)newSortType);
}


/**
 *	Static Python factory method. This is declared through the factory
 *	declaration in the class definition.
 *
 *	@param	args	The list of parameters passed from Python. This should
 *					just be a string (textureName.)
 */
PyObject *PyMeshParticleRenderer::pyNew( PyObject *args )
{
	BW_GUARD;
	char *nameFromArgs = "None";
	if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
	{
		PyErr_SetString( PyExc_TypeError, "MeshParticleRenderer() expects "
			"an optional visual name string" );
		return NULL;
	}

	MeshParticleRenderer * mpr = new MeshParticleRenderer();
	if ( bw_stricmp(nameFromArgs,"None") )
		mpr->visual( BW::string( nameFromArgs ) );
	PyMeshParticleRenderer * pmpr = new PyMeshParticleRenderer( mpr );

	return pmpr;
}

BW_END_NAMESPACE

// mesh_particle_renderer.cpp
