#include "pch.hpp"
#include "hierarchical_config.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "re2/re2.h"
#include <cstdmf/string_builder.hpp>

BW_USE_NAMESPACE

namespace HierarchicalConfig_Locals
{
	const char * TAG_DIRECTORY = "directory";
	const char * TAG_PATH = "path";
	const char * TAG_RULE = "rule";
	const char * TAG_PATTERN = "pattern";
	const char * TAG_NAME = "name";
	const char * TAG_IMPORT = "import";
	const char * TAG_IMPORT_DELIMETERS = ",";
}

HierarchicalConfig::HierarchicalConfig()
	: pRootSection_( NULL )
{

}

HierarchicalConfig::HierarchicalConfig( const DataSectionPtr pRootSection )
{
	load( pRootSection );
}

HierarchicalConfig::HierarchicalConfig( const StringRef & filename )
{
	load( filename );
}

HierarchicalConfig::HierarchicalConfig( const StringRef & filename, 
									   const StringRef & rootDirectory )
{
	load( filename, rootDirectory );
}

void HierarchicalConfig::load( const DataSectionPtr pRootSection )
{
	pRootSection_ = pRootSection;
}

void HierarchicalConfig::load( const StringRef & filename )
{
	pRootSection_ = BWResource::openSection( filename );
}

void HierarchicalConfig::load( const StringRef & filename, 
							   const StringRef & rootDirectory )
{
	// Build a hierarchical config from a directory structure
	XMLSectionTagCheckingStopper tagCheckingStopper;
	pRootSection_ = new XMLSection( rootDirectory );

	if (!rootDirectory.empty())
	{
		populateChildren( pRootSection_, filename, rootDirectory, StringHashMap< DataSectionPtr >() );
	}
	else
	{
		// If we dont specify a root directory construct it from all our resource paths
		int numPaths = BWResource::getPathNum();
		for (int i = numPaths; i > 0; --i)
		{
			BW::string path = BWResource::getPath( i - 1 );
			populateChildren( pRootSection_, filename, path, StringHashMap< DataSectionPtr >() );
		}
	}
}

DataSectionPtr HierarchicalConfig::get( const StringRef & path ) const
{
	if (pRootSection_ == NULL)
	{
		return NULL;
	}

	XMLSectionTagCheckingStopper tagCheckingStopper;
	DataSectionPtr pConfiguration = new XMLSection( "" );
	int numChildren = pRootSection_->countChildren();
	for (int i = 0; i < numChildren; ++i)
	{
		DataSectionPtr childSection = pRootSection_->openChild( i );
		if (childSection == NULL)
		{
			continue;
		}

		populateConfiguration( pConfiguration, childSection, path );
	}
	return pConfiguration;
}

void HierarchicalConfig::populateRule( DataSectionPtr pRule,
								       const DataSectionPtr pOverrides )
{
	// copy the children of the section into our rule.
	// override sections that already exist.
	int numChildren = pOverrides->countChildren();
	for (int i = 0; i < numChildren; ++i)
	{
		DataSectionPtr childSection = pOverrides->openChild( i );
		if (childSection == NULL || 
			childSection->sectionName() == HierarchicalConfig_Locals::TAG_NAME ||
			childSection->sectionName() == HierarchicalConfig_Locals::TAG_PATTERN ||
			childSection->sectionName() == HierarchicalConfig_Locals::TAG_IMPORT)
		{
			// Don't copy attributes
			continue;
		}

		DataSectionPtr dataSection = pRule->openSection( childSection->sectionName(), true );
		dataSection->copy( childSection );
	}
}

void HierarchicalConfig::populateChildren( const DataSectionPtr pSection, 
										   const StringRef & filename, 
										   const StringRef & directory,
										   const StringHashMap< DataSectionPtr > & namedRules )
{
	MultiFileSystem* fs = BWResource::instance().fileSystem();
	IFileSystem::Directory dir;
	fs->readDirectory( dir, directory );

	// Make a copy of the named rules so that we can push new named
	// rules into it for any recursive calls into populateChildren
	StringHashMap< DataSectionPtr > _namedRules( namedRules );

	BW::string formattedDirectory = BWUtil::formatPath( directory );
	char pathBuffer[BW_MAX_PATH];
	StringBuilder pathBuilder( pathBuffer, BW_MAX_PATH );

	// Process any config files in this directory
	for( IFileSystem::Directory::iterator it = dir.begin();
		it != dir.end(); ++it )
	{
		pathBuilder.clear();
		pathBuilder.append( formattedDirectory );
		pathBuilder.append( *it );
		const char * path = pathBuilder.string();

		IFileSystem::FileType ft = fs->getFileType( path );
		if (ft != IFileSystem::FT_FILE ||
			*it != filename)
		{
			continue;
		}

		DataSectionPtr fileSection = BWResource::openSection( path );
		if (fileSection == NULL)
		{
			continue;
		}

		int numChildren = fileSection->countChildren();
		for (int i = 0; i < numChildren; ++i)
		{
			DataSectionPtr childSection = fileSection->openChild(i);
			if (childSection == NULL)
			{
				continue;
			}

			if (childSection->sectionName() != HierarchicalConfig_Locals::TAG_RULE)
			{
				// We only care about rule sections
				continue;
			}

			DataSectionPtr ruleSection = pSection->newSection( HierarchicalConfig_Locals::TAG_RULE );

			DataSectionPtr importSection = childSection->findChild( HierarchicalConfig_Locals::TAG_IMPORT );
			if (importSection != NULL)
			{
				// If we have an import attribute, import the specified rules before
				// overriding with the ones defined locally

				// Take a copy of this sections contents so we can use bw_tokenise
				// with an array of stringrefs
				BW::string importString = importSection->asString();

				typedef BW::vector< BW::StringRef > StringArray;
				StringArray imports;
				bw_tokenise( BW::StringRef( importString ), HierarchicalConfig_Locals::TAG_IMPORT_DELIMETERS, imports );
				for ( StringArray::iterator it = imports.begin(); it != imports.end(); ++it )
				{
					StringHashMap< DataSectionPtr >::iterator 
						namedRuleIt = _namedRules.find( it->to_string() );
					if (namedRuleIt == _namedRules.end())
					{
						// Couldn't find the named rule
						WARNING_MSG( "Could not import rule %s.\n", it->to_string().c_str() );
						continue;
					}

					// Copy the import rule
					populateRule( ruleSection, namedRuleIt->second );
				}
			}
			// Copy the local rule
			populateRule( ruleSection, childSection );

			DataSectionPtr patternSection = childSection->findChild( HierarchicalConfig_Locals::TAG_PATTERN );
			if (patternSection != NULL)
			{
				// Copy the pattern associated with this rule.
				DataSectionPtr _patternSection = ruleSection->newSection( HierarchicalConfig_Locals::TAG_PATTERN );
				_patternSection->copy( patternSection );
				// Mark it as an attribute so it gets ignored by the populateRule function
				_patternSection->isAttribute( true );
			}
			else
			{
				// If the rule does not have a pattern then delete it from the root tree.
				pSection->delChild( ruleSection );
			}

			DataSectionPtr nameSection = childSection->findChild( HierarchicalConfig_Locals::TAG_NAME );
			if (nameSection != NULL)
			{
				// Insert the named rule into our map
				_namedRules[nameSection->asString()] = ruleSection;
			}

			if (patternSection == NULL &&
				nameSection == NULL)
			{
				WARNING_MSG( "Rule found in %s with no pattern or name.\n", path );
			}
		}
	}

	// Recursively process any sub directories
	for( IFileSystem::Directory::iterator it = dir.begin();
		it != dir.end(); ++it )
	{
		pathBuilder.clear();
		pathBuilder.append( formattedDirectory );
		pathBuilder.append( *it );
		const char * path = pathBuilder.string();

		IFileSystem::FileType ft = fs->getFileType( path );
		if (ft != IFileSystem::FT_DIRECTORY)
		{
			continue;
		}

		DataSectionPtr dirSection = pSection->newSection( HierarchicalConfig_Locals::TAG_DIRECTORY );
		DataSectionPtr pathSection = dirSection->newSection( HierarchicalConfig_Locals::TAG_PATH );
		pathSection->isAttribute( true );
		pathSection->setString( *it );
		populateChildren( dirSection, filename, path, _namedRules );
		if (dirSection->countChildren() <= 1)
		{
			// Delete the section created for this directory if
			// no config files were found recursively
			pSection->delChild( dirSection );
		}
	}
}

void HierarchicalConfig::populateConfiguration( DataSectionPtr pConfiguration, 
											    const DataSectionPtr pSection, 
												const StringRef & path )
{
	if (pSection->sectionName() == HierarchicalConfig_Locals::TAG_DIRECTORY)
	{
		DataSectionPtr pathSection = pSection->findChild( HierarchicalConfig_Locals::TAG_PATH );
		if (pathSection == NULL)
		{
			return;
		}

		BW::string directoryPath = pathSection->asString() + "/";
		if (path.length() < directoryPath.length() ||
			strncmp( directoryPath.c_str(), path.data(), directoryPath.length() ) != 0)
		{
			return;
		}

		StringRef subPath( path.data() + directoryPath.length(), path.length() - directoryPath.length());
		int numChildren = pSection->countChildren();
		for (int i = 0; i < numChildren; ++i)
		{
			DataSectionPtr childSection = pSection->openChild( i );
			if (childSection == NULL)
			{
				continue;
			}

			if (childSection->sectionName() == HierarchicalConfig_Locals::TAG_PATH)
			{
				continue;
			}

			populateConfiguration( pConfiguration, childSection, subPath );
		}
		return;
	}

	if (pSection->sectionName() != HierarchicalConfig_Locals::TAG_RULE)
	{
		return;
	}

	// We have a rule section. Make sure it has a pattern
	DataSectionPtr patternSection = pSection->findChild( HierarchicalConfig_Locals::TAG_PATTERN );
	if (patternSection == NULL)
	{
		WARNING_MSG( "Found rule with missing pattern.\n" );
		return;
	}
	
	BW::string pattern = patternSection->asString();

	// Ignore allocations inside re2 as it leaks a small number of statics
	BW::Allocator::allocTrackingIgnoreBegin();
	bool matched = RE2::FullMatch( 
		re2::StringPiece( path.data(), static_cast< int >( path.length() ) ),
		re2::StringPiece( pattern.c_str() ) );
	BW::Allocator::allocTrackingIgnoreEnd();

	if (!matched)
	{
		return;
	}
	
	populateRule( pConfiguration, pSection );
}
