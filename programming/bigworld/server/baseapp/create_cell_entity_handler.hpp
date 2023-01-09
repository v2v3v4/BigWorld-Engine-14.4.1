#ifndef CREATE_CELL_ENTITY_HANDLER_HPP
#define CREATE_CELL_ENTITY_HANDLER_HPP

#include "cstdmf/smartpointer.hpp"
#include "network/basictypes.hpp"
#include "network/interfaces.hpp"

#include <memory>


BW_BEGIN_NAMESPACE

class Base;
typedef SmartPointer< Base > BasePtr;


/**
 *	This class is used by createCellEntity and createInNewSpace to handle the
 *	reply to the cell entity creation request.
 */
class CreateCellEntityHandler : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	CreateCellEntityHandler( Base * pBase );

	void handleMessage( const Mercury::Address & source,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * arg );

	void handleException( const Mercury::NubException & exception, void * arg );

private:
	BasePtr pBase_;
};


/**
 *	This class is used by createCellEntity and createInNewSpace to handle the
 *	reply to the cell entity creation request.
 */
class CreateCellEntityViaBaseHandler :
	public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	CreateCellEntityViaBaseHandler( Base * pBase,
			Mercury::ReplyMessageHandler * pHandler,
			EntityID nearbyID );

	void handleMessage( const Mercury::Address & source,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data, void * arg );

	void handleException( const Mercury::NubException & exception, void * arg );

private:
	void cellCreationFailure();

	BasePtr pBase_;
	std::auto_ptr< Mercury::ReplyMessageHandler > pHandler_;
	EntityID nearbyID_;
};

BW_END_NAMESPACE

#endif // CREATE_CELL_ENTITY_HANDLER_HPP
