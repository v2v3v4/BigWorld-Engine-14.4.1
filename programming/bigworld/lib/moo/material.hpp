#ifndef MOO_MATERIAL_HPP
#define MOO_MATERIAL_HPP


#include "cstdmf/bw_vector.hpp"

#include "cstdmf/stdmf.hpp"
#include "cstdmf/smartpointer.hpp"
#include "resmgr/datasection.hpp"

#include "moo_math.hpp"
#include "texturestage.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This class implements a material definition for the
 *	Moo rendering library.
 *
 *	Materials are often defined in xml, as .mfm files.
 *
 *	Materials set most of the D3D device states required
 *	to colour pixels; in this way they represent a fixed-
 *	function pixel shader for legacy hardware.
 *
 *	Additionally however, materials set other states such
 *	as polygon winding oreder, z-buffer flags etc.  Thus
 *	when using a pixel shader, a material is often used too.
 *
 *	Materials can be created and edited using the BigWorld
 *	ModelViewer tool.
 */
class Material
{
public:
    Material();
    ~Material();

	typedef enum UV2Generator_
	{
		NONE = 0,
		CHROME,
		PROJECTION,
		NORMAL,
		ROLLING_UV,
		TRANSFORM,
		LAST_UV2_GENERATOR
	}UV2Generator;

	typedef enum UV2Angle_
	{
		TOP = 0,
		SIDE,
		FRONT,
		LAST_UV2_ANGLE
	}UV2Angle;

	typedef enum BlendType_
	{
		ZERO=1,
		ONE,
		SRC_COLOUR,
	 	INV_SRC_COLOUR,
		SRC_ALPHA,
		INV_SRC_ALPHA,
		DEST_ALPHA,
		INV_DEST_ALPHA,
		DEST_COLOUR,
		INV_DEST_COLOUR,
		SRC_ALPHA_SAT,
		BOTH_SRC_ALPHA,
		BOTH_INV_SRC_ALPHA,
		LAST_BLENDTYPE = 0x7FFFFFFF
	} BlendType;

	/**
	 *	This enum describes the render channel flags;
	 *	or which channel(s) a primitive group draws to.
	 *	Note SOLID and SORTED are mutually exclusive, but
	 *	SOLID and FLARE/SHIMMER etc. can be on ( or FLARE etc. and SORTED )
	 */
	enum Channel
	{
		SOLID =			1 << 0,
		SORTED =		1 << 1,
		SHIMMER =		1 << 2,
		FLARE =			1 << 3
	};

	const BW::string&		identifier() const;
	void					identifier( const BW::string &id );

	uint32					numTextureStages() const;
	class TextureStage &	textureStage( uint32 stageNum );
	void					addTextureStage( const TextureStage &textureStage );
	void					removeTextureStage( uint32 i );
	void					clearTextureStages();

	bool					alphaBlended() const;
	void					alphaBlended( bool blended );
	BlendType				srcBlend() const;
	void					srcBlend( BlendType blendType );
	BlendType				destBlend() const;
	void					destBlend( BlendType blendType );

	const Colour&			ambient() const;
	void					ambient( const Colour &c );
	const Colour&			diffuse() const;
	void					diffuse( const Colour &c );
	const Colour&			specular() const;
	void					specular( const Colour &c );

	uint32					alphaReference() const;
	void					alphaReference( uint32 alpha );
	bool					alphaTestEnable() const;
	void					alphaTestEnable( bool b );
	bool					zBufferRead() const;
	void					zBufferRead( bool b );
	bool					zBufferWrite() const;
	void					zBufferWrite( bool b );
	bool					solid() const;
	void					solid( bool b );
	bool					sorted() const;
	void					sorted( bool b );
	bool					flare() const;
	void					flare( bool b );
	bool					shimmer() const;
	void					shimmer( bool b );
	bool					doubleSided() const;
	void					doubleSided( bool b );

	uint8					channelFlags() const	{ return (channelFlags_ & channelMask_) | channelOn_; }
	void					channelFlags( uint8 f ) { channelFlags_ = f; }

	UV2Generator			uv2Generator() const;
	void					uv2Generator( UV2Generator mode );

	UV2Angle				uv2Angle() const;
	void					uv2Angle( UV2Angle mode );

	uint32					textureFactor() const;
	void					textureFactor( uint32 factor );

	bool					fogged() const;
	void					fogged( bool status );

	uint32					collisionFlags() const;
	void					collisionFlags( uint32 f );

	uint8					materialKind() const;
	void					materialKind( uint8 k );

	void					set( bool vertexAlphaOverride = false ) const;

	static void				setVertexColour();

	bool					load( const BW::string& resourceID );
	bool					load( DataSectionPtr spMaterialSection );

	const Vector3&			uvAnimation() const { return uvAnimation_; }
	void					uvAnimation(const Vector3& a) { uvAnimation_ = a; }

	const Vector4&			uvTransform();
	void					uvTransform(const Vector4& t) { uvTransform_ = t; }

	bool					bumpEnable() const { return bumpEnable_ && globalBump_; };
	const Colour&			specularColour() const { return specularColour_; };
	BaseTexturePtr			bumpTexture() const { return bumpTexture_; };
	BaseTexturePtr			diffuseTexture() const { return diffuseTexture_; };
	BaseTexturePtr			glowTexture() const { return glowTexture_; };
	bool					characterLighting() const { return characterLighting_; };

	void					bumpEnable( bool state ) { bumpEnable_ = state; };
	void					specularColour( const Colour& colour ) { specularColour_ = colour; };
	void					bumpTexture( BaseTexturePtr texture ) { bumpTexture_ = texture; };
	void					diffuseTexture( BaseTexturePtr texture ) { diffuseTexture_ = texture; };
	void					glowTexture( BaseTexturePtr texture ) { glowTexture_ = texture; };
	void					characterLighting( bool state ) { characterLighting_ = state; };

	static bool				disableMaterials;
	static bool				shadowMaterials;
	static bool				shimmerMaterials;

	static void				tick( float dTime );

	void					readTextureStageSection( Moo::TextureStage& t, DataSectionPtr spSection );
	void					initTextureStagesFromSingleTextureName( const BW::string& textureFile );
	BW::vector<class TextureStage>&	textureStages()	{ return textureStages_; }

	static void				channelMask( uint8 mask ) { channelMask_ = mask; }
	static uint8			channelMask( ) { return channelMask_; }
	static void				channelOn( uint8 on ) { channelOn_ = on; }
	static uint8			channelOn( ) { return channelOn_; }
	static void				globalBump( bool bumpOn ) { globalBump_ = bumpOn; }
	static bool				globalBump( ) { return globalBump_; }

private:

	BW::string				identifier_;

	Colour					ambient_;
	Colour					diffuse_;
	Colour					specular_;

	bool					doAlphaBlend_;
	BlendType				srcBlend_;
	BlendType				destBlend_;

	uint32					textureFactor_;

	bool					alphaTestEnable_;
	uint32					alphaReference_;
	bool					zBufferWrite_;
	bool					zBufferRead_;
	bool					doubleSided_;
	bool					fogged_;
	uint8					channelFlags_;

	UV2Generator			uv2Generator_;
	UV2Angle				uv2Angle_;

	uint32					collisionFlags_;

	BW::vector<class TextureStage>	textureStages_;

	Vector3					uvAnimation_;
	Vector4					uvTransform_;
	uint32					frameTimestamp_;
	uint64					lastTime_;

	bool						bumpEnable_;
	Colour						specularColour_;
	BaseTexturePtr				bumpTexture_;
	BaseTexturePtr				diffuseTexture_;
	BaseTexturePtr				glowTexture_;
	bool						characterLighting_;

	static uint32			initMaterialStuff();
	static uint64			currentTime_;

	static uint8			channelMask_;
	static uint8			channelOn_;
	static bool				globalBump_;

	friend bool operator == ( const Material& m1, const Material& m2 );
};

}

#ifdef CODE_INLINE
    #include "material.ipp"
#endif

BW_END_NAMESPACE

#endif // MOO_MATERIAL_HPP
