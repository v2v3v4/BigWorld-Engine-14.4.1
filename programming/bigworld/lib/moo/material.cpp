#include "pch.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#include <algorithm>
#include "material.hpp"
#include "material_loader.hpp"
#include "render_context.hpp"
#include "texture_manager.hpp"
#include "material_loader.hpp"
#include "resmgr/bwresource.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/timestamp.hpp"
#include "legacy_material_defs.hpp"



DECLARE_DEBUG_COMPONENT2( "Moo", 0 )


BW_BEGIN_NAMESPACE

bool Moo::Material::disableMaterials = false;
bool Moo::Material::shadowMaterials = false;
bool Moo::Material::shimmerMaterials = false;

#ifndef CODE_INLINE
    #include "material.ipp"
#endif

namespace Moo
{

uint64 Material::currentTime_ = 0;
uint8	Material::channelMask_ = 0xff;
uint8	Material::channelOn_ = 0x00;
bool	Material::globalBump_ = true;

static const int64 MATERIAL_TICKS_PER_SECOND = 1000;


SectionProcessors& SectionProcessors::instance()
{
	REGISTER_SINGLETON( SectionProcessors )
	SINGLETON_MANAGER_WRAPPER( SectionProcessors )
	static SectionProcessors s_sectionProcessors;
	return s_sectionProcessors;
}


Material::Material()
:
 doAlphaBlend_(false),
 srcBlend_(ONE),
 destBlend_(ZERO),
 alphaTestEnable_( false ),
 alphaReference_( 0 ),
 zBufferWrite_( true ),
 zBufferRead_( true ),
 channelFlags_( Material::SOLID ),
 uv2Generator_( NONE ),
 uv2Angle_( TOP ),
 textureFactor_( 0xffffffff ),
 fogged_( true ),
 doubleSided_( false ),
 collisionFlags_( 0 ),
 bumpEnable_( false ),
 uvAnimation_( 0, 0, 0 ),
 specularColour_( 0.f, 0.f, 0.f, 0.f ),
 characterLighting_( false )
{
	// Initialise material "stuff" the first time we construct a material
	static uint32 dummy = Material::initMaterialStuff();
}

Material::~Material()
{
}


static uint32 getColourArgument(TextureStage::ColourArgument argument)
{
	switch(argument)
	{
	case TextureStage::CURRENT:
		return D3DTA_CURRENT;
	case TextureStage::DIFFUSE:
		return D3DTA_DIFFUSE;
	case TextureStage::TEXTURE:
		return D3DTA_TEXTURE;
	case TextureStage::TEXTURE_FACTOR:
		return D3DTA_TFACTOR;
	case TextureStage::TEXTURE_ALPHA:
		return D3DTA_TEXTURE|D3DTA_ALPHAREPLICATE;
	case TextureStage::TEXTURE_INVERSE:
		return D3DTA_TEXTURE|D3DTA_COMPLEMENT;
	case TextureStage::TEXTURE_ALPHA_INVERSE:
		return D3DTA_TEXTURE|D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT;
	case TextureStage::DIFFUSE_ALPHA:
		return D3DTA_DIFFUSE|D3DTA_ALPHAREPLICATE;
	case TextureStage::DIFFUSE_INVERSE:
		return D3DTA_DIFFUSE|D3DTA_COMPLEMENT;
	case TextureStage::DIFFUSE_ALPHA_INVERSE:
		return D3DTA_DIFFUSE|D3DTA_ALPHAREPLICATE|D3DTA_COMPLEMENT;
	case TextureStage::LAST_COLORARG:
		break;
	}
	return D3DTA_CURRENT;
}

/**
 *	This method sets the string identifier for the material.
 */
void Material::identifier( const BW::string &id )
{                                                                               
	identifier_ = id;
}

/**
 *	This method sets the RenderContext material to this material.
 */
void Material::set( bool vertexAlphaOverride ) const
{
	BW_GUARD;
	if ( disableMaterials )
		return;

	//static DogWatch materialWatch( "Material" );
	//materialWatch.start();

	Moo::RenderContext& rc = Moo::rc();

	if( rc.device() )
	{
		BlendType srcBlend = srcBlend_;
		BlendType destBlend = destBlend_;
		bool doAlphaBlend = doAlphaBlend_;
		bool zBufferWrite = zBufferWrite_;
		bool zBufferRead = zBufferRead_;

		if ( vertexAlphaOverride )
		{
			doAlphaBlend = true;
			if ( destBlend_ == ZERO || alphaTestEnable_ )
			{
				destBlend = INV_SRC_ALPHA;
				if ( srcBlend_ == ONE )
					srcBlend = SRC_ALPHA;

				zBufferWrite = false;
				zBufferRead = true;
			}
		}

		//specific settings for shadowing
		if ( shadowMaterials )
		{
			rc.setRenderState( D3DRS_ALPHATESTENABLE, alphaTestEnable_ );
			rc.setRenderState( D3DRS_ALPHAREF, alphaReference_ );
			rc.setRenderState( D3DRS_CULLMODE, (doubleSided_ ? D3DCULL_NONE : 
								(rc.mirroredTransform() ? D3DCULL_CW :  D3DCULL_CCW)) );

			if ( alphaTestEnable_ )
			{
				for (uint32 i = 0; i < textureStages_.size(); i++)
				{
					const TextureStage& ts = textureStages_[ i ];
					if( ts.pTexture() )
					{
						rc.setTexture( i, ts.pTexture()->pTexture() );
					}
					else
					{
						rc.setTexture( i, NULL );
					}

					rc.setTextureStageState( i, D3DTSS_TEXCOORDINDEX, i );//ts.textureCoordinateIndex() );

					rc.setTextureStageState( i, D3DTSS_COLOROP, ts.colourOperation() );
					if( ts.colourOperation() != TextureStage::DISABLE )
					{
						rc.setTextureStageState( i, D3DTSS_COLORARG1, getColourArgument( ts.colourArgument1() ) );
						rc.setTextureStageState( i, D3DTSS_COLORARG2, getColourArgument( ts.colourArgument2() ) );
					}

					rc.setTextureStageState( i, D3DTSS_ALPHAOP, ts.alphaOperation() );
					if( ts.alphaOperation() != TextureStage::DISABLE )
					{
						rc.setTextureStageState( i, D3DTSS_ALPHAARG1, getColourArgument( ts.alphaArgument1() ) );
						rc.setTextureStageState( i, D3DTSS_ALPHAARG2, getColourArgument( ts.alphaArgument2() ) );
					}

					rc.setSamplerState( i, D3DSAMP_MAGFILTER, ts.magFilter() );
					rc.setSamplerState( i, D3DSAMP_MINFILTER, ts.minFilter() );
					rc.setSamplerState( i, D3DSAMP_ADDRESSU, ts.textureWrapMode() );
					rc.setSamplerState( i, D3DSAMP_ADDRESSV, ts.textureWrapMode() );
					rc.setSamplerState( i, D3DSAMP_MIPFILTER, ts.useMipMapping() ? D3DTEXF_LINEAR : D3DTEXF_NONE );
				}
			}
			return;
		}

		//specific settings for shimmer
		if ( shimmerMaterials )
		{
			if ( this->shimmer() )
			{
				if ( this->solid() || this->sorted() )
					rc.setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
				else
					rc.setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );
			}
			else
			{
				rc.setRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
			}
		}

		rc.setRenderState( D3DRS_ALPHABLENDENABLE, doAlphaBlend  );
		rc.setRenderState( D3DRS_SRCBLEND, srcBlend );
		rc.setRenderState( D3DRS_DESTBLEND, destBlend );
		rc.setRenderState( D3DRS_TEXTUREFACTOR, textureFactor_ );
		rc.setRenderState( D3DRS_ALPHATESTENABLE, alphaTestEnable_ );

		if (alphaTestEnable_)
			rc.setRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATER );

		rc.setRenderState( D3DRS_ALPHAREF, alphaReference_ );
		rc.setRenderState( D3DRS_ZWRITEENABLE, zBufferWrite );
		rc.setRenderState( D3DRS_ZENABLE, zBufferWrite || zBufferRead );
		rc.setRenderState( D3DRS_ZFUNC, zBufferRead ? D3DCMP_LESSEQUAL : D3DCMP_ALWAYS );
		rc.setRenderState( D3DRS_CULLMODE, (doubleSided_ ? D3DCULL_NONE : 
			(rc.mirroredTransform() ? D3DCULL_CW :  D3DCULL_CCW)) );

		// Fogging can be changed in a number of ways. It could be turned
		// on or off in fx/shaders, there is vertex fog and pixel fog that
		// can be enabled independently. For some fixed function we really
		// need to change whether or not fog is on so use the global fog
		// enable function to do this.
		rc.setRenderState( D3DRS_FOGENABLE, (DWORD)fogged_);

		// Need to handle fx file fog settings, vertex fog, pixel fog etc.
		// Just changing this next section to use FogHelper might
		// not be good enough because of cases where fixed function is used.
		//FogParams params = FogHelper::fogParams();
		//params.m_enabled = fogged_;
		//if (fogged_)
		//{
		//	// Override RC fog by setting our values directly to render state
		//	// At some stage we must reset it to match RC or we'll be confused.
		//	if (destBlend == ONE)
		//		params.m_color = D3DXCOLOR((DWORD)0x00000000); 
		//	else if (srcBlend == SRC_COLOUR && destBlend == INV_SRC_COLOUR)
		//		params.m_color = D3DXCOLOR((DWORD)0x00000000); 
		//	else if (srcBlend == ONE && destBlend == INV_SRC_ALPHA)
		//		params.m_color = D3DXCOLOR((DWORD)0x00000000); 
		//}
		//FogHelper::fogParams(params);

		for (uint32 i = 0; i < textureStages_.size(); i++)
		{
			const TextureStage& ts = textureStages_[ i ];
			if( ts.pTexture() )
			{
				rc.setTexture( i, ts.pTexture()->pTexture() );
			}
			else
			{
				rc.setTexture( i, NULL );
			}

			rc.setTextureStageState( i, D3DTSS_TEXCOORDINDEX, i );//ts.getTextureCoordinateIndex() );

			rc.setTextureStageState( i, D3DTSS_COLOROP, ts.colourOperation() );
			if( ts.colourOperation() != TextureStage::DISABLE )
			{
				rc.setTextureStageState( i, D3DTSS_COLORARG1, getColourArgument( ts.colourArgument1() ) );
				rc.setTextureStageState( i, D3DTSS_COLORARG2, getColourArgument( ts.colourArgument2() ) );
				if ( ts.colourOperation() >= TextureStage::MULTIPLYADD )
				{
					rc.setTextureStageState( i, D3DTSS_COLORARG0, getColourArgument( ts.colourArgument3() ) );
				}
			}

			rc.setTextureStageState( i, D3DTSS_ALPHAOP, ts.alphaOperation() );
			if( ts.alphaOperation() != TextureStage::DISABLE )
			{
				rc.setTextureStageState( i, D3DTSS_ALPHAARG1, getColourArgument( ts.alphaArgument1() ) );
				rc.setTextureStageState( i, D3DTSS_ALPHAARG2, getColourArgument( ts.alphaArgument2() ) );
				if ( ts.alphaOperation() >= TextureStage::MULTIPLYADD )
				{
					rc.setTextureStageState( i, D3DTSS_ALPHAARG0, getColourArgument( ts.alphaArgument3() ) );
				}
			}

			rc.setSamplerState( i, D3DSAMP_MAGFILTER, ts.magFilter() );
			rc.setSamplerState( i, D3DSAMP_MINFILTER, ts.minFilter() );
			rc.setSamplerState( i, D3DSAMP_ADDRESSU, ts.textureWrapMode() );
			rc.setSamplerState( i, D3DSAMP_ADDRESSV, ts.textureWrapMode() );
			rc.setSamplerState( i, D3DSAMP_MIPFILTER, ts.useMipMapping() ? D3DTEXF_LINEAR : D3DTEXF_NONE );
		}

		//if object alpha override is in effect, make sure the alpha gets modulated with vertex alpha
		if ( vertexAlphaOverride )
		{
			rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			rc.setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_CURRENT );
			rc.setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE );

			if (alphaTestEnable_)
			{
				rc.setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
			}
		}
	}
	//materialWatch.stop();
}



/**
 *	This static method sets the RenderContext material to the standard
 *	vertex colour material.
 */
void Material::setVertexColour()
{
	BW_GUARD;
	Moo::RenderContext& rc = Moo::rc();
	if (rc.device() == NULL) return;

	Material m;
	m.fogged( false );
	m.set();

	rc.setPixelShader( NULL );
	rc.setVertexShader( NULL );

	rc.setRenderState( D3DRS_LIGHTING, FALSE );
	rc.setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	rc.setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	rc.setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	rc.setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );

	rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	rc.setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
}


/**
 *	This method loads the resource at the given location into this material
 */
bool Material::load( const BW::string& resourceID )
{
	BW_GUARD;
	bool good = false;

	// Open the section
	DataSectionPtr spRoot = BWResource::instance().openSection( resourceID );
	if (spRoot)
	{
		DataSectionPtr spMS = spRoot->openSection( "Material" );
		if (!spMS) spMS = spRoot->openSection( "material" );
		if (spMS)
		{
			if (this->load( spMS ))
			{
				good = true;
			}
			else
			{
				WARNING_MSG( "Material::load: "
					"Error processing material section in %s\n",
					resourceID.c_str() );
			}
		}
		else
		{
			WARNING_MSG( "Material::load: "
				"Could not find any material section in %s\n",
				resourceID.c_str() );
		}
	}
    else
    {
        WARNING_MSG( "Material::load: "
			"Could not open resource %s\n",
			resourceID.c_str() );
    }

	return good;
}


/**
 *	This method loads a material from a material data section
 *	 (both legacy and new moo style materials)
 *
 * @param spMaterialSection	The data section that contains material definitions
 * @return					True for success, false for error.
 */
bool Material::load( DataSectionPtr spMaterialSection )
{
	BW_GUARD;
	bool good = false;

	if ( spMaterialSection )
	{
		good = true;

		SectionProcessors* rootProcessors = &SectionProcessors::instance();

		// Read and process each section
		for (DataSectionIterator it = spMaterialSection->begin();
			it != spMaterialSection->end();
			++it)
		{
			const BW::string & sectName = (*it)->sectionName();

			SectionProcessors::iterator found =
				rootProcessors->find( sectName.c_str() );
			if (found != rootProcessors->end())
			{
				SectionProcessor::Method m = *found->second;

                bool sectGood = (*m)( *this, *it );
                if (!sectGood)
                {
                    WARNING_MSG( "Material::load: Error loading section %s\n",
                        sectName.c_str() );
                    good = false;
                }
			}
			else
			{
				WARNING_MSG( "Material::load: "
					"An unknown section named %s was encountered\n",
					sectName.c_str() );
			}
		}

		// Add the final disabling stage
		TextureStage tsend;
		this->addTextureStage( tsend );
    }

	return good;
}


/**
 *	reads in a texture stage from a data section
 *
 * @param t the resultant texture stage
 * @param spSection the data section to read from.  It should point to the desired texture stage in the DataResource
 */
void
Material::readTextureStageSection( TextureStage& t, DataSectionPtr spSection )
{
	BW_GUARD;
	BW::string szTextureStage( "Texture_Stage" );
    BW::string szBitmapSection( "Bitmap" );

    //When reading from file, we are given a texture stage section ptr.

    //initialise texture stage with values from file
	int op = spSection->readInt( "Colour_Op", LegacyMaterialDefs::DISABLE );
	t.colourOperation( LegacyMaterialDefs::translateColourOp( LegacyMaterialDefs::ColourOperation( op ) ) );
    t.colourArgument1( (TextureStage::ColourArgument)( spSection->readInt(  "Colour_Arg1",  TextureStage::TEXTURE ) ) );
    t.colourArgument2( (TextureStage::ColourArgument)( spSection->readInt(  "Colour_Arg2",  TextureStage::DIFFUSE ) ) );

	op = spSection->readInt(  "Alpha_Op", LegacyMaterialDefs::DISABLE );
	t.alphaOperation( LegacyMaterialDefs::translateColourOp( LegacyMaterialDefs::ColourOperation( op ) ) );
    t.alphaArgument1( (TextureStage::ColourArgument)spSection->readInt(   "Alpha_Arg1",   TextureStage::TEXTURE ) );
    t.alphaArgument2( (TextureStage::ColourArgument)spSection->readInt(   "Alpha_Arg2",   TextureStage::DIFFUSE ) );

    t.textureCoordinateIndex( spSection->readInt( "Texture_Coordinate_Index", 0 ) );

    //check input file for a bitmap section for this texture stage
    DataSectionPtr spBmpSection = spSection->openSection( szBitmapSection );
    if (spBmpSection)
    {
		int wrapMode = spBmpSection->readInt(  "Wrap_Mode", LegacyMaterialDefs::REPEAT );
		t.textureWrapMode( (TextureStage::WrapMode)(wrapMode - LegacyMaterialDefs::REPEAT + TextureStage::REPEAT ) );
		t.useMipMapping(   spBmpSection->readBool( "Use_Mip-Mapping",true ) );
		t.pTexture( TextureManager::instance()->get( spBmpSection->readString( "Filename", "unknown.bmp" ) ) );

		int magFilter = spBmpSection->readInt( "Mag_Filter", LegacyMaterialDefs::LINEAR );
		int minFilter = spBmpSection->readInt( "Min_Filter", LegacyMaterialDefs::LINEAR );
		t.magFilter( LegacyMaterialDefs::translateFilterType( LegacyMaterialDefs::FilterType( magFilter ) ) );
		t.minFilter( LegacyMaterialDefs::translateFilterType( LegacyMaterialDefs::FilterType( minFilter ) ) );
    }
}


/**
 *	This method sets up the texture
 */
void Material::initTextureStagesFromSingleTextureName( const BW::string& textureFile )
{
	BW_GUARD;
	textureStages_.clear();

	BaseTexturePtr pTexture = TextureManager::instance()->get( textureFile );

	bool tga = false;
	BW::StringRef ext = BWResource::getExtension( textureFile );

	if( ext.size() == 3 )
	{
		if( ( ext[ 0 ] == 't' || ext[ 0 ] == 'T' ) &&
			( ext[ 1 ] == 'g' || ext[ 1 ] == 'G' ) &&
			( ext[ 2 ] == 'a' || ext[ 2 ] == 'A' ) )
		{
			tga = true;
		}
	}

	bool additive = false;

	if( identifier_.length() >= 3 )
	{
		if( identifier_.substr( 0, 3 ) == "AFX" )
		{
			additive = true;
		}
	}

	if( pTexture )
	{

		TextureStage ts;

		//Todo: change this to default to only modulate when
		//		all textures have been brightened up
		if (textureFile.substr(0,4) == "sets")
		{
			ts.colourOperation( TextureStage::MODULATE );
		}
		else
		{
			ts.colourOperation( TextureStage::MODULATE2X );
		}

		ts.pTexture( pTexture );

		if (additive || tga)
		{
			doAlphaBlend_ = true;
			this->sorted( true );
			zBufferWrite_ = false;
		}

		//if (additive)
		//	fogged_ = false;

		if (tga)
		{
//#define FORCED_ALPHATEST 
#ifdef FORCED_ALPHATEST
			doAlphaBlend_ = false;
			ts.alphaOperation( TextureStage::SELECTARG1 );
			this->sorted( false );
			zBufferWrite_ = true;
			alphaTestEnable_ = true;
			alphaReference_ = 8;
#else
			ts.alphaOperation( TextureStage::SELECTARG1 );
			destBlend_ = INV_SRC_ALPHA;
			srcBlend_ = SRC_ALPHA;
#endif
		}

		if (additive && !tga)
		{
			destBlend_ = ONE;
		}
		else if (additive && tga)
		{
			srcBlend_ = SRC_ALPHA;
			destBlend_ = ONE;
		}

		textureStages_.push_back( ts );
	}
	else
	{
		TextureStage ts;
		ts.colourOperation( TextureStage::SELECTARG2 );

		textureStages_.push_back( ts );
	}

	// note: we don't add a disabling stage because our caller does that for us
}




class TextureStage& Material::textureStage(uint32 stageNum)
{
	BW_GUARD;
	MF_ASSERT_DEBUG( stageNum < textureStages_.size() );

	return textureStages_[stageNum];
}

void Material::addTextureStage(const TextureStage &textureStage)
{
	BW_GUARD;
	textureStages_.push_back(textureStage);
}

void Material::removeTextureStage(uint32 i)
{
	BW_GUARD;
	MF_ASSERT_DEBUG( i < textureStages_.size() );

	textureStages_.erase(textureStages_.begin()+i);
}

void Material::clearTextureStages()
{
	textureStages_.clear();
}




/*
 *	Start of root material section processors
 */
bool readIdentifier( Material& mat, DataSectionPtr pSect )
{
	mat.identifier( pSect->asString( mat.identifier() ) );
	return true;
};

bool readUnused( Material& mat, DataSectionPtr pSect )
{
	return true;
}


bool readAlphaReference( Material& mat, DataSectionPtr pSect )
{
	mat.alphaReference( pSect->asInt( mat.alphaReference() ) );
	return true;
}

bool readZBufferWrite( Material& mat, DataSectionPtr pSect )
{
	mat.zBufferWrite( pSect->asBool( mat.zBufferWrite() ) );
	return true;
}

bool readZBufferRead( Material& mat, DataSectionPtr pSect )
{
	mat.zBufferRead( pSect->asBool( mat.zBufferRead() ) );
	return true;
}

bool readDoubleSided( Material& mat, DataSectionPtr pSect )
{
	mat.doubleSided( pSect->asBool( mat.doubleSided() ) );
	return true;
}

bool readAlphaBlended( Material& mat, DataSectionPtr pSect )
{
	mat.alphaBlended( pSect->asBool( mat.alphaBlended() ) );
	return true;
}

bool readAlphaTest( Material& mat, DataSectionPtr pSect )
{
	mat.alphaTestEnable( pSect->asBool( mat.alphaTestEnable() ) );
	return true;
}

bool readSolid( Material& mat, DataSectionPtr pSect )
{
	mat.solid( pSect->asBool( mat.solid() ));
	return true;
}

bool readSorted( Material& mat, DataSectionPtr pSect )
{
	mat.sorted( pSect->asBool( mat.sorted() ));
	return true;
}

bool readShimmer( Material& mat, DataSectionPtr pSect )
{
	mat.shimmer( pSect->asBool( mat.shimmer() ));
	return true;
}

bool readUV2Generator( Material& mat, DataSectionPtr pSect )
{
	mat.uv2Generator( (Material::UV2Generator)( pSect->asInt( mat.uv2Generator() )));
	return true;
}

bool readUV2Angle( Material& mat, DataSectionPtr pSect )
{
    mat.uv2Angle( (Material::UV2Angle)( pSect->asInt( mat.uv2Angle() )));
	return true;
}

bool readTextureFactor( Material& mat, DataSectionPtr pSect )
{
    mat.textureFactor( pSect->asInt( mat.textureFactor() ) );
	return true;
}

bool readSrcBlend( Material& mat, DataSectionPtr pSect )
{
	int src = pSect->asInt( mat.srcBlend() - Material::ZERO + LegacyMaterialDefs::ZERO );
	mat.srcBlend( (Material::BlendType)(src - LegacyMaterialDefs::ZERO + Material::ZERO ) );
	return true;
}

bool readDestBlend( Material& mat, DataSectionPtr pSect )
{
	int dest = pSect->asInt( mat.destBlend() - Material::ZERO + LegacyMaterialDefs::ZERO );
	mat.destBlend( (Material::BlendType)(dest - LegacyMaterialDefs::ZERO + Material::ZERO ) );
	return true;
}

bool readFogged( Material& mat, DataSectionPtr pSect )
{
	mat.fogged( pSect->asBool( mat.fogged() ) );
	return true;
}


/**
 *	A packed 32 bit integer.
 */
union PackedInt
{
	struct
	{
		uint8 low_low;
		uint8 low_high;
		uint8 high_low;
		uint8 high_high;
	} bytes;
	uint32 i;
};

bool readCollisionFlags( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	uint32 cflags = pSect->asInt( mat.collisionFlags() );

	if (cflags == -1)
	{
		// make sure it's -1, ie, non existent
		mat.collisionFlags( cflags );
	}
	else
	{
		// Only set the low 8 bits to the collison flags, preserving the rest
		PackedInt packed;
		packed.i = mat.collisionFlags();
		packed.bytes.low_low = (uint8) cflags;
		mat.collisionFlags( packed.i );
	}

	return true;
}

bool readMaterialKind( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	if (mat.collisionFlags() != -1)
	{
		// Set the high 8 bits of the low word of the collision flags to the
		// material kind
		PackedInt packed;
		packed.i = mat.collisionFlags();
		packed.bytes.low_high = (uint8) pSect->asInt( 0 );
		mat.collisionFlags( packed.i );
	}
	return true;
}

bool readTextureStage( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	TextureStage ts;
	mat.readTextureStageSection( ts, pSect );
	mat.textureStages().push_back( ts );
	return true;
}

bool readTexture( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	mat.initTextureStagesFromSingleTextureName( pSect->asString() );
	return true;
}

bool readUvAnimation( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	mat.uvAnimation( pSect->asVector3() );
	return true;
}


bool readBumpTexture( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	mat.bumpTexture( TextureManager::instance()->get(pSect->asString()) );
	return mat.bumpTexture().hasObject();
}

bool readDiffuseTexture( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	mat.diffuseTexture( TextureManager::instance()->get(pSect->asString()) );
	return mat.diffuseTexture().hasObject();
}

bool readBumpEnable( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	mat.bumpEnable( pSect->asBool() );
	return true;
}

bool readSpecularColour( Material& mat, DataSectionPtr pSect )
{
    BW_GUARD;
	mat.specularColour(  static_cast<Colour>(pSect->asVector4()) );
	return true;
}

bool readCharacterLighting( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	mat.characterLighting( pSect->asBool() );
	return true;
}

bool readGlowTexture( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	mat.glowTexture( TextureManager::instance()->get(pSect->asString()) );
	return mat.diffuseTexture().hasObject();
}

bool loadMFM( Material& mat, const BW::string& mfmname )
{
	BW_GUARD;
	// save the texture stages
	BW::vector<TextureStage>	ots = mat.textureStages();

	// clear them
	mat.clearTextureStages();

	// load the file
	mat.load( mfmname );

	// add ours back if there are none (or only the disabling stage)
	if (mat.textureStages().size() <= 1)
	{
		// note that this method means that you cannot accumulate texture
		// stages from different mfms. Because I reckon that you'd more
		// likely want to replace them rather than accumulate them.

		mat.textureStages() = ots;
	}
	else
	{
		// Remove the disabling stage, as readMFM will always be called
		// in the context of load(), and a disabling stage will get added
		// after this method. We don't need two.
		mat.textureStages().pop_back();
	}

	return true;
}

bool readMFM( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	return loadMFM( mat, pSect->asString() );
}

bool readTextureOrMFM( Material& mat, DataSectionPtr pSect )
{
	BW_GUARD;
	BW::string texName = pSect->asString();
	BW::string mfmName = BWResource::removeExtension( texName ) + ".mfm";

	// If a mfm exists with the same name as the texture
	if ( BWResource::fileExists( mfmName ) )
	{
		// load it instead
		return loadMFM( mat, mfmName );
	}
	else
	{
		// load the texture as normal
		mat.initTextureStagesFromSingleTextureName( texName );
	}

	return true;
}


/*
 *	End of root material section processors
 */

uint32 Material::initMaterialStuff()
{
	BW_GUARD;
	// set up the root section processors
	SectionProcessors * rootProcessors = &SectionProcessors::instance();

	rootProcessors->insert( SectProcEntry(
		"identifier",			&readIdentifier ) );
	rootProcessors->insert( SectProcEntry(
		"Identifier",			&readIdentifier ) );
	rootProcessors->insert( SectProcEntry(
		"Ambient_colour",		&readUnused ) );
	rootProcessors->insert( SectProcEntry(
		"Diffuse_colour",		&readUnused ) );
	rootProcessors->insert( SectProcEntry(
		"Specular_colour",		&readUnused ) );
	rootProcessors->insert( SectProcEntry(
		"Alpha_Reference",		&readAlphaReference ) );
	rootProcessors->insert( SectProcEntry(
		"Z-Buffer_write",		&readZBufferWrite ) );
	rootProcessors->insert( SectProcEntry(
		"Z-Buffer_read",		&readZBufferRead ) );
	rootProcessors->insert( SectProcEntry(
		"Double_sided",			&readDoubleSided ) );
	rootProcessors->insert( SectProcEntry(
		"Alpha_blended",		&readAlphaBlended ) );
	rootProcessors->insert( SectProcEntry(
		"Alpha_test",			&readAlphaTest ) );
	rootProcessors->insert( SectProcEntry(
		"Solid",				&readSolid ) );
	rootProcessors->insert( SectProcEntry(
		"Sorted",				&readSorted ) );
	rootProcessors->insert( SectProcEntry(
		"Shimmer",				&readShimmer ) );
	rootProcessors->insert( SectProcEntry(
		"UV2Generator",			&readUV2Generator ) );
	rootProcessors->insert( SectProcEntry(
		"UV2Angle",				&readUV2Angle ) );
	rootProcessors->insert( SectProcEntry(
		"Texture_Factor",		&readTextureFactor ) );
	rootProcessors->insert( SectProcEntry(
		"Src_Blend_type",		&readSrcBlend ) );
	rootProcessors->insert( SectProcEntry(
		"Dest_Blend_type",		&readDestBlend ) );
	rootProcessors->insert( SectProcEntry(
		"Fogged",				&readFogged ) );
	rootProcessors->insert( SectProcEntry(
		"CollisionFlags",		&readCollisionFlags ) );
	rootProcessors->insert( SectProcEntry(
		"MaterialKind",			&readMaterialKind ) );
	rootProcessors->insert( SectProcEntry(
		"Texture_Stage",		&readTextureStage ) );
	rootProcessors->insert( SectProcEntry(
		"uvAnimation",	&readUvAnimation ) );
	rootProcessors->insert( SectProcEntry(
		"bumpEnable",	&readBumpEnable ) );
	rootProcessors->insert( SectProcEntry(
		"specularColour",	&readSpecularColour ) );
	rootProcessors->insert( SectProcEntry(
		"diffuseTexture",	&readDiffuseTexture ) );
	rootProcessors->insert( SectProcEntry(
		"bumpTexture",	&readBumpTexture ) );
	rootProcessors->insert( SectProcEntry(
		"glowTexture", &readGlowTexture ) );
	rootProcessors->insert( SectProcEntry(
		"characterLighting", &readCharacterLighting ) );
	rootProcessors->insert( SectProcEntry(
		"texture",				&readTexture ) );
	rootProcessors->insert( SectProcEntry(
		"mfm",					&readMFM ) );
	rootProcessors->insert( SectProcEntry(
		"textureormfm",			&readTextureOrMFM ) );

	return 1;
}

/**
 *	This static method ticks over to the next frame.
 *	@param dTime the time since the last tick.
 */
void Material::tick( float dTime )
{
	currentTime_ += int64( double(dTime) * MATERIAL_TICKS_PER_SECOND );
}

const Vector4& Material::uvTransform()
{
	BW_GUARD;
	if( !rc().frameDrawn( frameTimestamp_ ) && uv2Generator_== Material::ROLLING_UV )
	{
		float timeDelta = float( double(int64(currentTime_ - lastTime_)) /
			double(MATERIAL_TICKS_PER_SECOND) );

        uvTransform_.x += timeDelta * uvAnimation_.x;
		uvTransform_.y += timeDelta * uvAnimation_.y;
		uvTransform_.z += timeDelta * uvAnimation_.z;

		uvTransform_.x -= floorf( uvTransform_.x );
		uvTransform_.y -= floorf( uvTransform_.y );
		uvTransform_.z -= floorf( uvTransform_.z );

		lastTime_ = currentTime_;
	}

	return uvTransform_;
}

bool operator == (const Material& m1, const Material& m2)
{
	if ((m1.doAlphaBlend_		==	m2.doAlphaBlend_) &&
		(m1.srcBlend_			==	m2.srcBlend_) &&
		(m1.destBlend_			==	m2.destBlend_) &&
		(m1.textureFactor_		==	m2.textureFactor_) &&
		(m1.alphaTestEnable_	==	m2.alphaTestEnable_) &&
		(m1.alphaReference_		==	m2.alphaReference_) &&
		(m1.zBufferWrite_		==	m2.zBufferWrite_) &&
		(m1.zBufferRead_		==	m2.zBufferRead_) &&
		(m1.channelFlags_		==	m2.channelFlags_) &&
		(m1.doubleSided_		==	m2.doubleSided_) &&
		(m1.fogged_				==	m2.fogged_) &&
		(m1.uv2Generator_		==	m2.uv2Generator_) &&
		(m1.uv2Angle_			==	m2.uv2Angle_) &&
		(m1.collisionFlags_		==	m2.collisionFlags_) &&
		(m1.textureStages_		==	m2.textureStages_) )
	{
		return true;
	}

	return false;
}


} // namespace Moo

BW_END_NAMESPACE

// material.cpp
