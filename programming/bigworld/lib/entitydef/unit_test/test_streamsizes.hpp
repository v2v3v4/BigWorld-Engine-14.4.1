#ifndef TEST_STREAMSIZES_HPP
#define TEST_STREAMSIZES_HPP

#include "test_harness.hpp"
#if defined( MF_SERVER )
#include "unittest_mailbox.hpp"
#endif // defined( MF_SERVER )

#include "cstdmf/memory_stream.hpp"
#include "entitydef/data_type.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/xml_section.hpp"
#include "chunk/user_data_object.hpp"
#include "script/script_object.hpp"

#include "entitydef/data_types/blob_data_type.hpp"
#include "entitydef/data_types/float_data_types.hpp"
#include "entitydef/data_types/integer_data_type.hpp"
#include "entitydef/data_types/long_integer_data_type.hpp"
#if defined( MF_SERVER )
#include "entitydef/data_types/mailbox_data_type.hpp"
#endif // defined( MF_SERVER )
#include "entitydef/data_types/python_data_type.hpp"
#include "entitydef/data_types/string_data_type.hpp"
#include "chunk/user_data_object_link_data_type.hpp"
#include "entitydef/data_types/unicode_string_data_type.hpp"
#include "entitydef/data_types/vector_data_types.hpp"
#include "entitydef/data_types/class_data_type.hpp"
#include "entitydef/data_types/fixed_dict_data_type.hpp"
#include "entitydef/data_types/array_data_type.hpp"
#include "entitydef/data_types/tuple_data_type.hpp"
#include "entitydef/data_types/user_data_type.hpp"

#include "entitydef/property_owner.hpp"
#include "entitydef/property_change.hpp"

#include "entitydef/script_data_source.hpp"
#include "entitydef/script_data_sink.hpp"

#include "network/exposed_message_range.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Utility method to get a DataSectionPtr given some XML
 */
DataSectionPtr getDataSectionForXML( const BW::string& typeDataSectionStr )
{
	BW::stringstream typeDataStrStream;
	typeDataStrStream << typeDataSectionStr;
	XMLSectionPtr xml = XMLSection::createFromStream( "", typeDataStrStream );
	return xml;
}


/**
 *	Simple DataType testing fixture
 */
class DataTypeFixture: public EntityDefUnitTestHarness
{
public:
	DataTypeFixture( const BW::string & typeXml ):
		EntityDefUnitTestHarness()
	{

		pType_ = DataType::buildDataType( typeXml );
	}

	/**
	 *	Utility method for checking streamSize of fixed-size DataTypes
	 */
	bool fixedStreamSizeOkay( const ScriptObject object, bool & result ) const
	{
		if (!pType_)
		{
			return false;
		}

		if (pType_->streamSize() == -1)
		{
			return false;
		}
		MemoryOStream stream;
		ScriptDataSource source( object );
		MF_ASSERT( stream.size() == 0 );
		result = pType_->addToStream( source, stream,
			/* isPersistentOnly */ false );
		return stream.size() == pType_->streamSize();
	}


	/**
	 *	Utility method for checking streamSize of dynamic-sized DataTypes
	 */
	bool variableStreamSizeOkay() const
	{
		if (!pType_)
		{
			return false;
		}
		return pType_->streamSize() == -1;
	}

protected:
	DataTypePtr pType_;
};

// Macros for use in a test with a DataTypeFixture
#define CHECK_TYPE_IS( type ) CHECK( pType_ );								\
	CHECK( dynamic_cast< type * >( pType_.get() ) )

// This macro takes the given value and tests it against pType
// using fixedStreamSizeOkay.
// If it fails to stream, we check that it didn't pass the sameType test either
// (XXX: Some DataTypes return true when given things that fail isSameType)
#define CHECK_FIXED_TYPE_STREAMSIZE( value ) { \
	ScriptObject valueObj = ScriptObject::createFrom( value );				\
	bool sameType = pType_->isSameType( valueObj );							\
	bool streamResult = false;												\
	CHECK( this->fixedStreamSizeOkay( valueObj, streamResult ) 				\
			|| !streamResult ); 											\
	CHECK( streamResult || !sameType ); 									\
}

// Macros for shortcutting the simple tests used for dynamically-sized types
#define TEST_VARIABLE_TYPE_NAMED( type, xml, testName ) \
TEST_FP( DataTypeFixture, DataTypeFixture( xml ), type##_##testName )		\
{																			\
	CHECK_TYPE_IS( type );													\
	CHECK( this->variableStreamSizeOkay() );								\
}

#define TEST_VARIABLE_TYPE( type, xml )										\
TEST_FP( DataTypeFixture, DataTypeFixture( xml ), type##Test )				\
{																			\
	CHECK_TYPE_IS( type );													\
	CHECK( this->variableStreamSizeOkay() );								\
}

#if defined( MF_SERVER )
// Fixtures for the various types of tests
struct MailBoxDataTypeFixture : DataTypeFixture
{
	MailBoxDataTypeFixture( const BW::string & typeXml ) :
		DataTypeFixture( typeXml ),
		pBaseViaCellMB( new UnittestMailBox(),
			ScriptObject::FROM_NEW_REFERENCE )
	{
		pBaseViaCellMB->id( 2405 );
		pBaseViaCellMB->address( Mercury::Address( 0xc0180010, 0x4567 ) );
		pBaseViaCellMB->component( EntityMailBoxRef::BASE_VIA_CELL );
		pBaseViaCellMB->typeID( 7 );
	}

	ScriptObjectPtr< UnittestMailBox > pBaseViaCellMB;
};
#endif // defined( MF_SERVER )

#if defined( SCRIPT_PYTHON )
struct UserDataObjectLinkDataTypeFixture : DataTypeFixture
{
	UserDataObjectLinkDataTypeFixture( const BW::string & typeXml ) :
		DataTypeFixture( typeXml ),
		pUdo( NULL )
	{
		// This should surely be part of UserDataObjectType::init()?
		ScriptModule BWModule =
			ScriptModule::import( "BigWorld", ScriptErrorPrint() );
		MF_ASSERT( BWModule.exists() );
		MF_VERIFY( BWModule.addObject( "UserDataObject",
			&UserDataObject::s_type_, ScriptErrorPrint() ) );
		// Should this be part of the fixture?
		MF_VERIFY( UserDataObjectType::init() );

		pUdo = ScriptObjectPtr< UserDataObject >( UserDataObject::createRef(
			UniqueID( 0x40U, 0x80U, 0x400U, 0x1200U ) ),
			ScriptObject::FROM_NEW_REFERENCE );
		MF_ASSERT( pUdo.exists() );
		MF_ASSERT( ScriptObject::createFrom( pUdo ).exists() );
		MF_ASSERT(
			UserDataObject::Check( ScriptObject::createFrom( pUdo ) ) );
	}

	ScriptObjectPtr< UserDataObject > pUdo;
};
#else // defined( SCRIPT_PYTHON )
// TODO: UDORefDataType, which lacks addStream
#endif // defined( SCRIPT_PYTHON )

struct VectorDataTypeFixture : DataTypeFixture
{
	VectorDataTypeFixture( const BW::string & typeXml, int typeLength ) :
		DataTypeFixture( typeXml )
	{
		MF_ASSERT( typeLength > 0 );
		shortGood = ScriptTuple::create( typeLength - 1 );
		shortBad = ScriptTuple::create( typeLength - 1 );
		idealGood = ScriptTuple::create( typeLength );
		idealBad = ScriptTuple::create( typeLength );
		longGood = ScriptTuple::create( typeLength + 1 );
		longBad = ScriptTuple::create( typeLength + 1 );

		shortGood.setItem( 0, ScriptObject::createFrom( 0.f ) );
		shortBad.setItem( 0, ScriptObject::createFrom( "NotAFloat" ) );
		idealGood.setItem( 0, ScriptObject::createFrom( 0.f ) );
		idealBad.setItem( 0, ScriptObject::createFrom( "NotAFloat" ) );
		longGood.setItem( 0, ScriptObject::createFrom( 0.f ) );
		longBad.setItem( 0, ScriptObject::createFrom( "NotAFloat" ) );

		for (int i = 1; i < typeLength + 1; ++i)
		{
			if (i < typeLength - 1)
			{
				shortGood.setItem( i, ScriptObject::createFrom( 0.f ) );
				shortBad.setItem( i, ScriptObject::createFrom( 0.f ) );
			}

			if (i < typeLength)
			{
				idealGood.setItem( i, ScriptObject::createFrom( 0.f ) );
				idealBad.setItem( i, ScriptObject::createFrom( 0.f ) );
			}
			longGood.setItem( i, ScriptObject::createFrom( 0.f ) );
			longBad.setItem( i, ScriptObject::createFrom( 0.f ) );
		}

		vector2 = ScriptObject::createFrom( Vector2( 0.f, 0.f ) );
		vector3 = ScriptObject::createFrom( Vector3( 0.f, 0.f, 0.f ) );
		vector4 = ScriptObject::createFrom( Vector4( 0.f, 0.f, 0.f, 0.f ) );

		notAVector = ScriptObject::createFrom( 0.f );
	}

	ScriptTuple shortGood;
	ScriptTuple shortBad;
	ScriptTuple idealGood;
	ScriptTuple idealBad;
	ScriptTuple longGood;
	ScriptTuple longBad;

	ScriptObject vector2;
	ScriptObject vector3;
	ScriptObject vector4;

	ScriptObject notAVector;
};

struct DictionaryDataTypeFixture : DataTypeFixture
{
	DictionaryDataTypeFixture( const BW::string & typeXml ) :
		DataTypeFixture( typeXml )
	{
		oneInt8 = ScriptDict::create();
		oneInt8.setItem( "a", ScriptObject::createFrom( (int8)0 ),
			ScriptErrorPrint() );

		twoInt8s = ScriptDict::create();
		twoInt8s.setItem( "a", ScriptObject::createFrom( (int8)0 ),
			ScriptErrorPrint() );
		twoInt8s.setItem( "b", ScriptObject::createFrom( (int8)0 ),
			ScriptErrorPrint() );

		threeInt8s = ScriptDict::create();
		threeInt8s.setItem( "a", ScriptObject::createFrom( (int8)0 ),
			ScriptErrorPrint() );
		threeInt8s.setItem( "b", ScriptObject::createFrom( (int8)0 ),
			ScriptErrorPrint() );
		threeInt8s.setItem( "c", ScriptObject::createFrom( (int8)0 ),
			ScriptErrorPrint() );

		oneString = ScriptDict::create();
		oneString.setItem( "a", ScriptObject::createFrom( "String" ),
			ScriptErrorPrint() );

		twoStrings = ScriptDict::create();
		twoStrings.setItem( "a", ScriptObject::createFrom( "String" ),
			ScriptErrorPrint() );
		twoStrings.setItem( "b", ScriptObject::createFrom( "String" ),
			ScriptErrorPrint() );

		threeStrings = ScriptDict::create();
		threeStrings.setItem( "a", ScriptObject::createFrom( "String" ),
			ScriptErrorPrint() );
		threeStrings.setItem( "b", ScriptObject::createFrom( "String" ),
			ScriptErrorPrint() );
		threeStrings.setItem( "c", ScriptObject::createFrom( "String" ),
			ScriptErrorPrint() );

		notADict = ScriptObject::createFrom( 0 );
	}

	ScriptDict oneInt8;
	ScriptDict oneString;
	ScriptDict twoInt8s;
	ScriptDict twoStrings;
	ScriptDict threeInt8s;
	ScriptDict threeStrings;

	ScriptObject notADict;
};


struct SequenceDataTypeFixture : DataTypeFixture
{
	SequenceDataTypeFixture( const BW::string & typeXml, int idealLength ) :
		DataTypeFixture( typeXml )
	{
		MF_ASSERT( idealLength > 0 );
		shortInts = ScriptList::create();
		shortStrings = ScriptList::create();
		idealInts = ScriptList::create();
		idealStrings = ScriptList::create();
		longInts = ScriptList::create();
		longStrings = ScriptList::create();

		anInt = ScriptObject::createFrom( (int8)0 );
		aString = ScriptObject::createFrom( "STRING" );
		aFloat = ScriptObject::createFrom( 0.f );

		for (int i = 0; i < idealLength + 1; ++i)
		{
			if (i < idealLength - 1)
			{
				shortInts.append( anInt );
				shortStrings.append( aString );
			}

			if (i < idealLength)
			{
				idealInts.append( anInt );
				idealStrings.append( aString );
			}
			longInts.append( anInt );
			longStrings.append( aString );
		}

		MF_ASSERT( shortInts.size() == idealLength - 1 );
		MF_ASSERT( shortStrings.size() == idealLength - 1 );
		MF_ASSERT( idealInts.size() == idealLength );
		MF_ASSERT( idealStrings.size() == idealLength );
		MF_ASSERT( longInts.size() == idealLength + 1 );
		MF_ASSERT( longStrings.size() == idealLength + 1 );
	}

	ScriptObject anInt;
	ScriptObject aString;
	ScriptObject aFloat;

	ScriptList shortInts;
	ScriptList shortStrings;
	ScriptList idealInts;
	ScriptList idealStrings;
	ScriptList longInts;
	ScriptList longStrings;
};


/**
 *	Simple DataDescription testing fixture
 */
struct DataDescriptionFixture: public EntityDefUnitTestHarness
{
	DataDescriptionFixture( const BW::string & typeXml ) :
		EntityDefUnitTestHarness(),
		pDDTemplate( new DataDescription() )
	{
		if ( !pDDTemplate->parse( getDataSectionForXML( typeXml ), "Properties",
			BW::string(), NULL ) )
		{
			ERROR_MSG( "DataDescriptionFixture: Failed to parse %s\n",
				typeXml.c_str() );
			delete pDDTemplate;
			pDDTemplate = NULL;
			return;
		}
	}

	virtual ~DataDescriptionFixture() { delete pDDTemplate; };

	DataDescription * pDDTemplate;

	/**
	 *	Utility method for checking streamSize of fixed-size DataDescriptions
	 */
	template< typename TYPE >
	bool fixedStreamSizeOkay( const TYPE value, int index,
		int clientServerIndex = -1 )
	{
		if (pDDTemplate == NULL)
		{
			return false;
		}

		ScriptObject valueObj = ScriptObject::createFrom( value );

		DataDescription DD = *pDDTemplate;
		DD.index( index );
		DD.clientServerFullIndex( clientServerIndex );

		if (DD.streamSize() < 0)
		{
			return false;
		}

		MemoryOStream stream;
		ScriptDataSource source( valueObj );
		MF_ASSERT( stream.size() == 0 );
		DD.addToStream( source, stream, /* isPersistentOnly */ false );
		return stream.size() == DD.streamSize();
	}

	/**
	 *	Utility method for checking streamSize of dynamic-sized DataDescriptions
	 */
	template< typename TYPE >
	bool variableStreamSizeOkay( const TYPE value, int index,
		int clientServerIndex = -1 )
	{
		if (pDDTemplate == NULL)
		{
			return false;
		}

		ScriptObject valueObj = ScriptObject::createFrom( value );

		DataDescription DD = *pDDTemplate;
		DD.index( index );
		DD.clientServerFullIndex( clientServerIndex );

		return DD.streamSize() < 0;
	}
};

// Macros for use in a test with a DataDescriptionFixture
#define CHECK_DESCRIPTION_TYPE_IS( type ) CHECK( pDDTemplate ); \
	CHECK( dynamic_cast< type * >( pDDTemplate->dataType() ) )


/**
 *	Simple MethodArgs testing fixture
 */
class MethodArgsFixture: public EntityDefUnitTestHarness
{
public:
	MethodArgsFixture( const BW::string & typeXml ) :
		EntityDefUnitTestHarness(),
		pMA( new MethodArgs() )
	{
		if ( !pMA->parse( getDataSectionForXML( typeXml ) ) )
		{
			ERROR_MSG( "MethodArgsFixture: Failed to parse %s\n",
				typeXml.c_str() );
			pMA = NULL;
			return;
		}

		int argCount = pMA->size();
		MF_ASSERT( argCount >= 0 );

		anInt = ScriptObject::createFrom( (int8)0 );
		aString = ScriptObject::createFrom( "STRING" );

		idealInts = ScriptTuple::create( argCount );
		idealStrings = ScriptTuple::create( argCount );
		idealInterleavedIntsStrings = ScriptTuple::create( argCount );
		idealInterleavedStringsInts = ScriptTuple::create( argCount );
		longInts = ScriptTuple::create( argCount + 1 );
		longStrings = ScriptTuple::create( argCount + 1 );
		longInterleavedIntsStrings = ScriptTuple::create( argCount + 1 );
		longInterleavedStringsInts = ScriptTuple::create( argCount + 1 );

		for (int i = 0; i < argCount + 1; ++i)
		{
			if (i < argCount)
			{
				idealInts.setItem( i, anInt );
				idealStrings.setItem( i, aString );
				idealInterleavedIntsStrings.setItem( i,
					i % 2 ? aString : anInt );
				idealInterleavedStringsInts.setItem( i,
					i % 2 ? anInt : aString );
			}
			longInts.setItem( i, anInt );
			longStrings.setItem( i, aString );
			longInterleavedIntsStrings.setItem( i, i % 2 ? aString : anInt );
			longInterleavedStringsInts.setItem( i, i % 2 ? anInt : aString );
		}

		MF_ASSERT( idealInts.size() == argCount );
		MF_ASSERT( idealStrings.size() == argCount );
		MF_ASSERT( idealInterleavedIntsStrings.size() == argCount );
		MF_ASSERT( idealInterleavedStringsInts.size() == argCount );

		MF_ASSERT( longInts.size() == argCount + 1 );
		MF_ASSERT( longStrings.size() == argCount + 1 );
		MF_ASSERT( longInterleavedIntsStrings.size() == argCount + 1 );
		MF_ASSERT( longInterleavedStringsInts.size() == argCount + 1 );
	}

	virtual ~MethodArgsFixture() { delete pMA; };

	MethodArgs * pMA;

	ScriptObject anInt;
	ScriptObject aString;

	ScriptTuple idealInts;
	ScriptTuple idealStrings;
	ScriptTuple idealInterleavedIntsStrings;
	ScriptTuple idealInterleavedStringsInts;
	ScriptTuple longInts;
	ScriptTuple longStrings;
	ScriptTuple longInterleavedIntsStrings;
	ScriptTuple longInterleavedStringsInts;

	/**
	 *	Utility method for checking streamSize of fixed-size MethodArgs
	 */
	bool fixedStreamSizeOkay( const ScriptTuple & args, bool & result )
	{
		if (!pMA)
		{
			return false;
		}

		if (pMA->streamSize() == -1)
		{
			return false;
		}

		MemoryOStream stream;
		MF_ASSERT( stream.size() == 0 );
		ScriptDataSource source( args );
		result = pMA->addToStream( source, stream );
		return stream.size() == pMA->streamSize();
	}

	/**
	 *	Utility method for checking streamSize of dynamic-sized MethodArgs
	 */
	bool variableStreamSizeOkay()
	{
		if (!pMA)
		{
			return false;
		}

		return pMA->streamSize() == -1;
	}
};

// Macros for use in a test with a MethodArgsFixture

// This macro takes the given ScriptTuple and tests it against pMA
// using fixedStreamSizeOkay.
// If it fails to stream, we check that it didn't pass the checkValid test
// either
#define CHECK_ARGS_FIXED_STREAMSIZE( valueTuple ) { \
	bool validArgs = pMA->checkValid( valueTuple, "" ); \
	CHECK( validArgs || PyErr_Occurred() ); \
	PyErr_Clear(); \
	bool streamResult = false; \
	CHECK( this->fixedStreamSizeOkay( valueTuple, streamResult ) ); \
	CHECK( streamResult || !validArgs ); \
}

// Macros for shortcutting the simple tests used for dynamically-sized types
#define TEST_ARGS_VARIABLE_STREAMSIZE_NAMED( xml, testName ) \
TEST_FP( MethodArgsFixture, MethodArgsFixture( xml ), MethodArgs_##testName ) \
{ \
	CHECK( this->variableStreamSizeOkay() ); \
}

/**
 *	Simple MethodDescription testing fixture
 */
class MethodDescriptionFixture: public EntityDefUnitTestHarness
{
public:
	MethodDescriptionFixture( const BW::string & methodXml,
		MethodDescription::Component component ) :
		EntityDefUnitTestHarness(),
		pMDTemplate_( new MethodDescription )
	{
		if (!pMDTemplate_->parse( "TestEntity",
				getDataSectionForXML( methodXml ), component, NULL ))
		{
			ERROR_MSG( "MethodDescriptionFixture: Failed to parse %s\n",
				methodXml.c_str() );
			delete pMDTemplate_;
			pMDTemplate_ = NULL;
			return;
		}

		if (component == MethodDescription::CLIENT)
		{
			pMDTemplate_->setExposedToAllClients();
		}
	}

	virtual ~MethodDescriptionFixture() { delete pMDTemplate_; }

	MethodDescription createMethodDescription( int index,
			int exposedIndex ) const
	{
		MethodDescription methodDesc = *pMDTemplate_;

		MF_ASSERT( methodDesc.isExposed() == (exposedIndex >= 0) );

		methodDesc.internalIndex( index );

		const int FIRST_MSG_ID = 56;
		const int LAST_MSG_ID = 92;

		const int NUM_EXPOSED_METHODS = 800;

		const ExposedMethodMessageRange range( FIRST_MSG_ID, LAST_MSG_ID );

		methodDesc.setExposedMsgID( exposedIndex, NUM_EXPOSED_METHODS, &range );

		return methodDesc;
	}

	/**
	 *	Utility method for checking streamSize of fixed-size MethodDescriptions
	 */
	bool fixedStreamSizeOkay( MethodDescription::Component source,
		ScriptTuple args, int index, int exposedIndex = -1 )
	{
		const bool isFromServer = (source != MethodDescription::CLIENT);

		if (pMDTemplate_ == NULL)
		{
			return false;
		}

		MethodDescription methodDesc =
			this->createMethodDescription( index, exposedIndex );

		MF_ASSERT( methodDesc.areValidArgs( isFromServer, args,
			/* generateException */ false ) );

		MemoryOStream stream;
		MF_ASSERT( stream.size() == 0 );

		if (isFromServer)
		{
			if (methodDesc.component() == MethodDescription::CLIENT)
			{
				// server->client stream
				ScriptDataSource dataSource( args );
				MF_VERIFY( methodDesc.addToClientStream( dataSource, stream,
					NULL_ENTITY_ID ) );
			}
			else
			{
				// server->server stream
				EntityID sourceEntityID;
				ScriptTuple remainingArgs =
					methodDesc.extractSourceEntityID( args, sourceEntityID );

				if (methodDesc.component() == MethodDescription::CELL &&
					methodDesc.isExposed())
				{
					MF_ASSERT( remainingArgs.size() == args.size() - 1 );
				}
				else
				{
					MF_ASSERT( remainingArgs.size() == args.size() );
					MF_VERIFY(
						!remainingArgs.compareTo( args, ScriptErrorPrint() ) );
					MF_ASSERT( sourceEntityID == NULL_ENTITY_ID );
				}

				ScriptDataSource dataSource( remainingArgs );

				MF_ASSERT( remainingArgs.exists() );
				MF_VERIFY( methodDesc.addToServerStream( dataSource, stream,
					sourceEntityID ) );
			}
		}
		else
		{
			// client->server stream
			ScriptDataSource dataSource( args );

			MF_ASSERT( methodDesc.component() != MethodDescription::CLIENT );
			MF_VERIFY( methodDesc.addToStream( dataSource, stream ) );
		}

		/*
		HACK_MSG( "%d, %d\n",
			stream.size(), methodDesc.streamSize( isFromServer ) );
		*/

		return stream.size() == methodDesc.streamSize( isFromServer );
	}

	/**
	 *	Utility method for checking streamSize of dynamic-size MethodDescriptions
	 */
	bool variableStreamSizeOkay( MethodDescription::Component source,
		ScriptTuple args, int index, int exposedIndex = -1 )
	{
		const bool isFromServer = (source != MethodDescription::CLIENT);

		if (pMDTemplate_ == NULL)
		{
			return false;
		}

		MethodDescription methodDesc =
			this->createMethodDescription( index, exposedIndex );

		MF_ASSERT( methodDesc.areValidArgs( isFromServer, args,
			/* generateException */ false ) );

		return methodDesc.streamSize( isFromServer ) < 0;
	}

private:
	MethodDescription * pMDTemplate_;
};

// Macros for use in a test with a MethodDescriptionFixture
#define CHECK_METHOD_FIXED_STREAMSIZE( source, args, index, exposedIndex ) \
	CHECK( this->fixedStreamSizeOkay( MethodDescription::source, args, index, \
		exposedIndex ) );

#define CHECK_METHOD_VARIABLE_STREAMSIZE( source, args, index, exposedIndex ) \
	CHECK( this->variableStreamSizeOkay( MethodDescription::source, args, index, \
		exposedIndex ) );

/**
 *	Not-so-Simple PropertyChange testing fixture
 */

/*
 *	Testing PropertyChange is complicated. PropertyChange needs a PropertyOwner.
 *	So we dummy one up, based on CellApp's Entity class's implementation
 *	It also fufils some of EntityDescription's tasks
 *	This uses maps and indexes mainly to make it potentially useful for
 *	other unit tests. The PropertyChange streamSize unit tests only deal
 *	with one slot at a time.
 */
struct DummyTopLevelPropertyOwner : public TopLevelPropertyOwner
{
	DummyTopLevelPropertyOwner() :
		maxKey_( -1 ),
		lastCalculatedStreamSize_( 0 ),
		lastMeasuredStreamSize_( 0 ),
		streamed_( false ) {}

	/* Our own interface */
	bool setTypeAndValue( int index, int clientServerIndex,
		const DataDescription & description, ScriptObject value )
	{
		if (!value)
		{
			value = description.pInitialValue();
		}

		// XXX: isCorrectType is non-const because FixedDictDataType::isSameType
		// does on-demand work in asOwner
		if (!const_cast<DataDescription &>(description).isCorrectType(
			value ))
		{
			return false;
		}
		MF_ASSERT( !values_[ index ] );
		descriptions_[ index ] = description;
		descriptions_[ index ].index( index );
		descriptions_[ index ].clientServerFullIndex( clientServerIndex );
		// XXX: attach is non-const because FixedDictDataType::attach
		// does on-demand work in asOwner
		values_[ index ] = const_cast<DataDescription &>(description).dataType()
			->attach( value, this, index );
		return true;
	}

	ScriptObject getValue( int index )
	{
		MF_ASSERT( values_[ index ] );
		return values_[ index ];
	}

	void setValue( int index, ScriptObject value )
	{
		streamed_ = false;
		MF_ASSERT( values_[ index ] );
		DataDescription & description = descriptions_[ index ];
		DataType & dataType = *description.dataType();

		// This one's a little tricky. It modifies-in-place its first parameter,
		// so you need to pass a reference to the relevant values_ entry.
		MF_VERIFY( this->changeOwnedProperty(
			values_[index], value, dataType, index ) );
	}

	int lastCalculatedStreamSize() const
	{
		return lastCalculatedStreamSize_;
	}

	int lastMeasuredStreamSize() const
	{
		return lastMeasuredStreamSize_;
	}

	bool streamed() const
	{
		return streamed_;
	}

	void streamed( bool value )
	{
		streamed_ = value;
	}

	/* TopLevelPropertyOwner interface */
	virtual bool onOwnedPropertyChanged( PropertyChange & change )
	{
		// XXX: Would we be better off putting aside the change?
		// We'd need to copy it and allocate a new change of the
		// correct type, so I don't think it's worth the confusion
		// to expose the actual change
		MF_ASSERT( !streamed_ );
		DataDescription & description = descriptions_[ change.rootIndex() ];
		MemoryOStream stream;
		MF_ASSERT( stream.size() == 0 );
		// We're only interested in streamSize, which doesn't apply for
		// nested changes.
		MF_ASSERT( !change.isNestedChange() );
		change.addValueToStream( stream );
		lastCalculatedStreamSize_ = description.streamSize();
		lastMeasuredStreamSize_ = stream.size();
		streamed_ = true;

		return true;
	}

	virtual bool getTopLevelOwner( PropertyChange & change,
			PropertyOwnerBase *& rpTopLevelOwner )
	{
		DEBUG_MSG( "getTopLevelOwner: %p\n", this );
		DEBUG_BACKTRACE();
		rpTopLevelOwner = this;
		return true;
	}

	// called going to the leaves of the tree
	virtual int getNumOwnedProperties() const
	{
		// Pretend we have all slots full
		return maxKey_ + 1;
	}

	virtual PropertyOwnerBase * getChildPropertyOwner( int index ) const
	{
		Descriptions::const_iterator descIt = descriptions_.find( index );
		if (descIt == descriptions_.end())
		{
			return NULL;
		}
		const DataDescription & DD = descIt->second;
		Values::const_iterator valueIt = values_.find( index );
		MF_ASSERT( valueIt != values_.end() );
		ScriptObject pPyObj = valueIt->second;

		// XXX: asOwner is non-const because FixedDictDataType does on-demand
		// work in asOwner
		return const_cast< DataType * >(DD.dataType())->
			asOwner( pPyObj );
	}

	virtual ScriptObject setOwnedProperty( int index, BinaryIStream & data )
	{
		Descriptions::const_iterator descIt = descriptions_.find( index );
		if (descIt == descriptions_.end())
		{
			return ScriptObject();
		}
		// Deconstifying, nothing being done here should change the DataType,
		// but I assume this is FixedDictDataType doing on-demand housekeeping.
		DataType * pDataType =
			const_cast< DataType * >(descIt->second.dataType());

		Values::const_iterator valueIt = values_.find( index );
		MF_ASSERT( valueIt != values_.end() );
		ScriptObject & slotRef = values_[ index ];
		ScriptObject pOldObj = values_[ index ];

		ScriptDataSink sink;

		if (!pDataType->createFromStream( data, sink, false ))
		{
			return ScriptObject();
		}

		ScriptObject pNewObj = sink.finalise();

		if (pOldObj != pNewObj)
		{
			pDataType->detach( slotRef );
			slotRef = pDataType->attach( pNewObj, this, index );
		}

		return pOldObj;
	}

private:
	typedef BW::map< int, DataDescription > Descriptions;
	typedef BW::map< int, ScriptObject > Values;
	Descriptions descriptions_;
	Values values_;
	int maxKey_;
	int lastCalculatedStreamSize_;
	int lastMeasuredStreamSize_;
	bool streamed_;
};

class PropertyChangeFixture: public EntityDefUnitTestHarness
{
public:
	PropertyChangeFixture( const BW::string & typeXml ) :
		EntityDefUnitTestHarness(),
		pDDTemplate( new DataDescription() ),
		index( 0 )
	{
		if ( !pDDTemplate->parse( getDataSectionForXML( typeXml ), "Properties",
			BW::string(), NULL ) )
		{
			ERROR_MSG( "PropertyChangeFixture: Failed to parse %s\n",
				typeXml.c_str() );
			delete pDDTemplate;
			pDDTemplate = NULL;
			return;
		}
	}

	virtual ~PropertyChangeFixture() { delete pDDTemplate; };

	int addProperty( int clientServerFullIndex )
	{
		if (!pDDTemplate)
		{
			return -1;
		}

		owner.setTypeAndValue( index, clientServerFullIndex, *pDDTemplate,
			ScriptObject() );

		return index++;
	}

	bool lastValueFixedSize() const
	{
		return owner.lastCalculatedStreamSize() >= 0;
	}

	bool lastValueFixedSizeOkay() const
	{
		return owner.lastCalculatedStreamSize() ==
			owner.lastMeasuredStreamSize();
	}

	DataDescription * pDDTemplate;
	DummyTopLevelPropertyOwner owner;
	int index;

};

BW_END_NAMESPACE

#endif // TEST_STREAMSIZES_HPP
