#ifndef INVALIDATE_FLAGS_HPP
#define INVALIDATE_FLAGS_HPP

BW_BEGIN_NAMESPACE

class InvalidateFlags
{
public:
	enum Flag
	{
		FLAG_NONE = 0,
		FLAG_NAV_MESH = 1,
		FLAG_SHADOW_MAP = 2,
		// This flag was removed as part of static lighting removal in BW 2.6
		// FLAG_STATIC_LIGHTING = 4, 
		FLAG_THUMBNAIL = 8,
		FLAG_TERRAIN_LOD = 16
	};

	InvalidateFlags( uint32 mask = FLAG_NONE )
		:mask_( mask )
	{
	}

	InvalidateFlags& operator |= ( const InvalidateFlags src )
	{
		mask_ |= src.mask_;
		return *this;
	}

	bool match( Flag& flag )
	{
		return ((this->mask_ & flag) != 0);
	}

private:
	uint32 mask_;
};

BW_END_NAMESPACE

#endif // INVALIDATE_FLAGS_HPP
