#ifndef SPACE_INFORMATION_HPP
#define SPACE_INFORMATION_HPP


#include "common/grid_coord.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

class SpaceInformation
{
public:
	SpaceInformation():
		spaceName_( "" ),
		localToWorld_( 0,0 ),
		gridWidth_( 0 ),
		gridHeight_( 0 )
	{
	}

	SpaceInformation( const BW::string& spaceName, const GridCoord& localToWorld, uint16 width, uint16 height ):
		spaceName_( spaceName ),
		localToWorld_( localToWorld ),
		gridWidth_( width ),
		gridHeight_( height )
	{
	}

	bool operator != ( const SpaceInformation& other ) const
	{
		return (other.spaceName_ != this->spaceName_ ||
				other.gridHeight_ != this->gridHeight_ ||
				other.gridWidth_ != this->gridWidth_ ||
				other.localToWorld_ != this->localToWorld_);
	}

	BW::string                 spaceName_;
	GridCoord                   localToWorld_; // transform from origin at (0,0) to origin at middle of map
	uint16	                    gridWidth_;
	uint16	                    gridHeight_;
};

BW_END_NAMESPACE

#endif // SPACE_INFORMATION_HPP
