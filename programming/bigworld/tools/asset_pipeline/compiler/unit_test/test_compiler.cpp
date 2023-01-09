#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "CppUnitLite2/src/CppUnitLite2.h"

#include "../test_compiler/test_compiler.hpp"

BW_BEGIN_NAMESPACE

HMODULE g_hPlugin;

bool loadPlugin( TestCompiler & testCompiler )
{
#ifdef _DEBUG
	g_hPlugin = testCompiler.loadPlugin( "testconverter_d" );
#else
	g_hPlugin = testCompiler.loadPlugin( "testconverter" );
#endif
	return g_hPlugin != NULL;
}

bool unloadPlugin( TestCompiler & testCompiler )
{
	HMODULE hPlugin = g_hPlugin;
	g_hPlugin = NULL;
	return testCompiler.unloadPlugin( hPlugin );
}

bool setupDirectories( TestCompiler & testCompiler )
{
	return testCompiler.setSourceFileDirectory( "tools/asset_pipeline/resources/testfiles/testconverter/" ) &&
		   testCompiler.setOutputFileDirectory( "tools/asset_pipeline/resources/testfiles/testconverter/" );
}

bool setupRecursiveDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "001.testrootasset" ) &&
		   testCompiler.addSourceFile( "001/001.testasset" ) &&
		   testCompiler.addSourceFile( "001/002.testasset" ) &&
		   testCompiler.addSourceFile( "001/003.testasset" ) &&
		   testCompiler.addSourceFile( "001/004.testasset" ) &&
		   testCompiler.addSourceFile( "001/005.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/001.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/002.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/003.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/004.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/005.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/001/001.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/001/002.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/001/003.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/001/004.testasset" ) &&
		   testCompiler.addSourceFile( "001/001/001/005.testasset" ) &&

		   testCompiler.addOutputFile( "001.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/002.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/003.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/004.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/005.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/001.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/002.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/003.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/004.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/005.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/001/001.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/001/002.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/001/003.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/001/004.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "001/001/001/005.testcompiledasset" );
}

bool setupRootDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "002.testrootasset" ) &&
		   testCompiler.addSourceFile( "002/001.testrootasset" ) &&

		   testCompiler.addOutputFile( "002.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "002/001.testcompiledasset" );
}

bool setupSelfDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "003.testrootasset" );
}

bool setupCyclicDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "004/001.testrootasset" ) &&
		   testCompiler.addSourceFile( "004/002.testrootasset" );
}

bool setupUnknownDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "005.testrootasset" );
}

bool setupMissingDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "006.testrootasset" );
}

bool setupDependencyErrorTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "007.testrootasset" );
}

bool setupConversionErrorTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "008.testrootasset" );
}

bool setupOutputPathTest( TestCompiler & testCompiler )
{
	return testCompiler.addSourceFile( "009.testrootasset" ) &&
		   testCompiler.addSourceFile( "009/001.testasset" ) &&
		   testCompiler.addSourceFile( "009/002.testasset" ) &&
		   testCompiler.addSourceFile( "009/003.testasset" ) &&
		   testCompiler.addSourceFile( "009/004.testasset" ) &&
		   testCompiler.addSourceFile( "009/005.testasset" ) &&

		   testCompiler.addOutputFile( "output/009.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "output/009/001.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "output/009/002.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "output/009/003.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "output/009/004.testcompiledasset" ) &&
		   testCompiler.addOutputFile( "output/009/005.testcompiledasset" );
}

bool buildDirectory( TestCompiler & testCompiler )
{
	return testCompiler.testBuildDirectory( "" );
}

bool VerifyRecursiveDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 16 &&
		   testCompiler.getTaskFailedCount() == 0 &&
		   !testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
	       !testCompiler.hasConversionError();
}

bool VerifyRootDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 2 &&
		   testCompiler.getTaskFailedCount() == 0 &&
		   !testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
		   !testCompiler.hasConversionError();
}

bool VerifySelfDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 1 &&
		   testCompiler.getTaskFailedCount() == 1 &&
		   testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
		   !testCompiler.hasConversionError();
}

bool VerifyCyclicDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 2 &&
		   testCompiler.getTaskFailedCount() == 2 &&
		   testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
		   !testCompiler.hasConversionError();
}

bool VerifyUnknownDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 1 &&
		   testCompiler.getTaskFailedCount() == 0 &&
		   !testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
		   !testCompiler.hasConversionError();
}

bool VerifyMissingDependencyTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 1 &&
		   testCompiler.getTaskFailedCount() == 1 &&
		   !testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
		   !testCompiler.hasConversionError();
}

bool VerifyDependencyErrorTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 1 &&
		   testCompiler.getTaskFailedCount() == 1 &&
		   !testCompiler.hasCyclicError() &&
		   testCompiler.hasDependencyError() &&
		   !testCompiler.hasConversionError();
}

bool VerifyConversionErrorTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 1 &&
		   testCompiler.getTaskFailedCount() == 1 &&
		   !testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
		   testCompiler.hasConversionError();
}

bool VerifyOutputPathTest( TestCompiler & testCompiler )
{
	return testCompiler.getTaskCount() == 6 &&
		   testCompiler.getTaskFailedCount() == 0 &&
	 	   !testCompiler.hasCyclicError() &&
		   !testCompiler.hasDependencyError() &&
		   !testCompiler.hasConversionError();
}

TEST( compiler_test_root_conversion_rule )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( testCompiler.addSourceFile( "000.testrootasset" ) );
	CHECK( testCompiler.testConversionRule( "000.testrootasset", "000.testcompiledasset", "TestConverter", "" ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_conversion_rule )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( testCompiler.addSourceFile( "000.testasset" ) );
	CHECK( testCompiler.testConversionRule( "000.testasset", "000.testcompiledasset", "TestConverter", "" ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_recursive_dependencies )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRecursiveDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRecursiveDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_recursive_dependencies_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRecursiveDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRecursiveDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_recursive_dependencies_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRecursiveDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRecursiveDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_recursive_dependencies_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRecursiveDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRecursiveDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_root_dependency )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRootDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRootDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_root_dependency_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRootDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRootDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_root_dependency_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRootDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRootDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_root_dependency_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupRootDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyRootDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_self_dependency )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupSelfDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifySelfDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_self_dependency_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupSelfDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifySelfDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_self_dependency_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupSelfDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifySelfDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_self_dependency_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupSelfDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifySelfDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_cyclic_dependency )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupCyclicDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyCyclicDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_cyclic_dependency_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupCyclicDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyCyclicDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_cyclic_dependency_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupCyclicDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyCyclicDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_cyclic_dependency_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupCyclicDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyCyclicDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_unknown_dependency )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupUnknownDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyUnknownDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_unknown_dependency_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupUnknownDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyUnknownDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_unknown_dependency_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupUnknownDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyUnknownDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_unknown_dependency_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupUnknownDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyUnknownDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_missing_dependency )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupMissingDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyMissingDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_missing_dependency_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupMissingDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyMissingDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_missing_dependency_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupMissingDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyMissingDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_missing_dependency_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupMissingDependencyTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyMissingDependencyTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_dependency_error )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupDependencyErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyDependencyErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_dependency_error_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupDependencyErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyDependencyErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_dependency_error_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupDependencyErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyDependencyErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_dependency_error_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupDependencyErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyDependencyErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_conversion_error )
{
	TestCompiler testCompiler( true, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupConversionErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyConversionErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_conversion_error_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupConversionErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyConversionErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_conversion_error_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupConversionErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyConversionErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_conversion_error_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupConversionErrorTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyConversionErrorTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_output_path )
{
	TestCompiler testCompiler( true, 1, "", "output/" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupOutputPathTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyOutputPathTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_output_path_multithreaded )
{
	TestCompiler testCompiler( true, 4, "", "output/" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupOutputPathTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyOutputPathTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_output_path_non_blocking )
{
	TestCompiler testCompiler( false, 1, "", "output/" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupOutputPathTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyOutputPathTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

TEST( compiler_test_output_path_non_blocking_multithreaded )
{
	TestCompiler testCompiler( false, 4, "", "output/" );
	CHECK( loadPlugin( testCompiler ) );
	CHECK( setupDirectories( testCompiler ) );
	CHECK( setupOutputPathTest( testCompiler ) );
	CHECK( buildDirectory( testCompiler ) );
	CHECK( VerifyOutputPathTest( testCompiler ) );
	CHECK( unloadPlugin( testCompiler ) );
}

BW_END_NAMESPACE
