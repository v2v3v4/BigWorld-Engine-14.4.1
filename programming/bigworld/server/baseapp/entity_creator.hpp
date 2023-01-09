#ifndef ENTITY_CREATOR_HPP
#define ENTITY_CREATOR_HPP

#include "Python.h"

#include "address_load_pair.hpp"

#include "script/py_script_object.hpp"
#include "cstdmf/shared_ptr.hpp"
#include "cstdmf/smartpointer.hpp"
#include "server/id_client.hpp"

#include "baseappmgr/baseappmgr_interface.hpp"


BW_BEGIN_NAMESPACE

class Base;
class EntityType;
class LoginHandler;

namespace Mercury
{
class ChannelOwner;
}

typedef SmartPointer< Base > BasePtr;
typedef SmartPointer< EntityType > EntityTypePtr;
typedef SmartPointer< PyObject > PyObjectPtr;
typedef Mercury::ChannelOwner DBApp;


/**
 *	This class is responsible for creating entities.
 */
class EntityCreator
{
public:
	static shared_ptr< EntityCreator > create( DBApp & idOwner,
			Mercury::NetworkInterface & intInterface );

	PyObject * createBaseRemotely( PyObject * args, PyObject * kwargs );
	PyObject * createBaseAnywhere( PyObject * args, PyObject * kwargs );
	PyObject * createBaseLocally( PyObject * args, PyObject * kwargs );
	PyObject * createBase( EntityType * pType, PyObject * pDict,
							PyObject * pCellData = NULL ) const;
	
	ScriptObject createBaseLocallyFromTemplate( const BW::string & entityType,
												const BW::string & templateID );
	
	ScriptObject createBaseRemotelyFromTemplate( const Mercury::Address & destAddr,
												 const BW::string & entityType,
												 const BW::string & templateID,
												 const ScriptObject & callback );
	
	ScriptObject createBaseAnywhereFromTemplate( const BW::string & entityType,
												 const BW::string & templateID,
												 const ScriptObject & callback );

	bool createBaseFromDB( const BW::string& entityType,
					const BW::string& name,
					PyObjectPtr pResultHandler );
	bool createBaseFromDB( const BW::string& entityType, DatabaseID dbID,
					PyObjectPtr pResultHandler );
	PyObject* createRemoteBaseFromDB( const char * entityType,
					DatabaseID dbID,
					const char * name,
					const Mercury::Address* pDestAddr,
					PyObjectPtr pCallback,
					const char* origAPIFuncName );

	void createBaseFromDB( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			BinaryIStream & data );

	void createBaseWithCellData( const Mercury::Address & srcAddr,
			const Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data,
			LoginHandler * pLoginHandler );

	void createBaseFromTemplate( const Mercury::Address& srcAddr,
			const Mercury::UnpackedMessageHeader& header,
			BinaryIStream & data );

	void setCreateBaseInfo( BinaryIStream & data );

	void handleBaseAppDeath( const Mercury::Address & addr );

	void returnIDs()				{ idClient_.returnIDs(); }
	EntityID getID()				{ return idClient_.getID(); }
	void putUsedID( EntityID id )	{ idClient_.putUsedID( id ); }

private:
	typedef BW::vector< BaseAppAddrLoadPair > DestinationBaseApps;

	// ---- Methods ----
	EntityCreator( Mercury::NetworkInterface & intInterface );
	bool init( DBApp & idOwner );

	DBApp & dbApp();
	float getLoad() const;

	BasePtr createBaseFromStream( BinaryIStream & data,
			Mercury::Address * pClientAddr = NULL,
			BW::string * pEncryptionKey = NULL );

	BasePtr createBaseFromTemplate( EntityID id,
									EntityTypeID typeID,
									const BW::string & templateID );

	PyObject * createBaseCommon( const Mercury::Address * pAddr,
		PyObject * args, PyObject * kwargs, bool hasCallback = true );

	bool addCreateBaseData( BinaryOStream & stream,
					EntityTypePtr pType, PyObjectPtr pDict,
					PyObject * pCallback, EntityID * pNewID = NULL );
	EntityTypePtr consolidateCreateBaseArgs( PyObjectPtr pDestDict,
			PyObject * args, int headOffset, int tailOffset ) const;

	bool shouldCreateLocally() const;

	const Mercury::Address &getAddressFromDestinationBaseApps();

	// ---- Data ----

	IDClient		idClient_;

	Mercury::NetworkInterface & intInterface_;

	DestinationBaseApps destinationBaseApps_;
};

BW_END_NAMESPACE

#endif // ENTITY_CREATOR_HPP
