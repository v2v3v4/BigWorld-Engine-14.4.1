#ifndef METHOD_DESCRIPTION_HPP
#define METHOD_DESCRIPTION_HPP

#if defined( SCRIPT_PYTHON )
#include "Python.h"
#endif

#include "member_description.hpp"
#include "method_args.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

#include "network/basictypes.hpp"
#include "network/misc.hpp"

#include "bwentity/bwentity_api.hpp"
#include "resmgr/datasection.hpp"
#include "script/script_object.hpp"
#include "server/recording_options.hpp"



BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;
class DataType;
class DataSink;
class DataSource;
class ExposedMethodMessageRange;
class MD5;
class Watcher;

namespace Mercury
{
	class NetworkInterface;
}

typedef SmartPointer<Watcher> WatcherPtr;

typedef SmartPointer<DataType> DataTypePtr;

typedef uint16 MethodIndex;

/**
 *	This class is used to describe a method associated with an entity class.
 *
 *	@ingroup entity
 */
class MethodDescription : public MemberDescription
{
public:
	MethodDescription();
	MethodDescription( const MethodDescription & description );
	virtual ~MethodDescription();

	MethodDescription & operator=( const MethodDescription & description );

	enum Component
	{
		CLIENT,
		CELL,
		BASE,
		NUM_COMPONENTS
	};

	static bool staticInitTwoWay();

	static ScriptObject createErrorObject( const char * excType,
			ScriptObject args );
	static ScriptObject createErrorObject( const char * excType,
			const char * msg );

	bool parse( const BW::string & interfaceName,
			DataSectionPtr pSection, Component component,
			unsigned int * pNumLatestEventMembers );

	bool areValidArgs( bool exposedExplicit, ScriptObject args,
		bool generateException ) const;

	BWENTITY_API
	bool addToStream( DataSource & source, BinaryOStream & stream ) const;

	bool addToServerStream( DataSource & source, BinaryOStream & stream,
		EntityID sourceEntityID ) const;

	bool addToClientStream( DataSource & source, BinaryOStream & stream,
		EntityID clientEntityID ) const;

	ScriptTuple extractSourceEntityID( const ScriptTuple args,
		EntityID & sourceEntityID ) const;

	// The number of bytes streamed by addToStream if fixed,
	// -1 * the expected number of bytes needed to carry the length if
	// variable
	BWENTITY_API int streamSize( bool isFromServer ) const;

	ScriptTuple convertKeywordArgs( ScriptTuple args, ScriptDict kwargs ) const;

	bool callMethod( ScriptObject self,
		BinaryIStream & data,
		EntityID sourceID = 0,
		int replyID = -1,
		const Mercury::Address* pReplyAddr = NULL,
		Mercury::NetworkInterface * pInterface = NULL ) const;

	ScriptObject getMethodFrom( ScriptObject object ) const;

	ScriptObject getArgsAsTuple( BinaryIStream & data,
		EntityID sourceID = 0 ) const;

	BWENTITY_API
	bool createArgsFromStream( BinaryIStream & data, DataSink & sink,
		EntityID sourceID ) const;

	BWENTITY_API
	bool createArgsFromStream( BinaryIStream & data, DataSink & sink ) const;

	uint8 createReturnValuesOrFailureFromStream(
			BinaryIStream & data, ScriptObject & pResult ) const;
	bool createReturnValuesFromStream( BinaryIStream & data,
		DataSink & sink ) const;
	ScriptDict createDictFromTuple( ScriptTuple args ) const;

	bool isMethodHandledBy( const ScriptObject & classObject ) const;

	// uint returnValues() const;

	bool areValidReturnValues( ScriptTuple args ) const;

	bool sendReturnValues( DataSource & source, int replyID,
		const Mercury::Address & replyAddr,
		Mercury::NetworkInterface & networkInterface ) const;

	bool addReturnValues( DataSource & source, BinaryOStream & stream ) const;

	static void sendReturnValuesError( const char * excType,
			const char * msg,
			int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface );

	static void sendReturnValuesError( ScriptObject pExc, int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface );

	static void sendCurrentExceptionReturnValuesError( int replyID,
			const Mercury::Address & replyAddr,
			Mercury::NetworkInterface & networkInterface );

	static void addReturnValuesError( ScriptObject pExc,
			BinaryOStream & stream );

	BWENTITY_API bool hasReturnValues() const	{ return hasReturnValues_; }

	enum ReplayExposureLevel
	{
		REPLAY_EXPOSURE_LEVEL_NONE,
		REPLAY_EXPOSURE_LEVEL_OTHER_CLIENTS,
		REPLAY_EXPOSURE_LEVEL_ALL_CLIENTS,
	};

	/**
	 *	This method returns the level at which this method is exposed to
	 *	recording for replay, by default.
	 */
	ReplayExposureLevel replayExposureLevel() const 
	{ 
		return replayExposureLevel_; 
	}

	bool shouldRecord( RecordingOption recordingOption ) const;

	/// This method returns true if the method is exposed at all.
	BWENTITY_API bool isExposed() const
	{
		return !!(flags_ & (IS_EXPOSED_TO_ALL_CLIENTS |
					IS_EXPOSED_TO_OWN_CLIENT ));
	}

	/// This method returns true if the method is exposed to own client.
	bool isExposedToOwnClientOnly() const
	{
		return (flags_ & IS_EXPOSED_TO_OWN_CLIENT) &&
				!(flags_ & IS_EXPOSED_TO_ALL_CLIENTS);
	}

	/// This method returns true if the method is exposed to all clients.
	bool isExposedToAllClientsOnly() const
	{
		return (flags_ & IS_EXPOSED_TO_ALL_CLIENTS ) &&
				!(flags_ & IS_EXPOSED_TO_OWN_CLIENT);
	}

	bool isExposedToDefault() const
	{
		return !!((flags_ & IS_EXPOSED_TO_OWN_CLIENT) &&
					(flags_ & IS_EXPOSED_TO_ALL_CLIENTS));
	}

	void setExposedToAllClients()	{ flags_ |= IS_EXPOSED_TO_ALL_CLIENTS; }
	void setExposedToOwnClient()	{ flags_ |= IS_EXPOSED_TO_OWN_CLIENT; }
	void setExposedToDefault()
	{
		flags_ |= IS_EXPOSED_TO_OWN_CLIENT | IS_EXPOSED_TO_ALL_CLIENTS;
	}

	BWENTITY_API
	Component component() const				{ return Component(flags_ & 0x3); }
	void component( Component c )			{ flags_ |= c; }

	MethodIndex internalIndex() const { return MethodIndex( internalIndex_ ); }

	void internalIndex( int index )			{ internalIndex_ = index; }

	BWENTITY_API int exposedIndex() const { return exposedIndex_; }

	Mercury::MessageID exposedMsgID() const
		{ return Mercury::MessageID( exposedMsgID_ ); }

	int16 exposedSubMsgID() const
		{ return exposedSubMsgID_; }

	bool hasSubMsgID() const	{ return exposedSubMsgID_ != -1; }

	void addSubMessageIDToStream( BinaryOStream & stream ) const;

	void setExposedMsgID( int exposedID, int numExposed,
		const ExposedMethodMessageRange * pRange );

	void addToMD5( MD5 & md5, int legacyExposedIndex ) const;

	float priority() const;

	int clientSafety() const;

	BWENTITY_API const MethodArgs & args() const { return args_; }
	BWENTITY_API const MethodArgs & returnArgs() const { return returnValues_; }

	ScriptTuple argumentTypesAsScript() const
	{
		return ScriptTuple( args_.typesAsScript() );
	}

	ScriptObject returnValueTypesAsScript() const
	{
		return returnValues_.typesAsScript();
	}

	ScriptObject methodSignature() const;

#if ENABLE_WATCHERS
	static WatcherPtr pWatcher();
#endif

	int returnValuesStreamSize( bool isFromServer ) const;

	bool addImplementingComponent( const BW::string & componentName );
	BWENTITY_API bool isImplementedBy( const BW::string & componentName ) const;
	BWENTITY_API bool isComponentised() const;

	bool isSignatureEqual( const MethodDescription & other ) const;

private:
	virtual BW::string toString() const;

	/* Override from MemberDescription. */
	virtual int getServerToClientStreamSize() const
	{
		return this->streamSize( /* isFromServer */ true );
	}

	/* Override from MemberDescription. */
	virtual uint getVarLenHeaderSize() const;

	void handleException( int replyID,
		const Mercury::Address & replyAddr,
		Mercury::NetworkInterface & networkInterface,
	   	bool shouldPrintError ) const;

	enum
	{
		IS_EXPOSED_TO_ALL_CLIENTS = 0x4,
		IS_EXPOSED_TO_OWN_CLIENT = 0x8
	};

	// Lowest two bits for which component. Next for whether it can be called by
	// the client.
	uint8			flags_;

	MethodArgs		args_;

	MethodArgs		returnValues_;
	bool			hasReturnValues_;

	int				internalIndex_;			///< Used within the server
	int				exposedIndex_;			///< Used between client and server
	int16			exposedMsgID_;			///< Used between client and server
	int16			exposedSubMsgID_;		///< Used to extend address space

	float 			priority_;

	ReplayExposureLevel replayExposureLevel_;

	typedef BW::set< BW::string > ComponentNames;
	ComponentNames	components_;	///< List of components method implemented by
	bool			isComponentised_;	///< Is delegated to a component

#if ENABLE_WATCHERS
	mutable TimeStamp timeSpent_;
	mutable TimeStamp timeSpentMax_;
	mutable uint64 timesCalled_;
#endif

	// NOTE: If adding data, check the assignment operator.
};

#ifdef CODE_INLINE
#include "method_description.ipp"
#endif

BW_END_NAMESPACE

#endif // METHOD_DESCRIPTION_HPP
