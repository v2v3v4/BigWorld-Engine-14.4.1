#include "pch.hpp"

#include "cstdmf/cstdmf_init.hpp"
#include "resmgr/packed_section.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

class Fixture : public ResMgrUnitTestHarness
{
public:
	Fixture():
		ResMgrUnitTestHarness(),
		fixture_( NULL )
	{
		new CStdMf;
		// Convert and open section
		const char * sourceXMLFile = "test_xml_section.xml";
		const char * fixtureFile = "test_packed_section";
		if (BWResource::instance().fileSystem()->getFileType( fixtureFile ) !=
			IFileSystem::FT_NOT_FOUND)
		{
			if (!BWResource::instance().fileSystem()->eraseFileOrDirectory(
				fixtureFile ))
			{
				ERROR_MSG("Failed to erase %s\n",
					BWResolver::resolveFilename( fixtureFile ).c_str());
				return;
			}
		}
		if(PackedSection::convert( BWResolver::resolveFilename( sourceXMLFile ),
									BWResolver::resolveFilename( fixtureFile ) ))
		{
			fixture_ = BWResource::openSection( fixtureFile );
		}
	}

	~Fixture()
	{
		fixture_ = NULL;
		BWResource::instance().purgeAll();	// Clear any cached values
		delete CStdMf::pInstance();
	}

	DataSectionPtr fixture_;
};

// Conversion tests

TEST_F( Fixture, PackedSection_AsBool )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr boolSection = fixture_->openSection( "test_bool" );
	RETURN_ON_FAIL_CHECK( boolSection != NULL );

	CHECK_EQUAL( true, boolSection->asBool() );
}

TEST_F( Fixture, PackedSection_AsInt )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr int8Section = fixture_->openSection( "test_int8" );
	RETURN_ON_FAIL_CHECK( int8Section != NULL );
	CHECK_EQUAL( std::numeric_limits<int8>::min(), (int8)int8Section->asInt() );

	DataSectionPtr int16Section = fixture_->openSection( "test_int16" );
	RETURN_ON_FAIL_CHECK( int16Section != NULL );
	CHECK_EQUAL( std::numeric_limits<int16>::min(), (int16)int16Section->asInt() );

	DataSectionPtr int32Section = fixture_->openSection( "test_int32" );
	RETURN_ON_FAIL_CHECK( int32Section != NULL );
	CHECK_EQUAL( std::numeric_limits<int32>::min(), int32Section->asInt() );
}

TEST_F( Fixture, PackedSection_AsUInt )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr uint8Section = fixture_->openSection( "test_uint8" );
	RETURN_ON_FAIL_CHECK( uint8Section != NULL );
	CHECK_EQUAL( std::numeric_limits<uint8>::max(), (uint8)uint8Section->asInt() );

	DataSectionPtr uint16Section = fixture_->openSection( "test_uint16" );
	RETURN_ON_FAIL_CHECK( uint16Section != NULL );
	CHECK_EQUAL( std::numeric_limits<uint16>::max(), (uint16)uint16Section->asInt() );

	DataSectionPtr uint32Section = fixture_->openSection( "test_uint32" );
	RETURN_ON_FAIL_CHECK( uint32Section != NULL );
	CHECK_EQUAL( std::numeric_limits<uint32>::max(), uint32Section->asUInt() );
}

TEST_F( Fixture, PackedSection_AsLong )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr longSection = fixture_->openSection( "test_long" );
	RETURN_ON_FAIL_CHECK( longSection != NULL );

	CHECK_EQUAL( std::numeric_limits<int32>::min(), longSection->asLong() );
}

TEST_F( Fixture, PackedSection_AsInt64 )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr int64Section = fixture_->openSection( "test_int64" );
	RETURN_ON_FAIL_CHECK( int64Section != NULL );

	CHECK_EQUAL( std::numeric_limits<int64>::min(), int64Section->asInt64() );

	int64Section = fixture_->openSection( "test_int64_2" );
	RETURN_ON_FAIL_CHECK( int64Section != NULL );

	CHECK_EQUAL( std::numeric_limits<int64>::max(), int64Section->asInt64() );

	char conversionBuf[ 64 ];
	bw_snprintf( conversionBuf, 63, "%" PRI64, int64Section->asInt64() );
	conversionBuf[ 63 ] = '\0';

	CHECK_EQUAL( BW::string( conversionBuf ), int64Section->asString() );
}

TEST_F( Fixture, PackedSection_AsUInt64 )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr uint64Section = fixture_->openSection( "test_uint64" );
	RETURN_ON_FAIL_CHECK( uint64Section != NULL );

	CHECK_EQUAL( std::numeric_limits<uint64>::max(), uint64Section->asUInt64() );

	uint64Section = fixture_->openSection( "test_uint64_2" );
	RETURN_ON_FAIL_CHECK( uint64Section != NULL );

	CHECK_EQUAL( uint64( std::numeric_limits<int64>::max() ) + 1, uint64Section->asUInt64() );
}

TEST_F( Fixture, PackedSection_AsFloat )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr floatSection = fixture_->openSection( "test_float" );
	RETURN_ON_FAIL_CHECK( floatSection != NULL );

	CHECK( isEqual( 3.142f, floatSection->asFloat() ) );
}

TEST_F( Fixture, PackedSection_AsString )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	// test that a long number-like string is read as a string.

	DataSectionPtr strSection = fixture_->openSection( "test_string" );
	RETURN_ON_FAIL_CHECK( strSection != NULL );

	CHECK_EQUAL(
			BW::string( "000600060006000600060006000300030003000300030003" ),
			strSection->asString() );
}

TEST_F( Fixture, PackedSection_AsVector2 )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr vec2Section = fixture_->openSection( "test_vector2" );
	RETURN_ON_FAIL_CHECK( vec2Section != NULL );

	Vector2 result( vec2Section->asVector2() );
	CHECK( isEqual( 1.0f, result.x ) );
	CHECK( isEqual( 2.0f, result.y ) );
}

TEST_F( Fixture, PackedSection_AsVector3 )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr vec3Section = fixture_->openSection( "test_vector3" );
	RETURN_ON_FAIL_CHECK( vec3Section != NULL );

	Vector3 result( vec3Section->asVector3() );
	CHECK( isEqual( 1.0f, result.x ) );
	CHECK( isEqual( 2.0f, result.y ) );
	CHECK( isEqual( 3.0f, result.z ) );
}

TEST_F( Fixture, PackedSection_AsVector4 )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr vec4Section = fixture_->openSection( "test_vector4" );
	RETURN_ON_FAIL_CHECK( vec4Section != NULL );

	Vector4 result( vec4Section->asVector4() );
	CHECK( isEqual( 1.0f, result.x ) );
	CHECK( isEqual( 2.0f, result.y ) );
	CHECK( isEqual( 3.0f, result.z ) );
	CHECK( isEqual( 4.0f, result.w ) );
}

TEST_F( Fixture, PackedSection_AsMatrix34 )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

	DataSectionPtr mat34Section = fixture_->openSection( "test_matrix34" );
	RETURN_ON_FAIL_CHECK( mat34Section != NULL );

	Matrix result( mat34Section->asMatrix34() );

	CHECK( isEqual( 1.0f, result[0].x ) );
	CHECK( isEqual( 2.0f, result[0].y ) );
	CHECK( isEqual( 3.0f, result[0].z ) );

	CHECK( isEqual( 4.0f, result[1].x ) );
	CHECK( isEqual( 5.0f, result[1].y ) );
	CHECK( isEqual( 6.0f, result[1].z ) );

	CHECK( isEqual( 7.0f, result[2].x ) );
	CHECK( isEqual( 8.0f, result[2].y ) );
	CHECK( isEqual( 9.0f, result[2].z ) );

	CHECK( isEqual( 10.0f, result[3].x ) );
	CHECK( isEqual( 11.0f, result[3].y ) );
	CHECK( isEqual( 12.0f, result[3].z ) );
}

// Conversion tests

TEST_F( Fixture, PackedSection_countChildren_prepacked )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );
	RETURN_ON_FAIL_CHECK( fixture_ != NULL );

 	// Count children for each subsection, using pre-packed section, and compare
	// with xml output

	DataSectionPtr x = BWResource::openSection( "test_xml_section.xml" );
	RETURN_ON_FAIL_CHECK( x != NULL );
	CHECK( fixture_->countChildren() > 0 );

	CHECK_EQUAL( x->countChildren(), fixture_->countChildren() );
}

TEST_F( Fixture, PackedSection_countChildren_convertInMemory )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );

	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();
	fileSystem->eraseFileOrDirectory( "result_packed_section" );

	// Count children for each subsection, after converting from an xml section
	// to a binary section on disk.

	DataSectionPtr	x = BWResource::openSection( "test_xml_section.xml" );
	RETURN_ON_FAIL_CHECK( x != NULL );

	int		n = x->countChildren();
	bool	r = PackedSection::convert( x , "result_packed_section" );

	CHECK_EQUAL( true, r );

	if ( r )
	{
		CHECK_EQUAL( n, x->countChildren() );
	}
}

TEST_F( Fixture, PackedSection_countChildren_convertOnDisk )
{
	RETURN_ON_FAIL_CHECK( this->isOK() );

	MultiFileSystemPtr	fileSystem = BWResource::instance().fileSystem();
	fileSystem->eraseFileOrDirectory( "result_packed_section2" );

	// Count children for each subsection, after converting from an xml section
	// to a binary section on disk. This uses the "other" convert method.

	DataSectionPtr	x = BWResource::openSection( "test_xml_section.xml" );
	BW::string		i = BWResolver::resolveFilename( "test_xml_section.xml" );
	BW::string		o = BWResolver::resolveFilename( "result_packed_section2" );
	bool			r = PackedSection::convert( i, o, NULL, false );

	CHECK_EQUAL( true, r );

	DataSectionPtr p = BWResource::openSection("result_packed_section2");

	RETURN_ON_FAIL_CHECK( p != NULL );
	RETURN_ON_FAIL_CHECK( x != NULL );

	CHECK_EQUAL( x->countChildren(), p->countChildren() );
}

TEST_F( Fixture, ResManagerPath_openSectionInTree )
{
	CHECK( BWResource::openSectionInTree( "test_path/test_openSectionInTree/testA/test1/test2/", "space.settings" ) != NULL );
	CHECK( BWResource::openSectionInTree( "test_path/test_openSectionInTree/testA/test1/test2/", "space.settings" )->readString("name") == "minspec" );
	CHECK( BWResource::openSectionInTree( "test_path/test_openSectionInTree/testA/test1/", "space.settings" ) != NULL );
	CHECK( BWResource::openSectionInTree( "test_path/test_openSectionInTree/testA/test1/", "space.settings" )->readString("name") == "highlands" );
	CHECK( BWResource::openSectionInTree( "test_path/test_openSectionInTree/testB/test1/test2/", "space.settings" ) == NULL );
	CHECK( BWResource::openSectionInTree( "test_path/test_openSectionInTree/DoesntExist/test1/test2/", "space.settings" ) == NULL );
}

BW_END_NAMESPACE

// test_packed_section.cpp
