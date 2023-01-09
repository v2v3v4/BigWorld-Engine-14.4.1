#include "pch.hpp"

#include "texture_usage.hpp"
#include "texture_streaming_manager.hpp"
#include "streaming_texture.hpp"

#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "moo/texture_detail_level.hpp"
#include "moo/texture_detail_level_manager.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

//-----------------------------------------------------------------------------
const float ModelTextureUsage::INVALID_DENSITY = -1.0f;
const float ModelTextureUsage::MIN_DENSITY = 0.0f;

ModelTextureUsage::ModelTextureUsage() :
	bounds_( Vector3::ZERO, 0.0f ),
	worldScale_( 1.0f ),
	uvDensity_( MIN_DENSITY )
{

}

void ModelTextureUsage::initialise()
{
	bounds_ = Sphere( Vector3::ZERO, 0.0f );
	worldScale_ = 1.0f;
	uvDensity_ = MIN_DENSITY;
}

//-----------------------------------------------------------------------------
void ModelTextureUsageContext::lock()
{
	textureUsageLock_.grab();
}

void ModelTextureUsageContext::unlock()
{
	textureUsageLock_.give();
}

ModelTextureUsageContext::Handle 
	ModelTextureUsageContext::registerUsage(
	const BaseTexturePtr& pTexture )
{
	static const bool isAssetProcessing = Moo::rc().assetProcessingOnly();
	if (!isAssetProcessing)
	{
		if (pTexture->type() != BaseTexture::STREAMING)
		{
			return Handle();
		}
	}
	else
	{
		TextureDetailLevelPtr textureDetail = 
			TextureManager::instance()->detailManager()->detailLevel( 
			pTexture->resourceID() );
		if (!textureDetail->streamable())
		{
			return Handle();
		}
	}

	// TODO: MF_ASSERT( textureUsageLock_.isHeldByCurrentThread() );

	Handle handle = textureUsage_.create();
	ModelTextureUsage& usage = textureUsage_[handle];

	usage.initialise();
	usage.pTexture_ = pTexture;

	return handle;
}

void ModelTextureUsageContext::unregisterUsage( 
	Handle handle )
{
	// TODO: MF_ASSERT( textureUsageLock_.isHeldByCurrentThread() );
	textureUsage_.release( handle );
}

ModelTextureUsage * ModelTextureUsageContext::getUsage(
	Handle handle )
{
	// While this lock is held, we're guaranteed not to be modifying
	// the set of usages
	// TODO: MF_ASSERT( textureUsageLock_.isHeldByCurrentThread() );
	return &textureUsage_[handle];
}

size_t ModelTextureUsageContext::size()
{
	return textureUsage_.size();
}

void ModelTextureUsageContext::getUsages( 
	const ModelTextureUsage** pUsages, size_t* numUsage )
{
	// Return the address of the packed usage data
	*numUsage = textureUsage_.size();
	if (*numUsage)
	{
		*pUsages = &(*textureUsage_.begin());
	}
}

//-----------------------------------------------------------------------------
ModelTextureUsageGroup::ModelTextureUsageGroup( 
	Data& data, Context& provider )
	: data_( data )
	, provider_( provider )
	, isObjectDataDirty_( false )
{

}

ModelTextureUsageGroup::~ModelTextureUsageGroup()
{

}

void ModelTextureUsageGroup::setWorldScale_FromTransform( const Matrix& xform )
{
	setWorldScale( calcMaxScale( xform ) );
}

void ModelTextureUsageGroup::setWorldScale( float scale )
{
	data_.worldScale_ = scale;
	isObjectDataDirty_ = true;
}

void ModelTextureUsageGroup::setWorldBoundSphere( const Sphere & sphere )
{
	data_.worldBoundSphere_ = sphere;
	isObjectDataDirty_ = true;
}

void ModelTextureUsageGroup::setTextureUsage( BaseTexture * pTexture, 
	float uvDensity )
{
	if (pTexture == NULL)
	{
		return;
	}

	// Attempt to find the texture's usage
	ModelTextureUsageMap::iterator findResult = data_.usages_.find( pTexture );
	if (findResult == data_.usages_.end())
	{
		Handle handle = provider_.registerUsage( pTexture );
		if (!handle.isValid())
		{
			// Texture streaming manager isn't worried about this
			// texture's usage information
			return;
		}

		findResult = 
			data_.usages_.insert( std::make_pair( pTexture, handle ) ).first;
	}

	// NOTE: This is not valid here because this flag is only used for 
	// object data. Not 'texture data'. This is the only place
	// that uvDensity data is ever published to a usage.
	//isObjectDataDirty_ = true;

	Handle handle = findResult->second;
	MF_ASSERT( handle.isValid() );

	{
		ModelTextureUsage * pUsage = provider_.getUsage( handle );
		pUsage->uvDensity_ = uvDensity;
		pUsage->bounds_ = data_.worldBoundSphere_;
		pUsage->worldScale_ = data_.worldScale_;
	}
}

void ModelTextureUsageGroup::removeTextureUsage( BaseTexture * pTexture )
{
	ModelTextureUsageMap::iterator findResult = data_.usages_.find( pTexture );
	if (findResult == data_.usages_.end())
	{
		return;
	}

	Handle handle = findResult->second;
	provider_.unregisterUsage( handle );

	data_.usages_.erase( findResult );
}

void ModelTextureUsageGroup::clearTextureUsage()
{
	for (ModelTextureUsageMap::iterator iter = data_.usages_.begin(), 
		end = data_.usages_.end();
		iter != end;
		++iter)
	{
		Handle handle = iter->second;
		provider_.unregisterUsage( handle );
	}
	data_.usages_.clear();
}

void ModelTextureUsageGroup::applyObjectChanges()
{
	if (!isObjectDataDirty_)
	{
		return;
	}
	isObjectDataDirty_ = false;

	for (ModelTextureUsageMap::iterator iter = data_.usages_.begin(), 
		end = data_.usages_.end();
		iter != end;
		++iter)
	{
		Handle handle = iter->second;
		ModelTextureUsage * pUsage = provider_.getUsage( handle );
		pUsage->bounds_ = data_.worldBoundSphere_;
		pUsage->worldScale_ = data_.worldScale_;
	}
}

float ModelTextureUsageGroup::calcMaxScale( const Matrix & xform )
{
	float maxSqrLength = xform.applyToUnitAxisVector( 0 ).lengthSquared();
	maxSqrLength = max(maxSqrLength, xform.applyToUnitAxisVector( 1 ).lengthSquared() );
	maxSqrLength = max(maxSqrLength, xform.applyToUnitAxisVector( 2 ).lengthSquared() );

	return sqrtf( maxSqrLength );
}

} // namespace Moo

BW_END_NAMESPACE

// texture_usage.cpp