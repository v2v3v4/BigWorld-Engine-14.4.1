#include "pch.hpp"

// Fixtures and macros are here
// Macros that create entire tests start with TEST
// Macros that run checks start with CHECK
#include "test_streamsizes.hpp"


BW_BEGIN_NAMESPACE

/**
 *	Test the follow DataTypes for streamSize coherency:
 *	BLOB: BlobDataType - Variable Size
 *	FLOAT32, FLOAT64: FloatDataType<> - Fixed Size
 *	(U)INT8, (U)INT16, (U)INT32, INT64: IntegerDataType<> - Fixed Size
 *	 - UINT32, INT64 if _LP64
 *	UINT32, (U)INT64: LongIntegerDataType<> - Fixed Size
 *	 - UINT32, INT64 if !_LP64
 *	MAILBOX: MailBoxDataType - Variable Size
 *	 - Server only, probably fixed but I don't know how...
 *	PYTHON: PythonDataType - Variable Size
 *	STRING: StringDataType - Variable Size
 *	UDO_REF: UserDataObjectLinkDataType - Variable Size
 *	 - Not in Objective-C
 *	UDO_REF: UDORefDataType - Variable Size
 *	 - Objective-C only, TODO: not tested here
 *	UNICODE_STRING: UnicodeStringDataType - Variable Size
 *	VECTOR2, VECTOR3, VECTOR4: VectorDataType<> - Fixed Size
 *	CLASS: ClassDataType - Variable Size
 *	 - unless None is not allowed, and all fields are fixed-size
 *	FIXED_DICT: FixedDictDataType - Variable Size
 *	 - unless None is not allowed, and all fields are fixed-size
 *	ARRAY, TUPLE: SequenceDataType - Variable Size
 *	 - unless fixed-length sequence of fixed-size elements
 *	USER_TYPE: UserDataType - Variable Size
 */

// BlobDataType
TEST_VARIABLE_TYPE( BlobDataType, "<type> BLOB </type>" );

// FloatDataType
TEST_FP( DataTypeFixture, DataTypeFixture( "<type> FLOAT32 </type>" ),
	FloatDataType_float )
{
	CHECK_TYPE_IS( FloatDataType< float > );
	CHECK_FIXED_TYPE_STREAMSIZE( 0.f );
	CHECK_FIXED_TYPE_STREAMSIZE( "NoAtFloat" );
}

TEST_FP( DataTypeFixture, DataTypeFixture( "<type> FLOAT64 </type>" ),
	FloatDataType_double )
{
	CHECK_TYPE_IS( FloatDataType< double > );
	CHECK_FIXED_TYPE_STREAMSIZE( 0.0 );
	CHECK_FIXED_TYPE_STREAMSIZE( "NoAtDouble" );
}

// IntegerDataType and LongIntegerDataType
#define TEST_INTEGER_TYPE( type, xmlType ) TEST_FP( DataTypeFixture, \
	DataTypeFixture( "<type> " #xmlType " </type>" ), \
	IntegerDataType_##type ) \
{ \
	CHECK_TYPE_IS( IntegerDataType< type > ); \
	CHECK_FIXED_TYPE_STREAMSIZE( (type)0 ); \
	CHECK_FIXED_TYPE_STREAMSIZE( "NotAnInteger" ); \
}

#define TEST_LONGINTEGER_TYPE( type, xmlType ) TEST_FP( DataTypeFixture, \
	DataTypeFixture( "<type> " #xmlType " </type>" ), \
	LongIntegerDataType_##type ) \
{ \
	CHECK_TYPE_IS( LongIntegerDataType< type > ); \
	CHECK_FIXED_TYPE_STREAMSIZE( (type)0 ); \
	CHECK_FIXED_TYPE_STREAMSIZE( "NotALongInteger" ); \
}

TEST_INTEGER_TYPE( int8, INT8 );
TEST_INTEGER_TYPE( uint8, UINT8 );
TEST_INTEGER_TYPE( int16, INT16 );
TEST_INTEGER_TYPE( uint16, UINT16 );
TEST_INTEGER_TYPE( int32, INT32 );
#ifdef _LP64
TEST_INTEGER_TYPE( uint32, UINT32 );
TEST_INTEGER_TYPE( int64, INT64 );
#else // _LP64
TEST_LONGINTEGER_TYPE( uint32, UINT32 );
TEST_LONGINTEGER_TYPE( int64, INT64 );
#endif // _LP64
TEST_LONGINTEGER_TYPE( uint64, UINT64 );

#if defined( MF_SERVER )
// MailBoxDataType
TEST_FP( MailBoxDataTypeFixture,
	MailBoxDataTypeFixture( "<type> MAILBOX </type>" ),
	MailBoxDataTypeTest )
{
	CHECK_TYPE_IS( MailBoxDataType );
	CHECK_FIXED_TYPE_STREAMSIZE( pBaseViaCellMB );
}
#endif // defined( MF_SERVER )

// PythonDataType
// XXX: addToStream fails when given NULL
TEST_VARIABLE_TYPE( PythonDataType, "<type> PYTHON </type>" );

// StringDataType
TEST_VARIABLE_TYPE( StringDataType, "<type> STRING </type>" );

#if defined( SCRIPT_PYTHON )
// UserDataObjectLinkDataType
TEST_FP( UserDataObjectLinkDataTypeFixture,
	UserDataObjectLinkDataTypeFixture( "<type> UDO_REF </type>" ),
	UserDataObjectLinkDataTypeTest )
{
	CHECK_TYPE_IS( UserDataObjectLinkDataType );

	CHECK_FIXED_TYPE_STREAMSIZE( pUdo );
	CHECK_FIXED_TYPE_STREAMSIZE( Py_None );
	CHECK_FIXED_TYPE_STREAMSIZE( "NotAUserDataObjectLink" );
}
#else // defined( SCRIPT_PYTHON )
// TODO: UDORefDataType, which lacks addStream
#endif // defined( SCRIPT_PYTHON )

// UnicodeStringDataType
// XXX: addToStream fails if PyUnicode_AsUTF8String fails
TEST_VARIABLE_TYPE( UnicodeStringDataType, "<type> UNICODE_STRING </type>" );

// VectorDataType
#define TEST_VECTOR_TYPE( length ) TEST_FP( VectorDataTypeFixture, \
	VectorDataTypeFixture( "<type> VECTOR" #length " </type>", length), \
	VectorDataType_Vector##length ) \
{ \
	CHECK_TYPE_IS( VectorDataType< Vector##length > ); \
 \
	CHECK_FIXED_TYPE_STREAMSIZE( vector2 ); \
	CHECK_FIXED_TYPE_STREAMSIZE( vector3 ); \
	CHECK_FIXED_TYPE_STREAMSIZE( vector4 ); \
 \
	CHECK_FIXED_TYPE_STREAMSIZE( shortGood ); \
	CHECK_FIXED_TYPE_STREAMSIZE( shortBad ); \
	CHECK_FIXED_TYPE_STREAMSIZE( idealGood ); \
	CHECK_FIXED_TYPE_STREAMSIZE( idealBad ); \
	CHECK_FIXED_TYPE_STREAMSIZE( longGood ); \
	CHECK_FIXED_TYPE_STREAMSIZE( longBad ); \
 \
	CHECK_FIXED_TYPE_STREAMSIZE( notAVector ); \
}

TEST_VECTOR_TYPE( 2 );
TEST_VECTOR_TYPE( 3 );
TEST_VECTOR_TYPE( 4 );

// DictionaryDataType: ClassDataType
TEST_VARIABLE_TYPE_NAMED( ClassDataType,
	"<type> CLASS <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> INT8 </Type> </b> "
	"</Properties> <AllowNone> false </AllowNone> </type>" ,
	twoInt8s_disallowNone );

TEST_VARIABLE_TYPE_NAMED( ClassDataType, 
	"<type> CLASS <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> INT8 </Type> </b> "
	"</Properties> <AllowNone> true </AllowNone> </type>",
	twoInt8s_allowNone );

TEST_VARIABLE_TYPE_NAMED( ClassDataType, 
	"<type> CLASS <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> false </AllowNone> </type>",
	Int8AndString_disallowNone );

TEST_VARIABLE_TYPE_NAMED( ClassDataType, 
	"<type> CLASS <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> true </AllowNone> </type>",
	Int8AndString_allowNone );

TEST_VARIABLE_TYPE_NAMED( ClassDataType, 
	"<type> CLASS <Properties> "
		" <a> <Type> STRING </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> false </AllowNone> </type>",
	twoStrings_disallowNone );

TEST_VARIABLE_TYPE_NAMED( ClassDataType, 
	"<type> CLASS <Properties> "
		" <a> <Type> STRING </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> true </AllowNone> </type>",
	twoStrings_allowNone );

// TODO: implementedBy... Somehow need to provide the
// relevant module.
TEST_FP( DictionaryDataTypeFixture, DictionaryDataTypeFixture(
	"<type> FIXED_DICT <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> INT8 </Type> </b> "
	"</Properties> <AllowNone> false </AllowNone> </type>" ),
	FixedDictDataType_twoInt8s_disallowNone )
{
	CHECK_TYPE_IS( FixedDictDataType );
	// TODO: Check allowNone()?

	CHECK_FIXED_TYPE_STREAMSIZE( oneInt8 );
	CHECK_FIXED_TYPE_STREAMSIZE( oneString );

	CHECK_FIXED_TYPE_STREAMSIZE( twoInt8s );
	CHECK_FIXED_TYPE_STREAMSIZE( twoStrings );

	CHECK_FIXED_TYPE_STREAMSIZE( threeInt8s );
	CHECK_FIXED_TYPE_STREAMSIZE( threeStrings );

	CHECK_FIXED_TYPE_STREAMSIZE( notADict );
}

// DictionaryDataType: FixedDictDataType
TEST_VARIABLE_TYPE_NAMED( FixedDictDataType, 
	"<type> FIXED_DICT <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> INT8 </Type> </b> "
	"</Properties> <AllowNone> true </AllowNone> </type>",
	twoInt8s_allowNone );

TEST_VARIABLE_TYPE_NAMED( FixedDictDataType, 
	"<type> FIXED_DICT <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> false </AllowNone> </type>",
	Int8AndString_disallowNone );

TEST_VARIABLE_TYPE_NAMED( FixedDictDataType, 
	"<type> FIXED_DICT <Properties> "
		" <a> <Type> INT8 </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> true </AllowNone> </type>",
	Int8AndString_allowNone );

TEST_VARIABLE_TYPE_NAMED( FixedDictDataType, 
	"<type> FIXED_DICT <Properties> "
		" <a> <Type> STRING </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> false </AllowNone> </type>",
	twoStrings_disallowNone );

TEST_VARIABLE_TYPE_NAMED( FixedDictDataType, 
	"<type> FIXED_DICT <Properties> "
		" <a> <Type> STRING </Type> </a> "
		" <b> <Type> STRING </Type> </b> "
	"</Properties> <AllowNone> true </AllowNone> </type>",
	twoStrings_allowNone );

// SequenceDataType: ArrayDataType
TEST_FP( SequenceDataTypeFixture, SequenceDataTypeFixture(
		"<type> ARRAY <size> 5 </size> <of> INT8 </of> </type>", 5 ),
	ArrayDataType_int8s_size5 )
{
	CHECK_TYPE_IS( ArrayDataType );

	CHECK_FIXED_TYPE_STREAMSIZE( anInt );
	CHECK_FIXED_TYPE_STREAMSIZE( aString );
	CHECK_FIXED_TYPE_STREAMSIZE( aFloat );

	CHECK_FIXED_TYPE_STREAMSIZE( shortInts );
	CHECK_FIXED_TYPE_STREAMSIZE( shortStrings );
	CHECK_FIXED_TYPE_STREAMSIZE( idealInts );
	CHECK_FIXED_TYPE_STREAMSIZE( idealStrings );
	CHECK_FIXED_TYPE_STREAMSIZE( longInts );
	CHECK_FIXED_TYPE_STREAMSIZE( longStrings );
}

TEST_VARIABLE_TYPE_NAMED( ArrayDataType, 
	"<type> ARRAY <of> INT8 </of> </type>",
	int8s_noSize );

TEST_VARIABLE_TYPE_NAMED( ArrayDataType, 
	"<type> ARRAY <size> 5 </size> <of> STRING </of> </type>",
	strings_size5 );

TEST_VARIABLE_TYPE_NAMED( ArrayDataType, 
	"<type> ARRAY <of> STRING </of> </type>",
	strings_noSize );

// SequenceDataType: TupleDataType
TEST_FP( SequenceDataTypeFixture, SequenceDataTypeFixture(
		"<type> TUPLE <size> 5 </size> <of> INT8 </of> </type>", 5 ),
	TupleDataType_int8s_size5 )
{
	CHECK_TYPE_IS( TupleDataType );

	CHECK_FIXED_TYPE_STREAMSIZE( anInt );
	CHECK_FIXED_TYPE_STREAMSIZE( aString );
	CHECK_FIXED_TYPE_STREAMSIZE( aFloat );

	CHECK_FIXED_TYPE_STREAMSIZE( shortInts );
	CHECK_FIXED_TYPE_STREAMSIZE( shortStrings );
	CHECK_FIXED_TYPE_STREAMSIZE( idealInts );
	CHECK_FIXED_TYPE_STREAMSIZE( idealStrings );
	CHECK_FIXED_TYPE_STREAMSIZE( longInts );
	CHECK_FIXED_TYPE_STREAMSIZE( longStrings );
}

TEST_VARIABLE_TYPE_NAMED( TupleDataType, 
	"<type> TUPLE <of> INT8 </of> </type>",
	int8s_noSize );

TEST_VARIABLE_TYPE_NAMED( TupleDataType, 
	"<type> TUPLE <size> 5 </size> <of> STRING </of> </type>",
	strings_size5 );

TEST_VARIABLE_TYPE_NAMED( TupleDataType, 
	"<type> TUPLE <of> STRING </of> </type>",
	strings_noSize );

// UserDataType
// XXX: addToStream fails in lots of cases...
TEST_VARIABLE_TYPE( UserDataType,
		"<type> USER_TYPE <implementedBy> UserDataTypeTest.UserDataTypeTest "
		"</implementedBy> </type>" );

/**
 *	Test that DataDescription::streamSize() is coherent
 */
TEST_FP( DataDescriptionFixture, DataDescriptionFixture(
		"<health> <!-- Overrides 'health' defined in interfaces/Avatar.def -->"
		"	<Type>			INT32				</Type>"
		"	<Flags>			CELL_PRIVATE		</Flags>"
		"	<Default>		40					</Default>"
		"	<Editable>		true			</Editable>"
		"	<Persistent>	true			</Persistent>"
		"</health>" ),
	DataDescription_fantasydemo_Guard_health )
{
	CHECK_DESCRIPTION_TYPE_IS( IntegerDataType< int32 > );

	// Covers the entire range of supported clientServer IDs...
	// If there's a way to do this in a loop, but still report
	// a useful failure... I don't know it.

	CHECK( this->fixedStreamSizeOkay( 0, 0, -1 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 0 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 1 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 2 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 3 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 4 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 5 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 6 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 7 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 8 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 9 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 10 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 11 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 12 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 13 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 14 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 15 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 16 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 17 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 18 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 19 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 20 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 21 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 22 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 23 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 24 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 25 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 26 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 27 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 28 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 29 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 30 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 31 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 32 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 33 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 34 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 35 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 36 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 37 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 38 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 39 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 40 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 41 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 42 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 43 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 44 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 45 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 46 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 47 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 48 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 49 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 50 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 51 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 52 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 53 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 54 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 55 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 56 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 57 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 58 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 59 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 60 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 61 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 62 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 63 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 64 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 65 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 66 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 67 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 68 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 69 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 70 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 71 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 72 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 73 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 74 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 75 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 76 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 77 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 78 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 79 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 80 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 81 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 82 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 83 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 84 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 85 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 86 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 87 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 88 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 89 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 90 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 91 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 92 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 93 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 94 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 95 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 96 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 97 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 98 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 99 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 100 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 101 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 102 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 103 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 104 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 105 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 106 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 107 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 108 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 109 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 110 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 111 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 112 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 113 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 114 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 115 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 116 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 117 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 118 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 119 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 120 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 121 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 122 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 123 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 124 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 125 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 126 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 127 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 128 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 129 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 130 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 131 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 132 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 133 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 134 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 135 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 136 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 137 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 138 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 139 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 140 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 141 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 142 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 143 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 144 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 145 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 146 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 147 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 148 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 149 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 150 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 151 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 152 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 153 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 154 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 155 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 156 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 157 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 158 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 159 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 160 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 161 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 162 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 163 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 164 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 165 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 166 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 167 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 168 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 169 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 170 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 171 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 172 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 173 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 174 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 175 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 176 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 177 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 178 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 179 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 180 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 181 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 182 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 183 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 184 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 185 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 186 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 187 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 188 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 189 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 190 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 191 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 192 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 193 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 194 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 195 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 196 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 197 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 198 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 199 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 200 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 201 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 202 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 203 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 204 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 205 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 206 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 207 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 208 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 209 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 210 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 211 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 212 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 213 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 214 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 215 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 216 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 217 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 218 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 219 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 220 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 221 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 222 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 223 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 224 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 225 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 226 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 227 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 228 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 229 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 230 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 231 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 232 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 233 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 234 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 235 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 236 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 237 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 238 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 239 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 240 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 241 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 242 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 243 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 244 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 245 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 246 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 247 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 248 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 249 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 250 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 251 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 252 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 253 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 254 ) );
	CHECK( this->fixedStreamSizeOkay( 0, 0, 255 ) );
}

TEST_FP( DataDescriptionFixture, DataDescriptionFixture(
		"<visibleList>"
		"	<Type>			ARRAY <of>INT32</of>	</Type>"
		"	<Flags>			CELL_PRIVATE		</Flags>"
		"</visibleList>" ),
	DataDescription_fantasydemo_Guard_visibleList )
{
	CHECK_DESCRIPTION_TYPE_IS( ArrayDataType );

	ScriptList ids = ScriptList::create();
	ids.append( ScriptObject::createFrom( 0 ) );
	ids.append( ScriptObject::createFrom( 1 ) );
	ids.append( ScriptObject::createFrom( 32767 ) );

	// Covers the entire range of supported clientServer IDs...
	// If there's a way to do this in a loop, but still report
	// a useful failure... I don't know it.

	CHECK( this->variableStreamSizeOkay( ids, 0, -1 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 0 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 1 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 2 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 3 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 4 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 5 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 6 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 7 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 8 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 9 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 10 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 11 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 12 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 13 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 14 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 15 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 16 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 17 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 18 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 19 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 20 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 21 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 22 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 23 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 24 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 25 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 26 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 27 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 28 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 29 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 30 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 31 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 32 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 33 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 34 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 35 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 36 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 37 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 38 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 39 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 40 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 41 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 42 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 43 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 44 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 45 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 46 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 47 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 48 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 49 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 50 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 51 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 52 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 53 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 54 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 55 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 56 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 57 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 58 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 59 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 60 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 61 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 62 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 63 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 64 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 65 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 66 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 67 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 68 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 69 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 70 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 71 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 72 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 73 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 74 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 75 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 76 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 77 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 78 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 79 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 80 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 81 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 82 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 83 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 84 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 85 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 86 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 87 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 88 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 89 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 90 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 91 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 92 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 93 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 94 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 95 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 96 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 97 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 98 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 99 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 100 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 101 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 102 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 103 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 104 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 105 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 106 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 107 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 108 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 109 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 110 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 111 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 112 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 113 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 114 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 115 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 116 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 117 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 118 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 119 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 120 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 121 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 122 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 123 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 124 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 125 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 126 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 127 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 128 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 129 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 130 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 131 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 132 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 133 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 134 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 135 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 136 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 137 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 138 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 139 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 140 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 141 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 142 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 143 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 144 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 145 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 146 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 147 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 148 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 149 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 150 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 151 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 152 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 153 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 154 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 155 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 156 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 157 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 158 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 159 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 160 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 161 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 162 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 163 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 164 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 165 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 166 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 167 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 168 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 169 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 170 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 171 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 172 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 173 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 174 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 175 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 176 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 177 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 178 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 179 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 180 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 181 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 182 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 183 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 184 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 185 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 186 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 187 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 188 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 189 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 190 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 191 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 192 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 193 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 194 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 195 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 196 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 197 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 198 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 199 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 200 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 201 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 202 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 203 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 204 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 205 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 206 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 207 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 208 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 209 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 210 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 211 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 212 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 213 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 214 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 215 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 216 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 217 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 218 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 219 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 220 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 221 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 222 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 223 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 224 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 225 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 226 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 227 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 228 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 229 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 230 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 231 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 232 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 233 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 234 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 235 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 236 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 237 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 238 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 239 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 240 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 241 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 242 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 243 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 244 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 245 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 246 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 247 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 248 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 249 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 250 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 251 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 252 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 253 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 254 ) );
	CHECK( this->variableStreamSizeOkay( ids, 0, 255 ) );
}

/**
 *	Test that MethodArgs::streamSize() is coherent
 */
TEST_FP( MethodArgsFixture, MethodArgsFixture(
		"<Args>"
		"	<arg1>			INT8	</arg1>"
		"	<arg2>			INT8	</arg2>"
		"	<arg3>			INT8	</arg3>"
		"	<arg4>			INT8	</arg4>"
		"	<arg5>			INT8	</arg5>"
		"</Args>" ),
	MethodArgs_int8s )
{
	// MethodArgs::addToStream MF_ASSERTs if given
	// a too-short sequence
	CHECK_ARGS_FIXED_STREAMSIZE( idealInts );
	CHECK_ARGS_FIXED_STREAMSIZE( idealStrings );
	CHECK_ARGS_FIXED_STREAMSIZE( idealInterleavedIntsStrings );
	CHECK_ARGS_FIXED_STREAMSIZE( idealInterleavedStringsInts );
	CHECK_ARGS_FIXED_STREAMSIZE( longInts );
	CHECK_ARGS_FIXED_STREAMSIZE( longStrings );
	CHECK_ARGS_FIXED_STREAMSIZE( longInterleavedIntsStrings );
	CHECK_ARGS_FIXED_STREAMSIZE( longInterleavedStringsInts );
}

TEST_ARGS_VARIABLE_STREAMSIZE_NAMED( 
	"<Args>"
	"	<arg1>			STRING	</arg1>"
	"	<arg2>			STRING	</arg2>"
	"	<arg3>			STRING	</arg3>"
	"	<arg4>			STRING	</arg4>"
	"	<arg5>			STRING	</arg5>"
	"</Args>",
	strings );

TEST_ARGS_VARIABLE_STREAMSIZE_NAMED( 
	"<Args>"
	"	<arg1>			INT8	</arg1>"
	"	<arg2>			STRING	</arg2>"
	"	<arg3>			INT8	</arg3>"
	"	<arg4>			STRING	</arg4>"
	"	<arg5>			INT8	</arg5>"
	"</Args>",
	int8string );

TEST_ARGS_VARIABLE_STREAMSIZE_NAMED( 
	"<Args>"
	"	<arg1>			STRING	</arg1>"
	"	<arg2>			INT8	</arg2>"
	"	<arg3>			STRING	</arg3>"
	"	<arg4>			INT8	</arg4>"
	"	<arg5>			STRING	</arg5>"
	"</Args>",
	stringint8 );


/**
 *	Test that MethodDescription::streamSize() is coherent
 */
// We only test exposed methods for now, since streamSize()
// is only really interesting between client and server.
TEST_FP( MethodDescriptionFixture, MethodDescriptionFixture(
		"<scanForTargets>"
		"	<Args>"
		"		<amplitude>			FLOAT				</amplitude>"
		"		<period>			FLOAT				</period>"
		"		<offset>			FLOAT				</offset>"
		"	</Args>"
		"</scanForTargets>",
		MethodDescription::CLIENT ),
	MethodDescription_fantasydemo_Guard_scanForTargets )
{
	ScriptTuple args = ScriptTuple::create( 3 );
	args.setItem( 0, ScriptObject::createFrom( 0.f ) );
	args.setItem( 1, ScriptObject::createFrom( 0.f ) );
	args.setItem( 2, ScriptObject::createFrom( 0.f ) );

	// Non-subindexed
	CHECK_METHOD_FIXED_STREAMSIZE(		BASE, args, 0, 0 );
	CHECK_METHOD_FIXED_STREAMSIZE(		CELL, args, 0, 0 );
	// Subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, args, 0, 500 );
}

TEST_FP( MethodDescriptionFixture, MethodDescriptionFixture(
		"<newFriendsList>"
		"	<Args>"
		"		<friendsList>	ARRAY <of> UNICODE_STRING </of>	</friendsList>"
		"	</Args>"
		"</newFriendsList>",
		MethodDescription::CLIENT ),
	MethodDescription_fantasydemo_Avatar_newFriendsList )
{
	ScriptTuple args = ScriptTuple::create( 1 );
	args.setItem( 0, ScriptList::create() );

	// Non-subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, args, 0, 0 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, args, 0, 0 );
	// Subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, args, 0, 500 );
}

TEST_FP( MethodDescriptionFixture, MethodDescriptionFixture(
		"<setBandwidthPerSecond>"
		"	<Exposed/>"
		"	<Args>"
		"		<bps>		UINT16		</bps>	<!-- bytes per second -->"
		"	</Args>"
		"</setBandwidthPerSecond>",
		MethodDescription::BASE ),
	MethodDescription_fantasydemo_Avatar_setBandwidthPerSecond )
{
	ScriptTuple args = ScriptTuple::create( 1 );
	args.setItem( 0, ScriptObject::createFrom( 0 ) );

	// Non-subindexed
	CHECK_METHOD_FIXED_STREAMSIZE( 		CLIENT, args, 0, 0 );
	CHECK_METHOD_FIXED_STREAMSIZE(		BASE, args, 0, 0 );
	CHECK_METHOD_FIXED_STREAMSIZE(		CELL, args, 0, 0 );

	// Subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CLIENT, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, args, 0, 500 );
}

TEST_FP( MethodDescriptionFixture, MethodDescriptionFixture(
		"<tryToTeleport>"
		"	<Exposed/>"
		"	<Args>"
		"		<spaceName>	STRING			</spaceName>"
		"		<pointName>	STRING			</pointName>"
		"		<spaceID>	INT32			</spaceID>"
		"	</Args>"
		"</tryToTeleport>",
		MethodDescription::BASE ),
	MethodDescription_fantasydemo_Avatar_tryToTeleport )
{
	ScriptTuple args = ScriptTuple::create( 3 );
	args.setItem( 0, ScriptObject::createFrom( "" ) );
	args.setItem( 1, ScriptObject::createFrom( "" ) );
	args.setItem( 2, ScriptObject::createFrom( 0 ) );

	// Non-subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE( 	CLIENT, args, 0, 0 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, args, 0, 0 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, args, 0, 0 );

	// Subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CLIENT, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, args, 0, 500 );
}

TEST_FP( MethodDescriptionFixture, MethodDescriptionFixture(
		"<commerceCancelRequest>"
		"	<Exposed/>"
		"</commerceCancelRequest>",
		MethodDescription::CELL ),
	MethodDescription_fantasydemo_Avatar_commerceCancelRequest )
{
	ScriptTuple args = ScriptTuple::create( 0 );

	ScriptTuple argsWithSource = ScriptTuple::create( 1 );
	argsWithSource.setItem( 0, ScriptObject::createFrom( -1 ) );

	// Non-subindexed
	CHECK_METHOD_FIXED_STREAMSIZE( 		CLIENT, args, 0, 0 );
	CHECK_METHOD_FIXED_STREAMSIZE(		BASE, argsWithSource, 0, 0 );
	CHECK_METHOD_FIXED_STREAMSIZE(		CELL, argsWithSource, 0, 0 );

	// Subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CLIENT, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, argsWithSource, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, argsWithSource, 0, 500 );
}

TEST_FP( MethodDescriptionFixture, MethodDescriptionFixture(
		"<summonEntity>"
		"	<Exposed/>"
		"	<Args>"
		"		<typeName>		STRING		</typeName>"
		"	</Args>"
		"</summonEntity>",
		MethodDescription::CELL ),
	MethodDescription_fantasydemo_Avatar_summonEntity )
{
	ScriptTuple args = ScriptTuple::create( 1 );
	args.setItem( 0, ScriptObject::createFrom( "" ) );

	ScriptTuple argsWithSource = ScriptTuple::create( 2 );
	argsWithSource.setItem( 0, ScriptObject::createFrom( -1 ) );
	argsWithSource.setItem( 1, ScriptObject::createFrom( "" ) );

	// Non-subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CLIENT, args, 0, 0 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, argsWithSource, 0, 0 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, argsWithSource, 0, 0 );

	// Subindexed
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CLIENT, args, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	BASE, argsWithSource, 0, 500 );
	CHECK_METHOD_VARIABLE_STREAMSIZE(	CELL, argsWithSource, 0, 500 );
}


/**
 *	Test that PropertyChange::streamSize() is coherent
 */
// Top-level fixed-size simple property
TEST_FP( PropertyChangeFixture, PropertyChangeFixture(
		"<int32>"
		"	<Type>			INT32				</Type>"
		"	<Flags>			ALL_CLIENTS			</Flags>"
		"	<Default>		40					</Default>"
		"</int32>"),
	PropertyChange_int32 )
{
	int currIndex;

	currIndex = this->addProperty( 0 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( 39 ) );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 60 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( 39 ) );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 61 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( 39 ) );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 62 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( 39 ) );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 63 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( 39 ) );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 254 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( 39 ) );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 255 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( 39 ) );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );
}

// Top-level variable-size simple property
TEST_FP( PropertyChangeFixture, PropertyChangeFixture(
		"<string>"
		"	<Type>			STRING				</Type>"
		"	<Flags>			ALL_CLIENTS			</Flags>"
		"	<Default>		DefaultString		</Default>"
		"</string>" ),
	PropertyChange_string )
{
	int currIndex;

	currIndex = this->addProperty( 0 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( "String" ) );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 60 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( "String" ) );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 61 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( "String" ) );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 62 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( "String" ) );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 63 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( "String" ) );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 254 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( "String" ) );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 255 );
	this->owner.setValue( currIndex, ScriptObject::createFrom( "String" ) );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );
}

// Top-level variable sequence of fixed-size properties
// Also tests top-level slice of variable sequence of fixed-size properties
// Also tests pathed update of single element of
// variable sequence of fixed-size properties
TEST_FP( PropertyChangeFixture, PropertyChangeFixture(
		"<int32s>"
		"	<Type>		ARRAY <of> INT32 </of>	</Type>"
		"	<Flags>			ALL_CLIENTS			</Flags>"
		"</int32s>"),
	PropertyChange_unsized_array_int32 )
{
	int currIndex;

	ScriptList list = ScriptList::create();

	for (int i = 0; i < 5; ++i)
	{
		list.append( ScriptObject::createFrom( 39 ) );
	}

	ScriptList newSlice = ScriptList::create();
	for (int i = 0; i < 3; ++i)
	{
		newSlice.append( ScriptObject::createFrom( 38 ) );
	}

	currIndex = this->addProperty( 0 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 60 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 61 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 62 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 63 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 254 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 255 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

}

// Top-level variable sequence of variable-size properties
// Also tests top-level slice of variable sequence of variable-size properties
TEST_FP( PropertyChangeFixture, PropertyChangeFixture(
		"<strings>"
		"	<Type>		ARRAY <of> STRING </of>	</Type>"
		"	<Flags>			ALL_CLIENTS			</Flags>"
		"</strings>"),
	PropertyChange_unsized_array_string )
{
	int currIndex;

	ScriptList list = ScriptList::create();

	for (int i = 0; i < 5; ++i)
	{
		list.append( ScriptObject::createFrom( "String" ) );
	}

	ScriptList newSlice = ScriptList::create();
	for (int i = 0; i < 3; ++i)
	{
		newSlice.append( ScriptObject::createFrom( "NewString" ) );
	}

	currIndex = this->addProperty( 0 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 60 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 61 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 62 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 63 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 254 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 255 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );
}

// Top-level fixed sequence of fixed-size properties
// Also tests top-level slice of fixed sequence of fixed-size properties
TEST_FP( PropertyChangeFixture, PropertyChangeFixture(
		"<int32x5>"
		"	<Type>	ARRAY <size> 5 </size> <of> INT32 </of>	</Type>"
		"	<Flags>			ALL_CLIENTS			</Flags>"
		"</int32x5>"),
	PropertyChange_sized_array_int32 )
{
	int currIndex;

	ScriptList list = ScriptList::create();

	for (int i = 0; i < 5; ++i)
	{
		list.append( ScriptObject::createFrom( 39 ) );
	}

	ScriptList newSlice = ScriptList::create();
	for (int i = 0; i < 3; ++i)
	{
		newSlice.append( ScriptObject::createFrom( 38 ) );
	}

	currIndex = this->addProperty( 0 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 60 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 61 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 62 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 63 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 254 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );

	currIndex = this->addProperty( 255 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( this->lastValueFixedSize() );
	CHECK( this->lastValueFixedSizeOkay() );
}

// Top-level fixed sequence of variable-size properties
// Also tests top-level slice of fixed sequence of variable-size properties
TEST_FP( PropertyChangeFixture, PropertyChangeFixture(
		"<stringx5>"
		"	<Type>	ARRAY <size> 5 </size> <of> STRING </of>	</Type>"
		"	<Flags>			ALL_CLIENTS			</Flags>"
		"</stringx5>"),
	PropertyChange_sized_array_string )
{
	int currIndex;

	ScriptList list = ScriptList::create();

	for (int i = 0; i < 5; ++i)
	{
		list.append( ScriptObject::createFrom( "String" ) );
	}

	ScriptList newSlice = ScriptList::create();
	for (int i = 0; i < 3; ++i)
	{
		newSlice.append( ScriptObject::createFrom( "NewString" ) );
	}

	currIndex = this->addProperty( 0 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 60 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 61 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 62 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 63 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 254 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );

	currIndex = this->addProperty( 255 );
	this->owner.setValue( currIndex, list );
	CHECK( this->owner.streamed() );
	CHECK( !this->lastValueFixedSize() );
}

BW_END_NAMESPACE

// test_streamsizes.cpp
