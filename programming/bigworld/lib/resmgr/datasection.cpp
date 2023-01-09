#include "pch.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

#include "datasection.hpp"

#include "bin_section.hpp"
#include "dataresource.hpp"
#include "data_section_census.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: DataSectionIterator
// -----------------------------------------------------------------------------

/**
 *	Default constructor for DataSectionIterator
 */
DataSectionIterator::DataSectionIterator() :
	dataSection_( reinterpret_cast<DataSection *>( NULL) ),
	index_(0)
{
	BW_GUARD;
}

/**
 *	Default constructor for DataSectionIterator
 *	This method opens a section with the name tag specified.
 *	It returns a pointer to the new section that was a subsection of the current
 *	If the specified subsection was not found through its tag, a NULL value is
 *	returned instead.
 *
 *	@param dataSection		Data section
 *	@param index			Index from which to start iteration
 */
DataSectionIterator::DataSectionIterator(
	const DataSectionPtr & dataSection,
	int index) :
	dataSection_(dataSection),
	index_(index)
{
	BW_GUARD;
}


/**
 *	This method returns whether or not the input iterator references the same
 *	data section as this iterator.
 *
 *	@return	True if the input iterator references the same data section as this
 *	iterator, otherwise false.
 */
bool DataSectionIterator::operator==(const DataSectionIterator& it) const
{
	BW_GUARD;
	return index_ == it.index_ && dataSection_ == it.dataSection_;
}


/**
 *	This method returns whether or not the input iterator references a different
 *	data section as this iterator.
 *
 *	@return	True if the input iterator references a different data section as
 *	this iterator, otherwise false.
 */
bool DataSectionIterator::operator!=(const DataSectionIterator& it) const
{
	BW_GUARD;
	return index_ != it.index_ || dataSection_ != it.dataSection_;
}


/**
 *	This method returns the data section referenced by this iterator.
 */
DataSectionPtr DataSectionIterator::operator*()
{
	BW_GUARD;
	DataSectionPtr pChild = dataSection_->openChild( index_ );
	if (!pChild)
		pChild = new BinSection( dataSection_->childSectionName( index_ ),
			new BinaryBlock( NULL, 0, "BinaryBlock/DataSection" ) );
	return pChild;
}


/**
 *	This method moves this iterator to reference the next data section in a
 *	sequence.
 */
const DataSectionIterator & DataSectionIterator::operator++()
{
	BW_GUARD;
	++index_;
	return *this;
}


/**
 *	This method moves this iterator to reference the next data section in a
 *	sequence.
 */
DataSectionIterator DataSectionIterator::operator++(int)
{
	BW_GUARD;
	++index_;
	return DataSectionIterator(dataSection_, index_ - 1);
}

// -----------------------------------------------------------------------------
// Section: DataSection
// -----------------------------------------------------------------------------

/**
 *	Destructor
 */
DataSection::~DataSection()
{
}


/**
 *	Safely destroy the DataSection if nothing else increments the ref count
 */
void DataSection::destroy() const
{
	BW_GUARD;
	// this will delete this object if it's safe to do so
	DataSectionCensus::tryDestroy( this );
}


/**	This method returns the index of the DataSectionPtr
 *
 * 	@param data		The DataSectionPtr to return the index of
 *
 *	@return			The index of the DataSectionPtr (-1 if not found)
 */
int DataSection::getIndex( DataSectionPtr data )
{
	BW_GUARD;
	for (int i=0; i<countChildren(); i++ )
	{
		DataSectionPtr test = openChild( i );
		if (data.getObject() == test.getObject())
		{
			return i;
		}
	}
	return -1;
}

/**
 *	This method returns the name of the child with the given index
 */
BW::string DataSection::childSectionName( int index )
{
	BW_GUARD;
	return this->openChild( index )->sectionName();
}

/**
 *	This method opens a section with the name tag specified. It returns a
 *	pointer to the new section that was a subsection of the current. If the
 *	specified subsection was not found, and makeNewSection is true, the new
 *	section will be created. Otherwise this function will return NULL.
 *
 *	@param tagPath			The path to search for.
 *	@param makeNewSection	If true and the section does not already exist, the
 *							section is created.
 *  @param creator			The DataSectionCreator to use if a new DataSection
 *                          is required.
 *
 *	@return		The associated section
 */
DataSectionPtr DataSection::openSection( const BW::StringRef & tagPath,
								bool makeNewSection,
								DataSectionCreator* creator )
{
	BW_GUARD;
	PROFILE_FILE_SCOPED( DSC_OpenSection );
	if (tagPath.empty()) return this;
	DataSectionPtr pChild;
	BW::string::size_type pos = tagPath.find_first_of("/");

	// Recurse down the path until we are left with a single child section

	if( pos != BW::string::npos )
	{
		pChild = this->findChild( tagPath.substr( 0, pos ) );

		if(!pChild)
		{
			if(makeNewSection)
				pChild = this->newSection( tagPath.substr( 0, pos ) );

			if(!pChild)
			{
				return (DataSection *)NULL;
			}
		}

		BW::string::size_type pastslash = tagPath.find_first_not_of( "/", pos );
		if ( pastslash == BW::string::npos )
		{
			// The string ended with a slash
			return pChild;
		}
		else
		{
			return pChild->openSection( tagPath.substr( pastslash ),
				makeNewSection, creator);
		}
	}

	// Then find that single child section

	pChild = this->findChild(tagPath, creator);

	if(!pChild && makeNewSection)
		pChild = this->newSection(tagPath, creator);

	return pChild;
}


/**	This method opens a vector of sections with the name tag specified. If
 *	there were no sections that match the tag specified, then the vector is
 *	left unchanged.
 *
 *	@param tagPath			The name/path of the sections to be read in.
 *	@param dest				The vector used for storing the sections.
 *  @param creator			This parameter is not used in this method.
 *
 *	@return None.
 */
void DataSection::openSections( const BW::StringRef &tagPath,
		BW::vector<DataSectionPtr> &dest, DataSectionCreator* creator )
{
	BW_GUARD;
	DataSectionPtr pChild;
	BW::string::size_type pos = tagPath.find_first_of( "/" );

	if( pos != BW::string::npos )
	{
		BW::string::size_type pastslash = tagPath.find_first_not_of( "/", pos );
		if ( pastslash != BW::string::npos )
		{
			pChild = this->findChild( tagPath.substr( 0, pos ) );

			if(pChild)
			{
				pChild->openSections( tagPath.substr( pastslash ), dest );
			}
			return;
		}
	}

	//TODO: use the creator for the iterators
	DataSectionIterator it;
	DataSectionIterator itEnd = this->end();
	for(it = this->begin(); it != itEnd; ++it)
	{
		if ( it.tag() == tagPath)
			dest.push_back( *it );
	}
}

BW::string DataSectionIterator::tag()
{
	BW_GUARD;
	return dataSection_->childSectionName( index_ );
}


/**
 *	This method opens the first sub-section within the section.
 *
 *	@return A pointer to the subsection. If the specified
 *			subsection was not found through its tag, a NULL value is returned
 *			instead.
 */
DataSectionPtr DataSection::openFirstSection()
{
	BW_GUARD;
	if(this->countChildren() == 0)
	{
		return (DataSection *)NULL;
	}
	else
	{
		return this->openChild(0);
	}
}


/**
 *	This method deletes the specified section under the current section.
 *	It can also delete a tag instead of a section. It will fail if the section
 *	does not exist.
 *
 *	@param tagPath		The name/path of the section to be deleted.
 *
 *	@return				True if successful, otherwise false.
 */
bool DataSection::deleteSection( const BW::StringRef & tagPath )
{
	BW_GUARD;

	BW::string::size_type tokenBegin = 0;
	BW::string::size_type tokenEnd = BW::string::npos;

	DataSectionPtr pCurrSection = this;
	DataSectionPtr pParent = (DataSection *)NULL;

	do
	{
		if (tokenEnd == BW::string::npos)
		{
			tokenBegin = 0;
		}
		else
		{
			tokenBegin = tokenEnd + 1;
		}

		tokenEnd = tagPath.find_first_of( "/", tokenBegin );

		BW::StringRef tag;

		if (tokenEnd < tagPath.size())
		{
			tag = tagPath.substr( tokenBegin, tokenEnd - tokenBegin );
		}
		else
		{
			tag = tagPath.substr( tokenBegin );
		}

		pParent = pCurrSection;
		pCurrSection = pCurrSection->findChild( tag );
	}
	while (tokenEnd != BW::string::npos && pCurrSection);

	if (pParent && pCurrSection)
	{
		pParent->delChild(pCurrSection->sectionName());
		return true;
	}

	return false;
}


void DataSection::deleteSections( const BW::StringRef &tagPath )
{
	BW_GUARD;

	while ( deleteSection( tagPath ) )
		;
}


/**
 *	This method returns an iterator pointing to the first element in
 *	the section.
 *
 *	@return				Iterator
 */
DataSectionIterator DataSection::begin()
{
	BW_GUARD;
	return DataSectionIterator(this, 0);
}


/**
 *	This method returns an iterator pointing after the last element in
 *	the section.
 *
 *	@return				Iterator
 */
DataSectionIterator DataSection::end()
{
	BW_GUARD;
	return DataSectionIterator(this, this->countChildren());
}


/**
 *	This method reads in the value of the specified tag as a boolean value.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The boolean value read in or the default value if the read failed.
 */
bool DataSection::readBool( const BW::StringRef &tagPath, bool defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asBool(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as an integer value.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The int value read in or the default value if the read failed.
 */
int DataSection::readInt( const BW::StringRef &tagPath, int defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asInt(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as an uint value.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The uint value read in or the default value if the read failed.
 */
unsigned int DataSection::readUInt( const BW::StringRef &tagPath, unsigned int defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asUInt(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a long integer
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param tagPath			The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The long value read in or the default value if the read failed.
 */
long DataSection::readLong( const BW::StringRef &tagPath, long defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asLong(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a 64 bits integer
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param tagPath			The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The int64 value read in or the default value if the read failed.
 */
int64 DataSection::readInt64( const BW::StringRef &tagPath, int64 defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asInt64(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a 64 bits uint
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param tagPath			The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The uint64 value read in or the default value if the read failed.
 */
uint64 DataSection::readUInt64( const BW::StringRef &tagPath, uint64 defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asUInt64(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a floating-point
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The float value read in or the default value if the read failed.
 */
float DataSection::readFloat( const BW::StringRef &tagPath,
		float defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asFloat(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a double
 *	floating-point value. With all of the read methods, a default value can
 *	be specified if the operation fails. The operation can fail if the tag
 *	given does not match any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The double value read in or the default value if the read failed.
 */
double DataSection::readDouble( const BW::StringRef &tagPath,
		double defaultVal)
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asDouble(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a string. With all
 *	of the read methods, a default value can be specified if the operation
 *	fails. The operation can fail if the tag given does not match any tags in
 *	the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *	@param flags		Any operational flags. These are bitwise ORed by the
 *						calling code. Flags accepted are:
 *						DS_TrimWhitespace and
 *						DS_IncludeWhitespace.
 *
 *	@return The string read in or the default value if the read failed.
 */
BW::string DataSection::readString( const BW::StringRef &tagPath,
		const BW::StringRef &defaultVal,
		int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asString(defaultVal);
	else
		return defaultVal.to_string();
}


/**
 *	This method reads in the value of the specified tag as a wide string. With
 *	all of the read methods, a default value can be specified if the operation
 *	fails. The operation can fail if the tag given does not match any tags in
 *	the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *	@param flags		Any operational flags. These are bitwise ORed by the
 *						calling code. Flags accepted are:
 *						DS_TrimWhitespace and
 *						DS_IncludeWhitespace.
 *
 *	@return The string read in or the default value if the read failed.
 */
BW::wstring DataSection::readWideString( const BW::StringRef &tagPath,
		const BW::WStringRef &defaultVal,
		int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asWideString(defaultVal);
	else
		return defaultVal.to_string();
}


/**
 *	This method reads in the value of the specified tag as a 2D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector2 read in or the default value if the read failed.
 */
Vector2 DataSection::readVector2( const BW::StringRef &tagPath,
		const Vector2 &defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asVector2(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a 3D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector3 read in or the default value if the read failed.
 */
Vector3 DataSection::readVector3( const BW::StringRef &tagPath,
		const Vector3 &defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asVector3(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a 4D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector4 read in or the default value if the read failed.
 */
Vector4 DataSection::readVector4( const BW::StringRef &tagPath,
		const Vector4 &defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asVector4(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a 3x4 matrix.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Matrix34 read in or the default value if the read failed.
 */
Matrix DataSection::readMatrix34( const BW::StringRef &tagPath,
		const Matrix &defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asMatrix34(defaultVal);
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as binary data.
 *	Data is returned as a smart pointer, so it will stay around until
 *	all references to it are deleted. If this function fails, a NULL
 *	pointer will be returned
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *
 *	@return The value read in or the NULL if the read failed.
 */
BinaryPtr DataSection::readBinary( const BW::StringRef &tagPath )
{
	DataSectionPtr pSection = this->openSection(tagPath, false);

	if(pSection)
		return pSection->asBinary();
	else
		return (BinaryBlock *)NULL;
}

/**
 *	This method reads in the value of the specified tag as a BLOB. With all
 *	of the read methods, a default value can be specified if the operation
 *	fails. The operation can fail if the tag given does not match any tags in
 *	the section.
 *
 *	@param tagPath		The name/path of the identifier to be read in.
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The string read in or the default value if the read failed.
 */
BW::string DataSection::readBlob( const BW::StringRef &tagPath,
		const BW::StringRef &defaultVal )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection( tagPath, false );

	if (pSection)
		return pSection->asBlob( defaultVal );
	else
		return defaultVal.to_string();
}

/**
 *	This method writes a boolean value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The boolean value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeBool( const BW::StringRef &tagPath, bool value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setBool( value );
	else
		return false;
}


/**
 *	This method writes an integer value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The integer value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeInt( const BW::StringRef &tagPath, int value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setInt( value );
	else
		return false;
}


/**
 *	This method writes an unsigned integer value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The unsigned integer value that will be associated with
 *						that identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeUInt( const BW::StringRef &tagPath, unsigned int value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setUInt( value );
	else
		return false;
}


/**
 *	This method writes a long value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The long value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeLong( const BW::StringRef &tagPath, long value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setLong( value );
	else
		return false;
}


/**
 *	This method writes an int64 value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The int64 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeInt64( const BW::StringRef &tagPath, int64 value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setInt64( value );
	else
		return false;
}


/**
 *	This method writes an uint64 value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The uint64 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeUInt64( const BW::StringRef &tagPath, uint64 value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setUInt64( value );
	else
		return false;
}


/**
 *	This method writes a floating-point value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The floating-point value that will be associated
 *						with that identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeFloat( const BW::StringRef &tagPath, float value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setFloat( value );
	else
		return false;
}


/**
 *	This method writes a double floating-point value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The double floating-point value that will be
 *						associated with that identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeDouble( const BW::StringRef &tagPath, double value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setDouble( value );
	else
		return false;
}


/**
 *	This method writes a string value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeString( const BW::StringRef &tagPath,
		const BW::StringRef &value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setString(value);
	else
		return false;
}


/**
 *	This method writes a wide string value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeWideString( const BW::StringRef &tagPath,
		const BW::WStringRef &value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setWideString(value);
	else
		return false;
}


/**
 *	This method writes a Vector2 value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The Vector2 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeVector2( const BW::StringRef &tagPath,
		const Vector2 &value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setVector2( value );
	else
		return false;
}


/**
 *	This method writes a Vector3 value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The Vector3 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeVector3( const BW::StringRef &tagPath,
		const Vector3 &value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setVector3( value );
	else
		return false;
}


/**
 *	This method writes a Vector4 value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The Vector4 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeVector4( const BW::StringRef &tagPath,
		const Vector4 &value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setVector4( value );
	else
		return false;
}


/**
 *	This method writes a Matrix34 value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The Matrix34 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeMatrix34( const BW::StringRef &tagPath,
		const Matrix &value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if(pSection)
		return pSection->setMatrix34( value );
	else
		return false;
}



/**
 *	This method writes a binary value to the tag specified. It uses the
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param pBinary		Data to write
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeBinary( const BW::StringRef &tagPath, BinaryPtr pBinary)
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true,
												 BinSection::creator());

	if(pSection)
		return pSection->setBinary(pBinary);
	else
		return false;
}

/**
 *	This method writes a BLOB value to the tag specified.
 *
 *	@param tagPath		The name/path of the identifier to be written to.
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool DataSection::writeBlob( const BW::StringRef &tagPath,
		const BW::StringRef &value )
{
	BW_GUARD;
	DataSectionPtr pSection = this->openSection(tagPath, true);

	if (pSection)
		return pSection->setBlob( value );
	else
		return false;
}

/**	This method reads in a vector of bools under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readBools( const BW::StringRef &tagPath, BW::vector<bool> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asBool());
	}
}

/**	This method reads in a vector of ints under the specified tag.
 *
 *	If a tag entry is an illegal value, it defaults to 0.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readInts( const BW::StringRef &tagPath, BW::vector<int> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asInt());
	}
}

/**	This method reads in a vector of long ints under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readLongs( const BW::StringRef &tagPath, BW::vector<long> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asLong());
	}
}

/**	This method reads in a vector of floats under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readFloats( const BW::StringRef &tagPath, BW::vector<float> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asFloat());
	}
}


/**	This method reads in a vector of doubles under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readDoubles( const BW::StringRef &tagPath, BW::vector<double> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asDouble());
	}
}


/**	This method reads in a vector of strings under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *	@param flags		Flags (eg DS_IncludeWhitespace)
 *
 *	@return None.
 */
void DataSection::readStrings( const BW::StringRef &tagPath,
		BW::vector<BW::string> &dest, int flags)
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asString("", flags));
	}
}


/**	This method reads in a vector of wide strings under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *	@param flags		Flags (eg DS_IncludeWhitespace)
 *
 *	@return None.
 */
void DataSection::readWideStrings( const BW::StringRef &tagPath,
		BW::vector<BW::wstring> &dest, int flags)
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asWideString(L"", flags));
	}
}

/**	This method reads in a vector of Vector2s under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readVector2s( const BW::StringRef &tagPath, BW::vector<Vector2> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asVector2());
	}
}

/**	This method reads in a vector of Vector3s under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readVector3s( const BW::StringRef &tagPath, BW::vector<Vector3> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asVector3());
	}
}


/**	This method reads in a vector of Vector4s under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readVector4s( const BW::StringRef &tagPath, BW::vector<Vector4> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asVector4());
	}
}


/**	This method reads in a vector of Matrix34s under the specified tag.
 *
 *	@param tagPath		The name/path of the vector to be read in.
 *	@param dest			The vector used for storing in the read results.
 *
 *	@return None.
 */
void DataSection::readMatrix34s( const BW::StringRef &tagPath, BW::vector<Matrix> &dest )
{
	BW_GUARD;
	DataSectionPtr pSection;
	DataSectionIterator it;
	BW::StringRef tag;

	pSection = this->splitTagPath(tagPath, tag, false);

	if(!pSection)
		return;

	for(it = pSection->begin(); it != pSection->end(); ++it)
	{
		DataSectionPtr pDS = *it;
		if(pDS->sectionName() == tag)
			dest.push_back(pDS->asMatrix34());
	}
}



/** This method creates a vector of integers under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeInts( const BW::StringRef &tagPath,
		BW::vector<int> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<int>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setInt(*it);
	}
}


/** This method creates a vector of bools under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeBools( const BW::StringRef &tagPath,
		BW::vector<bool> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<bool>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setBool(*it);
	}
}

/** This method creates a vector of longs under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeLongs( const BW::StringRef &tagPath,
		BW::vector<long> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<long>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setLong(*it);
	}
}

/** This method creates a vector of floats under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeFloats( const BW::StringRef &tagPath,
		BW::vector<float> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<float>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setFloat(*it);
	}
}

/** This method creates a vector of doubles under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeDoubles( const BW::StringRef &tagPath,
		BW::vector<double> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<double>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setDouble(*it);
	}
}

/** This method creates a vector of strings under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 */
void DataSection::writeStrings( const BW::StringRef &tagPath,
		BW::vector<BW::string> &src, int /*flags*/)
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<BW::string>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setString(*it);
	}
}


/** This method creates a vector of wide strings under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 */
void DataSection::writeWideStrings( const BW::StringRef &tagPath,
		BW::vector<BW::wstring> &src, int /*flags*/)
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<BW::wstring>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setWideString(*it);
	}
}

/** This method creates a vector of Vector2s under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeVector2s( const BW::StringRef &tagPath,
		BW::vector<Vector2> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<Vector2>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setVector2(*it);
	}
}

/** This method creates a vector of Vector3s under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeVector3s( const BW::StringRef &tagPath,
		BW::vector<Vector3> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<Vector3>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setVector3(*it);
	}
}


/** This method creates a vector of Vector4s under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeVector4s( const BW::StringRef &tagPath,
		BW::vector<Vector4> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<Vector4>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setVector4(*it);
	}
}


/** This method creates a vector of Matrix34s under the specified tag.
 *
 * 	@param tagPath		The name/path of the vector to write
 * 	@param src			The vector containing the data to write
 *	@param flags		Values DS_OverwriteVector or DS_AppendVector
 *						affects whether the previous vector information is
 *						deleted first or simply added to.
 *
 *	@return	None.
 */
void DataSection::writeMatrix34s( const BW::StringRef &tagPath,
		BW::vector<Matrix> &src, int /*flags*/ )
{
	BW_GUARD;
	DataSectionPtr pSection, pNewSection;
	BW::StringRef tag;
	BW::vector<Matrix>::iterator it;

	pSection = this->splitTagPath(tagPath, tag, true);

	if(!pSection)
		return;

	// TODO: remove existing elements if necessary.

	for(it = src.begin(); it != src.end(); it++)
	{
		pNewSection = pSection->newSection(tag);
		pNewSection->setMatrix34(*it);
	}
}

/* Stub versions of the 'as' and 'set' methods, so that subclasses that are unable
 * to provide these don't have to provide stubs.
 */

bool DataSection::asBool(bool defaultVal)
{
	BW_GUARD;
	return defaultVal;
}

int DataSection::asInt(int defaultVal)
{
	BW_GUARD;
	return defaultVal;
}


unsigned int DataSection::asUInt(unsigned int defaultVal)
{
	BW_GUARD;
	return defaultVal;
}


long DataSection::asLong(long defaultVal)
{
	BW_GUARD;
	return defaultVal;
}


int64 DataSection::asInt64(int64 defaultVal)
{
	BW_GUARD;
	return defaultVal;
}


uint64 DataSection::asUInt64(uint64 defaultVal)
{
	BW_GUARD;
	return defaultVal;
}


float DataSection::asFloat(float defaultVal)
{
	BW_GUARD;
	return defaultVal;
}

double DataSection::asDouble(double defaultVal)
{
	BW_GUARD;
	return defaultVal;
}

BW::string DataSection::asString( const BW::StringRef &defaultVal, int /*flags*/)
{
	BW_GUARD;
	return defaultVal.to_string();
}

BW::wstring DataSection::asWideString( const BW::WStringRef &defaultVal, int /*flags*/)
{
	BW_GUARD;
	return defaultVal.to_string();
}

Vector2 DataSection::asVector2(const Vector2 &defaultVal)
{
	BW_GUARD;
	return defaultVal;
}

Vector3 DataSection::asVector3(const Vector3 &defaultVal)
{
	BW_GUARD;
	return defaultVal;
}

Vector4 DataSection::asVector4(const Vector4 &defaultVal)
{
	BW_GUARD;
	return defaultVal;
}

Matrix DataSection::asMatrix34(const Matrix &defaultVal)
{
	BW_GUARD;
	return defaultVal;
}


BinaryPtr DataSection::asBinary()
{
	BW_GUARD;
	return (BinaryBlock *)NULL;
}

BW::string DataSection::asBlob( const BW::StringRef &defaultVal )
{
	BW_GUARD;
	return defaultVal.to_string();
}

bool DataSection::setBool( bool /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setInt( int /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setUInt( unsigned int /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setLong( long /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setInt64( int64 /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setUInt64( uint64 /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setFloat( float /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setDouble( double /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setString( const BW::StringRef & /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setWideString( const BW::WStringRef & /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setVector2( const Vector2 & /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setVector3( const Vector3 & /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setVector4( const Vector4 & /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setMatrix34( const Matrix & /*value*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setBinary( BinaryPtr /*pBinary*/ )
{
	BW_GUARD;
	return false;
}

bool DataSection::setBlob( const BW::StringRef & /*value*/ )
{
	BW_GUARD;
	return false;
}

/**	This method splits a tagPath into a path and a tag. It locates the
 *  DataSection that matches the everything except for the last tag in the path.
 *  This last tag is returned separately in 'tag'.
 *
 *	@param tagPath			The name/path.
 *	@param tag				The tag is returned here.
 *	@param makeNewSection	Make a new section if it does not exist.
 *
 *	@return The DataSection corresponding to the path supplied.
 */

DataSectionPtr DataSection::splitTagPath(const BW::StringRef& tagPath,
		BW::StringRef& tag,
		bool makeNewSection,
		DataSectionCreator* creator )
{
	BW_GUARD;
	DataSectionPtr pSection;
	BW::string::size_type pos;

	pos	= tagPath.find_last_of("/");

	// If the tagPath contains no path element, (just a tag),
	// then the DataSection is ourselves.

	if (pos > tagPath.size())
	{
		tag = tagPath;
		return this;
	}

	// Otherwise, find the section that matches the path.

	pSection = this->openSection(tagPath.substr(0, pos),
								 makeNewSection, creator);

	if(pSection)
	{
		tag = tagPath.substr(pos + 1);
		return pSection;
	}

	return (DataSection *)NULL;
}

BW_END_NAMESPACE

// These are really bad I know. Probably should be in BWResource...?
#include "xml_section.hpp"
#include "bin_section.hpp"
#include "packed_section.hpp"
#include "bwresource.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Static helper method to create a section approriate to
 *	the given data and/or name
 *  NOTE: the flag 'allowModifyData' is used when the caller wants to make
 *  sure that the binary data in 'pData' doesn't get modified. This is
 *  needed in some cases because XMLSection will modify the binary data.
 */
DataSectionPtr DataSection::createAppropriateSection(
	const BW::StringRef& tag, BinaryPtr pData, bool allowModifyData /*=true*/,
	 DataSectionCreator* creator )
{
	BW_GUARD;
	MF_ASSERT( pData.exists() );

	if (creator)
		return creator->load(NULL, tag, pData);

	if (pData->len() > 0)
	{
		//Ignore all leading whitespace
		const char * fileStr = pData->cdata();
		while (*fileStr)
		{
			if ((*fileStr != ' ') &&
				(*fileStr != '\t') &&
				(*fileStr != '\r') &&
				(*fileStr != '\n'))
					break;
			fileStr++;
		}

		if ((*fileStr) && (*fileStr == '<'))
		{
			BinaryPtr origData=pData;
			//g_longTime[0] = 0;
			//DiaryEntryPtr de = Diary::instance().add( "bin" );
			if ( !allowModifyData )
			{
				// copy the data to ensure it doesn't get modified
				pData = new BinaryBlock( pData->data(), pData->len(), "BinaryBlock/DataSection" );
			}
			XMLSectionPtr ptr = XMLSection::createFromBinary( tag, pData );
			//de->stop();
			/*
			if (g_longTime[0] != 0)
			{
				char nbuf[512];
				bw_snprintf( nbuf, sizeof(nbuf), "Parsing %s took %s\n",
					fullName.c_str(), g_longTime );
				OutputDebugString( nbuf );
			}
			*/

			if (ptr || allowModifyData)
				return ptr;
			else //not valid xml, treat as binary?
			{
				WARNING_MSG( "Invalid XML section '%s', treating as binary.\n", tag.to_string().c_str() );
				DataSectionPtr pDS = PackedSection::create( tag, origData );
				// BCB6 doesn't like the following line
				// return pDS ? pDS : new BinSection( tag, pData );
				if( pDS )
					return pDS;
				return new BinSection( tag, origData );
			}
		}
	}

	//TODO: move the packsection create into a creator
	DataSectionPtr pDS = PackedSection::create( tag, pData );
	// BCB6 doesn't like the following line
	// return pDS ? pDS : new BinSection( tag, pData );
	if( pDS )
		return pDS;
	return new BinSection( tag, pData );
}


/**
 *	Helper method to split up a save as filename into parent section and tag
 */
bool DataSection::splitSaveAsFileName( const BW::StringRef & fileName,
	DataSectionPtr & parent, BW::StringRef & tag )
{
	BW_GUARD;
	BW::string::size_type lastSlash = fileName.find_last_of( '/' );
	DataSectionPtr pNewParent;

	BW::StringRef theTag;
	if (lastSlash < fileName.length())
	{
		BW::StringRef parentName = fileName.substr( 0, lastSlash );
		pNewParent = BWResource::openSection( parentName );
		theTag = fileName.substr( lastSlash + 1 );
	}
	else
	{
		pNewParent = BWResource::instance().rootSection();
		theTag = fileName;
	}

	// TODO: worry about reversing slashes

	if (!pNewParent)
	{
		ERROR_MSG( "DataSection: Parent section for '%s' not found.\n",
			fileName.to_string().c_str() );
		return false;
	}

	// I'm not sure if these changes should be permanent.
	// They are for now however.
	tag = theTag;
	parent = pNewParent;

	return true;
}


/**
 *	This method sets the watcher values, rooted at the input path, to match this
 *	data section.
 *
 *	@param path	The path to the watcher subtree
 */
void DataSection::setWatcherValues( const BW::StringRef& path )
{
	BW_GUARD;
#if ENABLE_WATCHERS
	DataSection::iterator iter = this->begin();

	while (iter != this->end())
	{
		BW::string currPath =
			path.empty() ?	(*iter)->sectionName() :
							path + "/" + (*iter)->sectionName();

		(*iter)->setWatcherValues( currPath );

		++iter;
	}

	BW::string upath = unsanitise(path);

	// check for empty path as it makes no sense to set a watcher with no path
	if (upath.empty())
	{
		return;
	}

	bool succeeded = Watcher::rootWatcher().setFromString( NULL, upath.c_str(),
		this->asString().c_str() );

	if (!succeeded)
	{
		// As there are spaces in watcher names and space is not a valid character for
		// XML section name. So we will try the original section name first and then replace
		// all dots by spaces to allow both dot and space to be used in watcher names.
		std::replace (upath.begin(), upath.end(), '.', ' ');
		succeeded = Watcher::rootWatcher().setFromString( NULL, upath.c_str(),
			this->asString().c_str() );
	}

	if (!succeeded && !this->asString().empty())
	{
		WARNING_MSG( "DataSection::setWatcherValues: Failed to set %s to %s\n",
				upath.c_str(), this->asString().c_str() );
	}
#endif
}


/**
 *	This method copies the input section to this section.
 */
void DataSection::copy( const DataSectionPtr pSection, bool modifyCurrent )
{
	BW_GUARD;
	// If doing a BinSection->BinSection copy, and told to blow our own children
	// away, short-curcuit the recursion and just copy the binary blob.
	BinSection *me = dynamic_cast<BinSection *>( this );
	BinSection *pBinSection = dynamic_cast<BinSection *>( pSection.getObject() );
	if (me && pBinSection && modifyCurrent) {
		BinaryPtr pSrc = pBinSection->asBinary();
		BinaryPtr pData = new BinaryBlock( pSrc->data(), pSrc->len(), "DataSection::copy" );
		me->setBinary( pData );
		return;
	}

	if (modifyCurrent)
	{
		this->delChildren();

		this->setString( pSection->asString() );
		this->isAttribute( pSection->isAttribute() );
	}

	DataSection::iterator iter = pSection->begin();

	while (iter != pSection->end())
	{
		DataSectionPtr pNewSection = this->newSection( (*iter)->sectionName() );

		// TODO: Could look at removing the recursion.
		pNewSection->copy( *iter );

		++iter;
	}
}

void DataSection::copySections( const DataSectionPtr pSection, const BW::StringRef &tag )
{
	BW_GUARD;
	DataSection::iterator iter = pSection->begin();

	while (iter != pSection->end())
	{
		if ((*iter)->sectionName() == tag)
		{
			DataSectionPtr pNewSection = this->newSection( (*iter)->sectionName() );

			pNewSection->copy( *iter );
		}

		++iter;
	}
}

void DataSection::copySections( const DataSectionPtr pSection )
{
	BW_GUARD;
	this->setString( pSection->asString() );

	DataSection::iterator iter = pSection->begin();

	while (iter != pSection->end())
	{
		DataSectionPtr pNewSection = this->openSection( (*iter)->sectionName(), true );

		// TODO: Could look at removing the recursion.
		pNewSection->copySections( *iter );

		++iter;
	}
}

/// Compares this DataSection with another. Returns 0 if equal, > 0 if
/// this DataSection is "greater than" the other and < 0 if this
/// DataSection is "less than" the other.
int DataSection::compare( DataSectionPtr pOther )
{
	BW_GUARD;
	if (!pOther)
		return 1;

	int diff = this->sectionName().compare( pOther->sectionName() );
	if (diff != 0) return diff;

	diff = this->asString().compare( pOther->asString() );
	if (diff != 0) return diff;

	int numChildren = this->countChildren();
	diff = numChildren - pOther->countChildren();
	if (diff != 0) return diff;

	for ( int i = 0; i < numChildren; ++i )
	{
		diff = this->openChild( i )->compare( pOther->openChild( i ) );
		if (diff != 0) return diff;
	}

	return 0;
}


int DataSection::binaryCompare( DataSectionPtr pOther )
{
	BW_GUARD;
	if (!pOther)
		return 1;

	int diff = this->sectionName().compare( pOther->sectionName() );

	if (diff == 0)
	{
		int numOfChildren = this->countChildren();

		diff = numOfChildren - pOther->countChildren();

		if (diff == 0)
		{
			for (int i = 0; i < numOfChildren; ++i)
			{
				diff = this->openChild( i )->binaryCompare( pOther->openChild( i ) );

				if (diff != 0) 
				{
					break;
				}
			}

			if (numOfChildren == 0)
			{
				diff = this->asBinary()->compare( *pOther->asBinary() );
			}
		}
	}

	return diff;
}


BW::string DataSection::sanitise( const BW::StringRef & str ) const
{
	BW_GUARD;
	return str.to_string();
}


BW::string DataSection::unsanitise( const BW::StringRef & str ) const
{
	BW_GUARD;
	return str.to_string();
}


bool DataSection::sanitiseSectionName()
{
	BW_GUARD;
	return false;
}


bool DataSection::isValidSectionName() const
{
	BW_GUARD;
	return true;
}

BW_END_NAMESPACE

// datasection.cpp
