#include "pch.hpp"

#include "replay_checksum_scheme.hpp"

#include "cstdmf/checksum_stream.hpp"

#include "network/sha_checksum_scheme.hpp"
#include "network/elliptic_curve_checksum_scheme.hpp"

BW_BEGIN_NAMESPACE


/**
 *	This method constructs a new instance of the replay checksum scheme.
 *
 *	@param key 			The key to use, as a PEM encoded string. For signing,
 *						you'll need the private key, for verifying, you can use
 *						either (but generally will be the public key).
 *	@param isPrivate 	Whether the key is public or private.
 *
 *	@return 			A new checksum scheme object for use with replays. If
 *						the key is invalid, this will be reflected in the
 *						constructed checksum scheme object's isGood() /
 *						errorString() accessors.
 */
ChecksumSchemePtr ReplayChecksumScheme::create(
		const BW::string & key, bool isPrivate )
{
	return ChainedChecksumScheme::create(
		SHAChecksumScheme::create(),
		EllipticCurveChecksumScheme::create( key, isPrivate ) );
}

BW_END_NAMESPACE


// replay_checksum_scheme.cpp
