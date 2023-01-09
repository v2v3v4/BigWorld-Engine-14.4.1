#include "asset_pipeline/dependency/dependency.hpp"
#include "asset_pipeline/dependency/converter_dependency.hpp"
#include "asset_pipeline/dependency/converter_params_dependency.hpp"
#include "asset_pipeline/dependency/directory_dependency.hpp"
#include "asset_pipeline/dependency/intermediate_file_dependency.hpp"
#include "asset_pipeline/dependency/output_file_dependency.hpp"
#include "asset_pipeline/dependency/source_file_dependency.hpp"
#include "resmgr/xml_section.hpp"
#include "CppUnitLite2/src/CppUnitLite2.h"

BW_BEGIN_NAMESPACE

namespace DependencyUnitTest
{
	// Must be implemented for any new dependency type that is created.
	// Should create an instance of the dependency type and initialise it
	// with default values.
	template<class T>
	T * createTestDependency();

	// Must be implemented for any new dependency type that is created.
	// Should compare two dependencies of the same type and verify that
	// they are equal.
	template<class T>
	bool compareDependencies( const T & lhs, const T & rhs );

	template<class T>
	inline bool testDependencySerialisation( bool criticalDependency )
	{
		// Create a test xml section
		DataSectionPtr xmlSection = new XMLSection( "DependencyUnitTest" );
		// Create and initialise a dependency
		std::auto_ptr< T > dependency( createTestDependency<T>() );
		// Set the dependency to critical if we are testing critical dependencies
		dependency->setCritical( criticalDependency );
		// Attempt to serialise the dependency out
		if (!dependency->serialiseOut( xmlSection )) return false;
		// Create a new dependency
		std::auto_ptr< T > newDependency( new T() );
		// Attempt to serialise the new dependency in from the section we just serialised out
		if (!newDependency->serialiseIn( xmlSection )) return false;
		// Check the critical flag matches what we set it to
		if (newDependency->isCritical() != criticalDependency ) return false;
		// Check the dependencies are equal
		if (!compareDependencies( *dependency, *newDependency )) return false;
		return true;
	}

	template<class T>
	inline bool testDependencySerialisation()
	{
		// Test serialising a non critical dependency
		if (!testDependencySerialisation<T>(false)) return false;
		// Test serialising a critical dependency
		if (!testDependencySerialisation<T>(true)) return false;
		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	/* SourceFileDependency*/
	template<>
	SourceFileDependency * createTestDependency<SourceFileDependency>()
	{
		return new SourceFileDependency( "TestFileName" );
	}

	template<>
	bool compareDependencies<SourceFileDependency>( const SourceFileDependency & lhs, const SourceFileDependency & rhs )
	{
		return lhs.getFileName() == rhs.getFileName();
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/* IntermediateFileDependency */
	template<>
	IntermediateFileDependency * createTestDependency<IntermediateFileDependency>()
	{
		return new IntermediateFileDependency( "TestFileName" );
	}

	template<>
	bool compareDependencies<IntermediateFileDependency>( const IntermediateFileDependency & lhs, const IntermediateFileDependency & rhs )
	{
		return lhs.getFileName() == rhs.getFileName();
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/* OutputFileDependency */
	template<>
	OutputFileDependency * createTestDependency<OutputFileDependency>()
	{
		return new OutputFileDependency( "TestFileName" );
	}

	template<>
	bool compareDependencies<OutputFileDependency>( const OutputFileDependency & lhs, const OutputFileDependency & rhs )
	{
		return lhs.getFileName() == rhs.getFileName();
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/* ConverterDependency */
	template<>
	ConverterDependency * createTestDependency<ConverterDependency>()
	{
		return new ConverterDependency( 0xABC12345, "dEf.67890" );
	}

	template<>
	bool compareDependencies<ConverterDependency>( const ConverterDependency & lhs, const ConverterDependency & rhs )
	{
		return lhs.getConverterId() == rhs.getConverterId() &&
			   lhs.getConverterVersion() == rhs.getConverterVersion();
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/* ConverterParamsDependency */
	template<>
	ConverterParamsDependency * createTestDependency<ConverterParamsDependency>()
	{
		return new ConverterParamsDependency( "-a -b true -c 0 --def" );
	}

	template<>
	bool compareDependencies<ConverterParamsDependency>( const ConverterParamsDependency & lhs, const ConverterParamsDependency & rhs )
	{
		return lhs.getConverterParams() == rhs.getConverterParams();
	}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	/* DirectoryDependency */
	template<>
	DirectoryDependency * createTestDependency<DirectoryDependency>()
	{
		return new DirectoryDependency( "directory", "pattern", true, true );
	}

	template<>
	bool compareDependencies<DirectoryDependency>( const DirectoryDependency & lhs, const DirectoryDependency & rhs )
	{
		return lhs.getDirectory() == rhs.getDirectory() &&
			   lhs.getPattern() == rhs.getPattern() &&
			   lhs.isRecursive() == rhs.isRecursive() &&
			   lhs.isRecursive() == rhs.isRecursive();
	}
	//////////////////////////////////////////////////////////////////////////
}

TEST( dependency_serialisation_test )
{
#define X( type ) CHECK( DependencyUnitTest::testDependencySerialisation<type>() );
	DEPENDENCY_TYPES
#undef X
}

BW_END_NAMESPACE
