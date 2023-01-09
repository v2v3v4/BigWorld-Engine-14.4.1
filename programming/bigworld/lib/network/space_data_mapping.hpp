#ifndef SPACE_DATA_MAPPING_HPP
#define SPACE_DATA_MAPPING_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/smartpointer.hpp"

#include "basictypes.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to track space data for a certain spaceID
 */
class SpaceDataMapping
{
public:
	SpaceDataMapping() {}

	class DataValue
	{
	public:
		DataValue( uint16 key, const BW::string &data) : 
			key_( key ), data_( data )
		{}

		DataValue() : key_( uint16(-1) ) {}

		uint16 key() const { return key_; }
		const BW::string & data() const { return data_; }

		bool valid() const { return key_ != uint16(-1); }

	private:
		uint16 key_;
		BW::string data_;
	};
	typedef BW::map< SpaceEntryID, DataValue > DataEntryMap;
	typedef DataEntryMap::const_iterator const_iterator;

	// Section: Data manipulation
	void clear();
	bool addDataEntry( const SpaceEntryID & spaceEntryID,
						uint16 key,
						const BW::string & data );
	bool delDataEntry( const SpaceEntryID & spaceEntryID );
	bool delDataEntry( const SpaceEntryID & spaceEntryID,
						DataValue & oldValue );

	// Section: Data retrieval
	const BW::string * dataRetrieveFirst( uint16 key ) const;
	const DataValue dataRetrieveSpecific( const SpaceEntryID & spaceEntryID ) const;

	// Section: iterativeness
	DataEntryMap::const_iterator begin() const { return dataEntries_.begin(); }
	DataEntryMap::const_iterator end() const { return dataEntries_.end(); }
	uint32 size() const { return (uint32)dataEntries_.size(); }

private:
	SpaceID spaceID_;
	DataEntryMap dataEntries_;
};


BW_END_NAMESPACE

#endif // SPACE_DATA_MAPPING_HPP
