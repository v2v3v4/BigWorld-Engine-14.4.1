#ifndef STREAM_HELPER_HPP
#define STREAM_HELPER_HPP

#include "cstdmf/binary_stream.hpp"

#include "math/vector3.hpp"

#include "network/basictypes.hpp"
#include "network/misc.hpp"


BW_BEGIN_NAMESPACE

namespace StreamHelper
{
	/**
	 *	This helper function is used to stream on data that will be streamed off
	 *	in the constructor of RealEntity.
	 */
	inline void addRealEntity( BinaryOStream & /*stream*/ )
	{
	}

	/**
	 *	This helper function is used to stream on data that will be streamed off
	 *	in the constructor of Witness and its called methods as detailed below
	 */
	inline void addRealEntityWithWitnesses( BinaryOStream & stream,
			int maxPacketSize = 2000,
			float aoiRadius = 0.f,
			float aoiHyst = 5.f,
			bool isAoIRooted = false,
			float aoiRootX = 0.f,
			float aoiRootZ = 0.f )
	{
		// Needs to match Witness::readOffloadData
		stream << maxPacketSize;				// Max packet size
		stream << 0.f;							// Stealth factor

		// Needs to match Witness::readSpaceDataEntries
		stream << uint32(0);					// SpaceData entry count

		// Needs to match Witness::readAoI and Witness::writeAoI
		stream << uint32(0);					// Entity queue size
		stream << aoiRadius;					// AoI radius
		stream << aoiHyst;						// AoI Hyst
		stream << isAoIRooted;					// AoI is rooted away from an Entity?

		if (isAoIRooted)
		{
			stream << aoiRootX << aoiRootZ;		// The fixed root of the AoI
		}
	}

#pragma pack( push, 1 )
	/** @internal */
	struct AddEntityData
	{
		enum Flags {
			isOnGroundFlag  = 0x1,
			hasTemplateFlag = 0x2
		};

		AddEntityData() :
			id( 0 ),
			flags_( 0 ),
			lastEventNumber( 1 ),
			baseAddr( Mercury::Address::NONE )
		{}
		AddEntityData( EntityID _id, const Vector3 & _position,
				bool _isOnGround, EntityTypeID _typeID,
				const Direction3D & _direction, bool _hasTemplate,
				Mercury::Address _baseAddr = Mercury::Address::NONE ) :
			id( _id ),
			typeID( _typeID ),
			position( _position ),
			flags_( ( _isOnGround ? isOnGroundFlag : 0 ) |
					( _hasTemplate ? hasTemplateFlag : 0 ) ),
			direction( _direction ),
			lastEventNumber( 1 ),
			baseAddr( _baseAddr )
		{}
		bool isOnGround() const { return (flags_ & isOnGroundFlag); }
		bool hasTemplate() const { return (flags_ & hasTemplateFlag); }

		// Read by Cell::createEntityInternal
		EntityID id;
		EntityTypeID typeID;
		// Read by (Cell)Entity::readRealDataInEntityFromStreamForInitOrRestore
		Vector3 position;	// Also used by CellAppMgr::createEntityCommon
		uint8 flags_;
		Direction3D direction;
		EventNumber lastEventNumber;
		Mercury::Address baseAddr;
	};
#pragma pack( pop )


	/**
	 *	This helper function is used to stream on data that will be streamed off
	 *	in the initialisation of the entity. See Entity::initReal.
	 */
	inline void addEntity( BinaryOStream & stream, const AddEntityData & data )
	{
		stream << data;
	}


	/**
	 *	This helper function is used to remove data from a stream that was
	 *	streamed on by StreamHelper::addEntity.
	 *
	 *	@return The amount of data that was read off.
	 */
	inline int removeEntity( BinaryIStream & stream, AddEntityData & data )
	{
		stream >> data;

		return sizeof( data );
	}


	/**
	 *	This helper function is used to add the cell entity's log-off data.
	 */
	inline void addEntityLogOffData( BinaryOStream & stream,
			EventNumber number )
	{
		stream << number;
	}


	/**
	 *	This helper function is used to remove the cell entity's log-off data.
	 */
	inline void removeEntityLogOffData( BinaryIStream & stream,
			EventNumber & number )
	{
		stream >> number;
	}


	/**
	 *	This contains the data that is at the start of the backup data sent from
	 *	the cell entity to the base entity.
	 */
	class CellEntityBackupFooter
	{
	public:
		CellEntityBackupFooter( int32 cellClientSize, EntityID vehicleID ) :
			cellClientSize_( cellClientSize ),
			vehicleID_( vehicleID )
		{
		}

		int32		cellClientSize() const	{ return cellClientSize_; }
		EntityID	vehicleID() const		{ return vehicleID_; }

	private:
		int32		cellClientSize_;
		EntityID	vehicleID_;
	};
}

BW_END_NAMESPACE

#endif // STREAM_HELPER_HPP

