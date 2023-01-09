#include "pch.hpp"

#include <stdio.h>

#include "cstdmf/concurrency.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"

#include "bin_section.hpp"
#include "file_system.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

BW_BEGIN_NAMESPACE

namespace { // anonymous

const uint32 BIN_SECTION_MAGIC = 0x42a14e65;
SimpleMutex s_binSectionIntrospectLock;

} //  namespace anonymous


// -----------------------------------------------------------------------------
// Section: Documentation
// -----------------------------------------------------------------------------

/*
 *	This is a description of the BinSection file format.
 *
 *	A BinSection can be interpreted as a binary blob of data. It can also be
 *	viewed as containing other data sections.
 *
 *	Pseudo BNF format
 *	<BinSection> ::= <MagicNumber><ChildSectionData>*<IndexTable>
 *	<MagicNumber> ::= 0x42a14e65 (4 bytes - little endian)
 *	<ChildSectionData> ::= BinaryBlob padded to 4 byte boundary
 *	<IndexTable> ::= <DataSectionEntry>*<IndexTableLength>
 *	<DataSectionEntry> ::= <BlobLength><ReservedData><String>
 *	<BlobLength> ::= 4 byte little endian integer for length of section's data
 *	<ReservedData> ::= 16 bytes of reserved data (set this to 0).
 *	<String> ::= <StringLength><CharData>
 *	<StringLength> ::= 4 byte little endian integer
 *	<CharData> ::= Sequence of characters padded to 4 byte boundary
 *
 *	The file starts with a 4 byte magic number:
 *		0x42a14e65. (65 4e a1 42 big-endian).
 *
 *	Then the data for each child section is concatenated in order. Each child
 *	data block is padded with up to 3 bytes to make sure it ends on a 4 byte
 *	boundary.
 *
 *	At the end of the block (and "file") is a section that has the keys are maps
 *	these keys to the child data blocks. The last 4 bytes of the file is a 4
 *	byte little-endian integer that is the size of the index (not including
 *	those 4 bytes).
 *
 *	From the start of this block, there is an entry record for each child
 *	section. It starts with a 4 byte integer for size of its data. This is the
 *	next lot of data in the child data section. After this is 16 bytes of
 *	reserved data (uint32 preloadLen, uint32 version, uint64 modified) and then
 *	a size prefixed string that is the sections tag. The size prefix is a 4 byte
 *	little-endian integer followed by the string of this length. This is padded
 *	to a 4 byte boundary.
 */

// -----------------------------------------------------------------------------
// Section: BinSection
// -----------------------------------------------------------------------------

/**
 *	Constructor for BinSection.
 *
 *	@param pBinary		BinaryBlock containing data for this section
 *	@param tag			The tag name of this section
 *	@return 			None
 */
BinSection::BinSection( const BW::StringRef& tag, BinaryPtr pBinary ) :
	tag_( tag.data(), tag.size() ),
	binaryData_( pBinary ),
	introspected_( false )
{
	BW_GUARD;
}

/**
 *	Destructor
 */
BinSection::~BinSection()
{
	BW_GUARD;
}


/**
 *	This method returns the number of children of this section-
 *
 *	@return 	Number of children.
 */
int BinSection::countChildren()
{
	BW_GUARD;
	this->introspect();

	return static_cast<int>(children_.size());
}


/**
 *	Open the child with the given index
 *
 *	@param index	Index of the child to return.
 *
 *	@return 		Pointer to the specified child.
 */
DataSectionPtr BinSection::openChild( int index )
{
	BW_GUARD;
	this->introspect();

	if (uint32(index) < children_.size()) return children_[ index ];
	return NULL;
}


class BinSectionCreator : public DataSectionCreator
{
public:
	virtual DataSectionPtr create( DataSectionPtr pSection,
									const BW::StringRef& tag )
	{
		BW_GUARD;
		PROFILER_SCOPED( BinSectionCreator_create );
		return new BinSection(tag, NULL);
	}

	virtual DataSectionPtr load(DataSectionPtr pSection,
									const BW::StringRef& tag,
									BinaryPtr pBinary = NULL)
	{
		BW_GUARD;
		PROFILER_SCOPED( BinSectionCreator_load );
		return new BinSection(tag, pBinary);
	}
};

DataSectionCreator* BinSection::creator()
{
	BW_GUARD;
	static BinSectionCreator s_creator;
	return &s_creator;
}

/**
 *	Create a new section with the given tag
 *
 *	@param tag		Name of tag of new child
 *  @param creator	This parameter is not used in this method.
 *
 *	@return 	A DataSectionPtr to the newly created DataSection.
 */
DataSectionPtr BinSection::newSection( const BW::StringRef & tag,
										DataSectionCreator* creator )
{
	BW_GUARD;
	this->introspect();

	DataSectionPtr pChild = new BinSection( tag, new BinaryBlock( NULL, 0, "BinaryBlock/BinSection" ) );
	children_.push_back( pChild );
	return pChild;
	// TODO: Allow the creation of XML sections here...

}


/**
 *	Find the child with the given tag name
 *
 *	@param tag		Name of child to find
 *  @param creator	This parameter is unused in this method.
 *
 *	@return	A DataSectionPtr to the child section matching tag on success,
 *          NULL if the child is not found.
 */
DataSectionPtr BinSection::findChild( const BW::StringRef & tag,
										DataSectionCreator* creator )
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( BinSection_findChild );
	this->introspect();

	for (uint i = 0; i < children_.size(); i++)
	{
		DataSectionPtr ds = children_[i];
		if (ds->sectionName() == tag)
		{
			return ds;
		}
	}
	return NULL;
}


/**
 * 	Erase the first child with the given tag name
 *
 *	@param tag		Name of child to erase
 *
 *	@return 		NULL
 */
void BinSection::delChild( const BW::StringRef & tag )
{
	BW_GUARD;
	this->introspect();

	for (uint i = 0; i < children_.size(); i++)
	{
		if (children_[i]->sectionName() == tag)
		{
			children_.erase( children_.begin() + i );
			return;
		}
	}
}

/**
 * 	Erase the given child
 *
 *	@param pSection		Child to erase
 *
 *	@return 		NULL
 */
void BinSection::delChild( DataSectionPtr pSection )
{
	BW_GUARD_PROFILER( BinSection_delChild );
	this->introspect();

	Children::iterator found = std::find(
		children_.begin(), children_.end(), pSection );
	if (found != children_.end())
		children_.erase( found );
}

/**
 *	Erase every child
 */
void BinSection::delChildren()
{
	BW_GUARD;
	SimpleMutexHolder sh( s_binSectionIntrospectLock );

	introspected_ = true;
	children_.clear();

	binaryData_ = new BinaryBlock( NULL, 0, "BinaryBlock/BinSection" );
}


/**
 *	This method returns the binary data owned by this section.
 *
 *	@return			Smart pointer to binary data object
 */
BinaryPtr BinSection::asBinary()
{
	BW_GUARD;

	bool needsInstrospect = false;

	{
		SimpleMutexHolder sh( s_binSectionIntrospectLock );

		// bundle up our children and children's children, etc.
		// Have to introspect again after the recollect to avoid memory leaks,
		// because otherwise the children will be kept alive and pointing to
		// the old binary block!!! (kept in the pOwner of the children).
		// TODO: see if this bit of code is actually needed. I'm keeping it
		// in because there was a comment saying 'slow but safe', referring
		// to the recollect() call to rebuild it.
		needsInstrospect = introspected_;
	}

	if (needsInstrospect)
	{
		setBinary( this->recollect() );	
		introspect();
	}

	return binaryData_;
}

/**
 * 	This method sets the binary data for this section.
 *
 * 	@param pBinary	Pointer to the binary data to assign into this section.
 *
 * 	@return			true if succesful, false otherwise
 */
bool BinSection::setBinary( BinaryPtr pBinary )
{
	BW_GUARD;

	SimpleMutexHolder sh( s_binSectionIntrospectLock );

	binaryData_ = pBinary;

	introspected_ = false;
	children_.clear();

	return true;
}


/**
 * 	This method returns the name of the section
 *
 *	@return 		The section name
 */
BW::string BinSection::sectionName() const
{
	return tag_;
}


/**
 * 	This method returns the number of bytes used by this section.
 *
 *	@return 		Number of bytes used by this BinSection.
 */
int BinSection::bytes() const
{
	BW_GUARD;
	if(binaryData_)
		return binaryData_->len();
	else
		return 0;
}

/**
 *	This method sets the parent used to save data
 */
void BinSection::setParent( DataSectionPtr pParent )
{
	BW_GUARD;
	parent_ = pParent;
}


/**
 * 	This method saves the given section to a file.
 */
bool BinSection::save( const BW::StringRef& saveAsFileName )
{
	BW_GUARD;
	// change our allegiance if we're doing a save as
	if (!saveAsFileName.empty())
	{
		BW::StringRef tag;
		if (!DataSection::splitSaveAsFileName( saveAsFileName, parent_, tag ))
			return false;
		tag_.assign( tag.data(), tag.size() );
	}

	// and now save
	if (!parent_)
	{
		ERROR_MSG( "BinSection: Can't save a section without a parent.\n" );
		return false;
	}

	return parent_->saveChild( this, true );
}

/**
 *	This method looks inside the binary data to see if it contains
 *	any subsections.
 */
void BinSection::introspect()
{
	BW_GUARD;

	SimpleMutexHolder sh( s_binSectionIntrospectLock );
	if (introspected_)
	{
		return;
	}

	introspected_ = true;

	{
		PROFILER_SCOPED( BinSection_introspect );

		if (!binaryData_) return;
		if (binaryData_->len() < int(sizeof(int) + sizeof(int)*2)) return;

		char * data = (char*)binaryData_->data();
		int len = binaryData_->len();

		int entryDataOffset = 0;
		int entryNameLenPad = 0;
		if (*(uint32*)data == BIN_SECTION_MAGIC)
		{
			entryDataOffset = 4;
			entryNameLenPad = 3;
		}

		// read the index info
		int indexLen = *(int*)(data + len - sizeof(int));
		int offset = len - sizeof(int) - indexLen;

		// make sure it is valid
		if (offset < entryDataOffset || offset >= len-int(sizeof(int)))
			return;

		// read the directory out of the file
		while (offset <= len - (int)(sizeof(int) + sizeof(int)*2))
		{
			// read the lengths of directory entry
			int entryDataLen = *(int*)(data+offset);
			offset += sizeof(int);
			if (((entryDataLen & (1<<31)) || entryNameLenPad != 0) &&
				offset + sizeof(uint32)*4 <= len - (sizeof(int) + sizeof(int)))
			{
				// skip extended data
				entryDataLen &= ~(1<<31);
				// uint32 preloadLen, uint32 version, uint64 modified
				offset += sizeof(uint32)*4;
			}
			int entryNameLen = *(int*)(data+offset);
			offset += sizeof(int);

			// make sure they make sense
			if (uint32(entryDataLen) > 256*1024*1028 ||
				uint32(entryDataOffset + entryDataLen) >
					uint32(len - sizeof(int) - indexLen) ||
				uint32(entryNameLen) > 4096 ||
				uint32(offset + entryNameLen) > uint32(len - sizeof(int)))
			{
				children_.clear();
				return;
			}

			// read its name
			BW::string entryStr( data+offset, entryNameLen );
			offset += (entryNameLen + entryNameLenPad) & (~entryNameLenPad);

			// add it to our list of children
			children_.push_back( DataSection::createAppropriateSection(
				entryStr,
				new BinaryBlock(
					data+entryDataOffset, entryDataLen, "BinaryBlock/BinSection", binaryData_ ),
					false, BinSection::creator() ) );

			// move on the data offset
			entryDataOffset += (entryDataLen + 3) & (~3L);
		}
	}
}


/**
 *	This method collects up all the children of this section into
 *	one big binary block.
 */
BinaryPtr BinSection::recollect()
{
	BW_GUARD;
	if (children_.empty())
	{
		return this->binaryData_;
	}

	uint32 bsz = 4;	// for magic

	// collect our children
	BW::vector<BinaryPtr> childBlocks;
	for (uint i = 0; i < children_.size(); i++)
	{
		childBlocks.push_back( children_[i]->asBinary() );
		bsz += (childBlocks.back()->len() + 3) & (~3L);
	}

	// build the directory
	BW::string stream;
	for (uint i = 0; i < children_.size(); i++)
	{
		const BW::string & sectName = children_[i]->sectionName();
		int lens[6] = { childBlocks[i]->len(), 0, 0, 0, 0,
			(int)sectName.length() };
		stream.append( (char*)lens, sizeof(lens) );
		stream.append( sectName.data(), lens[5] );
		stream.append( (4-lens[5]) & 3, '\0' );
	}
	int streamLen = static_cast<int>(stream.length());
	stream.append( (char*)&streamLen, sizeof(int) );
	bsz += static_cast<int>(stream.length());

	// now make the mega-block and return
	BinaryPtr pBinary = new BinaryBlock( NULL, bsz, "BinaryBlock/BinSection" );
	char * dataPtr = (char*)pBinary->data();
	*((uint32*&)dataPtr)++ = BIN_SECTION_MAGIC;
	for (uint i = 0; i < children_.size(); i++)
	{
		uint len = childBlocks[i]->len();
		memcpy( dataPtr, childBlocks[i]->data(), len );
		dataPtr += (len + 3) & (~3L);
	}
	memcpy( dataPtr, stream.data(), stream.length() );

	// and that's it
	return pBinary;
}

BW_END_NAMESPACE

// bin_section.cpp
