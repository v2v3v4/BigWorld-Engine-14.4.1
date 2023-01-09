#include "pch.hpp"

#include "cstdmf/bw_set.hpp"

#include "super_model_dye.hpp"

#include "model.hpp"
#include "super_model.hpp"
#include "tint.hpp"


DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

 /**
 * if our pSuperModel_ has been reloaded, 
 * update the related data.
 */
void SuperModelDye::onReloaderReloaded( Reloader* pReloader )
{
	BW_GUARD;
	if (pReloader && pSuperModel_->hasModel( (Model*)pReloader), true)
	{
		this->pullInfoFromSuperModel();
	}
}

 /**
 *	This method pull the info from our pSuperModel_
 */
void SuperModelDye::pullInfoFromSuperModel()
{
	BW_GUARD;
	SuperModel::ReadGuard guard( pSuperModel_ );
	BW::set<int>	gotIndices;

	modelDyes_.clear();
	properties_.clear();

	modelDyes_.reserve( pSuperModel_->nModels() );

	for (int i = 0; i < pSuperModel_->nModels(); i++)
	{
		// Get the dye
		Tint * pTint = NULL;
		modelDyes_.push_back( pSuperModel_->topModel(i)->getDye( matter_, tint_, &pTint ) );

		// Add all its properties to our list
		if (pTint != NULL)
		{
			for (uint j = 0; j < pTint->properties_.size(); j++)
			{
				DyeProperty & dp = pTint->properties_[j];

				if (gotIndices.insert( dp.index_ ).second)
				{
					properties_.push_back( DyePropSetting(
						dp.index_, dp.default_ ) );
				}
			}
		}
	}
}
/**
 *	SuperModelDye constructor
 */
SuperModelDye::SuperModelDye( SuperModel & superModel,
					const BW::string & matter, const BW::string & tint ):
pSuperModel_( &superModel ),
matter_( matter ),
tint_( tint )
{
	BW_GUARD;
	// Mutually exclusive of pSuperModel_ reloading, start
	pSuperModel_->beginRead();

	this->pullInfoFromSuperModel();
	pSuperModel_->registerModelListener( this );
	// Mutually exclusive of pSuperModel_ reloading, end
	pSuperModel_->endRead();
}


/**
 *	This method applies the dyes represented by this class to the
 *	input supermodel. It must be the same as it was created with.
 */
void SuperModelDye::dress( SuperModel & superModel )
{
	BW_GUARD;
	// apply property settings to global table
	if (!properties_.empty())
	{
		// totally sucks that we have to take this lock here!
		SimpleMutexHolder smh( Model::propCatalogueLock() );

		for (uint i = 0; i < properties_.size(); i++)
		{
			DyePropSetting & ds = properties_[i];
			(Model::propCatalogueRaw().begin() + ds.index_)->second = ds.value_;
		}
	}

	// now soak the model in our dyes
	for (int i = 0; i < superModel.nModels(); i++)
	{
		Model * cM = superModel.curModel(i);
		if (cM != NULL && !modelDyes_[i].isNull())
			cM->soak( modelDyes_[i] );
	}
}


/**
 *	This method returns whether or not this dye is at all effective
 *	on supermodels it was fetched for. Ineffictive dyes (i.e. dyes
 *	whose matter wasn't found in any of the models) are not returned
 *	to the user from SuperModel's getDye ... NULL is returned instead.
 */
bool SuperModelDye::effective( const SuperModel & superModel )
{
	BW_GUARD;
	for (int i = 0; i < superModel.nModels(); i++)
	{
		if (!modelDyes_[i].isNull())
			return true;
	}

	return false;
}

BW_END_NAMESPACE

// super_model_dye.cpp
