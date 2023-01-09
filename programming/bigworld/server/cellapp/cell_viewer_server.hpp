#ifndef CELL_VIEWER_SERVER_HPP
#define CELL_VIEWER_SERVER_HPP

#include "network/endpoint.hpp"
#include "network/interfaces.hpp"


BW_BEGIN_NAMESPACE

class Cell;
class CellApp;
class CellViewerConnection;

namespace Mercury
{
class EventDispatcher;
}


/**
 *	An object of this type is responsible for maintaining the instances of
 *	SpaceViewerServer.
 */
class CellViewerServer : public Mercury::InputNotificationHandler
{
public:
	CellViewerServer( const CellApp & cellApp );
	virtual ~CellViewerServer();

	bool startup( Mercury::EventDispatcher & dispatcher, uint16 port = 0  );
	void shutDown();
	void deleteConnection( CellViewerConnection* pConnection );
	uint16 port() const;

private:
	virtual	int	handleInputNotification(int fd);

	BW::vector<CellViewerConnection*> connections_;

	Endpoint listener_;
	Mercury::EventDispatcher * pDispatcher_;
	const CellApp & cellApp_;
};

BW_END_NAMESPACE

#endif // CELL_VIEWER_SERVER_HPP
