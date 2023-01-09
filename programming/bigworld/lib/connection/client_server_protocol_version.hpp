#ifndef CLIENT_SERVER_PROTOCOL_VERSION_HPP
#define CLIENT_SERVER_PROTOCOL_VERSION_HPP


#include "cstdmf/binary_stream.hpp"
#include "cstdmf/stdmf.hpp"


// Version 2.0.6 (0):	Converted from old-style protocol 59
// Version 2.0.8 (0):	Network optimisations
// Version 2.1.2 (0):	Add extra float to createCellEntity message
// Version 2.2.255 (0):	Server-controlled entities don't expect physics
//						corrections to be acknowledged
// Version 2.2.255 (1):	Remove dummy uint8 from
//						BaseAppExtInterface::ackWardPhysicsCorrection
// Version 2.2.255 (2):	ClientInterface::switchBaseApp is a fixed-length message
// Version 2.2.255 (3):	Drop SpaceID in BaseAppExtInterface::avatarUpdateExplict
//						and BaseAppExtInterface::avatarUpdateWardExplict, and
//						convert bool onGround into uint8 flags
// Version 2.2.255 (4):	Removed dummy uint8s from various otherwise-empty
//						messages
// Version 2.2.255 (5):	Refactor high-precision volatile position updates out of
//						detailedPosition, and improve forcedPosition and
//						updateEntity messages.
// Version 2.3.0 (0):   Preparing for 2.3.0 release (see previous 2.2.255
//                      protocol version changes).
// Version 2.5.0 (0):   Adding signatures to replay recordings.
// Version 2.5.255 (0): Adding support for TCP and WebSockets.
// Version 2.6.0 (0):   Preparing for 2.6.0 release (see previous notes for
//                      actual changes).
// Version 2.6.255 (0): Using writePackedInt for streaming sequence sizes.
// Version 2.6.255 (1): BWT-26975 LoginApp::login can reply with
//						just the status back to client (no extra description)
// Version 2.6.255 (2): BWT-24775 Make UDO MD5 stream type names properly
// Version 2.6.255 (3): BWT-27010 Python updating may lead to potential pickle
//						format change.
// Version 2.6.255 (4): BWT-26862 Make BaseAppExtInterface::baseAppLogin
//						fixed-size.
// Version 2.6.255 (5): Adding login challenges (cuckoo_cycle, delay, fail).
// Version 2.6.255 (6): Replay recordings now stream exposed method description
//						ID as an uint8 instead of an int.
// Version 2.9.0 (0):   For 2.9.0 release (done after tagging release)

// These should correspond to the version number of the BigWorld release when
// the protocol was changed.


BW_BEGIN_NAMESPACE


const uint8 CLIENT_SERVER_PROTOCOL_VERSION_MAJOR = 2;
const uint8 CLIENT_SERVER_PROTOCOL_VERSION_MINOR = 9;
const uint8 CLIENT_SERVER_PROTOCOL_VERSION_PATCH = 0;
const uint8 CLIENT_SERVER_PROTOCOL_VERSION_SUBPATCH = 0;


/**
 *	This class represents values for the client-server protocol version.
 */
class ClientServerProtocolVersion
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param major 	The major version.
	 *	@param minor 	The minor version.
	 *	@param patch 	The patch version.
	 *	@param subpatch The subpatch version.
	 */
	ClientServerProtocolVersion( uint8 major, uint8 minor, uint8 patch, 
				uint8 subpatch ) :
			major_( major ),
			minor_( minor ),
			patch_( patch ),
			subpatch_( subpatch )
	{
		buf_[0] = 0;
	}


	/**
	 *	Empty constructor.
	 */
	ClientServerProtocolVersion() :
		major_( 0 ),
		minor_( 0 ),
		patch_( 0 ),
		subpatch_( 0 )
	{
		buf_[0] = 0;
	}


	/**
	 *	This method converts this object into a string representation.
	 */
	const char * c_str() const
	{
		if (buf_[0] == '\0')
		{
			if (subpatch_ == 0)
			{
				bw_snprintf( const_cast< char * >( buf_ ), sizeof( buf_ ),
						"%d.%d.%d",
						major_, minor_, patch_ );
			}
			else
			{
				bw_snprintf( const_cast< char * >( buf_ ), sizeof( buf_ ),
						"%d.%d.%d (%d)",
						major_, minor_, patch_, subpatch_ );	
			}
		}

		return buf_;
	}


	/**
	 *	This method reads a version from the given stream.
	 *
	 *	@param stream 	The input stream.
	 */
	void readFromStream( BinaryIStream & stream )
	{
		stream >> subpatch_ >> patch_ >> minor_ >> major_;
	}


	/**
	 *	This method writes this version to the given stream.
	 *
	 *	@param stream 	The output stream.
	 */
	void writeToStream( BinaryOStream & stream ) const
	{
		stream << subpatch_ << patch_ << minor_ << major_;
	}


	/** 
	 *	This method returns queries the support for the given version relative
	 *	to this version.
	 *
	 *	@param other 	The version to compare.
	 *
	 *	@return 		true if the given version is supported, false
	 *					otherwise.
	 */
	bool supports( const ClientServerProtocolVersion & other ) const
	{
		return
			(major_ == other.major_) &&
			(minor_ == other.minor_) &&
			(patch_ == other.patch_) &&
			(subpatch_ == other.subpatch_);
	}


	/**
	 *	This static method returns the current client-server protocol version.
	 */
	static ClientServerProtocolVersion currentVersion()
	{
		ClientServerProtocolVersion version(
				CLIENT_SERVER_PROTOCOL_VERSION_MAJOR,
				CLIENT_SERVER_PROTOCOL_VERSION_MINOR,
				CLIENT_SERVER_PROTOCOL_VERSION_PATCH,
				CLIENT_SERVER_PROTOCOL_VERSION_SUBPATCH );

		return version;
	}


	/**
	 *	This static method returns the stream size of this object.
	 */
	static int streamSize()
	{
		// Major, minor, patch, subpatch are all encoded as uint8.
		return 4 * sizeof( uint8 );
	}


private:
	uint8 major_;
	uint8 minor_;
	uint8 patch_;
	uint8 subpatch_;

	char buf_[ 20 ];
};


/** 
 *	Out-streaming operator.
 */
inline BinaryOStream &
operator<<( BinaryOStream & b, const ClientServerProtocolVersion & v )
{
	v.writeToStream( b );

	return b;
}


/**
 *	In-streaming operator.
 */
inline BinaryIStream &
operator>>( BinaryIStream & stream, ClientServerProtocolVersion & v )
{
	v.readFromStream( stream );

	return stream;
}


BW_END_NAMESPACE


#endif // CLIENT_SERVER_PROTOCOL_VERSION_HPP
