#include "source_file_dependency.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

namespace SourceFileDependency_Locals
{
	// XML tag names
	const char * FILE_TAG = "File";
}

SourceFileDependency::SourceFileDependency()
{
}

SourceFileDependency::SourceFileDependency( const BW::string & fileName )
{
	MF_ASSERT( BWResource::pathIsRelative( fileName ) );
	fileName_ = fileName;
}

SourceFileDependency::~SourceFileDependency()
{
}

bool SourceFileDependency::serialiseIn( DataSectionPtr pSection )
{
	// Search for the file tag
	DataSectionPtr filePtr = pSection->findChild( SourceFileDependency_Locals::FILE_TAG );
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

bool SourceFileDependency::serialiseOut( DataSectionPtr pSection ) const
{
	// Create a file sub section in the provided section 
	// and save out our file name.
	DataSectionPtr filePtr = pSection->newSection( SourceFileDependency_Locals::FILE_TAG );
	MF_ASSERT( filePtr.hasObject() );
	MF_VERIFY( filePtr->isAttribute( true ) );
	MF_VERIFY( filePtr->setString( fileName_ ) );
	return Dependency::serialiseOut( pSection );
}

BW_END_NAMESPACE
