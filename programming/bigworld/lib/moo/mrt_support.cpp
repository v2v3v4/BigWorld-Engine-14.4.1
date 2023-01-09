#include "pch.hpp"
#include "mrt_support.hpp"
#include "effect_visual_context.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: class MRTSupport
// -----------------------------------------------------------------------------


namespace Moo
{

bool MRTSupport::TextureSetter::operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
{
	BW_GUARD;
	//If this assert goes off, then a shader is trying to fetch "DepthTex", but
	//MRTSupport::bind() was not called first - meaning the second render target
	//may well be set on the device currently, which is bad.
	MF_ASSERT_DEV( !MRTSupport_->isEnabled() || MRTSupport_->bound() == true )

	if (map_.hasComObject())
		pEffect->SetTexture( constantHandle, map_.pComObject() );
	else
		pEffect->SetTexture( constantHandle, NULL );
	return true;
}


MRTSupport::MRTSupport():
	bound_( false ),
	mrtSetting_ ( NULL )
{
}


void MRTSupport::TextureSetter::map(DX::BaseTexture* pTexture )
{
	map_ = pTexture;
}

ComObjectWrap<DX::BaseTexture> MRTSupport::TextureSetter::map()
{
	return map_;
}

/**
  *
  */
void MRTSupport::configureKeywordSetting(Moo::EffectMacroSetting & setting)
{	
	BW_GUARD;
	bool supported = Moo::rc().mrtSupported();
}


bool MRTSupport::init()
{
	BW_GUARD;
	MF_ASSERT(mrtSetting_ == NULL);

	bound_ = false;
	mapSetter_ = new TextureSetter( this );
	*Moo::rc().effectVisualContext().getMapping( "DepthTex" ) = mapSetter_;
	return true;
}

/**
 *
 */
bool MRTSupport::fini()
{
	BW_GUARD;
	*Moo::rc().effectVisualContext().getMapping( "DepthTex" ) = NULL;
	mapSetter_ = NULL;

	return false;
}


void MRTSupport::onSelectPSVersionCap(int psVerCap)
{	
	BW_GUARD;
	MF_ASSERT( mrtSetting_ );
	if (psVerCap < 3 && mrtSetting_->activeOption()==0)
		mrtSetting_->selectOption(1); //disable
}


bool MRTSupport::isEnabled()
{
	BW_GUARD;
	return false;
}

void MRTSupport::bind()
{
	BW_GUARD_PROFILER(MRTSupport_bind);
	if ( !MRTSupport::isEnabled() )
	{
		return;
	}

	MF_ASSERT_DEV( !bound_ )

	//save the current render target state, this will keep a copy of RT2
	//if one is currently set (if not, someone else has pushRenderTarget'd
	//already, so it is still saved)
	rc().pushRenderTarget();

	//unbind RT2 from the render context, as we are now allowing access
	//to it via samplers.
	rc().setRenderTarget(1,NULL);

	mapSetter_->map( Moo::rc().secondRenderTargetTexture().pComObject() );

	bound_ = true;
}


void MRTSupport::unbind()
{
	BW_GUARD_PROFILER(MRTSupport_unbind);
	if ( !MRTSupport::isEnabled() )
	{
		return;
	}

	MF_ASSERT_DEV( bound_ )

	mapSetter_->map( NULL );
	Moo::rc().popRenderTarget();
	bound_ = false;
}

}	//namespace Moo

BW_END_NAMESPACE

// mrt_support.cpp
