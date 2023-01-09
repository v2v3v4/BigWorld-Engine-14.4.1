#include "pch.hpp"

#include "asset_list.hpp"
#include "string_table.hpp"

#include "cstdmf/string_builder.hpp"
#include "cstdmf/profiler.hpp"

#include "resmgr/bwresource.hpp"
#include "model/model.hpp"
#include "moo/texture_manager.hpp"

namespace BW {
namespace CompiledSpace {

class AssetList::IAssetReference : public SafeReferenceCount
{
public:
	virtual ~IAssetReference()
	{}
};


class AssetList::Detail
{
public:

	template <class BaseAssetType>
	class AssetReference : public AssetList::IAssetReference
	{
	public:
		AssetReference( BaseAssetType* pAsset ) :
			pAsset_( pAsset )
		{}

	private:
		SmartPointer<BaseAssetType> pAsset_;
	};

	// Templated reference creation code, so that we are no longer
	// relying upon the implementation of our reference counting system to
	// hold references to these objects of different types. 
	// Previously we had assumed that all assets we were using were deriving 
	// from ReferenceCount. Now we're only assuming that they support the
	// reference counting interface of smart pointers.
	template <class BaseAssetType>
	IAssetReference* makeAssetReference( BaseAssetType* pAsset )
	{
		return new AssetReference<BaseAssetType>( pAsset );
	}

	template <class BaseAssetType>
	static IAssetReference* makeAssetReference( 
		SmartPointer<BaseAssetType> pAsset )
	{
		return new AssetReference<BaseAssetType>( pAsset.get() );
	}
};


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
AssetList::AssetList() :
	pReader_( NULL ),
	pStream_( NULL ),
	numPreloadsProcessed_( 0 )
{

}


// ----------------------------------------------------------------------------
AssetList::~AssetList()
{
	this->close();
}


// ----------------------------------------------------------------------------
bool AssetList::read( BinaryFormat& reader )
{
	PROFILER_SCOPED( AssetList_ReadFromSpace );

	typedef AssetListTypes::Entry Entry;

	MF_ASSERT( pReader_ == NULL );
	pReader_ = &reader;

	pStream_ = pReader_->findAndOpenSection( AssetListTypes::FORMAT_MAGIC,
		AssetListTypes::FORMAT_VERSION, "AssetList" );
	if (!pStream_)
	{
		this->close();
		return false;
	}

	if (!pStream_->read( entries_ ))
	{
		this->close();
	}
	return true;
}


// ----------------------------------------------------------------------------
void AssetList::close()
{
	this->releasePreloadedAssets();
	numPreloadsProcessed_ = 0;
	entries_.reset();

	if (pReader_ && pStream_)
	{
		pReader_->closeSection( pStream_ );
	}

	pReader_ = NULL;
	pStream_ = NULL;
}


// ----------------------------------------------------------------------------
bool AssetList::isValid() const
{
	return entries_.valid();
}


// ----------------------------------------------------------------------------
size_t AssetList::size() const
{
	MF_ASSERT( this->isValid() );
	return entries_.size();
}


// ----------------------------------------------------------------------------
bool AssetList::empty() const
{
	return entries_.empty();
}


// ----------------------------------------------------------------------------
AssetListTypes::AssetType AssetList::assetType( size_t idx ) const
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( idx < this->size() );

	return entries_[idx].type_;
}


// ----------------------------------------------------------------------------
size_t AssetList::stringTableIndex( size_t idx ) const
{
	MF_ASSERT( this->isValid() );
	MF_ASSERT( idx < this->size() );

	return (size_t)entries_[idx].stringTableIndex_;
}


// ----------------------------------------------------------------------------
void AssetList::preloadAssets( const StringTable& stringTable )
{
	PROFILER_SCOPED( CompiledSpace_AssetList_PreloadAssets );

	if (entries_.empty())
	{
		return;
	}

	MF_ASSERT( this->isValid() );
	MF_ASSERT( preloadedAssets_.empty() );

	preloadedAssets_.reserve( this->size() );
	numPreloadsProcessed_ = 0;

	for (size_t assetIdx = 0; assetIdx < this->size(); ++assetIdx)
	{
		size_t stIdx = this->stringTableIndex( assetIdx );
		MF_ASSERT( stIdx < stringTable.size() );

		StringRef resourceID( stringTable.entry( stIdx ),
			stringTable.entryLength( stIdx ) );

		preloadedAssets_.push_back(
			preloadAsset( this->assetType( assetIdx ), resourceID ) );

		++numPreloadsProcessed_;
	}
}


// ----------------------------------------------------------------------------
AssetList::IAssetReference* AssetList::preloadAsset( 
	AssetListTypes::AssetType type,
	const StringRef& resourceID )
{
	PROFILER_SCOPED_DYNAMIC_STRING( resourceID.to_string().c_str() );

	IAssetReference* pAsset = NULL;
	switch (type)
	{
	case AssetListTypes::ASSET_TYPE_DATASECTION:
		{
			pAsset = Detail::makeAssetReference(
				BWResource::openSection( resourceID ) );
			if (!pAsset)
			{
				ASSET_MSG( "Failed to preload '%s' as DATASECTION.\n",
					resourceID.to_string().c_str() );
			}
			break;
		}
	case AssetListTypes::ASSET_TYPE_TEXTURE:
		{
			pAsset = Detail::makeAssetReference(
				Moo::TextureManager::instance()->get( resourceID ) );
			if (!pAsset)
			{
				ASSET_MSG( "Failed to preload '%s' as TEXTURE.\n",
					resourceID.to_string().c_str() );
			}
			break;
		}
	case AssetListTypes::ASSET_TYPE_MODEL:
		{
			pAsset = Detail::makeAssetReference(
				Model::get( resourceID ) );
			if (!pAsset)
			{
				ASSET_MSG( "Failed to preload '%s' as MODEL.\n",
					resourceID.to_string().c_str() );
			}
			break;
		}
	default: 
		ASSET_MSG( "AssetList: unknown resource type %d ('%s').\n",
			type, resourceID.to_string().c_str() );
		break;
	}

	return pAsset;
}


// ----------------------------------------------------------------------------
void AssetList::releasePreloadedAssets()
{
	preloadedAssets_.clear();
}


// ----------------------------------------------------------------------------
float AssetList::percentLoaded() const
{
	if (!this->isValid())
	{
		return 0.f;
	}
	else if (this->empty())
	{
		return 1.f;
	}
	else
	{
		return static_cast<float>( numPreloadsProcessed_ ) /
			static_cast<float>( this->size() );
	}
}

} // namespace CompiledSpace
} // namespace BW