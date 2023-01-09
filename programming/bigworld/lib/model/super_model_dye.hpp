#ifdef _MSC_VER 
#pragma once
#endif

#ifndef SUPER_MODEL_DYE_HPP
#define SUPER_MODEL_DYE_HPP

#include "forward_declarations.hpp"
#include "dye_prop_setting.hpp"
#include "fashion.hpp"
#include "model_dye.hpp"
#include "moo/reload.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class represents a material override from the SuperModel
 *	perspective
 */
class SuperModelDye : 
	public MaterialFashion,
	/*
	*	Note: SuperModelAction is always listening if it's superModel_ has 
	*	been reloaded, if you are pulling info out from it
	*	then please update it again in onReloaderReloaded which is 
	*	called when the superModel_ is reloaded so that related data
	*	will need be update again.and you might want to do something
	*	in onReloaderPriorReloaded which happen right before the superModel_ reloaded.
	*/
	public ReloadListener
{
	friend class SuperModel;

	SuperModelDye(	SuperModel & superModel,
				const BW::string & matter,
				const BW::string & tint );
public:
	typedef BW::vector< DyePropSetting > DyePropSettings;
	DyePropSettings		properties_;

	virtual void onReloaderReloaded( Reloader* pReloader );


private:
	virtual void dress( SuperModel & superModel );
	bool	effective( const SuperModel & superModel );
	void pullInfoFromSuperModel();

	typedef BW::vector< ModelDye >		ModelDyes;
	ModelDyes		modelDyes_;
	SuperModel* pSuperModel_;
	BW::string matter_;
	BW::string tint_;
};

BW_END_NAMESPACE

#endif // SUPER_MODEL_DYE_HPP

