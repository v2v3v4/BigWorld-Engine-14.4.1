#ifndef LOGIN_REPLY_RECORD_HPP
#define LOGIN_REPLY_RECORD_HPP

#include "cstdmf/stdmf.hpp"
#include "cstdmf/memory_stream.hpp"

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

/**
 * 	This structure contains the reply from a successful login.
 */
struct LoginReplyRecord
{
	Mercury::Address	serverAddr;			// send to here
	uint32				sessionKey;			// use this session key
};

inline BinaryIStream& operator>>(
	BinaryIStream &is, LoginReplyRecord &lrr )
{
	return is >> lrr.serverAddr >> lrr.sessionKey;
}

inline BinaryOStream& operator<<(
	BinaryOStream &os, const LoginReplyRecord &lrr )
{
	return os << lrr.serverAddr << lrr.sessionKey;
}

BW_END_NAMESPACE

#endif // LOGIN_REPLY_RECORD_HPP

