#ifndef FORCED_LOD_MAP_HPP
#define FORCED_LOD_MAP_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/moo_dx.hpp"
#include "resmgr/datasection.hpp"
#include "worldeditor/project/space_information.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class manages and displays a view of the currently
 *	Forced LOD areas of the whole space.
 */
class ForcedLODMap
{
public:
	ForcedLODMap();
	~ForcedLODMap();

	void setSpaceInformation(const SpaceInformation& info);

	void updateTexture();

	void clearLODData();
	void loadLODDataFromFiles();
	void updateLODElement(int16 offsetX, int16 offsetY, int forcedLod);

	void setTexture(uint8 textureStage);

    ComObjectWrap<DX::Texture> lockTexture()
	{
		return lockTexture_;
	}

private:
	void setGridSize(uint32 width, uint32 height);

	void releaseLockTexture();
	void createLockTexture();

	SpaceInformation spaceInformation_;

	ComObjectWrap<DX::Texture> lockTexture_;
	uint32 gridWidth_;
	uint32 gridHeight_;
	bool requiresFileLoad_;

	BW::vector<int> fileLodValues_;
	BW::vector<int> dynamicLodValues_;

	enum { MAX_LOD_LOCK_LEVELS = 10 };

	uint32 colourUnlocked_;
	uint32 colourLODLockLevel_[MAX_LOD_LOCK_LEVELS];
};

BW_END_NAMESPACE

#endif // FORCED_LOD_MAP_HPP
