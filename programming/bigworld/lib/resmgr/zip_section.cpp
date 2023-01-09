/**
 *	@file
 */

#include "pch.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef _WIN32
#include "cstdmf/cstdmf_windows.hpp"
#else
#include <dirent.h>
#endif

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

#include "access_monitor.hpp"
#include "data_section_cache.hpp"
#include "data_section_census.hpp"

#include "bwresource.hpp"
#include "multi_file_system.hpp"

#include "zip_section.hpp"
#include "xml_section.hpp"
#include "bin_section.hpp"

#include "multi_file_system.hpp"
#include "zip_file_system.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

namespace { // anonymous
//
SimpleMutex s_zipSectionIntrospectLock;
//
} //  namespace anonymous


// -----------------------------------------------------------------------------
// Section: Documentation
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Section: ZipSection
// -----------------------------------------------------------------------------

/**
 *	Constructor for outmost ZipSection
 */
ZipSection::ZipSection( const BW::StringRef& fullPath, FileSystemPtr parentSystem,
					   BW::vector<DataSectionPtr>* pChildren ) :
	tag_( "" ),
	introspected_( false )
{
	PROFILE_FILE_SCOPED( ZipSection1 );
	if (pChildren) // newly created / converted section
	{
		pFileSystem_ = new ZipFileSystem( fullPath, parentSystem, true );
		BW::vector<DataSectionPtr>::iterator it = pChildren->begin();
		while(it!=pChildren->end())
		{
			if (*it)
				(*it)->setParent( NULL );
			it++;
		}
		introspected_ = true; //children are already established.
		children_.insert(children_.end(), pChildren->begin(), pChildren->end());

		for (uint i=0; i<children_.size();i++)
		{
			DataSectionPtr pChild = children_[i];
			if (pChild && pChild->countChildren())
			{
				pChild->setParent( this );
				children_[i] = pChild->convertToZip("",this);
			}
		}
	}
	else // being loaded off disk
	{
		pFileSystem_ = new ZipFileSystem( fullPath, parentSystem );// outmost node
		introspected_ = false; // children aren't loaded.

		children_.resize(pFileSystem_->fileCount( pFileSystem_->tag() ), NULL);
	}
}

/**
 *	Constructor for nested zipSection
 */
ZipSection::ZipSection( ZipFileSystemPtr parentZip,
					   const BW::StringRef& tag,
					   BW::vector<DataSectionPtr>* pChildren ) :
	tag_( tag.data(), tag.size() ),
	introspected_( false )
{
	PROFILE_FILE_SCOPED( ZipSection2 );
	if (pChildren) // newly created / converted section
	{
		pFileSystem_ = new ZipFileSystem( parentZip, tag, true );
		BW::vector<DataSectionPtr>::iterator it = pChildren->begin();
		while(it!=pChildren->end())
		{
			if (*it)
				(*it)->setParent( NULL );
			it++;
		}
		introspected_ = true; //children are already established.
		children_.insert(children_.end(), pChildren->begin(), pChildren->end());

		for (uint i=0; i<children_.size();i++)
		{
			DataSectionPtr pChild = children_[i];
			if (pChild && pChild->countChildren())
			{
				pChild->setParent( this );
				children_[i] = pChild->convertToZip("",this);
			}
		}
	}
	else // being loaded off disk
	{
		pFileSystem_ = new ZipFileSystem( parentZip, tag);
		children_.resize(pFileSystem_->fileCount( pFileSystem_->tag() ), NULL);// pFileSystem_->tag() can be different from tag_
	}
}

/**
 *	Destructor
 */
ZipSection::~ZipSection()
{
	pFileSystem_ =  NULL;

	children_.clear();
}


BW::string ZipSection::absolutePath() 
{ 
	return pFileSystem_->absolutePath();
}


/**
 *	This method returns the number of children of this section-
 *
 *	@return 	Number of children.
 */
int ZipSection::countChildren()
{
	return static_cast<int>(children_.size());
}


/**
 *	Open the child with the given index
 *
 *	@param index	Index of the child to return.
 *
 *	@return 		Pointer to the specified child.
 */
DataSectionPtr ZipSection::openChild( int index )
{
	MF_ASSERT_DEBUG(pFileSystem_);
	if (introspected_)
	{
		if (uint32(index) < children_.size()) return children_[ index ];
	}
	else
	{
		if (children_[index] != NULL)
			return children_[index];

		DataSectionPtr pSection = NULL;
		BW::string name = pFileSystem_->fileName( pFileSystem_->tag(), index );
		if (pFileSystem_->dirTest( name ))// a folder nested in the parent zip file
		{
			pSection = new ZipSection( pFileSystem_, name );
		}
		else
		{
			BinaryPtr pBinary = pFileSystem_->readFile( pFileSystem_->tag(), index );
			if (pBinary && ZipFileSystem::zipTest(pBinary)) // a zip file nested in the parent zip file
			{
				pSection = new ZipSection( pFileSystem_, name );
			}
			else if (pBinary) // a non-zip file nested in the parent zip file
			{
				pSection = DataSection::createAppropriateSection( name, pBinary );
			}
		}

		// add it to our list of children
		if (pSection)
		{
			children_[index] = pSection;
		}
		return pSection;
	}
	return NULL;
}

/**
 *	Retrieve the a child section name.
 *
 *	@param index	Index of the child name to return.
 *
 *	@return 		Child name.
 */
BW::string ZipSection::childSectionName( int index )
{
	if (introspected_)
	{
		MF_ASSERT(index < (int)children_.size());
		MF_ASSERT_DEBUG(children_[index]);
		return children_[index]->sectionName();
	}
	else
	{
		MF_ASSERT_DEBUG( index < (int)pFileSystem_->fileCount( pFileSystem_->tag() ) );
		return pFileSystem_->fileName( pFileSystem_->tag(), index );
	}
}


class ZipCreator : public DataSectionCreator
{
public:
	virtual DataSectionPtr create( DataSectionPtr pParentSection,
									const BW::StringRef& tag )
	{
		PROFILER_SCOPED( ZipSectionCreator_create );
		DataSectionPtr newSection;
		if (pParentSection)
		{
			BW::vector<DataSectionPtr> children;
			newSection = pParentSection->convertToZip( tag, NULL, &children );
		}
		else
			newSection = new ZipSection(tag, 
				(IFileSystem*)BWResource::instance().fileSystem(),
				NULL);
		return newSection;
	}

	virtual DataSectionPtr load( DataSectionPtr pParentSection,
									const BW::StringRef& tag,
									BinaryPtr pBinary = NULL)
	{
		PROFILER_SCOPED( ZipSectionCreator_load );
		if (pBinary)
		{
			// Check the file is not a zip file.
			if (!ZipFileSystem::zipTest(pBinary))
			{
				DataSectionPtr pSection =
						DataSection::createAppropriateSection( tag,
						pBinary, false );
				if (pSection && pParentSection)
				{
					pSection->setParent( pParentSection );
					pSection = pSection->convertToZip();
				}
				return pSection;
			}
			else
				MF_ASSERT(!"ZipCreator::load: this should not be called: error in zip creation\n");
		}
		else
		{
			return this->create(pParentSection, tag);
		}
		return NULL;
	}
};

DataSectionCreator* ZipSection::creator()
{
	static ZipCreator s_creator;
	return &s_creator;
}


/**
 *	Create a new section with the given tag
 *
 *	@param tag		Name of tag of new child.
 *  @param creator	The DataSection creator to use when creating a section.
 *
 *	@return 		The newly created section.
 */
DataSectionPtr ZipSection::newSection( const BW::StringRef & tag,
										DataSectionCreator* creator )
{
	if (!introspected_) this->introspect();

	DataSectionPtr pChild;
	if (creator)
	{
		pChild = creator->create(this, tag);
	}
	else
		pChild = new BinSection( tag, new BinaryBlock( NULL, 0, "BinaryBlock/BinSection" ) );
		//TODO: default to bin or zip??? zip probably

	children_.push_back( pChild );
	return pChild;
}


/**
 *	This method locates a child by name. It first searches the cache,
 *	and returns a cached DataSection if there was one. Otherwise,
 *	it will create a new ZipSection, XmlSection, or BinSection. It will
 *	return a NULL pointer if the DataSection could not be created.
 *
 *	@param tag		Name of the child to locate
 *  @param creator	DataSectionCreator used to create the new section
 *                  if necessary.
 *
 *	@return 		Pointer to DataSection representing the child.
 */
DataSectionPtr ZipSection::findChild( const BW::StringRef &tag, 
									DataSectionCreator* creator )
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( zipsection_findChild );
	MF_ASSERT_DEBUG(pFileSystem_);
	BW::string fullName;
	DataSectionPtr pSection;

	BW::string path = this->absolutePath();
	bool hasSlash = (path.empty() || path[path.size() - 1] == '/');
	if (hasSlash)
	{
		fullName = path + tag;
	}
	else
	{
		fullName = path + "/" + tag;
	}
#ifdef EDITOR_ENABLED
	const BW::string & fsFullName = fullName;
#endif

	// Search the loaded children first. 
	// (would probably already have been found in the cache above)
	for (uint i = 0; i < children_.size(); i++)
	{
		if (children_[i])
		{
			BW::string childTag = children_[i]->sectionName();
			if ( childTag == (tag) )
				return children_[i];
		}
	}

	if (introspected_) // children_ are in authority
		return NULL;

	pSection = DataSectionCache::instance()->find(fullName);

	if(pSection)
	{
		int index = pFileSystem_->fileIndex( tag );
		if ( index < 0 )
		{
			return NULL;
		}
		MF_ASSERT( index < (int)children_.size() );
		children_[index] = pSection;
		return pSection;
	}

	// Also see if it's in the intrusive object list
	pSection = DataSectionCensus::find( fullName );

	if (pSection)
	{
		// add it back into the cache
		DataSectionCache::instance()->add(fullName, pSection);
		int index = pFileSystem_->fileIndex( tag );
		if ( index < 0 )
		{
			return NULL;
		}
		MF_ASSERT( index < (int)children_.size() );
		children_[index] = pSection;
		return pSection;
	}

	switch(pFileSystem_->getFileTypeEx(tag))
	{
		case IFileSystem::FT_DIRECTORY:
		case IFileSystem::FT_ARCHIVE:
		{
			PROFILE_FILE_SCOPED( zipArchive );
			ZipSection* pZipSection = new ZipSection( pFileSystem_, tag );
			pSection = pZipSection;
			break;
		}


		case IFileSystem::FT_FILE:
		{
			PROFILE_FILE_SCOPED( zipFile );
			BinaryPtr pBinary = pFileSystem_->readFile(tag);

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
						bw_stricmp( s.c_str(), "root" ) != 0 )
					{
						ERROR_MSG( "DirSection::findChild: file %s uses an incorrect root tag : <%s>\n",
							fsFullName.c_str(), s.c_str() );
					}
				}
#endif//EDITOR_ENABLED
				pSection =
					DataSection::createAppropriateSection( tag, pBinary, true, creator );
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
		pSection = DataSectionCensus::add( fullName, pSection );
		DataSectionCache::instance()->add( fullName, pSection );

		int index = pFileSystem_->fileIndex( tag );
		MF_ASSERT( index >= 0 );
		MF_ASSERT( index < (int)children_.size() );
		children_[index] = pSection;
	}

	return pSection;
}


/**
 * 	Erase the given child.
 *
 *	@param tag		tag to delete
 *
 */
void ZipSection::delChild (const BW::StringRef & tag )
{
	if (!introspected_) this->introspect();	

	for (uint i = 0; i < children_.size(); i++)
	{
		if ( children_[i] && children_[i]->sectionName() == tag)
		{
			children_.erase( children_.begin() + i );
			return;
		}
	}
}

/**
 * 	Erase the given child.
 *
 *	@param pSection		pSection to delete
 *
 *	@return 			NULL
 */
void ZipSection::delChild( DataSectionPtr pSection )
{
	BW_GUARD_PROFILER( ZipSection_delChild );
	if (!introspected_) this->introspect();

	Children::iterator found = std::find(
		children_.begin(), children_.end(), pSection );
	if (found != children_.end())		
	{		
		children_.erase( found );
	}
}

/**
 * 	Erase every child
 */
void ZipSection::delChildren()
{
	MF_ASSERT_DEBUG(pFileSystem_);
	introspected_ = true;
	children_.clear();
	pFileSystem_->clear( pFileSystem_->tag() );
}


/**
 *	This method returns the binary data owned by this section.
 *
 *	@return			Smart pointer to binary data object
 */
BinaryPtr ZipSection::asBinary()
{
	if (!introspected_) this->introspect();


	// bundle up our children and children's children, etc.
	if (introspected_&&children_.size())
	{	
		this->recollect();
	}

	BinaryPtr pBinary = NULL;
	if (pFileSystem_)
		pBinary = pFileSystem_->createZip();

	pFileSystem_->clear();
	return pBinary;
}


/**
 * 	This method returns the name of the section
 *
 *	@return 		The section name
 */
BW::string ZipSection::sectionName() const
{
	return tag_;
}


/**
 * 	This method returns the number of bytes used by this section.
 *	Note that a ZipSection does not store the data, it is similar to a 
 *	DirSection because it just manages its children.
 *
 *	@return 		Number of bytes used by this ZipSection.
 */
int ZipSection::bytes() const
{
	//This 'magic' value is used for consistency with the DirSection
	// version of this function. Previously this function was sometimes
	// returning zero which would cause problems with the DataSectionCache.
	// Having a value like this allow this section to be added to the cache
	// but doesn't indicate the amount of data stored in it's children.
	return 1024;
}

/**
 *	This method sets the parent used to save data
 */
void ZipSection::setParent( DataSectionPtr pParent )
{
	parent_ = pParent;
}

/**
 * 	Save this data section and all it's children.
 *	Zip sections don't directly save to disk, they generate
 *	a binary block representing the file and then rely
 *	on the parent section to save to the file system.
 *
 *	@return 		true if successful.
 */
bool ZipSection::save(const BW::StringRef& saveAsFileName)
{
	// change our allegiance if we're doing a save as
	if (!saveAsFileName.empty())
	{
		BW::StringRef tag;
		if (!DataSection::splitSaveAsFileName( saveAsFileName, parent_, tag ))
			return false;
		tag_.assign( tag.data(), tag.size() );
	}
	if (!parent_)
	{
		ERROR_MSG( "ZipSection: Can't save a section without a parent.\n" );
		return false;
	}

	return parent_->saveChild( this, true );
}


/**
 *	This method saves the given data section which is a child of this section.
 */
bool ZipSection::saveChild( DataSectionPtr pChild, bool isBinary )
{
	if (introspected_ && pChild)
	{
		BinaryPtr childBinary = pChild->asBinary();

		//commented out the following because it's not needed
		// when treating nested zips as ZipSections
		//if (childBinary == NULL && (pChild->countChildren() >0) ) 
		//// or empty as well???
		//{
		//	BW::vector<DataSectionPtr> childList;
		//	for (int i=0; i<pChild->countChildren(); i++)
		//	{
		//		childList.push_back( pChild->openChild(i) );
		//	}
		//	pFileSystem_->clear( absolutePath() + "/" + pChild->sectionName() );
		//	for (int i=0; i<(int)childList.size(); i++)
		//	{
		//		DataSectionPtr child = childList[i];
		//		if (child)
		//			pChild->saveChild(child, isBinary );
		//	}
		//	return true;
		//}
		//else
		{
			return pFileSystem_->writeToStructure( pChild->sectionName(), childBinary );
		}
	}
	else
		return false;
}


/**
 *	This method loads all the files in the zip.
 *  
 *
 */
void ZipSection::introspect()
{
	SimpleMutexHolder sh(s_zipSectionIntrospectLock);
	if (introspected_)
		return;
	introspected_ = true;

	for (uint i=0; i<(uint)pFileSystem_->fileCount( pFileSystem_->tag() ); i++)
	{
		BW::string tag = pFileSystem_->fileName( pFileSystem_->tag(), i );
		DataSectionPtr pSection=NULL;

		if (children_[i] != NULL) //already loaded
			continue;

		if (pFileSystem_->getFileType(tag) == IFileSystem::FT_DIRECTORY)
		{
			ZipSection* pDirSection = new ZipSection( pFileSystem_, tag );
			pSection = pDirSection;
		}
		else
		{
			BinaryPtr pBinary = pFileSystem_->readFile(pFileSystem_->tag(), i);
			if (pBinary)
			{
				// Check for zip sections
				if (pFileSystem_->zipTest(pBinary))
					pSection = new ZipSection( pFileSystem_, tag );
				else
					pSection = DataSection::createAppropriateSection( tag, pBinary );

			}
		}
		
		// add it to our list of children
		if (pSection)
			children_[i] = pSection;
	}
	pFileSystem_->clear(pFileSystem_->tag());
}


/**
 *	This method collects up all the children of this section into the
 *	zip file system.
 */
BinaryPtr ZipSection::recollect()
{
	pFileSystem_->clear(pFileSystem_->tag());

	// update / add all the kids
	for ( uint i=0; i<children_.size(); i++ )
	{
		if (children_[i])
			saveChild( children_[i], true );
	}
	return NULL;
}


/**
 * 	This method converts the passed section data into a zip section.
 *	The conversion is done here to allow access to the file system
 *	(only in DirSection). A child will pass it's data to the parent
 *	and ask to be converted.
 *
 *	@return 		The converted data section.
 */
DataSectionPtr DirSection::convertToZip( const BW::StringRef& saveAs,
									 DataSectionPtr pRoot, 
									 BW::vector<DataSectionPtr>* children )
{
	if (children && pRoot)
	{
		BW::string fullName;
		bool hasSlash = (fullPath_.empty() || fullPath_[fullPath_.size() - 1] == '/');
		if (hasSlash)
		{
			fullName = fullPath_ + pRoot->sectionName();
		}
		else
		{
			fullName = fullPath_ + "/" + pRoot->sectionName();
		}
		ZipSectionPtr newSection = new ZipSection(fullName, pFileSystem_, children);
		DataSectionPtr pSection=NULL;
		if (newSection)
		{
			newSection->tag(pRoot->sectionName());
			newSection->setParent(this);

			pSection = DataSectionCensus::add( fullName, newSection );
			DataSectionCache::instance()->add( fullName, pSection );		

		}
		return pSection;
	}
	else 
	{
		DataSectionPtr pSection = NULL;

		// pRoot is set to be the parent zip file when converting a nested DirSection.
		if (pRoot)
		{
			DataSectionCensus::del(this);
			DataSectionCache::instance()->remove(this->fullPath_);

			BW::vector<DataSectionPtr> childData;
			childData.resize(children_.size());

			// Build a list of children for the new zip.
			for (uint i=0; i<children_.size(); i++)
			{
				childData[i] = this->openChild(i);
			}

			// Need to remove all the children from the parent zip.
			for (uint i=0; i<children_.size(); i++)
			{
				if (childData[i])
					pRoot->delChild(tag_ + "/" + childData[i]->sectionName());

			}
			return pRoot->convertToZip(saveAs, this, &childData);
		}
		else if (saveAs != "")
		{
			//TODO: need to use saveAs extractor?			
			BW::StringRef tag;
			tag = saveAs;

			BW::string fullName;
			bool hasSlash = (fullPath_.empty() || fullPath_[fullPath_.size() - 1] == '/');
			if (hasSlash)
				fullName = fullPath_ + tag;
			else
				fullName = fullPath_ + "/" + tag;
			ZipSectionPtr newSection = new ZipSection( fullName, pFileSystem_, children );
			DataSectionPtr pSection=NULL;
			if (newSection)
			{
				newSection->tag(tag);
				newSection->setParent(this);

				pSection = DataSectionCensus::add( fullName, newSection );
				DataSectionCache::instance()->add( fullName, pSection );		

			}
			return pSection;

		}
		else
		{
			ERROR_MSG( "DirSection: Can't convert to zip without a root specified.\n" );		
			return this; //return NULL???
		}
	}
}

/**
 * 	This method converts the passed data into a zip section.
 *	This is similar to the convertToZip method in DirSection but
 *	handles nested zip file conversion.
 *
 *	@return 		The converted data section.
 */
DataSectionPtr ZipSection::convertToZip( const BW::StringRef& saveAs,
										DataSectionPtr pRoot,
										BW::vector<DataSectionPtr>* children )
{
	BW::string tag;
	if (pRoot)
		tag = pRoot->sectionName();
	else
		tag.assign( saveAs.data(), saveAs.size() );
	if (children)
	{
		BW::string fullName;
		BW::string path = this->absolutePath();
		bool hasSlash = (path.empty() || path[path.size() - 1] == '/');
		if (hasSlash)
			fullName = path + tag;
		else
			fullName = path + "/" + tag;
		ZipSectionPtr newSection = new ZipSection( pFileSystem_, tag, children );
		DataSectionPtr pSection = NULL;
		if (newSection)
		{
			pSection = DataSectionCensus::add( fullName, newSection );
			DataSectionCache::instance()->add( fullName, pSection );
		}

		for (uint i = 0; i < children_.size(); i++)
		{
			if (children_[i])
			{
				BW::string childTag = children_[i]->sectionName();
				if ( childTag == (tag) )
				{
					children_[i] = pSection;
					break;
				}
			}
		}
		return pSection;
	}
	else
		return this;
}


/**
 * 	This method passes it's data for conversion to the parent section.
 *
 *	@return 		The converted data section.
 */
DataSectionPtr BinSection::convertToZip( const BW::StringRef& saveAs, DataSectionPtr pRoot, BW::vector<DataSectionPtr>* children )
{
	if (!introspected_) this->introspect();

	if (!saveAs.empty())
	{
		BW::StringRef tag;
		if (!DataSection::splitSaveAsFileName( saveAs, parent_, tag ))
			return NULL;
		tag_.assign( tag.data(), tag.size() );
	}

	DataSectionPtr pSection = NULL;
	if (parent_)
	{
		DataSectionCensus::del(this);
		//DataSectionCache::instance()->remove(this->fullPath_);
		//how to remove from cache? clear it?
		pSection = parent_->convertToZip( saveAs, this, &children_ );
	}
	else
	{
		// TODO: perhaps default to the root section as the parent?
		ERROR_MSG( "BinSection: Can't convert to zip without a parent.\n" );		
	}
	return pSection;
}

BW_END_NAMESPACE

// zip_section.cpp
