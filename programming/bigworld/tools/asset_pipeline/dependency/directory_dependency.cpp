#include "directory_dependency.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

namespace DirectoryDependency_Locals
{
	// XML tag names
	const char * DIRECTORY_TAG = "Directory";
	const char * PATTERN_TAG = "Pattern";
	const char * REGEX_TAG = "Regex";
	const char * RECURSIVE_TAG = "Recursive";
}

DirectoryDependency::DirectoryDependency()
{
}

DirectoryDependency::DirectoryDependency( const BW::string & directory, 
	const BW::string & pattern, bool regex, bool recursive )
{
	MF_ASSERT( directory.empty() || BWResource::pathIsRelative( directory ) );
	directory_ = directory;
	pattern_ = pattern;
	regex_ = regex;
	recursive_ = recursive;
}

DirectoryDependency::~DirectoryDependency()
{
}

bool DirectoryDependency::serialiseIn( DataSectionPtr pSection )
{
	// Search for the directory tag
	DataSectionPtr directoryPtr = pSection->findChild( DirectoryDependency_Locals::DIRECTORY_TAG );
	if (!directoryPtr || !directoryPtr->isAttribute())
	{
		// Could not find the directory tag,
		// there is an error with this data section
		return false;
	}

	// Search for the pattern tag
	DataSectionPtr patternPtr = pSection->findChild( DirectoryDependency_Locals::PATTERN_TAG );
	if (!patternPtr || !patternPtr->isAttribute())
	{
		// Could not find the pattern tag,
		// there is an error with this data section
		return false;
	}

	// Search for the regex tag
	DataSectionPtr regexPtr = pSection->findChild( DirectoryDependency_Locals::REGEX_TAG );
	if (!regexPtr || !regexPtr->isAttribute())
	{
		// Could not find the recursive tag,
		// there is an error with this data section
		return false;
	}

	// Search for the recursive tag
	DataSectionPtr recursivePtr = pSection->findChild( DirectoryDependency_Locals::RECURSIVE_TAG );
	if (!recursivePtr || !recursivePtr->isAttribute())
	{
		// Could not find the recursive tag,
		// there is an error with this data section
		return false;
	}

	// Parse the directory section
	directory_ = directoryPtr->asString();
	// Parse the pattern section
	pattern_ = patternPtr->asString();
	// Parse the recursive section
	regex_ = regexPtr->asBool();
	// Parse the recursive section
	recursive_ = recursivePtr->asBool();

	return Dependency::serialiseIn( pSection );
}

bool DirectoryDependency::serialiseOut( DataSectionPtr pSection ) const
{
	// Create a directory sub section in the provided section 
	// and save out our directory.
	DataSectionPtr directoryPtr = pSection->newSection( DirectoryDependency_Locals::DIRECTORY_TAG );
	MF_ASSERT( directoryPtr.hasObject() );
	MF_VERIFY( directoryPtr->isAttribute( true ) );
	MF_VERIFY( directoryPtr->setString( directory_ ) );

	// Create a pattern sub section in the provided section 
	// and save out our pattern.
	DataSectionPtr patternPtr = pSection->newSection( DirectoryDependency_Locals::PATTERN_TAG );
	MF_ASSERT( patternPtr.hasObject() );
	MF_VERIFY( patternPtr->isAttribute( true ) );
	MF_VERIFY( patternPtr->setString( pattern_ ) );

	// Create a regex sub section in the provided section 
	// and save out our regex flag.
	DataSectionPtr regexPtr = pSection->newSection( DirectoryDependency_Locals::REGEX_TAG );
	MF_ASSERT( regexPtr.hasObject() );
	MF_VERIFY( regexPtr->isAttribute( true ) );
	MF_VERIFY( regexPtr->setBool( regex_ ) );

	// Create a recursive sub section in the provided section 
	// and save out our recursive flag.
	DataSectionPtr recursivePtr = pSection->newSection( DirectoryDependency_Locals::RECURSIVE_TAG );
	MF_ASSERT( recursivePtr.hasObject() );
	MF_VERIFY( recursivePtr->isAttribute( true ) );
	MF_VERIFY( recursivePtr->setBool( recursive_ ) );

	return Dependency::serialiseOut( pSection );
}

BW_END_NAMESPACE
