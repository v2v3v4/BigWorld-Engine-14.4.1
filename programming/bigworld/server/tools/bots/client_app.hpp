#ifndef CLIENT_APP_HPP
#define CLIENT_APP_HPP

#include "Python.h"

#include "main_app.hpp"

#include "common/space_data_types.hpp"

#include "connection/server_connection_handler.hpp"

#include "connection_model/bw_connection.hpp"
#include "connection_model/bw_entity_factory.hpp"
#include "connection_model/bw_server_connection.hpp"
#include "connection_model/bw_space_data_listener.hpp"
#include "connection_model/bw_stream_data_handler.hpp"
#include "connection_model/simple_space_data_storage.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include <queue>


BW_BEGIN_NAMESPACE

class BWEntity;
class Entity;
class MovementController;

/*~ class BigWorld.ClientApp 
 *  @components{ bots } 
 *  
 *  Instances of the ClientApp class represent simulated clients
 *  running on the Bots application.
 *
 *  An instance of ClientApp behaves exactly as a normal BigWorld
 *  client in terms of data exchange with BigWorld server. After a
 *  ClientApp is created, it performs standard login procedure with
 *  either randomn or specfied name/password. It creates a simplified
 *  version of player entity under instructions from the server. From
 *  here on the player entity ID becomes the ClientApp ID. The
 *  ClientApp receives all data within the player AOI, remote
 *  procedure calls from the server as a normal BigWorld client would.
 *  However, it only process limited amount of these data, as most
 *  data is ignored. The default behaviour of the ClientApp (while it
 *  is connected with a server) is to send new player entity positions
 *  to the server. The player entity motion is driven either randomnly
 *  or by the movement controller that the Bots application is
 *  using. Additional game logic can be implemented in Python script
 *  to make simulation closer to the scenarios found in the real world
 *  gameplay. The ClientApp can also simulate realistic inter-network
 *  environment where network delay and dropoff often occur.
 */

/**
 *	This class is used to hold a connection to the server.
 */
class ClientApp : public PyObjectPlusWithWeakReference,
	public ServerConnectionHandler,
	public BWEntityFactory,
	public BWSpaceDataListener,
	public BWStreamDataHandler
{
	Py_Header( ClientApp, PyObjectPlusWithWeakReference )

public:
	ClientApp( const BW::string & name, const BW::string & password,
		ConnectionTransport transport, const BW::string & tag, 
		PyTypeObject * pType = &ClientApp::s_type_ );

	// Lifecycle interface
	bool tick( float dTime );
	void destroy();

	// Connection interface
	void logOn();
	void logOff();					
	void dropConnection();
	void setConnectionLossRatio( float lossRatio );
	void setConnectionLatency( float latencyMin, float latencyMax );
	void connectionSendTimeReportThreshold( double threshold );

	// Accessors
	const BW::string & tag() const			{ return tag_; }
	EntityID id() const;
	SpaceID spaceID() const;
	Entity * pPlayerEntity() const;
	ScriptObject player() const;
	bool isPlayerMovable() const;
	bool canSetMovementController() const;
	double clientTime();
	double serverTime();
	BWConnection * pConnection() const		{ return pConnection_.get(); }
	uint64 logOnRetryTime() const			{ return logOnRetryTime_; }


	// Movement controller interface
	bool setDefaultMovementController();
	bool setMovementController( const BW::string & type,
			const BW::string & data );
	void moveTo( const Position3D & pos );
	void faceTowards( const Position3D & pos );
	void snapTo( const Position3D & pos ) { this->position( pos ); }
	bool isMoving() const { return pDest_.get() != NULL; };
	void stop();

	// Direct movement interface
	void position( const Position3D & pos );
	const Position3D position() const;

	void direction( const Direction3D & dir );
	const Direction3D direction() const;

	void yaw( float val );
	float yaw() const;

	void pitch( float val );
	float pitch() const;

	void roll( float val );
	float roll() const;

	void updatePosition( const Position3D & pos, const Direction3D & dir );

	// Timer interface
	int addTimer( float interval, PyObjectPtr callback, bool repeat );
	void delTimer( int id );
	bool isDestroyed() const { return isDestroyed_; }


private:
	// May only self-delete via PyObjectPlus's decRef().
	~ClientApp();

	void addMove( double time );

	// Attributes
	SimpleSpaceDataStorage				spaceDataStorage_;
	std::auto_ptr< BWServerConnection >	pConnection_;

	bool			isDestroyed_;
	bool			isDormant_;

	uint64			logOnRetryTime_;

	BW::string		userName_;
	BW::string		userPasswd_;
	ConnectionTransport transport_;

	BW::string		tag_;
	float			speed_;

	std::auto_ptr< MovementController >	pMovementController_;
	bool								autoMove_;
	std::auto_ptr< Vector3 >			pDest_;

	ScriptObject	entities_;

	// Timer management
	class TimerRec
	{
	public:
		TimerRec( float gameTime, float interval, PyObjectPtr &pFunc, bool repeat ) :
			id_( ID_TICKER++ ), interval_( interval ), startTime_( gameTime ),
			pFunc_( pFunc ), repeat_( repeat )
		{
			// Go back to 0 on overflow, since negative return values from
			// addTimer() indicate failure
			if (ID_TICKER < 0)
			{
				ID_TICKER = 0;
			}
		}

		bool operator< ( const TimerRec &other ) const
		{
			return finTime() >= other.finTime();
		}

		bool elapsed( float gameTime ) const
		{
			return finTime() <= gameTime;
		}

		int id() const { return id_; }
		float interval() const { return interval_; }
		float finTime() const { return startTime_ + interval_; }
		bool repeat() const { return repeat_; }
		PyObject* func() const { return pFunc_.get(); }
		void restart( float gameTime ) { startTime_ = gameTime; }

	private:
		static int ID_TICKER;

		int id_;
		float interval_;
		float startTime_;
		PyObjectPtr pFunc_;
		bool repeat_;
	};

	std::priority_queue< TimerRec > timerRecs_;
	BW::list< int > deletedTimerRecs_;
	void processTimers();

	// Script methods and attributes
	PY_RO_ATTRIBUTE_DECLARE( this->id(), id );
	PY_RO_ATTRIBUTE_DECLARE( this->spaceID(), spaceID );
	PY_RO_ATTRIBUTE_DECLARE( this->player(), player );
	PY_RO_ATTRIBUTE_DECLARE( userName_, loginName );
	PY_RO_ATTRIBUTE_DECLARE( userPasswd_, loginPassword );
	PY_RO_ATTRIBUTE_DECLARE( pConnection_->isOnline(), isOnline );
	PY_RO_ATTRIBUTE_DECLARE( transport_, transport );
	PY_RO_ATTRIBUTE_DECLARE( isDestroyed_, isDestroyed );
	PY_RO_ATTRIBUTE_DECLARE( this->isMoving(), isMoving );
	PY_RW_ATTRIBUTE_DECLARE( tag_, tag );
	PY_RW_ATTRIBUTE_DECLARE( speed_, speed );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Position3D, position, position );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, yaw, yaw );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, pitch, pitch );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, roll, roll );
	PY_RW_ATTRIBUTE_DECLARE( autoMove_, autoMove );

	PY_AUTO_METHOD_DECLARE( RETVOID, logOn, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, logOff, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, dropConnection, END );
	PY_AUTO_METHOD_DECLARE( RETVOID, setConnectionLossRatio, ARG( float, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, setConnectionLatency, ARG( float, ARG( float, END ) ) );

	PY_AUTO_METHOD_DECLARE( RETOK, setMovementController,
		ARG( BW::string, ARG( BW::string, END ) ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, moveTo,
		ARG( Vector3, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, faceTowards,
		ARG( Vector3, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, snapTo,
		ARG( Vector3, END ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, stop, END );

	PY_AUTO_METHOD_DECLARE( RETDATA, addTimer,
		ARG( float, ARG( PyObjectPtr, ARG( bool, END ) ) ) );
	PY_AUTO_METHOD_DECLARE( RETVOID, delTimer,
		ARG( int, END ) );

	PY_AUTO_METHOD_DECLARE( RETDATA, clientTime, END );
	PY_AUTO_METHOD_DECLARE( RETDATA, serverTime, END );

	PY_RO_ATTRIBUTE_DECLARE( entities_, entities );

	// ServerConnectionHandler overrides
	void onLoggedOff();
	void onLoggedOn() {};
	void onLogOnFailure( const LogOnStatus & status,
		const BW::string & message );

	// BWEntityFactory override
	BWEntity * doCreate( EntityTypeID entityTypeID,
		BWConnection * pConnection );

	// BWSpaceDataListener overrides
	void onUserSpaceData( SpaceID spaceID, uint16 key, bool isInsertion,
		const BW::string & data );

	void onGeometryMapping( BW::SpaceID spaceID, BW::Matrix matrix,
			const BW::string & name );

	// BWStreamDataHandler overrides
	void onStreamDataComplete( uint16 streamID, const BW::string & rDescription,
		BinaryIStream & rData );
};

typedef ScriptObjectPtr< ClientApp > ClientAppPtr;

BW_END_NAMESPACE

#endif // CLIENT_APP_HPP
