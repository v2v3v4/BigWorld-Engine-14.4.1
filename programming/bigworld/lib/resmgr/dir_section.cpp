/**
 *	@file
 */

#include "pch.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#if defined(_WIN32)
#include "cstdmf/cstdmf_windows.hpp"
#else
#include <dirent.h>
#endif

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/string_utils.hpp"
#include "access_monitor.hpp"
#include "data_section_cache.hpp"
#include "data_section_census.hpp"

#include "dir_section.hpp"
#include "xml_section.hpp"
#include "bin_section.hpp"
#include "zip_section.hpp"

#include "cstdmf/profiler.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

void (*g_versionPointPass)( const BW::StringRef & path ) = NULL;
THREADLOCAL(bool) g_versionPointSafe( false );

/**
 *	Constructor for DirSection
 *
 *	@param fullPath		Full absolute of the directory.
 *	@param pFileSystem	An IFileSystem to populate the DirSection with.
 *
 *	@return 			None
 */
DirSection::DirSection(const BW::StringRef& fullPath, 
						IFileSystem* pFileSystem) :
	fullPath_( fullPath.data(), fullPath.size() ),
	tag_("root"),
	pFileSystem_(pFileSystem)
{
	BW_GUARD;

	if (g_versionPointPass && !g_versionPointSafe)
		(*g_versionPointPass)( fullPath );

	pFileSystem_->readDirectory( children_, fullPath_ );
}

/**
 *	Destructor
 */
DirSection::~DirSection()
{
}


/**
 *	This method returns the number of children of this node.
 *
 *	@return 	Number of children.
 */
int DirSection::countChildren()
{
	BW_GUARD;
	return static_cast<int>(children_.size());
}


/**
 *	This method returns the child with the given index
 *
 *	@param index	Index of the child to return.
 *
 *	@return 		Pointer to the specified child.
 */
DataSectionPtr DirSection::openChild( int index )
{
	BW_GUARD;
	MF_ASSERT(index < (int)children_.size());
	return this->findChild(children_[index]);
}

/**
 *	This method returns the name of the child with the given index
 */
BW::string DirSection::childSectionName( int index )
{
	BW_GUARD;
	MF_ASSERT(index < (int)children_.size());
	return children_[index];
}

/**
 *	This method creates a new child. The child will either be
 *	a dirsection, or an xml section, depending on whether the
 *	file has an extension. Creating of binary sections is not
 *	supported.
 *
 *	@param tag		Ignored
 *  @param creator  DataSectionCreator to use when creating the new DataSection.
 *
 *	@return		A DataSectionPtr to the new DataSection.
 */
DataSectionPtr DirSection::newSection( const BW::StringRef & tag,
										DataSectionCreator* creator )
{
	BW_GUARD;
	BW::string::size_type pos;
	DataSectionPtr pSection;
	BW::string fullPath;

	if (fullPath_.empty() || fullPath_[ fullPath_.size() - 1 ] == '/')
	{
		fullPath = fullPath_ + tag;
	}
	else
	{
		fullPath = fullPath_ + "/" + tag;
	}

	pos = tag.find_first_of( '.' );

	if (creator)
	{
		pSection = creator->create( this, tag );
		
		pSection->setParent( this );
		DataSectionCache::instance()->add( fullPath, pSection );
		children_.push_back( tag.to_string() );
		
	}
	else if (pos != BW::string::npos)
	{
		pSection = new XMLSection( tag );
		pSection->setParent( this );
		DataSectionCache::instance()->add( fullPath, pSection );
		children_.push_back( tag.to_string() );
	}
	else if (pFileSystem_->makeDirectory( fullPath ))
	{
		pSection = new DirSection( fullPath, pFileSystem_ );
		DataSectionCache::instance()->add( fullPath, pSection );
		children_.push_back( tag.to_string() );
	}

	return pSection;
}

/**
 *	This method locates a child by name. It first searches the cache,
 *	and returns a cached DataSection if there was one. Otherwise,
 *	it will create a new DirSection, XmlSection, or BinSection. It will
 *	return a NULL pointer if the DataSection could not be created.
 *
 *	@param tag		Name of the child to locate
 *  @param creator	The DataSectionCreator to use if a new DataSection
 *                  is required.
 *
 *	@return 		Pointer to DataSection representing the child.
 */
DataSectionPtr DirSection::findChild( const BW::StringRef &tag,
										DataSectionCreator* creator )
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( DirSection_findChild );

	char fullName[BW_MAX_PATH];
	bw_str_copy( fullName, ARRAY_SIZE( fullName ), fullPath_ );
	if (!fullPath_.empty() && *fullPath_.rbegin() != '/')
	{
		bw_str_append( fullName, ARRAY_SIZE( fullName ), "/" );
	}
	bw_str_append( fullName, ARRAY_SIZE( fullName ), tag );

	// If it's cached, return cached copy.
	// How do we handle new files in the filesystem?

	DataSectionPtr pSection = DataSectionCache::instance()->find( fullName );

	if(pSection)
		return pSection;

	// Also see if it's in the intrusive object list
	pSection = DataSectionCensus::find( fullName );
	if (pSection)
	{
		// add it back into the cache
		DataSectionCache::instance()->add( fullName, pSection );
		return pSection;
	}

	IFileSystem::FileType ft = pFileSystem_->getFileTypeEx( fullName );

	switch( ft )
	{
		case IFileSystem::FT_DIRECTORY:
		{
			{
				PROFILE_FILE_SCOPED( FT_DIRECTORY );
				DirSection* pDirSection = new DirSection( fullName,
					pFileSystem_);
				pDirSection->tag_.assign( tag.data(), tag.size() );

				pSection = pDirSection;
			}
			break;
		}

		case IFileSystem::FT_ARCHIVE:
		{
			{
				PROFILE_FILE_SCOPED( FT_ARCHIVE );
				// Disabling this creator because generally we don't want to load a zip as a binary...
				// using the creator would force this at the moment... needs evaluation/fixing

				// ZipSection load via creator is not currently implemented:
				if (creator && creator != ZipSection::creator())
				{
					pSection = creator->load( this, tag,
						pFileSystem_->readFile( fullName ));
				}
				else
				{
					// ZipSections avoid loading the entire file.
					ZipSection* pZipSection = new ZipSection( fullName,
						pFileSystem_);
					pZipSection->tag(tag);

					pSection = pZipSection;
					if (pSection)
						pSection->setParent( this );
					break;
				}
			break;
			}
		}

		case IFileSystem::FT_FILE:
		{
			{
				PROFILE_FILE_SCOPED( FT_FILE );
				BinaryPtr pBinary = pFileSystem_->readFile( fullName );

				// Do a dump of the file if it is a file.
				AccessMonitor::instance().record( fullName );

				if (pBinary)
				{
#ifdef EDITOR_ENABLED
					static bool firstTime = true;
					static bool checkRootTag;
					if( firstTime )
					{
						checkRootTag = wcsstr( GetCommandLine(), L"-checkroottag" ) ||
							wcsstr( GetCommandLine(), L"/checkroottag" );
						firstTime = false;
					}
					if( checkRootTag )
					{
						// TODO:UNICODE: Make sure this still works with unicode (utf8) 
						// strings as expected.

						// The following chunk of code is being used to check if the root is conforming to our standard
						bool isXML = pBinary->len() > 0 && *pBinary->cdata() == '<';
						BW::string s;
						static const BW::string::size_type MAX_TAG_NAME_SIZE = 128;
						int offset = 1;
						while( offset < pBinary->len() && offset < MAX_TAG_NAME_SIZE && pBinary->cdata()[ offset ] != '>' )
						{
							s += pBinary->cdata()[ offset ];
							++offset;
						}
						if( offset < pBinary->len() && pBinary->cdata()[ offset ] == '>' &&
							_stricmp( s.c_str(), "root" ) != 0 )
						{
							ERROR_MSG( "DirSection::findChild: file %s uses an incorrect root tag : <%s>\n",
								fullName, s.c_str() );
						}
					}
#endif//EDITOR_ENABLED
					pSection =
						DataSection::createAppropriateSection( tag,
						pBinary, true, creator );
					if (pSection)
						pSection->setParent( this );
				}
			}
			break;
		}

		case IFileSystem::FT_NOT_FOUND:
			// pSection is NULL.
			break;
	}

	// If we have a valid section, add it to the cache.

	if(pSection)
	{
		PROFILE_FILE_SCOPED( add );
		pSection = DataSectionCensus::add( fullName, pSection );
		DataSectionCache::instance()->add(fullName, pSection);
	}

	return pSection;
}


/**
 * 	This method is not implemented for DirSection.
 *
 *	@param tag		Ignored
 *
 *	@return 		NULL
 */
void DirSection::delChild (const BW::StringRef & tag )
{
}

/**
 * 	This method is not implemented for DirSection.
 *
 *	@param pSection		Ignored
 *
 *	@return 			NULL
 */
void DirSection::delChild( DataSectionPtr pSection )
{
}

/**
 * 	This method is not implemented for DirSection.
 */
void DirSection::delChildren()
{
//	MF_ASSERT(!"DirSection::delChildren() not implemented");
}


/**
 * 	This method returns the name of the section
 *
 *	@return 		The section name
 */
BW::string DirSection::sectionName() const
{
	BW_GUARD;
	return tag_;
}


/**
 * 	This method returns the number of bytes used by this section.
 *
 *	@return 		Number of bytes used by this DirSection.
 */
int DirSection::bytes() const
{
	// Close enough for now.
	return 1024;
}


/**
 * 	This method saves the given section to a file.
 */
bool DirSection::save(const BW::StringRef& filename)
{
	BW_GUARD;
	if(filename != "")
	{
		WARNING_MSG("DirSection: Saving to filename not supported.\n");
		return false;
	}

	for(int i = 0; i < this->countChildren(); i++)
	{
		DataSectionPtr pSection = this->openChild(i);
		if(!pSection->save())
			return false;
	}

	return true;
}

/**
 *	This method saves the given data section which is a child of this section.
 */
bool DirSection::saveChild( DataSectionPtr pChild, bool isBinary )
{
	BW_GUARD;
	BW::string fullName;

	bool hasSlash = (fullPath_.empty() || fullPath_[fullPath_.size() - 1] == '/');
	if (hasSlash)
	{
		fullName = fullPath_ + pChild->sectionName();
	}
	else
	{
		fullName = fullPath_ + "/" + pChild->sectionName();
	}

	return pFileSystem_->writeFile( fullName, pChild->asBinary(), isBinary );
}

/**
 *	This method saves the given data section which is a child of this section.
 */
bool DirSection::saveChildIfExists( DataSectionPtr pChild, bool isBinary )
{
	BW_GUARD;
	BW::string fullName;

	bool hasSlash = (fullPath_.empty() || fullPath_[fullPath_.size() - 1] == '/');
	if (hasSlash)
	{
		fullName = fullPath_ + pChild->sectionName();
	}
	else
	{
		fullName = fullPath_ + "/" + pChild->sectionName();
	}

	return pFileSystem_->writeFileIfExists( fullName, pChild->asBinary(), isBinary );
}

BW_END_NAMESPACE

// dir_section.cpp
