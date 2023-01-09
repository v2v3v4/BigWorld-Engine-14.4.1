#ifndef EFFECT_VISUAL_CONTEXT_HPP
#define EFFECT_VISUAL_CONTEXT_HPP

#include "visual.hpp"
#include "moo/image_texture.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

	//-- Sun light structure.
	//-- ToDo (b_sviglo): move it to the right place.
	//-- Warning: watch the structure memory alignment because we send this structure as is to
	//--			 the GPU and GPU has 16 bytes alignment.
	//----------------------------------------------------------------------------------------------
	struct SunLight
	{
		Vector3 m_dir;
		Colour  m_color;
		Colour  m_ambient;
	};

	//-- types of the auto constants.
	enum ESharedConstantType
	{
		CONSTANTS_PER_FRAME		=	1 << 0,
		CONSTANTS_PER_SCREEN	=	1 << 1,
		CONSTANTS_PER_VIEW		=	1 << 2,
		CONSTANTS_ALL			=	CONSTANTS_PER_FRAME | CONSTANTS_PER_SCREEN | CONSTANTS_PER_VIEW
	};

/**
 * This class is a helper class for setting up automatic constants while rendering
 * a visual using effect files. The automatic constants are referenced by using
 * the d3dx effect file semantics.
 */
class EffectVisualContext
{
public:
	//-- types of the auto constants.
	enum EAutoConstantType
	{
		AUTO_CONSTANT_TYPE_PER_FRAME,
		AUTO_CONSTANT_TYPE_PER_SCREEN,
		AUTO_CONSTANT_TYPE_PER_VIEW,
		AUTO_CONSTANT_TYPE_PER_OBJECT,
		AUTO_CONSTANT_TYPE_COUNT
	};

	static const uint32 NUM_VECTOR4_PER_PALETTE_ENTRY = 3;

	EffectVisualContext();
	virtual ~EffectVisualContext();

	virtual void	init();

	void initConstants();
	void updateSharedConstants(uint32 flags);
	void tick( float dTime );

	void				worldMatrix( const Matrix* worldMatrix );
	const Matrix*		worldMatrix() const { return worldMatrix_; }
	const Matrix&		invWorld() { return *invWorldMatrix_; }
	float				invWorldScale() { return invWorldScale_; }

	void				worldMatrixPalette( const Vector4* worldPalette, uint32 worldPaletteSize );
	const Vector4*		worldMatrixPalette() const { return worldMatrixPalette_; }
	uint32				worldMatrixPaletteSize() const { return worldMatrixPaletteSize_; }

	DX::CubeTexture* pNormalisationMap() { return pNormalisationMap_.pComObject(); };
	void isOutside( bool value ) { isOutside_ = value; }
	bool isOutside( ) const { return isOutside_; }
	
	//--
	DX::BaseTexture*	pNoiseMap()	const							{ return pNoiseMap_->pTexture(); }
	DX::BaseTexture*	pBitwiseLUT() const							{ return pBitwiseLUT_->pTexture(); }
	DX::BaseTexture*	pAtan2LUT() const							{ return pAtan2LUT_->pTexture(); }
	ID3DXEffectPool*	pEffectPool() const							{ return pEffectPoolHandle_.pComObject(); }
	
	//-- ToDo (b_sviglo): access to the sun light and ambient color.
	void				sunLight(const SunLight& sun)				{ sunLight_ = sun; }
	const SunLight&		sunLight() const							{ return sunLight_; }

	//-- ToDo (b_sviglo): reconsider this. Currently this stuff is used only for particles rendering. But
	//--	   we are using Influx's particle system. So maybe we can safely remove this without
	//--	   any outside effects and to improve simplicity of the whole source code.
	void overrideConstants(bool value)	{ overrideConstants_ = value; }
	bool overrideConstants() const { return overrideConstants_; }

	//-- register shared and per-object auto-constants.
	//-- Note: This method mostly used for adding shared auto-constant without breaking
	//--	   encapsulation between different engine's modules, however you be able to add a new
	//--	   per-object constant to the EffectVisualContext, but be sure that you did this
	//--	   before creating managed effect because determination of per-object auto-constant
	//--	   is performed during creating managed effect file.
	void				registerAutoConstant(EAutoConstantType type, const BW::string& name, const Moo::EffectConstantValuePtr& autoConst);
	void				unregisterAutoConstant(EAutoConstantType type, const BW::string& name);

	EffectConstantValuePtr* getMapping( const BW::StringRef& identifier,
								bool createIfMissing = true );

	float				currentObjectID() const { return currentObjectID_; }
	void				currentObjectID(float id) { currentObjectID_=id; }

private:
	//--
	bool createNoiseMap();
	bool createBitwiseLUTMap();
	bool createAtan2LUTMap();
	void initConstantsInternal(EAutoConstantType type);



	bool							inited_;

	class TimeConstant;

	// instance data explicitly set for shader constant use
	// it's either world matrix for non skinned data
	// or world matrix palette for skinned data
	const Vector4*			worldMatrixPalette_;
	uint32					worldMatrixPaletteSize_;

	const Matrix*			worldMatrix_;
	const Matrix*			invWorldMatrix_;
	// we calculate inverse world transform matrix immediately after worldMatrix is set
	// because a lot of shader constants are using it and we don't want to calculate it multiple times
	Matrix					invWorldCached_;
	float					invWorldScale_;

	bool					isOutside_;
	bool					overrideConstants_;
	ComObjectWrap<DX::CubeTexture> pNormalisationMap_;
	void createNormalisationMap();

	//--
	SunLight						sunLight_;
	Moo::BaseTexturePtr				pNoiseMap_;
	ImageTexture8Ptr				pBitwiseLUT_;
	ImageTexture8Ptr				pAtan2LUT_;
	ComObjectWrap<ID3DXEffectPool>	pEffectPoolHandle_;
	ManagedEffectPtr				pEffectPools_[AUTO_CONSTANT_TYPE_COUNT - 1];

	SmartPointer<TimeConstant>	timeConstant_;

	typedef BW::map< EffectConstantValuePtr *, EffectConstantValuePtr > ConstantMappings;
	ConstantMappings constantMappings_[AUTO_CONSTANT_TYPE_COUNT];
	typedef BW::StringRefMap< EffectConstantValuePtr * > Mappings;
	Mappings	mappings_;

	float					currentObjectID_;

};

} //namespace Moo

BW_END_NAMESPACE

#endif // EFFECT_VISUAL_CONTEXT_HPP
