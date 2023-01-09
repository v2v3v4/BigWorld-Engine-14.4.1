#ifndef CONTROLLED_SHUTDOWN_HANDLER_HPP
#define CONTROLLED_SHUTDOWN_HANDLER_HPP

#include "network/basictypes.hpp"
#include "network/misc.hpp"


BW_BEGIN_NAMESPACE

class Bases;
class SqliteDatabase;

namespace ControlledShutdown
{

void start( SqliteDatabase * pSecondaryDB,
			const Bases & bases,
			Bases & localServiceFragments,
			Mercury::ReplyID replyID,
			const Mercury::Address & srcAddr );

} // namespace ControlledShutdown

BW_END_NAMESPACE

#endif // CONTROLLED_SHUTDOWN_HANDLER_HPP
