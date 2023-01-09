#include "appmgr/commentary.hpp"

#define ME_INFO_MSG( msg ) \
	INFO_MSG( msg ); \
	Commentary::instance().addMsg( msg, Commentary::COMMENT );

#define ME_WARNING_MSG( msg ) \
	WARNING_MSG( msg ); \
	Commentary::instance().addMsg( msg, Commentary::WARNING );
	
#define ME_INFO_MSGW( msg ) \
	INFO_MSG( bw_wtoacp( msg ).c_str() ); \
	Commentary::instance().addMsg( msg, Commentary::COMMENT );

#define ME_WARNING_MSGW( msg ) \
	WARNING_MSG( bw_wtoacp( msg ).c_str() ); \
	Commentary::instance().addMsg( msg, Commentary::WARNING );

BW_BEGIN_NAMESPACE
BW_END_NAMESPACE

