#include "output_file_dependency.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

namespace OutputFileDependency_Locals
{
	// XML tag names
	const char * FILE_TAG = "File";
}

OutputFileDependency::OutputFileDependency()
{
}

OutputFileDependency::OutputFileDependency( const BW::string & fileName )
{
	MF_ASSERT( BWResource::pathIsRelative( fileName ) );
	fileName_ = fileName;
}

OutputFileDependency::~OutputFileDependency()
{
}

bool OutputFileDependency::serialiseIn( DataSectionPtr pSection )
{
	// Search for the file tag
	DataSectionPtr filePtr = pSection->findChild( OutputFileDependency_Locals::FILE_TAG );
	if (!filePtr || !filePtr->isAttribute())
	{
		// Could not find the file tag,
		// there is an error with this data section
		return false;
	}

	// Parse the file section
	fileName_ = filePtr->asString();
	return Dependency::serialiseIn( pSection );
}

bool OutputFileDependency::serialiseOut( DataSectionPtr pSection ) const
{
	// Create a file sub section in the provided section 
	// and save out our file name.
	DataSectionPtr filePtr = pSection->newSection( OutputFileDependency_Locals::FILE_TAG );
	MF_ASSERT( filePtr.hasObject() );
	MF_VERIFY( filePtr->isAttribute( true ) );
	MF_VERIFY( filePtr->setString( fileName_ ) );
	return Dependency::serialiseOut( pSection );
}

BW_END_NAMESPACE
