#include "pch.hpp"
#include "asset_lock.hpp"
#include "asset_client.hpp"

BW_BEGIN_NAMESPACE

AssetClient * AssetLock::s_pAssetClient_ = NULL;

AssetLock::AssetLock()
{
	MF_ASSERT( s_pAssetClient_ != NULL );
	s_pAssetClient_->lock();
}

AssetLock::~AssetLock()
{
	MF_ASSERT( s_pAssetClient_ != NULL );
	s_pAssetClient_->unlock();
}

void AssetLock::setAssetClient( AssetClient * pAssetClient )
{
	s_pAssetClient_ = pAssetClient;
}

BW_END_NAMESPACE