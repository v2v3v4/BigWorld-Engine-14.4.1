#include "pch.hpp"

#include "loader.hpp"

namespace BW {
namespace CompiledSpace {

ILoader::ILoader() :
	loadStatus_( ILoader::UNLOADED )
{

}


bool ILoader::loadFromSpace( ClientSpace* pSpace,
	BinaryFormat& reader,
	const DataSectionPtr& pSpaceSettings,
	const Matrix& mappingTransform,
	const StringTable& stringTable )
{
	MF_ASSERT( loadStatus_ != ILoader::LOADING );
	MF_ASSERT( loadStatus_ != ILoader::LOADED );

	pSpace_ = pSpace;
	loadStatus_ = ILoader::LOADING;

	bool success = doLoadFromSpace( pSpace,
		reader,
		pSpaceSettings,
		mappingTransform,
		stringTable );

	loadStatus_ = success ? ILoader::LOADED : ILoader::FAILED;
	
	return success;
}


ClientSpace& ILoader::space() const
{
	return *pSpace_;
}


void ILoader::unload()
{
	doUnload();
	loadStatus_ = UNLOADED;
}


float ILoader::loadStatus() const
{
	switch (loadStatus_)
	{
	case ILoader::UNLOADED:
		{
			return 0.0f;
		}
	case ILoader::LOADING:
		{
			float progress = percentLoaded();
			if (almostEqual(progress, 1.0f, FLT_EPSILON))
			{
				loadStatus_ = ILoader::LOADED;
			}
			return progress;
		}
	case ILoader::LOADED:
	case ILoader::FAILED:
		{
			return 1.0f;
		}
	}
	return 1.0f;
}


bool ILoader::bind()
{
	if (loadStatus_ == ILoader::FAILED)
	{
		return false;
	}

	MF_ASSERT( loadStatus_ == ILoader::LOADED );
	return doBind();
}


bool ILoader::doBind()
{
	// Nothing required by default
	return true;
}

} // namespace CompiledSpace
} // namespace BW

// loader.cpp