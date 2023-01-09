#ifndef MRT_SUPPORT_HPP
#define MRT_SUPPORT_HPP

#include "effect_manager.hpp"
#include "effect_macro_setting.hpp"
#include "effect_constant_value.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	MRT management.
 */
class MRTSupport : private Moo::EffectManager::IListener
{
public:
	MRTSupport();
	bool isEnabled();
	void bind();
	void unbind();
	bool bound() const	{ return bound_; }

	bool init();
	bool fini();

	//IListener methods
	void onSelectPSVersionCap(int psVerCap);
private:
	/**
	 * Texture setter is an effect constant binding that also holds a reference
	 * to a texture.
	 */
	class TextureSetter : public Moo::EffectConstantValue
	{
	public:
		TextureSetter( MRTSupport* MRTSupportIn ) : MRTSupport_( MRTSupportIn ) {}
		bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle);
		void map(DX::BaseTexture* pTexture );
		ComObjectWrap<DX::BaseTexture> map();
	private:
		ComObjectWrap<DX::BaseTexture> map_;
		MRTSupport* MRTSupport_;
	};

	static void configureKeywordSetting(Moo::EffectMacroSetting & setting);
	Moo::EffectMacroSetting::EffectMacroSettingPtr mrtSetting_;
	SmartPointer<TextureSetter> mapSetter_;
	bool bound_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif	//MRT_SUPPORT_HPP
