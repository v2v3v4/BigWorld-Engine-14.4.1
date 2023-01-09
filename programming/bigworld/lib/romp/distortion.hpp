#ifndef DISTORTION_HPP
#define DISTORTION_HPP

#include "cstdmf/singleton.hpp"

#include "effect_parameter_cache.hpp"
#include "moo/visual.hpp"
#include "moo/graphics_settings.hpp"
#include "moo/render_target.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class EffectMaterial;
};

/**
 * Texture setter is an effect constant binding that also holds a reference
 * to a texture.
 */
class TextureSetter : public Moo::EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle);
	void map(DX::BaseTexture* pTexture );
	ComObjectWrap<DX::BaseTexture> map();
private:
	ComObjectWrap<DX::BaseTexture> map_;
};

/**
 *	This class draws a screen-distort effect.  Users only need to call begin()
 *	and end()
 *
 *	begin()
 *		- copies back buffer to off-screen texture
 *		- pushes a full-screen render target
 *
 *  in the BW Client, this is the place we also need to fix
 *	up the back buffer for the player fader effect, as the
 *	player may not have yet been drawn to the screen
 *
 *	end()
 *		- draw masks;  use alpha channel of render target
 *		to draw objects that should distort:
 *			- the water mask is explicitly drawn here
 *			- visuals registered with the distortion channel draw 'as a mask', i.e. unsorted polygon soup
 *		- pop full-screen render target
 *		- bind MRT depth and Distortion RT
 *		- draw the actual water
 *		- draw the distortion channel objects
 *		- unbind MRT depth
 *	
 */
class Distortion : public Moo::DeviceCallback
{
	typedef Distortion This;

public:
	Distortion();
	~Distortion();

	bool init();
	void finz();
	static bool isSupported();
	bool isEnabled();
#ifdef EDITOR_ENABLED
	void setEditorEnabled( bool state )			{ editorEnabled_ = state; }
#endif

	void tick( float dTime );
	bool begin();
	void end();

	virtual bool recreateForD3DExDevice() const;

	void deleteUnmanagedObjects();
	uint drawCount() const;
private:
	void drawScene();
	void drawMasks();
	void drawDistortionChannel( bool clear = true );
	void copyBackBuffer();
	bool pushRT()
	{
		if (s_pRenderTexture_ && s_pRenderTexture_->push())
		{
			Moo::rc().beginScene();
			this->setViewport();
			return true;
		}
		return false;
	}
	void popRT()
	{
		if (s_pRenderTexture_)
		{
			Moo::rc().endScene();
			s_pRenderTexture_->pop();
		}
	}
	void setViewport();
#ifdef EDITOR_ENABLED
	bool							editorEnabled_;
#endif
	bool							inited_;
	bool							watcherEnabled_;
	float							dTime_;
	Moo::VisualPtr					visual_;
	EffectParameterCache			parameters_;
	Moo::EffectMaterialPtr			effectMaterial_;
	static Moo::RenderTargetPtr		s_pRenderTexture_;

	typedef Moo::GraphicsSetting::GraphicsSettingPtr GraphicsSettingPtr;
	GraphicsSettingPtr distortionSettings_;
	void setDistortionOption(int) {}
};

BW_END_NAMESPACE

#endif //DISTORTION_HPP
