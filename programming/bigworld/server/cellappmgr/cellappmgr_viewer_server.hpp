#ifndef CELLAPPMGR_VIEWER_SERVER_HPP
#define CELLAPPMGR_VIEWER_SERVER_HPP

#include "math/math_extra.hpp"

#include "network/endpoint.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class EventDispatcher;
}

// Forward declarations
class Space;

namespace CellAppMgrViewerExport
{
	enum RequestTypes
	{
		GET_CELLS = 'b',
		REMOVE_CELL = 'c',
		STOP_CELL_APP = 'd',
		GET_VERSION = 'e',
		GET_SPACE_GEOMETRY_MAPPINGS = 'f'
	};
};

class CellAppMgr;

/**
 *	This class represents a single TCP connection for viewer access.
 */
class CellAppMgrViewerConnection: public Mercury::InputNotificationHandler
{
public:
	CellAppMgrViewerConnection( CellAppMgr & cellAppMgr,
			Mercury::EventDispatcher & dispatcher, int fd );
	virtual ~CellAppMgrViewerConnection();
	void sendReply( const void * buf, uint32 len );

	int fileno() const		{ return socket_.fileno(); }
	Endpoint & socket()		{ return socket_; }

private:
	virtual int handleInputNotification( int fd );

	void sendSpaceGeometryMappings( const Space& space );

	Mercury::EventDispatcher & dispatcher_;
	Endpoint socket_;
	CellAppMgr & cellAppMgr_;
};

/**
 *	This class provides access to information need by viewer app. It starts
 *	listening on a given port, and creates a new CellAppMgrViewerConnection for
 *	every connection it receives.
 */
class CellAppMgrViewerServer : public Mercury::InputNotificationHandler
{
public:
	CellAppMgrViewerServer( CellAppMgr & cellAppMgr );
	virtual ~CellAppMgrViewerServer();

	bool startup( Mercury::EventDispatcher & dispatcher, uint16 port = 0  );
	void shutDown();
	void deleteConnection( CellAppMgrViewerConnection* pConnection );
	uint16 port() const;

private:
	virtual	int	handleInputNotification(int fd);

	BW::vector<CellAppMgrViewerConnection*> connections_;

	Endpoint listener_;
	Mercury::EventDispatcher * pDispatcher_;
	CellAppMgr& cellAppMgr_;
	uint16 port_;
};

BW_END_NAMESPACE

#endif // CELLAPPMGR_VIEWER_SERVER_HPP

