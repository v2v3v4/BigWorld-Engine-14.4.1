#ifndef DATA_LOD_LEVEL_HPP
#define DATA_LOD_LEVEL_HPP

#include "network/basictypes.hpp"

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_string.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used by DataLoDLevels. If the priority goes below the low
 *	value, the consumer should more to a more detailed level. If the priority
 *	goes above the high value, we should move to a less detailed level.
 */
class DataLoDLevel
{
public:
	DataLoDLevel();

	BWENTITY_API float low() const					{ return low_; }
	BWENTITY_API float high() const					{ return high_; }

	BWENTITY_API float start() const					{ return start_; }
	BWENTITY_API float hyst() const					{ return hyst_; }

	void set( const BW::string & label, float start, float hyst )
	{
		label_ = label;
		start_ = start;
		hyst_ = hyst;
	}

	BWENTITY_API const BW::string & label() const	{ return label_; }

	void finalise( DataLoDLevel * pPrev, bool isLast );

	int index() const			{ return index_; }
	void index( int i )			{ index_ = i; }

	enum
	{
		OUTER_LEVEL = -2,
		NO_LEVEL = -1
	};

private:
	float low_;
	float high_;
	float start_;
	float hyst_;
	BW::string label_;

	// Only used when starting up. It is used to translate detailLevel if the
	// detail levels were reordered because of a derived interface.
	int index_;
};


/**
 *	This class is used to store where the "Level of Detail" transitions occur.
 */
class DataLoDLevels
{
public:
	DataLoDLevels();
	bool addLevels( DataSectionPtr pSection );

	BWENTITY_API int size() const;
	BWENTITY_API const DataLoDLevel & getLevel( int i ) const
		{ return level_[i]; }

	BWENTITY_API DataLoDLevel *  find( const BW::string & label );
	bool findLevel( int & level, DataSectionPtr pSection ) const;

	bool needsMoreDetail( int level, float priority ) const;
	bool needsLessDetail( int level, float priority ) const;

private:
	// TODO: Reconsider what MAX_DATA_LOD_LEVELS needs to be.
	DataLoDLevel level_[ MAX_DATA_LOD_LEVELS + 1 ];

	int size_;
};


#ifdef CODE_INLINE
#include "data_lod_level.ipp"
#endif

BW_END_NAMESPACE

#endif // DATA_LOD_LEVEL_HPP
