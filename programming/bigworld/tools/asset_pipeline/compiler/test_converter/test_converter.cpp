#include "test_converter.hpp"

#include "asset_pipeline/dependency/dependency_list.hpp"
#include "asset_pipeline/compiler/compiler.hpp"
#include "cstdmf/bw_hash64.hpp"
#include "cstdmf/string_builder.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

const uint64 TestConverter::s_TypeId = Hash64::compute( TestConverter::getTypeName() );
const char * TestConverter::s_Version = "2.6";

namespace
{
	SimpleMutex g_ResourceLock;

	bool ExtractFiles( const BW::string & sourcefile, BW::string & directory, BW::vector<BW::string> & files )
	{
		StringRef ext = BWResource::getExtension( sourcefile );
		MF_ASSERT( ext == "testrootasset" || ext == "testasset" );

		g_ResourceLock.grab();
		DataSectionPtr pDataSection = BWResource::openSection( sourcefile );
		g_ResourceLock.give();
		if (pDataSection == NULL)
		{
			return false;
		}

		BinaryPtr pBinary = pDataSection->asBinary();
		char * data = ( char * )pBinary->data();

		BW::StringRef::size_type fileNamePos = 
			std::min( sourcefile.find_last_of( '/' ), sourcefile.find_last_of( '\\' ) );
		MF_ASSERT( fileNamePos != BW::StringRef::npos );
		directory = sourcefile.substr( 0, fileNamePos + 1 );

		StringBuilder file( MAX_PATH );
		int index = 0;
		do 
		{
			char c = data[index++];
			if (c == '\n' || index == pBinary->len())
			{
				if (file.length() > 0)
				{
					files.push_back( file.string() );
					file.clear();
				}
			}
			else if (c != ' ' && c != '\r')
			{
				file.append( c );
			}
		} while (index < pBinary->len());

		return true;
	}
}

/* construct a converter with parameters. */
TestConverter::TestConverter( const BW::string& params )
	: Converter( params )
{
}

TestConverter::~TestConverter()
{
}

/* builds the dependency list for a source file. */
bool TestConverter::createDependencies( const BW::string& sourcefile,
									    const Compiler & compiler,
										DependencyList & dependencies )
{
	BW::string directory;
	BW::vector<BW::string> files;
	if (!ExtractFiles( sourcefile, directory, files ))
	{
		return false;
	}

	for (BW::vector<BW::string>::const_iterator it =
		files.cbegin(); it != files.cend(); ++it)
	{
		BW::string dependency = directory + *it;
		compiler.resolveRelativePath( dependency );
		StringRef extension = BWResource::getExtension( dependency );
		if (extension == "testcompiledasset")
		{
			dependencies.addSecondaryOutputFileDependency( dependency, true );
		}
		else if (extension == "testdummyasset")
		{
			dependencies.addSecondaryOutputFileDependency( dependency, false );
		}
		else if (extension == "testasset")
		{
			dependencies.addSecondarySourceFileDependency( dependency, true );
		}
		else if (extension == "testdependencyerror")
		{
			ERROR_MSG( "DEPENDENCY ERROR!\n" );
			return false;
		}
		else if (extension != "testconverterror")
		{
			MF_ASSERT( false && "Unknown dependency extension" );
		}
	}

	return true;
}

/* convert a source file. */
/* work out the output filename & insert it into the converted files vector */
bool TestConverter::convert( const BW::string& sourcefile,
							 const Compiler & compiler,
							 BW::vector< BW::string > & intermediateFiles,
							 BW::vector< BW::string > & outputFiles )
{
	BW::string directory;
	BW::vector<BW::string> files;
	if (!ExtractFiles( sourcefile, directory, files ))
	{
		return false;
	}

	for (BW::vector<BW::string>::const_iterator it =
		files.cbegin(); it != files.cend(); ++it)
	{
		BW::string dependency = directory + *it;
		StringRef extension = BWResource::getExtension( dependency );
		if (extension == "testcompiledasset")
		{
			compiler.resolveOutputPath( dependency );
			MF_ASSERT( BWResource::fileExists( dependency ) );
		}
		else if (extension == "testasset")
		{
			MF_ASSERT( BWResource::fileExists( dependency ) );
		}
		else if (extension == "testconverterror")
		{
			ERROR_MSG( "CONVERSION ERROR!\n" );
			return false;
		}
	}

	BW::string outputFile = BWResource::changeExtension( sourcefile, ".testcompiledasset" );
	compiler.resolveOutputPath( outputFile );

	g_ResourceLock.grab();
	DataSectionPtr pDataSection = BWResource::openSection( outputFile, true );
	g_ResourceLock.give();
	if (pDataSection == NULL)
	{
		return false;
	}

	pDataSection->delChildren();
	pDataSection->writeStrings( "", files );
	pDataSection->save();

	MF_ASSERT( BWResource::fileExists( outputFile ) );
	outputFiles.push_back( outputFile );

	return true;
}

BW_END_NAMESPACE
