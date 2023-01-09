#include "pch.hpp"
#include "resmgr/bwresource.hpp"

#define UNIX_PATH_SEPARATOR '/'
#define WINDOWS_PATH_SEPARATOR '\\'

BW_BEGIN_NAMESPACE

bool checkStripResult( const char* raw, const char* expected )
{
	BW::StringRef strippedPath = BWResource::removeExtension( raw );
	if( strippedPath.compare( expected ) == 0 )
		return true;
	return false;
}

bool checkExtensionResult( const char* raw, const char * expected )
{
	BW::StringRef strippedExtension = BWResource::getExtension( raw );
	if( strippedExtension.compare( expected ) == 0 )
		return true;
	return false;
}


/**
 *	Convert slashes in path to use '//' or '\' based on operating system.
 *	@param path the path to replace the slashes in.
 */
void conformSlash( BW::string& path )
{
#if defined( WIN32 )
	std::replace( path.begin(), path.end(),
		UNIX_PATH_SEPARATOR, WINDOWS_PATH_SEPARATOR );
#else // defined( WIN32 )
	std::replace( path.begin(), path.end(),
		WINDOWS_PATH_SEPARATOR, UNIX_PATH_SEPARATOR );
#endif // defined( WIN32 )
}


bool checkCaseResult( const char* raw, const char* expected )
{
#if defined( WIN32 )
	const BW::string corrected = BWResource::correctCaseOfPath( raw );

	BW::string expectedOnDisk( expected );
	conformSlash( expectedOnDisk );

	if (corrected.compare( expectedOnDisk ) == 0)
	{
		return true;
	}
	return false;
#else // defined( WIN32 )
	// BWResource::correctCaseOfPath() Not implemented in UnixFileSystem
	return true;
#endif // defined( WIN32 )
}


/**
 *	This tests multiple paths and the stripping of their extensions.
 */
TEST( ResManagerPath_Chopping )
{
	CHECK( checkStripResult( "../../../fantasydemo/res/test.visual",
		"../../../fantasydemo/res/test" ) );
	CHECK( checkStripResult( "../../../fantasydemo/res/test",
		"../../../fantasydemo/res/test" ) );
	CHECK( checkStripResult( "../../../wales 1.0.0/res/test.visual",
		"../../../wales 1.0.0/res/test" ) );
	CHECK( checkStripResult( "../../../wales 1.0.0/res/test",
		"../../../wales 1.0.0/res/test" ) );
	CHECK( checkStripResult( "../../../../bigworld/tools/exporter/resources/test.visual",
		"../../../../bigworld/tools/exporter/resources/test" ) );
	CHECK( checkStripResult( "../../../../bigworld/tools/exporter/resources/test",
		"../../../../bigworld/tools/exporter/resources/test" ) );
	CHECK( checkStripResult( "D:/Bigworld/fantasydemo/res/spaces/test.visual",
		"D:/Bigworld/fantasydemo/res/spaces/test" ) );
	CHECK( checkStripResult( "D:/Bigworld/fantasydemo/res/spaces/test",
		"D:/Bigworld/fantasydemo/res/spaces/test" ) );
	CHECK( checkStripResult( "D:/wales 1.0.0/res/test.visual",
		"D:/wales 1.0.0/res/test" ) );
	CHECK( checkStripResult( "D:/wales 1.0.0/res/test", "D:/wales 1.0.0/res/test" ) );
	CHECK( checkStripResult( "..\\..\\..\\fantasydemo\\res\\test.visual",
		"..\\..\\..\\fantasydemo\\res\\test" ) );
	CHECK( checkStripResult( "..\\..\\..\\fantasydemo\\res\\test",
		"..\\..\\..\\fantasydemo\\res\\test" ) );
	CHECK( checkStripResult( "..\\..\\..\\wales 1.0.0\\res\\test.visual",
		"..\\..\\..\\wales 1.0.0\\res\\test" ) );
	CHECK( checkStripResult( "..\\..\\..\\wales 1.0.0\\res\\test",
		"..\\..\\..\\wales 1.0.0\\res\\test" ) );
	CHECK( checkStripResult( "..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test.visual",
		"..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test" ) );
	CHECK( checkStripResult( "..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test",
		"..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test" ) );
	CHECK( checkStripResult( "D:\\Bigworld\\fantasydemo\\res\\spaces\\test.visual",
		"D:\\Bigworld\\fantasydemo\\res\\spaces\\test" ) );
	CHECK( checkStripResult( "D:\\Bigworld\\fantasydemo\\res\\spaces\\test",
		"D:\\Bigworld\\fantasydemo\\res\\spaces\\test" ) );
	CHECK( checkStripResult( "D:\\wales 1.0.0\\res\\test.visual",
		"D:\\wales 1.0.0\\res\\test" ) );
	CHECK( checkStripResult( "D:\\wales 1.0.0\\res\\test",
		"D:\\wales 1.0.0\\res\\test" ) );
	CHECK( checkStripResult( "test.thing", "test" ) );
	CHECK( checkStripResult( "test", "test" ) );
	CHECK( checkStripResult( "./test.thing", "./test" ) );
	CHECK( checkStripResult( "./test", "./test" ) );
	CHECK( checkStripResult( ".\\test.thing", ".\\test" ) );
	CHECK( checkStripResult( ".\\test", ".\\test" ) );
	CHECK( checkStripResult( "", "" ) );
}

TEST( ResManagerPath_extension )
{
	CHECK( checkExtensionResult( "../../../fantasydemo/res/test.visual", "visual" ) );
	CHECK( checkExtensionResult( "../../../fantasydemo/res/test", "" ) );
	CHECK( checkExtensionResult( "../../../wales 1.0.0/res/test.visual", "visual" ) );
	CHECK( checkExtensionResult( "../../../wales 1.0.0/res/test", "" ) );
	CHECK( checkExtensionResult( "../../../../bigworld/tools/exporter/resources/test.visual",
		"visual" ) );
	CHECK( checkExtensionResult( "../../../../bigworld/tools/exporter/resources/test",
		"" ) );
	CHECK( checkExtensionResult( "D:/Bigworld/fantasydemo/res/spaces/test.visual",
		"visual" ) );
	CHECK( checkExtensionResult( "D:/Bigworld/fantasydemo/res/spaces/test",
		"" ) );
	CHECK( checkExtensionResult( "D:/wales 1.0.0/res/test.visual", "visual" ) );
	CHECK( checkExtensionResult( "D:/wales 1.0.0/res/test", "" ) );
	CHECK( checkExtensionResult( "..\\..\\..\\fantasydemo\\res\\test.visual",
		"visual" ) );
	CHECK( checkExtensionResult( "..\\..\\..\\fantasydemo\\res\\test",
		"" ) );
	CHECK( checkExtensionResult( "..\\..\\..\\wales 1.0.0\\res\\test.visual",
		"visual" ) );
	CHECK( checkExtensionResult( "..\\..\\..\\wales 1.0.0\\res\\test",
		"" ) );
	CHECK( checkExtensionResult( "..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test.visual",
		"visual" ) );
	CHECK( checkExtensionResult( "..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test",
		"" ) );
	CHECK( checkExtensionResult( "D:\\Bigworld\\fantasydemo\\res\\spaces\\test.visual",
		"visual" ) );
	CHECK( checkExtensionResult( "D:\\Bigworld\\fantasydemo\\res\\spaces\\test",
		"" ) );
	CHECK( checkExtensionResult( "D:\\wales 1.0.0\\res\\test.visual", "visual" ) );
	CHECK( checkExtensionResult( "D:\\wales 1.0.0\\res\\test", "" ) );
	CHECK( checkExtensionResult( "test.thing", "thing" ) );
	CHECK( checkExtensionResult( "test", "" ) );
	CHECK( checkExtensionResult( "./test.thing", "thing" ) );
	CHECK( checkExtensionResult( "./test", "") );
	CHECK( checkExtensionResult( ".\\test.thing", "thing" ) );
	CHECK( checkExtensionResult( ".\\test", "" ) );
	CHECK( checkExtensionResult( "", "" ) );
}

TEST_F( ResMgrUnitTestHarness, ResManagerPath_case )
{
	CHECK( checkCaseResult(
		"../../../fantasydemo/res/test.visual",
		"../../../fantasydemo/res/test.visual" ) );

	CHECK( checkCaseResult(
		"../../../fantasydemo/res/test",
		"../../../fantasydemo/res/test" ) );

	CHECK( checkCaseResult(
		"../../../wales 1.0.0/res/test.visual",
		"../../../wales 1.0.0/res/test.visual" ) );

	CHECK( checkCaseResult(
		"../../../wales 1.0.0/res/test",
		"../../../wales 1.0.0/res/test" ) );

	CHECK( checkCaseResult(
		"../../../../bigworld/tools/exporter/resources/test.visual",
		"../../../../bigworld/tools/exporter/resources/test.visual" ) );

	CHECK( checkCaseResult(
		"../../../../bigworld/tools/exporter/resources/test",
		"../../../../bigworld/tools/exporter/resources/test" ) );

	CHECK( checkCaseResult(
		"D:/Bigworld/fantasydemo/res/spaces/test.visual",
		"D:/Bigworld/fantasydemo/res/spaces/test.visual" ) );

	CHECK( checkCaseResult(
		"D:/Bigworld/fantasydemo/res/spaces/test",
		"D:/Bigworld/fantasydemo/res/spaces/test" ) );

	CHECK( checkCaseResult(
		"D:/wales 1.0.0/res/test.visual",
		"D:/wales 1.0.0/res/test.visual" ) );

	CHECK( checkCaseResult(
		"D:/wales 1.0.0/res/test",
		"D:/wales 1.0.0/res/test" ) );

	CHECK( checkCaseResult(
		"..\\..\\..\\fantasydemo\\res\\test.visual",
		"../../../fantasydemo/res/test.visual" ) );

	CHECK( checkCaseResult(
		"..\\..\\..\\fantasydemo\\res\\test",
		"..\\..\\..\\fantasydemo\\res\\test" ) );

	CHECK( checkCaseResult(
		"..\\..\\..\\wales 1.0.0\\res\\test.visual",
		"../../../wales 1.0.0/res/test.visual" ) );

	CHECK( checkCaseResult(
		"..\\..\\..\\wales 1.0.0\\res\\test",
		"../../../wales 1.0.0/res/test" ) );

	CHECK( checkCaseResult(
		"..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test.visual",
		"../../../../bigworld/tools/exporter/resources/test.visual" ) );

	CHECK( checkCaseResult(
		"..\\..\\..\\..\\bigworld\\tools\\exporter\\resources\\test",
		"../../../../bigworld/tools/exporter/resources/test" ) );

	CHECK( checkCaseResult(
		"D:\\Bigworld\\fantasydemo\\res\\spaces\\test.visual",
		"D:/Bigworld/fantasydemo/res/spaces/test.visual" ) );

	CHECK( checkCaseResult(
		"D:\\Bigworld\\fantasydemo\\res\\spaces\\test",
		"D:/Bigworld/fantasydemo/res/spaces/test" ) );

	CHECK( checkCaseResult(
		"D:\\wales 1.0.0\\res\\test.visual",
		"D:/wales 1.0.0/res/test.visual" ) );

	CHECK( checkCaseResult(
		"D:\\wales 1.0.0\\res\\test",
		"D:/wales 1.0.0/res/test" ) );

	CHECK( checkCaseResult( "test.thing", "test.thing" ) );
	CHECK( checkCaseResult( "test", "test" ) );
	CHECK( checkCaseResult( "./test.thing", "./test.thing" ) );
	CHECK( checkCaseResult( "./test", "./test" ) );
	CHECK( checkCaseResult( ".\\test.thing", "./test.thing" ) );
	CHECK( checkCaseResult( ".\\test", "./test" ) );
	CHECK( checkCaseResult( "", "" ) );
}

BW_END_NAMESPACE

// test_path_resource.cpp

