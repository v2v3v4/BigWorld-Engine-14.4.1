#include "asset_pipeline/compiler/compiler.hpp"
#include "cstdmf/bw_hash64.hpp"

#include "converter_dependency.hpp"
#include "converter_params_dependency.hpp"
#include "dependency.hpp"
#include "dependency_list.hpp"
#include "directory_dependency.hpp"
#include "intermediate_file_dependency.hpp"
#include "output_file_dependency.hpp"
#include "source_file_dependency.hpp"

BW_BEGIN_NAMESPACE

namespace DependencyList_Locals
{
	// XML tag names
	const char * PRIMARY_INPUTS_TAG = "PrimaryInputs";
	const char * SECONDARY_INPUTS_TAG = "SecondaryInputs";
	const char * INTERMEDIATE_OUTPUTS_TAG = "IntermediateOutputs";
	const char * OUTPUTS_TAG = "Outputs";
	const char * DEPENDENCY_TAG = "Dependency";
	const char * DEPENDENCY_TYPE_TAG = "Type";
	const char * COMPILED_TAG = "Compiled";
	const char * FILE_TAG = "File";
	const char * HASH_TAG = "Hash";
}

// Automatically declare creation functions for all known dependency types.
#define X( type ) Dependency * Create##type() { return new type(); }
	DEPENDENCY_TYPES
#undef X

// Create an array of function pointers to the declared creation functions.
// This will be used to automatically create a dependency object from a
// dependency type when serialising dependencies from a file on disk.
#define X( type ) Create##type,
	Dependency * (*CreateDependency[InvalidDependencyType])() = { DEPENDENCY_TYPES };
#undef X

DependencyList::DependencyList( Compiler & compiler )
	: compiler_( compiler )
{

}

DependencyList::~DependencyList()
{
	reset();
}

void DependencyList::reset()
{
	// Clean up all out inputs. Make sure to delete them as we own them
	BW::vector< Input >::const_iterator it;
	for ( it = primaryInputs_.cbegin(); it != primaryInputs_.cend(); ++it )
	{
		delete it->first;
	}
	for ( it = secondaryInputs_.cbegin(); it != secondaryInputs_.cend(); ++it )
	{
		delete it->first;
	}
	primaryInputs_.clear();
	secondaryInputs_.clear();
	intermediateOutputs_.clear();
	outputs_.clear();
}

void DependencyList::initialise( const BW::string & source,
						         uint64 converterId,
						         const BW::string & converterVersion,
						         const BW::string & converterParams )
{
	reset();

	addPrimarySourceFileDependency( source );
	addPrimaryConverterDependency( converterId, converterVersion );
	addPrimaryConverterParamsDependency( converterParams );
}

void DependencyList::addPrimarySourceFileDependency( const BW::string & filename )
{
	Dependency * dependency = new SourceFileDependency( filename );
	dependency->setCritical( true );
	// Push the dependency into our secondary inputs.
	primaryInputs_.push_back( std::make_pair( dependency, compiler_.getHash( *dependency ) ) );
}

void DependencyList::addPrimaryConverterDependency( uint64 converterId, const BW::string & converterVersion )
{
	Dependency * dependency = new ConverterDependency( converterId, converterVersion );
	dependency->setCritical( true );
	// Push the dependency into our primary inputs.
	primaryInputs_.push_back( std::make_pair( dependency, compiler_.getHash( *dependency ) ) );
}

void DependencyList::addPrimaryConverterParamsDependency( const BW::string & converterParams )
{
	Dependency * dependency = new ConverterParamsDependency( converterParams );
	dependency->setCritical( true );
	// Push the dependency into our primary inputs.
	primaryInputs_.push_back( std::make_pair( dependency, compiler_.getHash( *dependency ) ) );
}

void DependencyList::addSecondarySourceFileDependency( const BW::string & filename, bool critical )
{
	Dependency * dependency = new SourceFileDependency( filename );
	dependency->setCritical( critical );
	// Push the dependency into our secondary inputs.
	secondaryInputs_.push_back( std::make_pair( dependency, 0 ) );
}

void DependencyList::addSecondaryIntermediateFileDependency( const BW::string & filename, bool critical )
{
	Dependency * dependency = new IntermediateFileDependency( filename );
	dependency->setCritical( critical );
	// Push the dependency into our secondary inputs.
	secondaryInputs_.push_back( std::make_pair( dependency, 0 ) );
}

void DependencyList::addSecondaryOutputFileDependency( const BW::string & filename, bool critical )
{
	Dependency * dependency = new OutputFileDependency( filename );
	dependency->setCritical( critical );
	// Push the dependency into our secondary inputs.
	secondaryInputs_.push_back( std::make_pair( dependency, 0 ) );
}

void DependencyList::addSecondaryDirectoryDependency( const BW::string & directory, 
	const BW::string & pattern, bool regex, bool recursive, bool critical )
{
	Dependency * dependency = new DirectoryDependency( directory, pattern, regex, recursive );
	dependency->setCritical( critical );
	// Push the dependency into our secondary inputs.
	secondaryInputs_.push_back( std::make_pair( dependency, 0 ) );
}

uint64 DependencyList::getInputHash( bool includeSecondary )
{
	uint64 hash = 0;

	Hash64::compute( primaryInputs_.size() );
	BW::vector< Input >::const_iterator it;
	for ( it = primaryInputs_.cbegin(); it != primaryInputs_.cend(); ++it )
	{
		Hash64::combine( hash, it->second );
	}

	if (includeSecondary)
	{
		Hash64::combine( hash, secondaryInputs_.size() );
		for ( it = secondaryInputs_.cbegin(); it != secondaryInputs_.cend(); ++it )
		{
			Hash64::combine( hash, it->second );
		}
	}
	
	return hash;
}

bool DependencyList::serialiseIn( DataSectionPtr pSection )
{
	reset();

	if(!pSection.hasObject())
	{
		// The data section is null. Something is wrong
		return false;
	}

	// Serialise in our dependencies and our outputs.
	if (!serialiseIn( pSection->findChild( DependencyList_Locals::PRIMARY_INPUTS_TAG ), primaryInputs_ ) ||
		!serialiseIn( pSection->findChild( DependencyList_Locals::SECONDARY_INPUTS_TAG ), secondaryInputs_ ) ||
		!serialiseIn( pSection->findChild( DependencyList_Locals::INTERMEDIATE_OUTPUTS_TAG ), intermediateOutputs_ ) ||
		!serialiseIn( pSection->findChild( DependencyList_Locals::OUTPUTS_TAG ), outputs_ ))
	{
		// Something went wrong serialising in our dependencies or outputs.
		// Reset ourselves before returning
		reset();
		return false;
	}

	return true;
}

bool DependencyList::serialiseOut( DataSectionPtr pSection ) const
{
	// Make sure the section we are saving to is clean.
	pSection->delChildren();

	// Serialise out our dependencies and outputs.
	if (!serialiseOut( pSection->newSection( DependencyList_Locals::PRIMARY_INPUTS_TAG ), primaryInputs_ ) ||
		!serialiseOut( pSection->newSection( DependencyList_Locals::SECONDARY_INPUTS_TAG ), secondaryInputs_ ) ||
		!serialiseOut( pSection->newSection( DependencyList_Locals::INTERMEDIATE_OUTPUTS_TAG ), intermediateOutputs_ ) ||
		!serialiseOut( pSection->newSection( DependencyList_Locals::OUTPUTS_TAG ), outputs_ ))
	{
		// Something went wrong serialising out our dependencies or outputs.
		// No point to continue.
		return false;
	}

	return true;
}

bool DependencyList::serialiseIn( DataSectionPtr pSection, BW::vector< Input > & o_dependencies )
{
	MF_ASSERT(o_dependencies.empty());

	if (!pSection.hasObject())
	{
		return false;
	}

	DataSectionIterator it;
	for( it = pSection->begin(); it != pSection->end(); ++it )
	{
		// Unnecessary sanity check. Disable to avoid a string compare
		/*if (it.tag() != DependencyList_Locals::DEPENDENCY_TAG)
		{
			continue;
		}*/

		// Search for the dependency type tag
		DataSectionPtr typePtr = (*it)->findChild( DependencyList_Locals::DEPENDENCY_TYPE_TAG );
		if (!typePtr || !typePtr->isAttribute())
		{
			// Could not find the dependency type tag,
			// there is an error with this data section - ignore it
			continue;
		}

		// Search for the hash tag
		DataSectionPtr hashPtr = (*it)->findChild( DependencyList_Locals::HASH_TAG );
		if (!hashPtr || !hashPtr->isAttribute())
		{
			// Could not find the hash tag,
			// there is an error with this data section - ignore it
			continue;
		}

		// Parse the dependency type section
		unsigned int type = typePtr->asUInt( InvalidDependencyType );
		// Parse the hash section
		uint64 hash = hashPtr->asUInt64();

		if (type >= InvalidDependencyType)
		{
			// Found an unknown dependency type - ignore it
			continue;
		}

		// Create a new dependency object based on the type
		Dependency * dependency = CreateDependency[type]();
		// Call the dependency's internal serialise in function
		if (!dependency->serialiseIn( *it ))
		{
			// Something went wrong - delete the dependency and ignore it
			delete dependency;
			return false;
		}

		// Push back the new dependency
		o_dependencies.push_back( std::make_pair( dependency, hash ) );
	}

	return true;
}

bool DependencyList::serialiseOut( DataSectionPtr pSection, const BW::vector< Input > & dependencies ) const
{
	if (!pSection.hasObject())
	{
		return false;
	}

	MF_ASSERT( pSection->countChildren() == 0 );

	BW::vector< Input >::const_iterator it;
	for ( it = dependencies.cbegin(); it != dependencies.cend(); ++it )
	{
		// Create a dependency sub section
		DataSectionPtr dependencyPtr = pSection->newSection( DependencyList_Locals::DEPENDENCY_TAG );
		MF_ASSERT( dependencyPtr.hasObject() );
		
		// Create a dependency type sub section
		DataSectionPtr typePtr = dependencyPtr->newSection( DependencyList_Locals::DEPENDENCY_TYPE_TAG );
		MF_ASSERT( typePtr.hasObject() );
		MF_VERIFY( typePtr->isAttribute( true ) );
		MF_VERIFY( typePtr->setUInt( it->first->getType() ) );

		// Create a hash sub section
		DataSectionPtr hashPtr = dependencyPtr->newSection( DependencyList_Locals::HASH_TAG );
		MF_ASSERT( hashPtr.hasObject() );
		MF_VERIFY( hashPtr->isAttribute( true ) );
		MF_VERIFY( hashPtr->setUInt64( it->second ) );

		// Call the dependency's internal serialise out function
		if ( !it->first->serialiseOut( dependencyPtr ) )
		{
			// Something went wrong. Return a failure
			return false;
		}
	}

	return true;
}

bool DependencyList::serialiseIn( DataSectionPtr pSection, BW::vector< Output > & o_outputs )
{
	MF_ASSERT(o_outputs.empty());

	if (!pSection.hasObject())
	{
		return false;
	}

	DataSectionIterator it;
	for( it = pSection->begin(); it != pSection->end(); ++it )
	{
		// Unnecessary sanity check. Disable to avoid a string compare
		/*if (it.tag() != DependencyList_Locals::COMPILED_TAG)
		{
			continue;
		}*/

		// Search for the file tag
		DataSectionPtr filePtr = (*it)->findChild( DependencyList_Locals::FILE_TAG );
		if (!filePtr || !filePtr->isAttribute())
		{
			// Could not find the file tag,
			// there is an error with this data section - ignore it
			continue;
		}

		// Search for the hash tag
		DataSectionPtr hashPtr = (*it)->findChild( DependencyList_Locals::HASH_TAG );
		if (!hashPtr || !hashPtr->isAttribute())
		{
			// Could not find the hash tag,
			// there is an error with this data section - ignore it
			continue;
		}

		// Parse the file section
		BW::string file = filePtr->asString();
		// Parse the hash section
		uint64 hash = hashPtr->asUInt64();
		// Push back the file and hash into out outputs
		o_outputs.push_back( std::make_pair( file, hash ) );
	}

	return true;
}

bool DependencyList::serialiseOut( DataSectionPtr pSection, const BW::vector< Output > & outputs ) const
{
	if (!pSection.hasObject())
	{
		return false;
	}

	// Serialise out our outputs
	BW::vector< Output >::const_iterator it;
	for ( it = outputs.cbegin(); it != outputs.cend(); ++it )
	{
		// Create a compiled sub section
		DataSectionPtr compiledPtr = pSection->newSection( DependencyList_Locals::COMPILED_TAG );
		MF_ASSERT( compiledPtr.hasObject() );

		// Create a file sub section
		DataSectionPtr filePtr = compiledPtr->newSection( DependencyList_Locals::FILE_TAG );
		MF_ASSERT( filePtr.hasObject() );
		MF_VERIFY( filePtr->isAttribute( true ) );
		MF_VERIFY( filePtr->setString( it->first ) );

		// Create a hash sub section
		DataSectionPtr hashPtr = compiledPtr->newSection( DependencyList_Locals::HASH_TAG );
		MF_ASSERT( hashPtr.hasObject() );
		MF_VERIFY( hashPtr->isAttribute( true ) );
		MF_VERIFY( hashPtr->setUInt64( it->second ) );
	}

	return true;
}

BW_END_NAMESPACE
