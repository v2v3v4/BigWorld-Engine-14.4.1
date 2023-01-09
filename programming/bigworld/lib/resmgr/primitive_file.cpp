#include "pch.hpp"
#include "primitive_file.hpp"

#include "binary_block.hpp"
#include "bin_section.hpp"
#include "bwresource.hpp"

#include "cstdmf/debug.hpp"
#include <algorithm>

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "primitive_file.ipp"
#endif


// -----------------------------------------------------------------------------
// Section: PrimitiveFile statics
// -----------------------------------------------------------------------------


#if 0	// ifdef'd out since functionality moved to BinSection
uint32 g_primFileSize = 0;
typedef BW::map< BW::string, PrimitiveFile * > PrimitiveFileMap;
static PrimitiveFileMap * s_pfm = NULL;
#endif

/**
 *	The static primitive file get method.
 *
 *	This method always succeeds - an empty primitive file is created
 *	(but not saved) if the resource is not found.
 */
DataSectionPtr PrimitiveFile::get( const BW::string & resourceID )
{
	static DataSectionPtr s_miniCache;
#if 0	// ifdef'd out since functionality moved to BinSection
	// if there's already one of these objects still in memory,
	// return that, so that no changes can get lost or overwritten
	if (s_pfm != NULL)
	{
		PrimitiveFileMap::iterator it = s_pfm->find( resourceID );
		if (it != s_pfm->end()) return it->second;
	}

	PrimitiveFile * pNew = new PrimitiveFile(
		BWResource::instance().rootSection()->readBinary( resourceID ) );
	PrimitiveFile::add( resourceID, pNew );
#else
	DataSectionPtr pNew =
		BWResource::instance().openSection( resourceID + ".processed" );

	if (!pNew)
	{
		pNew = BWResource::instance().openSection( resourceID );
	}
#endif
	s_miniCache = pNew;
	return pNew;
}

#if 0	// ifdef'd out since functionality moved to BinSection
/**
 *	The static primitive file add method (inserts into cache)
 */
void PrimitiveFile::add( const BW::string & id,
	PrimitiveFile * pPrimitiveFile )
{
	if (s_pfm == NULL) s_pfm = new PrimitiveFileMap();

	s_pfm->insert( std::make_pair( id, pPrimitiveFile ) );
}

/**
 *	The static primitive file del method (removes from cache)
 */
void PrimitiveFile::del( PrimitiveFile * pPrimitiveFile )
{
	if (s_pfm != NULL)
	{
		PrimitiveFileMap::iterator i = s_pfm->begin();
		while (i != s_pfm->end())
		{
			if (i->second == pPrimitiveFile)
			{
				s_pfm->erase( i );
                break;
			}
			else
			{
				i++;
			}
		}

		if (s_pfm->empty())
		{
			bw_safe_delete(s_pfm);
		}
	}
}


// -----------------------------------------------------------------------------
// Section: PrimitiveFile members
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PrimitiveFile::PrimitiveFile( BinaryPtr pFile ) :
	file_( pFile )
{
	if (!pFile) return;

	char * data = (char*)pFile->data();
	int len = pFile->len();

	g_primFileSize += len;

	int entryDataOffset = 0;

	// read the directory out of the file
	int indexLen = *(int*)(data + len - sizeof(int));
	int offset = len - sizeof(int) - indexLen;
	while (offset < len - (int)sizeof(int))
	{
		// read this directory entry
		int entryDataLen = *(int*)(data+offset);
		offset += sizeof(int);
		int entryNameLen = *(int*)(data+offset);
		offset += sizeof(int);
		BW::string entryStr( data+offset, entryNameLen );
		offset += entryNameLen;

		// insert it into our map
		dir_.insert( std::make_pair( entryStr, new BinaryBlock(
			data+entryDataOffset, entryDataLen, pFile ) ) );

		// move on the data offset
		entryDataOffset += (entryDataLen + 3) & (~3L);
	}
}


/**
 *	Destructor.
 */
PrimitiveFile::~PrimitiveFile()
{
	PrimitiveFile::del( this );

	if (file_) g_primFileSize -= file_->len();
}

/**
 *	Read the binary section with the given name
 *	(just looks it up in the directory)
 */
BinaryPtr PrimitiveFile::readBinary( const BW::string & name )
{
	Directory::iterator it = dir_.find( name );
	if (it != dir_.end())
	{
		return it->second;
	}

	return NULL;
}

/**
 *	Adds or replaces a binary section of this length in the directory
 */
BinaryPtr PrimitiveFile::updateBinary( const BW::string & name,
	void * data, int len )
{
	BinaryPtr ret = new BinaryBlock( data, len );

	dir_[ name ] = ret;	// note: OK to replace an existing binaryptr

	return ret;
}


/**
 *	Adds or replaces a binary section of this length in the directory
 */
void PrimitiveFile::updateBinary( const BW::string & name,
	BinaryPtr bp )
{
	dir_[ name ] = bp;
}


/**
 *	Saves out this primitive file
 */
void PrimitiveFile::save( const BW::string & resourceID )
{
	BW::string fileName = BWResource::resolveFilename( resourceID );

	FILE * pFile = bw_fopen( fileName.c_str(), "wb" );
	if (!pFile)
	{
		WARNING_MSG( "Could not save %s as the file "
			"could not be opened for writing\n", resourceID.c_str() );
		return;
	}

	char align[4] = {0,0,0,0};

	// first write out the (4-byte aligned) data
	Directory::iterator end = dir_.end();
	for (Directory::iterator it = dir_.begin(); it != end; it++)
	{
		if (it->second)
		{
			int len = it->second->len();
			fwrite( it->second->data(), len, 1, pFile );
			if ((len & 3) != 0)
			{
				fwrite( align, 4 - (len&3), 1, pFile );
			}
		}
	}

	int dirDataLen = 0;

	// now write out the directory
	for (Directory::iterator it = dir_.begin(); it != end; it++)
	{
		if (it->second)
		{
			int lens[2] = { it->second->len(), it->first.length() };
			fwrite( lens, sizeof(lens), 1, pFile );
			dirDataLen += sizeof(lens);
			fwrite( it->first.data(), lens[1], 1, pFile );
			dirDataLen += lens[1];
		}
	}

	// and finally the directory length
	fwrite( &dirDataLen, sizeof(int), 1, pFile );

	fclose( pFile );
}
#endif // ifdef'd out since functionality moved to BinSection


// -----------------------------------------------------------------------------
// Section: Utility
// -----------------------------------------------------------------------------

/**
 *	Split an old-style primitive name into its real file (resource) name
 *	and the part name inside that resource.
 */
void splitOldPrimitiveName( const BW::string & resourceID,
	BW::string & file, BW::string & part)
{
	int slashPos = static_cast<int>(resourceID.size()-1);
	int dotPos = slashPos;
	int dotPre = dotPos;
	while (slashPos >= 0)
	{
		char c = resourceID[slashPos];
		if (c == '.') { dotPre = dotPos; dotPos = slashPos; }
		if (c == '/' || c == '\\') break;
		slashPos--;
	}

	// don't take off a .static
	if (dotPre - dotPos == 7 && resourceID.compare(
		dotPos, dotPre-dotPos, ".static" ) == 0) dotPos = dotPre;

	file = resourceID.substr( 0, dotPos );
	part = resourceID.substr( dotPos+1 );
}


/**
 *	Get the given primitive part from the file, but if it's not there
 *	then look under the old name and put that in there.
 */
BinaryPtr fetchOldPrimitivePart( BW::string & file, BW::string & part )
{
	BW::string id = file + ".primitives";

	DataSectionPtr pfp = PrimitiveFile::get( id );
	if (!pfp) pfp = new BinSection( "temp", new BinaryBlock( NULL, 0, "BinaryBlock/PrimitiveFile" ) );
	BinaryPtr binary = pfp->readBinary( part );
	if (!binary)
	{
		binary = BWResource::instance().rootSection()->readBinary(
			file + "." + part );
		if (binary)
		{
			pfp->newSection( part )->setBinary( binary );
			//pfp->updateBinary( part, binary );
			pfp->save( id );	// this changes 'temp' tag above
		}
	}

	return binary;
}

BW_END_NAMESPACE

// primitive_file.cpp
