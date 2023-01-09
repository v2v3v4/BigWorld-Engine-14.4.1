#ifndef REPLAY_CHECKSUM_SCHEME_HPP
#define REPLAY_CHECKSUM_SCHEME_HPP


#include "cstdmf/bw_string.hpp"
#include "cstdmf/checksum_stream.hpp"


BW_BEGIN_NAMESPACE


/**
 *	Namespace for the checksum/digest scheme used for replay files.
 */
namespace ReplayChecksumScheme
{
	ChecksumSchemePtr create( const BW::string & key, bool isPrivate );
} // end namespace ReplayChecksumScheme


BW_END_NAMESPACE

#endif // REPLAY_CHECKSUM_SCHEME_HPP

