#ifndef ASSET_LOCK_HPP
#define ASSET_LOCK_HPP

#include "cstdmf/bw_string_ref.hpp"

BW_BEGIN_NAMESPACE

class AssetClient;

class AssetLock
{
public:
	AssetLock();
	~AssetLock();

	static void setAssetClient( AssetClient * pAssetClient );

private:
	static AssetClient * s_pAssetClient_;
};

BW_END_NAMESPACE

#endif // ASSET_LOCK_HPP