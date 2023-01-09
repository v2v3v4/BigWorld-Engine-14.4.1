#ifndef MEMBER_DESCRIPTION_HPP
#define MEMBER_DESCRIPTION_HPP

#include "entity_member_stats.hpp"

#include "bwentity/bwentity_api.hpp"
#include "network/basictypes.hpp"
#include "network/interface_element.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

class MD5;

class MemberDescription
{
public:
	enum OversizeWarnLevel
	{
		OVERSIZE_NO_WARNING = 0,
		OVERSIZE_SHOULD_LOG,
		OVERSIZE_SHOULD_PRINT_CALLSTACK,
		OVERSIZE_SHOULD_RAISE_EXCEPTION
	};
	
	static const char COMPONENT_NAME_SEPARATOR = '.';

	MemberDescription( const char * name = NULL ) :
		interfaceName_(),
		name_(),
		latestEventIndex_( -1 ),
		isReliable_( true ),
		varLenHeaderSize_( Mercury::DEFAULT_VARIABLE_LENGTH_HEADER_SIZE ),
		oversizeWarnLevel_( OVERSIZE_SHOULD_LOG ),
		stats_()
	{
		if (name)
		{
			name_ = name;
		}
	}

	/** Destructor. */
	virtual ~MemberDescription() {}

	bool parse( const BW::string & interfaceName, DataSectionPtr pSection,
		bool isForClient, unsigned int * pNumLatestEventMembers );

	BWENTITY_API
	const BW::string & name() const	{ return name_; }

	int latestEventIndex() const			{ return latestEventIndex_; }
	bool isReliable() const					{ return isReliable_; }

	bool shouldSendLatestOnly() const		{ return latestEventIndex_ != -1; }

	void setLatestEventIndex( int index );

	bool checkForOversizeLength( size_t length, EntityID entityID ) const;

	EntityMemberStats & stats() const { return stats_; }

	BW::string oversizeWarnLevelAsString() const;

	void watcherOversizeWarnLevelFromString( BW::string warnLevel );

protected:
	void addToMD5( MD5 & md5 ) const;

	bool oversizeWarnLevelFromString( const BW::string & warnLevel );

	/**
	 *	This method is used to describe this object in a human-readable way.
	 */
	virtual BW::string toString() const
	{
		return "MemberDescription";
	}

	/**
	 *	This method is used to get the size of the stream size when sending to
	 *	the client from the server.
	 */
	virtual int getServerToClientStreamSize() const { return -1; }

	/**
	 * This method returns the header size for a variable length message.
	 */
	virtual uint getVarLenHeaderSize() const { return varLenHeaderSize_; }

	BW::string interfaceName_;
	BW::string name_;

	// This is an index into an array pointing to the latest instance of an
	// event in an EventHistory. When <SendLatestOnly> is false, this is -1.
	int latestEventIndex_;
	bool isReliable_;
	bool isForClient_;

	uint 				varLenHeaderSize_;
	OversizeWarnLevel 	oversizeWarnLevel_;

	mutable EntityMemberStats stats_;
};

BW_END_NAMESPACE

#endif // MEMBER_DESCRIPTION_HPP
