#include "pch.hpp"

#include "test_harness.hpp"

#include "entitydef/data_type.hpp"
#include "entitydef/meta_data_type.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/string_builder.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/xml_section.hpp"

#define TEST_SUB_FUNCTOR_LINENO(x) \
	TEST_SUB_FUNCTOR_NAMED(x, "Called from line: " << __LINE__)

BW_BEGIN_NAMESPACE

// Anonymous namespace
namespace
{

/**
 *	Utility method to create a DataTypePtr given some XML
 */
static DataTypePtr getDT( const BW::string & xmlStr )
{
	return DataType::buildDataType( xmlStr );
}


class IteratorCheck
{
public:
	IteratorCheck( DataType::const_iterator & iter,
		DataTypePtr pType, const BW::string & metaDataTypeName,
		bool isSubstreamEnd = false ) :
	iter_( iter ),
	pType_( pType ),
	metaDataTypeName_( metaDataTypeName ),
	fieldName_(),
	size_( std::numeric_limits<size_t>::max() ),
	dictName_(),
	isNoneAble_( false ),
	isSubstreamStart_( false ),
	isSubstreamEnd_( isSubstreamEnd )
	{
		MF_ASSERT( pType_ );
	}


	IteratorCheck( DataType::const_iterator & iter,
		DataTypePtr pType, const BW::string & metaDataTypeName,
		int containerSize, bool isSubstreamStart = false ) :
	iter_( iter ),
	pType_( pType ),
	metaDataTypeName_( metaDataTypeName ),
	fieldName_(),
	size_( containerSize ),
	dictName_(),
	isNoneAble_( false ),
	isSubstreamStart_( isSubstreamStart ),
	isSubstreamEnd_( false )
	{
		MF_ASSERT( pType_ );
	}


	IteratorCheck( DataType::const_iterator & iter,
		DataTypePtr pType, const BW::string & metaDataTypeName,
		const char * dictName, int containerSize,
		bool isNoneAble = false ) :
	iter_( iter ),
	pType_( pType ),
	metaDataTypeName_( metaDataTypeName ),
	fieldName_(),
	size_( containerSize ),
	dictName_( dictName ),
	isNoneAble_( isNoneAble ),
	isSubstreamStart_( false ),
	isSubstreamEnd_( false )
	{
		MF_ASSERT( pType_ );
	}


	IteratorCheck( DataType::const_iterator & iter,
		DataTypePtr pType, const BW::string & metaDataTypeName,
		const char * fieldName ) :
	iter_( iter ),
	pType_( pType ),
	metaDataTypeName_( metaDataTypeName ),
	fieldName_( fieldName ),
	size_( std::numeric_limits<size_t>::max() ),
	dictName_(),
	isNoneAble_( false ),
	isSubstreamStart_( false ),
	isSubstreamEnd_( false )
	{
		MF_ASSERT( pType_ );
	}


	TEST_SUB_FUNCTOR_OPERATOR
	{
		RETURN_ON_FAIL_CHECK( iter_ != pType_->endPersistent() );
		CHECK_EQUAL( metaDataTypeName_,
			iter_->getType().pMetaDataType()->name() );
		CHECK_EQUAL( isNoneAble_, iter_->isNoneAbleType() );
		CHECK_EQUAL( isSubstreamStart_, iter_->isSubstreamStart() );
		CHECK_EQUAL( isSubstreamEnd_, iter_->isSubstreamEnd() );

		BW::string iterFieldName;
		if (fieldName_.empty())
		{
			CHECK( !iter_->getFieldName( iterFieldName ) );
		}
		else
		{
			CHECK( iter_->getFieldName( iterFieldName ) );
			CHECK_EQUAL( fieldName_, iterFieldName );
		}

		size_t iterContainerSize;
		if (size_ == std::numeric_limits<size_t>::max() ||
			iter_->isCustomStreamingType())
		{
			CHECK( !iter_->getSize( iterContainerSize ) );
			CHECK( !iter_->isVariableSizedType() );
		}
		else
		{
			if (iter_->isVariableSizedType())
			{
				iter_.setSize( size_ );
			}
			else
			{
				CHECK( iter_->getSize( iterContainerSize ) );
				CHECK_EQUAL( size_, iterContainerSize );
			}
		}

		BW::string iterDictName;
		if (dictName_.empty())
		{
			CHECK( !iter_->getStructureName( iterDictName ) );
		}
		else
		{
			CHECK( iter_->getStructureName( iterDictName ) );
			CHECK_EQUAL( dictName_, iterDictName );
		}
	}


private:
	DataType::const_iterator & iter_;
	const DataTypePtr pType_;
	const BW::string metaDataTypeName_;
	const BW::string fieldName_;
	const size_t size_;
	const BW::string dictName_;
	const bool isNoneAble_;
	const bool isSubstreamStart_;
	const bool isSubstreamEnd_;
};

class SingleElementCheck
{
public:
	SingleElementCheck( const BW::string & typeStr,
		int expectedStreamSize ) :
	typeStr_( typeStr ),
	streamSize_( expectedStreamSize )
	{
		StringBuilder builder( 64 );
		builder.appendf( "<Type> %s </Type>", typeStr.c_str() );
		pType_ = getDT( builder.string() );
	}

	SingleElementCheck( const BW::string & typeStr,
		int expectedStreamSize, DataTypePtr pType ) :
	typeStr_( typeStr ),
	streamSize_( expectedStreamSize ),
	pType_( pType )
	{}

	TEST_SUB_FUNCTOR_OPERATOR
	{
		RETURN_ON_FAIL_CHECK( pType_ );
		CHECK_EQUAL( typeStr_, pType_->pMetaDataType()->name() );
		CHECK_EQUAL( streamSize_, pType_->streamSize() );

		DataType::const_iterator it = pType_->beginPersistent();
		TEST_SUB_FUNCTOR( IteratorCheck( it, pType_, typeStr_ ) );
		RETURN_ON_FAIL_CHECK( it != pType_->endPersistent() );
		CHECK_EQUAL( pType_, &it->getType() );
		CHECK_EQUAL( streamSize_, it->getType().streamSize() );
		// We test UserDataType, which passes this.
//		CHECK_EQUAL( false, it->isCustomStreamingType() );

		++it;
		CHECK( pType_->endPersistent() == it );
	}
private:
	const BW::string & typeStr_;
	int streamSize_;
	DataTypePtr pType_;
};

} // Anonymous namespace

TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_basic_fixed_size_types )
{
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "FLOAT32", 4 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "FLOAT64", 8 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "INT8", 1 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "INT16", 2 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "INT32", 4 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "INT64", 8 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "UINT8", 1 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "UINT16", 2 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "UINT32", 4 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "UINT64", 8 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "VECTOR2", 8 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "VECTOR3", 12 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "VECTOR4", 16 ));
#if defined MF_SERVER
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "MAILBOX", 12 ));
#endif // defined MF_SERVER
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "UDO_REF", 16 ));
}


TEST_F( EntityDefUnitTestHarness,
	DataType_persistent_only_const_iterator_basic_variable_size_types )
{
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "BLOB", -1 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "PYTHON", -1 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "STRING", -1 ));
	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "UNICODE_STRING", -1 ));
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_USER_TYPE_1 )
{
	const BW::string userTypeStr(
		"<Type> USER_TYPE "
			"<implementedBy> UserTypes.intStr </implementedBy> "
		"</Type>"
	);
	DataTypePtr pType = getDT( userTypeStr );

	TEST_SUB_FUNCTOR_LINENO(SingleElementCheck( "USER_TYPE", -1, pType ));
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_implementedBy_1 )
{
	const BW::string userTypeStr(
		"<Type> FIXED_DICT "
			"<implementedBy> FixedDictWrappers.stream </implementedBy> "
	        "<Properties> "
		        " <int8_5> <Type> INT8 </Type> </int8_5> "
			    " <str_TestString> <Type> STRING </Type> </str_TestString> "
			"</Properties> "
			"<AllowNone> false </AllowNone> "
			"<TypeName> TestFD_PO_IB_1 </TypeName> "
		"</Type>"
	);
	DataTypePtr pType = getDT( userTypeStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_IB_1", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );
	CHECK_EQUAL( true, it->isCustomStreamingType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_1 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
			"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
			"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
		"</Properties> <AllowNone> false </AllowNone> "
		"<TypeName> TestFD_PO1 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 6, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO1", 3 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );
	CHECK( !it->isVariableSizedType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_2 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> false </AllowNone> "
				"<TypeName> TestFD_PO2_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> false </AllowNone> "
				"<TypeName> TestFD_PO2_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> false </AllowNone> "
		"<TypeName> TestFD_PO2 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 12, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO2", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO2_A", 3 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO2_B", 3 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_3 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
			"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
			"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO3 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO3", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );
	CHECK( !it->isVariableSizedType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_4 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO4_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO4_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO4 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO4", 2,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO4_A", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO4_B", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_5 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
			"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
			"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO5 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO5", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );
	CHECK( !it->isVariableSizedType() );

	it.setNone( true );
	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_6 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO6_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO6_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO6 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO6", 2,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO6_A", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	it.setNone( true );
	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO6_B", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	it.setNone( true );
	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_7 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO7_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO7_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO7 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO7", 2,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setNone( true );
	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_nonPersistentint_1 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<int8Field1>		<Type>	INT8	</Type>	"
				"<Persistent> false </Persistent> </int8Field1> "
			"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
			"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
		"</Properties> <AllowNone> false </AllowNone> "
		"<TypeName> TestFD_PO_NP_1 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 6, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_1", 3 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );
	CHECK( !it->isVariableSizedType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_nonPersistentint_2 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> false </AllowNone> "
				"<TypeName> TestFD_PO_NP_2_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> false </AllowNone> "
				"<TypeName> TestFD_PO_NP_2_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> false </AllowNone> "
		"<TypeName> TestFD_PO_NP_2 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 12, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_2", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_2_A", 3 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_2_B", 3 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_nonPersistentint_3 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<int8Field1>		<Type>	INT8	</Type>	"
				"<Persistent> false </Persistent> </int8Field1> "
			"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
			"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO_NP_3 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_3", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );
	CHECK( !it->isVariableSizedType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_nonPersistentint_4 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO_NP_4_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO_NP_4_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO_NP_4 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_4", 2,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_4_A", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_4_B", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< int8 >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SimpleStreamElement< float >
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_nonPersistentint_5 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<int8Field1>		<Type>	INT8	</Type>	"
				"<Persistent> false </Persistent> </int8Field1> "
			"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
			"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO_NP_5 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_5", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );
	CHECK( !it->isVariableSizedType() );

	it.setNone( true );
	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_nonPersistentint_6 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO_NP_6_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO_NP_6_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO_NP_6 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_6", 2,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_6_A", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	it.setNone( true );
	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "fixedDictField2" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_6_B", 3,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	it.setNone( true );
	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_nonPersistentint_7 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<fixedDictField1> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO_NP_7_A </TypeName> </Type> "
			"</fixedDictField1> "
			"<fixedDictField2> <Type> FIXED_DICT <Properties> "
				"<int8Field1>		<Type>	INT8	</Type>	"
					"<Persistent> false </Persistent> </int8Field1> "
				"<int8Field2>		<Type>	INT8	</Type>	</int8Field2> "
				"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
				"</Properties> <AllowNone> true </AllowNone> "
				"<TypeName> TestFD_PO_NP_7_B </TypeName> </Type> "
			"</fixedDictField2> "
		"</Properties> <AllowNone> true </AllowNone> "
		"<TypeName> TestFD_PO_NP_7 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_NP_7", 2,
			/* isNoneAble */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setNone( true );
	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_ARRAY_1 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> INT8 </of> <size> 4 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 4, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_ARRAY_2 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> "
			"ARRAY <of> INT8 </of> <size> 4 </size> "
		"</of> <size> 2 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 8, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 4 ) );
	CHECK( !it->isVariableSizedType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}

TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_ARRAY_1 )
{
	const BW::string arrayStr( "<Type> ARRAY <of> INT8 </of> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 0 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 0 );
	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_ARRAY_2 )
{
	const BW::string arrayStr( "<Type> ARRAY <of> INT8 </of> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_ARRAY_3 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> "
			"ARRAY <of> INT8 </of> "
		"</of> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 2 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_ARRAY_persistAsBlob_1 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> <size> 4 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 4, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_ARRAY_persistAsBlob_2 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> ARRAY <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> <size> 4 </size> </of> "
		"<persistAsBlob> true </persistAsBlob> <size> 2 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 8, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", 2, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", 4, /* isSubstreamStart */ true ) );
	CHECK( !it->isVariableSizedType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}

TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_ARRAY_persistAsBlob_1 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", 0, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 0 );
	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_ARRAY_persistAsBlob_2 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_ARRAY_persistAsBlob_3 )
{
	const BW::string arrayStr(
		"<Type> ARRAY <of> ARRAY <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> </of> "
		"<persistAsBlob> true </persistAsBlob> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 2, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 2 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "ARRAY", /* isSubstreamEnd */ true ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_TUPLE_1 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> INT8 </of> <size> 4 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 4, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_TUPLE_2 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> "
			"TUPLE <of> INT8 </of> <size> 4 </size> "
		"</of> <size> 2 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 8, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4 ) );
	CHECK( !it->isVariableSizedType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}

TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_TUPLE_1 )
{
	const BW::string arrayStr( "<Type> TUPLE <of> INT8 </of> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 0 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 0 );
	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_TUPLE_2 )
{
	const BW::string arrayStr( "<Type> TUPLE <of> INT8 </of> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_TUPLE_3 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> "
			"TUPLE <of> INT8 </of> "
		"</of> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 2 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_TUPLE_persistAsBlob_1 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> <size> 4 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 4, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_fixed_TUPLE_persistAsBlob_2 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> TUPLE <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> <size> 4 </size> </of> "
		"<persistAsBlob> true </persistAsBlob> <size> 2 </size> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( 8, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", 2, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", 4, /* isSubstreamStart */ true ) );
	CHECK( !it->isVariableSizedType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}

TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_TUPLE_persistAsBlob_1 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", 0, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 0 );
	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_TUPLE_persistAsBlob_2 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_variable_TUPLE_persistAsBlob_3 )
{
	const BW::string arrayStr(
		"<Type> TUPLE <of> TUPLE <of> INT8 </of> "
			"<persistAsBlob> true </persistAsBlob> </of> "
		"<persistAsBlob> true </persistAsBlob> </Type>" );
	DataTypePtr pType = getDT( arrayStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 2, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	it.setSize( 2 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", 4, /* isSubstreamStart */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 4 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "TUPLE", /* isSubstreamEnd */ true ) );
	CHECK_EQUAL( pType, &it->getType() );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	CHECK( it == pType->endPersistent() );
}


TEST_F( EntityDefUnitTestHarness, DataType_persistent_only_const_iterator_FIXED_DICT_and_ARRAY_1 )
{
	const BW::string fixedDictStr(
		"<Type> FIXED_DICT <Properties> "
			"<tupleField1>		<Type>	TUPLE "
				"<of> INT8 </of> <size> 4 </size> </Type>	</tupleField1> "
			"<arrayField1>		<Type>	ARRAY "
				"<of> INT8 </of> </Type>	</arrayField1> "
			"<int8Field1>		<Type>	INT8	</Type>	</int8Field1> "
			"<float32Field1>	<Type>	FLOAT32	</Type>	</float32Field1> "
		"</Properties> <AllowNone> false </AllowNone> "
		"<TypeName> TestFD_PO_and_ARRAY_1 </TypeName> </Type>" );
	DataTypePtr pType = getDT( fixedDictStr );
	RETURN_ON_FAIL_CHECK( pType );
	CHECK_EQUAL( -1, pType->streamSize() );

	DataType::const_iterator it = pType->beginPersistent();
	// FixedDictStartStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "TestFD_PO_and_ARRAY_1", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "tupleField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE", 4 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( !it->isVariableSizedType() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "TUPLE" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "arrayField1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// SequenceStartStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY", 2 ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK( it->isVariableSizedType() );

	it.setSize( 2 );
	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEnterItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceLeaveItemStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// SequenceEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "ARRAY" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "int8Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "INT8" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEnterFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO(
		IteratorCheck( it, pType, "FIXED_DICT", "float32Field1" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FLOAT32" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );

	++it;
	// FixedDictLeaveFieldStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	// FixedDictEndStreamElement
	TEST_SUB_FUNCTOR_LINENO( IteratorCheck( it, pType, "FIXED_DICT" ) );
	RETURN_ON_FAIL_CHECK( it != pType->endPersistent() );
	CHECK_EQUAL( pType, &it->getType() );

	++it;
	CHECK( it == pType->endPersistent() );
}

BW_END_NAMESPACE

// test_datatype_persistent_only_const_iterator.cpp
