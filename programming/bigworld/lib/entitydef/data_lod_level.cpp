#include "pch.hpp"

#include "data_lod_level.hpp"

#include <float.h>

DECLARE_DEBUG_COMPONENT2( "DataDescription", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "data_lod_level.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: DataLoDLevels
// -----------------------------------------------------------------------------

/**
 *	DataLoDLevel constructor.
 */
DataLoDLevel::DataLoDLevel() :
	low_( FLT_MAX ),
	high_( FLT_MAX ),
	start_( FLT_MAX ),
	hyst_( 0.f ),
	label_(),
	index_( -1 )
{
}

void DataLoDLevel::finalise( DataLoDLevel * pPrev, bool isLast )
{
	if (pPrev)
	{
		float v = pPrev->start_;
		low_ = v * v;
	}
	else
	{
		low_ = -1;
	}

	if (!isLast)
	{
		float v = start_ + hyst_;
		high_ = v * v;
	}
}


/**
 *	Constructor.
 */
DataLoDLevels::DataLoDLevels() : size_( 1 )
{
	for (unsigned int i = 0; i < sizeof( level_ )/sizeof( level_[0] ); ++i)
	{
		level_[i].index( i );
	}

	// Make the initial state valid.
	level_[0].finalise( NULL, true );
}


namespace
{
bool compareLevels( const DataLoDLevel & level1, const DataLoDLevel & level2 )
{
	return level1.start() < level2.start();
}
}

/**
 *	This method initialises the data LoD levels.
 */
bool DataLoDLevels::addLevels( DataSectionPtr pSection )
{
	// It's fine to have no section. It means that there is only the one lod
	// level.

	if (!pSection)
	{
		return true;
	}

	DataSection::iterator iter = pSection->begin();

	while (iter != pSection->end())
	{
		float start = (*iter)->asFloat();
		float hyst = (*iter)->readFloat( "hyst", 10.f );
		BW::string label = (*iter)->readString( "label" );
		DataLoDLevel * pLevel = this->find( label );

		if (pLevel == NULL)
		{
			if (size_ <= MAX_DATA_LOD_LEVELS)
			{
				pLevel = &level_[ size_ - 1 ];
				size_++;
			}
			else
			{
				ERROR_MSG( "DataLoDLevels::addLevels: "
						"Only allowed %d levels.\n", MAX_DATA_LOD_LEVELS );
				return false;
			}
		}

		pLevel->set( label, start, hyst );

		iter++;
	}

	// Sort and adjust levels.
	{
		IF_NOT_MF_ASSERT_DEV( size_ <= int(sizeof( level_ )/sizeof( DataLoDLevel)) )
		{
			return false;
		}

		std::sort( &level_[0], &level_[size_-1], compareLevels );

		DataLoDLevel * pPrev = NULL;

		for (int i = 0; i <= size_-1; ++i)
		{
			level_[i].finalise( pPrev, i == size_-1 );
			pPrev = &level_[i];
		}
	}

	return true;
}


/**
 *	This method returns the detail level with the input label.
 */
DataLoDLevel * DataLoDLevels::find( const BW::string & label )
{
	for (int i = 0; i < size_ - 1; ++i)
	{
		if (level_[i].label() == label)
		{
			return &level_[i];
		}
	}

	return NULL;
}


/**
 *	This method finds the Detail Level with the input label.
 */
bool DataLoDLevels::findLevel( int & level, DataSectionPtr pSection ) const
{
	if (pSection)
	{
		const BW::string label = pSection->asString();

		for (int i = 0; i < size_-1; ++i)
		{
			if (label == level_[ i ].label())
			{
				level = level_[ i ].index();
				return true;
			}
		}

		level = 0;

		ERROR_MSG( "DataLoDLevels:findLevel: Did not find '%s'\n",
				label.c_str() );
	}
	else
	{
		// No section means that it is in the outer detail level.
		level = DataLoDLevel::OUTER_LEVEL;
		return true;
	}

	return false;
}

BW_END_NAMESPACE

// data_lod_level.cpp
