/*
 * A class that encapsulates the document into data sections. The class allows
 * manipulation and access to elements by hierarchical data sections.
 */

#include "pch.hpp"
#include "sanitise_helper.hpp"

#include "cstdmf/base64.h"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/guard.hpp"
#include "cstdmf/locale.hpp"
#include "cstdmf/string_utils.hpp"
#include "cstdmf/watcher.hpp"

#include <algorithm>
#include <errno.h>

// Support for XML escape sequences, such as &amp; and &#65, has been added.
// If for some reason, you need to disable this support and fall back to the
// old parsing, just un comment the line below:
//#define NO_XML_ESCAPE_SEQUENCE 1

// Standard Library Headers.
#include <fstream>
#include <algorithm>
#include <stack>
#include <sstream>
#include <iomanip>


// Application Specific Headers.
#include "xml_section.hpp"
#include "xml_special_chars.hpp"
#include "file_system.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )


BW_BEGIN_NAMESPACE

// Dodgy thing to get server compiling.
#ifndef INLINE
#define INLINE
#endif

bool XMLSection::s_shouldReadXMLAttributes = true;
bool XMLSection::s_shouldWriteXMLAttributes = true;

static bool s_shouldCheckXMLTag = true;

#if ENABLE_WATCHERS
namespace
{
class WatcherIniter
{
public:
	WatcherIniter()
	{
		BW_GUARD;
		MF_WATCH( "config/shouldReadXMLAttributes",
				XMLSection::shouldReadXMLAttributes,
				XMLSection::shouldReadXMLAttributes );
		MF_WATCH( "config/shouldWriteXMLAttributes",
				XMLSection::shouldWriteXMLAttributes,
				XMLSection::shouldWriteXMLAttributes );
	}
};

WatcherIniter g_watcherIniter;
} // anonymous namespace
#endif

template < class T >
class ScopedPointerArray : public BW::vector< T* >
{
public:
	ScopedPointerArray() { BW_GUARD; }

	virtual ~ScopedPointerArray()
	{
		BW_GUARD;
		for ( uint i = 0; i < this->size(); i++ )
		{
			bw_safe_delete(this->operator[](i));
		}
	}
};

/**
 *	Check a converted integer value against the original string value
 *	and print out a warning message if the string value is not a number
 */
template <typename T>
bool XMLSection::checkConversion( T t, const char * str, const char * method )
{
	BW_GUARD;
	if ((errno == ERANGE) ||
			((t == 0) && strcmp( str, "0" ) != 0))
	{
		WARNING_MSG( "XMLSection::%s: Invalid value \"%s\" in section \"%s\"\n",
			method, str, this->sectionName().c_str() );
		return false;
	}

	return true;
}


// -----------------------------------------------------------------------------
//	Constructor(s) for XMLSection.
// -----------------------------------------------------------------------------


/**
 *	This does a quick check to see whether a tag is valid XML.  It is not
 *	exhaustive.  Rather we check to commonly allowed errors in tag names,
 *	and in particular tags beginning with numbers, tags containing spaces
 *	or slashes.
 *
 *	@param str					The tag to check.
 *	@returns					False if the tag begins with a number or
 *								contains spaces, true otherwise.
 */
/*static*/bool XMLSection::isValidXMLTag(const char *str)
{
	BW_GUARD;
	if (!s_shouldCheckXMLTag)
	{
		return true;
	}
	// Handle degenerate cases:
	if (str == NULL || str[0]=='\0')
		return false;

	// The tag cannot begin with a number:
	// (Note that str can accidentally be binary data.  If it is then we need
	// to cast str[0] to unsigned otherwise an error in the runtime can occur).
	if (str[0]<0 || ::isdigit(str[0]) != 0)
		return false;

	// The tag cannot have spaces or slashes:
	for (size_t i = 0; str[i] != '\0'; ++i)
	{
		if (str[i] == ' ' || str[i] == '\\' || str[i] == '/')
			return false;
	}

	return true; // probably ok
}


/**
 *	Ordinary constructor for XMLSection.
 *
 *	@param tag						The tag name for this section
 *
 */
XMLSection::XMLSection( const BW::StringRef & tag ) :
	ctag_( NULL ),
	cval_( NULL ),
	tag_( tag.data(), tag.size() ),
	numChildren_( 0 ),
	isAttribute_( false ),
	noXMLEscapeSequence_( false )
{
	BW_GUARD;
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		tag_.capacity(), 0 )
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 )
#endif
	if (!isValidXMLTag(tag_.c_str()))
		WARNING_MSG("%s is not a valid XML tag\n", tag_.c_str());
}

/**
 *	Constructor for XMLSection when we don't want to copy the tag string
 */
XMLSection::XMLSection( const char * tag, bool /*cstrToken*/ ) :
	ctag_( tag ),
	cval_( NULL ),
	numChildren_( 0 ),
	isAttribute_( false ),
	noXMLEscapeSequence_( false )
{
	BW_GUARD;
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 )
#endif
	if (ctag_ != NULL && !isValidXMLTag(ctag_))
		WARNING_MSG("%s is not a valid XML tag\n", ctag_);
}


// -----------------------------------------------------------------------------
// Methods of accessing with XML elements and sections for XMLSection.
// -----------------------------------------------------------------------------

/**	This method returns the number of sections under this  one.
 *
 *	@return Number of sections.
 */
int XMLSection::countChildren()
{
	BW_GUARD;
	return static_cast<int>(children_.size());
}

/**	This method opens the section with the given index.
 *
 * 	@param index			Index of the section to open
 *
 *	@return A pointer to the section.
 */
DataSectionPtr XMLSection::openChild( int index )
{
	BW_GUARD;
	// MF_ASSERT( (uint)index < children_.size() );
	if ((uint)index < children_.size())
	{
		return children_[index].getObject();
	}
	else
	{
		ERROR_MSG( "XMLSection::openChild: "
					"Invalid index %d. Children = %" PRIzu "\n",
				index, children_.size() );

		return NULL;
	}
}

/**
 * This method returns the immediate child of this section that is associated
 * with the input tag.
 */
DataSectionPtr XMLSection::findChild( const BW::StringRef & tag,
	DataSectionCreator * /* creator */ )
{
	BW_GUARD;
	Children::const_iterator iter = children_.begin();

	while (iter != children_.end())
	{
		if ((*iter)->ctag_)
		{
			if (tag == (*iter)->ctag_)
			{
				return (*iter).getObject();
			}
		}
		else if (tag == (*iter)->tag_)
		{
			return (*iter).getObject();
		}

		iter++;
	}

	return (DataSection *)NULL;
}

class XMLSectionCreator : public DataSectionCreator
{
public:
	virtual DataSectionPtr create( DataSectionPtr pSection,
									const BW::StringRef& tag )
	{
		BW_GUARD;
		PROFILER_SCOPED( XmlSectionCreator_create );
		return new XMLSection( tag );
	}

	virtual DataSectionPtr load(DataSectionPtr pSection,
									const BW::StringRef& tag,
									BinaryPtr pBinary = NULL)
	{
		BW_GUARD;
		PROFILER_SCOPED( XmlSectionCreator_load );
		XMLSectionPtr xmlSection = XMLSection::createFromBinary(tag, pBinary);

		return xmlSection.getObject();
	}
};

DataSectionCreator* XMLSection::creator()
{
	BW_GUARD;
	static XMLSectionCreator s_creator;
	return &s_creator;
}


/**
 *	This method creates a new section under the current section.
 *	It will create a new section regardless of whether a section under the
 *	same tag exists at the current section.
 *
 *	@param tag		The name/path of the section to be added.
 *  @param creator  The DataSectionCreator to use to create a new section with.
 *
 *	@return	A pointer to the DataSection added if successful. NULL otherwise.
 */
DataSectionPtr XMLSection::newSection( const BW::StringRef & tag,
	DataSectionCreator * /* creator */ )
{
	BW_GUARD;
	XMLSection * pNewSect = this->addChild( NULL );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		pNewSect->tag_.capacity(), 0 )
#endif
	pNewSect->tag_.assign( tag.data(), tag.size() );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		pNewSect->tag_.capacity(), 0 )
#endif

	return pNewSect;
}

/**
 *	This method creates a new section at the index specified.
 *	It will create a new section regardless of whether a section under the
 *	same tag exists at the current section.
 *
 *	@param tag		The name/path of the section to be added.
 *  @param index	The index at which to insert the new DataSection
 *
 *	@return	A pointer to the DataSection added if successful. NULL otherwise.
 */
DataSectionPtr XMLSection::insertSection( const BW::StringRef &tag, int index /* = -1 */ )
{
	BW_GUARD;
	XMLSection * pNewSect = this->addChild( NULL, index );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		pNewSect->tag_.capacity(), 0 )
#endif
	pNewSect->tag_.assign( tag.data(), tag.size() );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		pNewSect->tag_.capacity(), 0 )
#endif

	return pNewSect;
}


/**
 * This method deletes the immediate child of this section that is associated
 * with the input tag.
 */
void XMLSection::delChild( const BW::StringRef & tag )
{
	BW_GUARD;
	Children::iterator iter = children_.begin();

	while (iter != children_.end())
	{
		if ((*iter)->sectionName() == tag)
		{
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				children_.capacity() * sizeof(children_[0]), 0 )
#endif
			children_.erase(iter);
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				children_.capacity() * sizeof(children_[0]), 0 )
#endif
			return;
		}

		iter++;
	}
}

/**
 * This method deletes the immediate child of this section that is associated
 * with the input pointer.
 */
void XMLSection::delChild( DataSectionPtr pSection )
{
	BW_GUARD;
	Children::iterator iter = children_.begin();

	while (iter != children_.end())
	{
		if (iter->getObject() == pSection.getObject())
		{
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				children_.capacity() * sizeof(children_[0]), 0 )
#endif
			children_.erase(iter);
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				children_.capacity() * sizeof(children_[0]), 0 )
#endif
			return;
		}

		iter++;
	}
}

/**
 *	This method deletes all children of this section.
 */
void XMLSection::delChildren()
{
	BW_GUARD;
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		children_.capacity() * sizeof(children_[0]), 0 )
#endif
	children_.clear();
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		children_.capacity() * sizeof(children_[0]), 0 )
#endif
}


/**
 * This method returns the name of this section.
 *
 * @return	The name of this section.
 */
BW::string XMLSection::sectionName() const
{
	BW_GUARD;
	if (ctag_) return ctag_;
	return tag_;
}



/**
 *	This method sets the parent of this section.
 *
 *	Take care that when this method is used, the parent does NOT keep a
 *	reference to to its children, or else a circular reference will result.
 *
 *	@param pParent	The new parent of the section.
 */
void XMLSection::setParent( DataSectionPtr pParent )
{
	BW_GUARD;
	parent_ = pParent;
}

/**
 *	This method gets the size of this section's overhead,
 *	including that of all its children.
 */
uint32 XMLSection::sizeInBytes() const
{
	BW_GUARD;
	uint32 sz = 0;
	sz += static_cast<uint32>(tag_.length() + value_.length());
	sz += static_cast<uint32>(children_.capacity() * sizeof(children_[0]));
	for (Children::const_iterator it = children_.begin();
		it != children_.end();
		it++)
	{
		sz += (*it)->sizeInBytes();
	}
	return sz;
}

/**
 * This method returns the approximate number of bytes allocated by this
 * section. Since the XMLSection owns its children, this should be recursive.
 *
 * @return	Number of bytes allocated
 */

int XMLSection::bytes() const
{
	BW_GUARD;
	return std::max( 1024, block_ ? block_->len() : 0 );
}

// -----------------------------------------------------------------------------
// Methods of reading in values of XML elements for XMLSection.
// -----------------------------------------------------------------------------

/**
 *	This method reads in the value of the specified tag as a boolean value.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The boolean value read in or the default value if the read failed.
 */
bool XMLSection::asBool( bool defaultVal )
{
	BW_GUARD;
	// Read in the string ignoring leading and trailing whitespaces.
	BW::string readValue = this->asString( "", DS_TrimWhitespace );

	if (!bw_stricmp( readValue.c_str(), "true" ))
	{
		return true;
	}
	if (!bw_stricmp( readValue.c_str(), "false" ))
	{
		return false;
	}

	WARNING_MSG( "XMLSection::asBool: Invalid bool '%s' for %s\n",
			readValue.c_str(), this->sectionName().c_str() );

	return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as an integer value.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The int value read in or the default value if the read failed.
 */
int XMLSection::asInt( int defaultVal )
{
	BW_GUARD;
	const char * readValue = this->cval();

	if (*readValue != 0)
	{
		// errno is checked by checkConversion
		errno = 0;
		int result = atoi( readValue );

		if (this->checkConversion( result, readValue, "asInt" ))
		{
			return result;
		}
	}

	return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a uint32 value.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The uint32 value read in or the default value if the read failed.
 */
unsigned int XMLSection::asUInt( unsigned int defaultVal )
{
	BW_GUARD;
	BW::string readValue = this->asString( );

	if (readValue != "")
	{
		// errno is checked by checkConversion
		errno = 0;
		unsigned int result = static_cast< unsigned int >(
								strtoul( readValue.c_str(), NULL, 10 ) );

		if (this->checkConversion( result, readValue.c_str(), "asUInt" ))
		{
			return result;
		}
	}

	return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a long integer
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The long value read in or the default value if the read failed.
 */
long XMLSection::asLong( long defaultVal )
{
	BW_GUARD;
	BW::string readValue = this->asString( );

	if ( readValue == "True" )
		return 1;
	else if ( readValue == "False" )
		return 0;

	if ( readValue != "" )
	{
		// errno is checked by checkConversion
		errno = 0;
		long result = atol( readValue.c_str() );

		if (this->checkConversion( result, readValue.c_str(), "asLong" ))
		{
			return result;
		}
	}

	return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a long integer
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The long value read in or the default value if the read failed.
 */
int64 XMLSection::asInt64( int64 defaultVal )
{
	BW_GUARD;
	BW::string readValue = this->asString( );

	BW::stringstream ss( readValue );
	int64 result = 0;

	ss >> result;

	if (ss.fail())
	{
		WARNING_MSG( "XMLSection::asInt64: "
				"Invalid value \"%s\" in section \"%s\"\n",
			readValue.c_str(), this->sectionName().c_str() );
		return defaultVal;
	}

	return result;
}


/**
 *	This method reads in the value of the specified tag as a 64-bit uint
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The 64-bit uint read in or the default value if the read failed.
 */
uint64 XMLSection::asUInt64( uint64 defaultVal )
{
	BW_GUARD;
	BW::string readValue = this->asString();

	if (readValue != "")
	{
		uint64 result = defaultVal;

		if (sscanf( readValue.c_str(), "%" PRIu64, &result ) == 1)
		{
			if ((readValue[0] != '+') && (readValue[0] != '0'))
			{
				char buf[ 30 ];
				bw_snprintf( buf, sizeof( buf ), "%" PRIu64, result );

				if (readValue != buf)
				{
					WARNING_MSG( "XMLSection::asUInt64: "
							"Value %s in section \"%s\" is out of range\n",
						readValue.c_str(), this->sectionName().c_str() );
				}
			}

			return result;
		}
		else
		{
			WARNING_MSG( "XMLSection::asUInt64: "
					"Invalid value \"%s\" in section \"%s\"\n",
				readValue.c_str(), this->sectionName().c_str() );
		}
	}

	return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a floating-point
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The float value read in or the default value if the read failed.
 */
inline float XMLSection::asFloat( float defaultVal )
{
	BW_GUARD;
	return (float) asDouble( defaultVal );
}


/**
 *	This method returns the value of this section as a double.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The double value read in or the default value if the read failed.
 */
double XMLSection::asDouble( double defaultVal )
{
	BW_GUARD;
	const char * readValue = cval();
	char * end;

	double d = strtod( readValue, &end );

	if (readValue != end)
	{
		return d;
	}

	if (strlen(readValue) != 0)
	{
		ERROR_MSG( "XMLSection::asDouble: Cannot convert %s.\n",
				readValue );
	}

	return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a string. With all
 *	of the read methods, a default value can be specified if the operation
 *	fails. The operation can fail if the tag given does not match any tags in
 *	the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *	@param flags		Any operational flags. These are bitwise ORed by the
 *						calling code. Flags accepted are:
 *						DS_TrimWhitespace and
 *						DS_IncludeWhitespace.
 *
 *	@return The string read in or the default value if the read failed.
 */
BW::string XMLSection::asString( const BW::StringRef & /* defaultVal */,
	int /* flags */ )
{
	BW_GUARD;
	// #### Need to consider stripping the white space.
	if (cval_)
		return cval_;

	return value_;
}


/**
 *	This method reads in the value of the specified tag as a wide string.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *	@param flags		Any operational flags. These are bitwise ORed by the
 *						calling code. Flags accepted are:
 *						DS_TrimWhitespace and
 *						DS_IncludeWhitespace.
 *
 *	@return The string read in or the default value if the read failed.
 */
BW::wstring XMLSection::asWideString( const BW::WStringRef & /*defaultVal*/,
		int /*flags*/ )
{
	BW_GUARD;
	return decodeWideString( this->asString() );
}


/**
 *	This method reads in the value of the specified tag as a 2D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector2 read in or the default value if the read failed.
 */
Vector2 XMLSection::asVector2( const Vector2 &defaultVal )
{
	BW_GUARD;
	BW::string readValue = this->asString( );

	Vector2 v;

	BW::istringstream istr( readValue );
	istr.imbue( Locale::standardC() );
	istr >> v.x >> v.y;

	if (istr.fail() || !istr.eof())
	{
		ERROR_MSG( "XMLSection::asVector2: Bad format '%s' in section %s\n",
			readValue.c_str(),
			this->sectionName().c_str() );
		return defaultVal;
	}

	return v;
}


/**
 *	This method reads in the value of the specified tag as a 3D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector3 read in or the default value if the read failed.
 */
Vector3 XMLSection::asVector3( const Vector3 &defaultVal )
{
	BW_GUARD;
	Vector3 v;

	const char * readValue = this->cval();
	char * end;

	for (int i = 0; i < 3; ++i)
	{
		v[i] = (float)strtod( readValue, &end );

		if (end == readValue)
		{
			ERROR_MSG( "XMLSection::asVector3: Bad format '%s' in section %s\n",
				this->cval(),
				this->sectionName().c_str() );
			return defaultVal;
		}

		readValue = end;
	}

	return v;
}


/**
 *	This method reads in the value of the specified tag as a 4D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector4 read in or the default value if the read failed.
 */
Vector4 XMLSection::asVector4( const Vector4 &defaultVal )
{
	BW_GUARD;
	BW::string readValue = this->asString( );

	Vector4 v;

	BW::istringstream istr( readValue );
	istr.imbue( Locale::standardC() );
	istr >> v.x >> v.y >> v.z >> v.w;

	if (istr.fail() || !istr.eof())
	{
		ERROR_MSG( "XMLSection::asVector4: Bad format '%s' in section %s\n",
			readValue.c_str(),
			this->sectionName().c_str() );
		return defaultVal;
	}

	return v;
}


/**
 *	This method reads in the value of the specified tag as a 3x4 matrix.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Matrix read in or the default value if the read failed.
 */
Matrix XMLSection::asMatrix34( const Matrix &defaultVal )
{
	BW_GUARD;
	Matrix	ret;

	ret[0] = this->readVector3( "row0", defaultVal[0] );
	ret[1] = this->readVector3( "row1", defaultVal[1] );
	ret[2] = this->readVector3( "row2", defaultVal[2] );
	ret[3] = this->readVector3( "row3", defaultVal[3] );

	ret.m[0][3] = 0.f;
	ret.m[1][3] = 0.f;
	ret.m[2][3] = 0.f;
	ret.m[3][3] = 1.f;

	return ret;
}


/**
 *	This method converts the whole XML section to a binary pointer.
 *	This is called in order to save the data in this section.
 */
BinaryPtr XMLSection::asBinary()
{
	BW_GUARD;
	BW::stringstream stream;

	if (!this->writeToStream(stream, 0)) return NULL;

	BinaryPtr pBinary = new BinaryBlock(stream, stream.tellp(), "BinaryBlock/XMLSection");

	return pBinary;
}

/**
 *	This method reads in the value of the specified tag as a BLOB. With all
 *	of the read methods, a default value can be specified if the operation
 *	fails. The operation can fail if the tag given does not match any tags in
 *	the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The string read in or the default value if the read failed.
 */
BW::string XMLSection::asBlob( const BW::StringRef & /*defaultVal*/ )
{
	BW_GUARD;
	BW::string encoded = this->asString();
	// asssume the input string is in Base64 encoding.
	size_t encLen = encoded.length();
	char * decoded = new char[ encLen ];
	int decLen = Base64::decode( encoded, decoded, encLen );
	BW::string retString( decoded, decLen );
	bw_safe_delete_array(decoded);
	return retString;
}

// -----------------------------------------------------------------------------
// Methods of writing out values of document elements for XMLSection.
// -----------------------------------------------------------------------------

/**
 *	This method writes a boolean value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The boolean value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setBool( bool value )
{
	BW_GUARD;
	char formattedString[512];

	// TODO:PM Does it make sense for these to have a format string?
	bw_snprintf( formattedString, sizeof(formattedString), "%s", value ? "true" : "false" );
	return this->setString( formattedString );
}


/**
 *	This method writes an integer value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The integer value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setInt( int value )
{
	BW_GUARD;
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), "%d", value );
	return this->setString( formattedString );
}


/**
 *	This method writes an integer value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The integer value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setUInt( unsigned int value )
{
	BW_GUARD;
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), "%u", value );
	return this->setString( formattedString );
}


/**
 *	This method writes a long value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The long value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setLong( long value )
{
	BW_GUARD;
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), "%ld", value );
	return this->setString( formattedString );
}


/**
 *	This method writes an int64 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The int64 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setInt64( int64 value )
{
	BW_GUARD;
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), "%" PRI64, value );
	return this->setString( formattedString );
}


/**
 *	This method writes an int64 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The int64 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setUInt64( uint64 value )
{
	BW_GUARD;
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), "%" PRIu64, value );
	return this->setString( formattedString );
}


/**
 *	This method writes a floating-point value to the tag specified. It uses
 *	the write method for the string to avoid code duplication.
 *
 *	@param value		The floating-point value that will be associated
 *						with that identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setFloat( float value )
{
	BW_GUARD;
	BW::ostringstream ostr;
	ostr.imbue( Locale::standardC() );
	ostr << std::setprecision( 6 ) << std::fixed << value;
	return !ostr.fail() && this->setString( ostr.str() );
}


/**
 *	This method writes a double floating-point value to the tag specified.
 *	It uses the write method for the string to avoid code duplication.
 *
 *	@param value		The double floating-point value that will be
 *						associated with that identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setDouble( double value )
{
	BW_GUARD;
	BW::ostringstream ostr;
	ostr.imbue( Locale::standardC() );
	ostr << std::setprecision( 17 ) << std::fixed << value;
	return !ostr.fail() && this->setString( ostr.str() );
}


/**
 *	This method writes a string value to the tag specified.
 *
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setString( const BW::StringRef &value )
{
	BW_GUARD;
	cval_ = NULL;
	{
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			value_.capacity(), 0 )
#endif
	}
	value_.assign( value.data(), value.size() );
	{
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			value_.capacity(), 0 )
#endif
	}
	return true;
}


/**
 *	This method writes a wide string value to the tag specified.
 *
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setWideString( const BW::WStringRef &value )
{
	BW_GUARD;
	this->setString( encodeWideString( value.to_string() ) );
	return true;
}


/**
 *	This method writes a Vector2 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The Vector2 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setVector2( const Vector2 &value )
{
	BW_GUARD;
	BW::ostringstream ostr;
	ostr.imbue( Locale::standardC() );
	ostr << std::setprecision( 6 ) << std::fixed;
	ostr << value.x << " " << value.y;
	return !ostr.fail() && this->setString( ostr.str() );
}


/**
 *	This method writes a Vector3 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The Vector3 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setVector3( const Vector3 &value )
{
	BW_GUARD;
	BW::ostringstream ostr;
	ostr.imbue( Locale::standardC() );
	ostr << std::setprecision( 6 ) << std::fixed;
	ostr << value.x << " " << value.y << " " << value.z;
	return !ostr.fail() && this->setString( ostr.str() );
}


/**
 *	This method writes a Vector4 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The Vector4 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setVector4( const Vector4 &value )
{
	BW_GUARD;
	BW::ostringstream ostr;
	ostr.imbue( Locale::standardC() );
	ostr << std::setprecision( 6 ) << std::fixed;
	ostr << value.x << " " << value.y << " " << value.z << " " << value.w;
	return !ostr.fail() && this->setString( ostr.str() );
}


/**
 *	This method writes a Matrix34 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *
 *	@param value		The Vector3 value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setMatrix34( const Matrix &value )
{
	BW_GUARD;
	bool good = true;

	good &= this->writeVector3( "row0", value[0] );
	good &= this->writeVector3( "row1", value[1] );
	good &= this->writeVector3( "row2", value[2] );
	good &= this->writeVector3( "row3", value[3] );

	return good;
}


/**
 *	This method writes a BLOB value to the tag specified. The
 *	This means the that string will have to be formatted before calling this
 *	method.
 *
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setBlob( const BW::StringRef &value )
{
	BW_GUARD;
	BW::string encoded = Base64::encode( value.data(), value.length() );
	return this->setString( encoded );
}

/**
 * The destructor does not do much as it cannot delete any of the pointers in
 * the data. All other objects have their own destructors.
 */
XMLSection::~XMLSection()
{
	BW_GUARD;
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		tag_.capacity(), 0 )
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		value_.capacity(), 0 )
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		sizeof(*this), 0 )
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
		children_.capacity() * sizeof(children_[0]), 0 )
#endif
}


// -----------------------------------------------------------------------------
// Private Helper Methods.
// -----------------------------------------------------------------------------

/**
 *	This method strips the whitespace from a string. It works by taking the
 *	input string (null-terminated) and marking off the null-terminator before
 *	any trailing whitespace characters. It then returns a pointer to the first
 *	non-whitespace character in the string.
 *
 *	@param inputString A pointer to a null-terminated ASCII string.
 *
 *	@return The string with trailing and leading whitespace removed.
 */
char *XMLSection::stripWhitespace( char *inputString )
{
	BW_GUARD;
	// Find the end of the string.
	char *stringPos = inputString;
	while ( *stringPos != '\0' )
	{
		stringPos++;
	}

	// Find and mark the start of trailing whitespace.
	while ( isspace(*( stringPos - 1 ) ) &&
		( stringPos != inputString ) )
	{
		stringPos--;
	}
	*stringPos = '\0';

	// Find end of leading whitespace and return that as the new start of
	// string.
	stringPos = inputString;
	while ( isspace( *stringPos ) && ( *stringPos != '\0' ) )
	{
		stringPos++;
	}
	return stringPos;
}



/**
 * This method is used to add a child to this section.
 */
XMLSection * XMLSection::addChild( const char * tag, int index /* = -1 */ )
{
	BW_GUARD;
	XMLSection * pNewSection = new XMLSection( tag, true );

	// Attributes can not have children, force this section to be a subsection.
	isAttribute_ = false;

	if (index == -1)
	{
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			children_.capacity() * sizeof(children_[0]), 0 )
#endif
		children_.push_back( pNewSection );
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			children_.capacity() * sizeof(children_[0]), 0 )
#endif
	}
	else
	{
		if (index > (int)children_.size()) index = (int)children_.size(); // Just in case
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			children_.capacity() * sizeof(children_[0]), 0 )
#endif
		children_.insert( children_.begin() + index, pNewSection );
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
			children_.capacity() * sizeof(children_[0]), 0 )
#endif
	}

	return pNewSection;
}


inline bool isWhiteSpace( char c )
{
	BW_GUARD;
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

template <class C>
inline void skipWhiteSpace( C & stream )
{
	BW_GUARD;
	while (stream && isWhiteSpace( stream.peek() ))
	{
		stream.get();
	}
}

// TODO:PM Should get rid of these constants.
const int MAX_TAG_LENGTH = 1024;
const int MAX_VALUE_LENGTH = 1024;


/**
 * This static method creates a tree of XMLSections from the input file.
 *
 * @param filename	The full path to the file to load.
 * @param returnEmptySection 	True if an empty section should be returned
 *								on failure.
 *
 * @return			The root of the tree on success, NULL otherwise.
 */
XMLSectionPtr XMLSection::createFromFile( const char * filename,
		bool returnEmptySection)
{
	BW_GUARD;
	BinaryPtr binBlock = BWResource::instance().nativeFileSystem()->readFile( filename );

	if ( !binBlock )
	{
		if ( returnEmptySection )
		{
			return new XMLSection("root");
		}
		else
		{
			ERROR_MSG( "XMLSection: Could not open %s.\n", filename );
			return (XMLSection *)NULL;
		}
	}
	XMLSectionPtr ret = XMLSection::createFromBinary( "root", binBlock );
	return ret;
}


/**
 * This static method creates a tree of XMLSections from the input stream.
 *
 * @param tag		The name of the root section.
 * @param astream	Stream containing the file to load.
 *
 * @return			The root of the tree on success, NULL otherwise.
 */
XMLSectionPtr XMLSection::createFromStream(
	const BW::StringRef & tag, std::istream& astream )
{
	BW_GUARD;
	std::streampos bufnow = astream.tellg();
	astream.seekg( 0, std::ios_base::end );
	std::streampos buflen = astream.tellg() - bufnow;
	astream.seekg( bufnow, std::ios_base::beg );

	BinaryPtr pBlock = new BinaryBlock( astream, buflen, "BinaryBlock/XMLSection" );

	return XMLSection::createFromBinary( tag, pBlock );
	//istrstream stream((char *)pBinary->data(), pBinary->len());
	//return XMLSection::createFromStream(stream);
}





//#include "cstdmf/profile.hpp"

/**
 *	A local class based on VectorNoDestructor for our special
 *	parse stack requirements.
 */
template<class _Ty>
class MyVNoDest : public BW::vector< _Ty >
{
	typedef BW::vector< _Ty > BASE_TYPE;
public:

	MyVNoDest() :
		nElements_( 0 )
	{
		BW_GUARD;
	}

	INLINE void push_back( const _Ty & element )
	{
		BW_GUARD;
		MF_ASSERT_DEBUG( nElements_ >= 0 );
		if( BASE_TYPE::size() <= (typename BASE_TYPE::size_type)nElements_ )
		{
			BASE_TYPE::push_back( element );
			nElements_ = static_cast<int>(BASE_TYPE::size());
		}
		else
		{
			*( this->begin() + nElements_ ) = element;
			nElements_++;
		}
	}

	INLINE typename BASE_TYPE::iterator end()
	{
		BW_GUARD;
		return this->begin() + nElements_;
	}

	INLINE typename BASE_TYPE::const_iterator end() const
	{
		BW_GUARD;
		return this->begin() + nElements_;
	}

	INLINE typename BASE_TYPE::reverse_iterator rbegin()
	{
		BW_GUARD;
		return BASE_TYPE::reverse_iterator(this->begin() + nElements_);
	}

	INLINE typename BASE_TYPE::const_reverse_iterator rbegin() const
	{
		BW_GUARD;
		return BASE_TYPE::const_reverse_iterator(this->begin() + nElements_);
	}

	INLINE typename BASE_TYPE::reverse_iterator rend()
	{
		BW_GUARD;
		return BASE_TYPE::reverse_iterator(this->begin());
	}

	INLINE typename BASE_TYPE::const_reverse_iterator rend() const
	{
		BW_GUARD;
		return BASE_TYPE::const_reverse_iterator(this->begin());
	}


	INLINE typename BASE_TYPE::size_type size()
	{
		BW_GUARD;
		return nElements_;
	}

	INLINE typename BASE_TYPE::size_type size() const
	{
		BW_GUARD;
		return nElements_;
	}

	INLINE void reduceTo( int n )
	{
		BW_GUARD;
		nElements_ = n;
	}

	INLINE bool empty() const
	{
		BW_GUARD;
		return nElements_ == 0;
	}

	INLINE void clear()
	{
		BW_GUARD;
		nElements_ = 0;
	}

	INLINE _Ty &back()
	{
		BW_GUARD;
		MF_ASSERT_DEBUG( nElements_ > 0 );
		return *(this->begin() + nElements_ - 1 );
	}

	INLINE const _Ty &back() const
	{
		BW_GUARD;
		MF_ASSERT_DEBUG( nElements_ > 0 );
		return *(this->begin() + nElements_ - 1 );
	}

	INLINE void pop_back()
	{
		BW_GUARD;
		MF_ASSERT_DEBUG( nElements_ > 0 );
		nElements_ --;
	}

	INLINE void erase( typename BASE_TYPE::iterator _P )
	{
		BW_GUARD;
		std::copy(_P + 1, end(), _P);
		nElements_ --;
	}

private:
	int nElements_;
};

typedef MyVNoDest<XMLSection*> XMLParseStack;

static THREADLOCAL(XMLParseStack *) s_parseStack;
static THREADLOCAL(XMLParseStack *) s_parseKids;
//MF_THREAD_STORAGE char g_longTime[512];

// We store a vector a XMLParseStacks that can only be accessed by one thread at at time
static SimpleMutex s_parseStackMutex;
ScopedPointerArray< XMLParseStack > s_ParseStacks;

/**
 *	Helper class to dress a BinaryBlock up as a stream
 */
class WrapperStream
{
public:
	WrapperStream( BinaryPtr pBlock ) :
		pBlock_( pBlock ),
		cursor_( 0 )
	{ BW_GUARD; }
	~WrapperStream()
	{ BW_GUARD; }

	operator bool() const
	{
		BW_GUARD;
		return cursor_ < uint32(pBlock_->len());
	}

	uint32 tellg() const
	{
		BW_GUARD;
		return cursor_;
	}

	char * pCurr()
	{
		BW_GUARD;
		return &((char*)pBlock_->data())[cursor_];
	}

	char peek() const
	{
		BW_GUARD;
		if (cursor_ >= uint32(pBlock_->len())) return 0;
		return ((char*)(pBlock_->data()))[cursor_];
	}

	char get()
	{
		BW_GUARD;
		if (cursor_ >= uint32(pBlock_->len())) return 0;
		return ((char*)(pBlock_->data()))[cursor_++];
	}

	char * seek( char c )
	{
		BW_GUARD;
		char * data = (char*)pBlock_->data();
		while (*this)
		{
			if (data[ cursor_++ ] == c)
			{
				return &data[ cursor_ - 1 ];
			}
		}

		return NULL;
	}

private:
	BinaryPtr	pBlock_;
	uint32		cursor_;
};


/**
 *	This static method processes a "<!" section. This may be a comment or a
 *	CDATA section.
 */
bool XMLSection::processBang( WrapperStream & stream, XMLSection * pCurrNode )
{
	BW_GUARD;
	MF_VERIFY( stream.get() == '!' );

	char typeChar = stream.get();

	if (typeChar == '-')
	{
		if (stream.get() != '-')
		{
			ERROR_MSG( "XMLSection::createFromStream: "
				"Bad Comment: Expecting \"--\" following \"<!\"\n" );
			return false;
		}

		// Consume the comment.
		int commentCount = 1;

		char prev3 = '\0';
		char prev2 = '\0';
		char prev1 = stream.get();
		char currChar = stream.get();

		bool printedNestedError = false;

		while (stream && commentCount != 0)
		{
			prev3 = prev2;
			prev2 = prev1;
			prev1 = currChar;
			currChar = stream.get();

			if (prev2 == '-' && prev1 == '-' && currChar == '>')
			{
				commentCount--;
			}
			else if (prev3 == '<' && prev2 == '!' && prev1 == '-' &&
					currChar == '-')
			{
				commentCount++;
				if (!printedNestedError && commentCount > 1)
				{
					WARNING_MSG( "XMLSection %s contains nested comments\n",
						pCurrNode->sectionName().c_str() );
					printedNestedError = true;
				}
			}
		}

		if (!stream)
		{
			ERROR_MSG( "XMLSection::processBang: Comment not closed.\n" );
		}
	}
	else if (typeChar == '[')
	{
		if (!((stream.get() == 'C') &&
			(stream.get() == 'D') &&
			(stream.get() == 'A') &&
			(stream.get() == 'T') &&
			(stream.get() == 'A') &&
			(stream.get() == '[')))
		{
			ERROR_MSG( "XMLSection::createFromStream: "
				"Bad CDATA: Expecting \"CDATA[ following \"<[\"\n" );
			return false;
		}

		char * pTagStart = stream.pCurr();

		while (stream)
		{
			if (stream.get() == ']')
			{
				char * pTagEnd = stream.pCurr() - 1;
				if (stream.get() == ']' && stream.get() == '>')
				{
					*pTagEnd = '\0';
					if (pCurrNode == NULL)
					{
						ERROR_MSG( "XMLSection::processBang: "
								"CDATA outside section.\n" );
						return false;
					}
					else if (pCurrNode->cval_ == NULL)
					{
						pCurrNode->cval_ = pTagStart;
						return true;
					}
					else
					{
						ERROR_MSG( "XMLSection::processBang: "
								"Trying to set the value twice\n"
								"Tag = \"%s\"\n"
								"Old value = \"%s\"\n"
								"New value = \"%s\"\n",
								pCurrNode->sectionName().c_str(),
								pCurrNode->asString().c_str(),
								pTagStart );

						return false;
					}
				}
			}
		}

		ERROR_MSG( "XMLSection::processBang: CDATA section not closed.\n" );
	}

	return bool( stream );
}


/**
 *	This static method processes a "<?" section. If present, it is usually the
 *	XML description at the top of the file.
 */
bool XMLSection::processQuestionMark( WrapperStream & stream )
{
	BW_GUARD;
	MF_VERIFY( stream.get() == '?' );

	char prev1 = stream.get();
	char currChar = stream.get();

	while (stream && !(prev1 == '?' && currChar == '>'))
	{
		prev1 = currChar;
		currChar = stream.get();
	}

	if (!stream)
	{
		ERROR_MSG( "XMLSection::processQuestionMark: "
				"\"<?\" section not closed.\n" );
	}

	return bool( stream );
}

/**
 *	This static method creates a tree of XMLSections from the input buffer.
 *  NOTE: the data in pBlock will get modified. If you need to keep pBlock
 *  unmodified, you must pass in a deep copy of it instead.
 *
 *	@param rootTag	The tag of the root section.
 * 	@param pBlock	Binary block containing XML
 *
 * 	@return			The root of the tree on success, NULL otherwise.
 */
XMLSectionPtr XMLSection::createFromBinary(
	const BW::StringRef & rootTag, BinaryPtr pBlock )
{
	BW_GUARD;
	PROFILER_SCOPED( XmlSection_createFromBinary );
	bool isInError = false;
	if (s_parseStack == NULL)
	{
		s_parseStack = new XMLParseStack();
		SimpleMutexHolder parseStackMutexHolder( s_parseStackMutex );
		s_ParseStacks.push_back( s_parseStack );
	}
	s_parseStack->clear();

	if (s_parseKids == NULL)
	{
		s_parseKids = new XMLParseStack();
		SimpleMutexHolder parseStackMutexHolder( s_parseStackMutex );
		s_ParseStacks.push_back( s_parseKids );
	}
	s_parseKids->clear();

	char currChar = '\0';
	XMLSection * pCurrNode = NULL;
	XMLSectionPtr pRootNode = NULL;

	pRootNode = 0;

	// make a new stream
	//istrstream stream( bigbuf, buflen );
	WrapperStream stream( pBlock );
	char * bigbuf = (char*)pBlock->data();

	//uint64 addnodeTime = 0;
	//uint64 vsetTime = 0;
	//int nnodes = 0;
	//uint64 fullTime = timestamp();

	char lastPeek = stream.peek();
	do
	{
		if (isWhiteSpace( lastPeek ))
		{
			stream.get();
			skipWhiteSpace( stream );
			lastPeek = stream.peek();
		}

		if (lastPeek == '<')
		{
			// Consume '<'
			stream.get();

			const char peek = stream.peek();

			if (peek == '!')
			{
				isInError = !XMLSection::processBang( stream, pCurrNode );
			}
			else if (peek == '?')
			{
				isInError = !XMLSection::processQuestionMark( stream );
			}
			else
			{
				// Process the tag
				bool isEndTag = (stream.peek() == '/');

				if (isEndTag)
				{
					stream.get();
				}

				// Get the tag's string.
				int tagBeg = stream.tellg();
				char * tag = bigbuf + tagBeg;
				char * pTagEnd = stream.seek( '>' );

				if (pTagEnd)
				{
					*pTagEnd = '\0';

					while ((pTagEnd > tag) && isWhiteSpace( *(pTagEnd-1) ))
					{
						--pTagEnd;
					}

					*pTagEnd = '\0';
				}

				// Is it the root node?

				if (pTagEnd == NULL)
				{
					ERROR_MSG( "XMLSection::createFromStream: "
							"Unclosed tag\n" );
					isInError = true;
				}
				else if (pCurrNode == NULL && isEndTag)
				{
					ERROR_MSG( "XMLSection::createFromStream: "
							"Cannot have an end tag at start of file\n" );
					isInError = true;
				}
				else if (isEndTag)
				{

					bool matched = true;

					// Check whether the start and end tags match. If they
					// do not, we still check whether the start tag has
					// attributes.

					if (strcmp( pCurrNode->ctag_, tag ) != 0)
					{
						matched = false;
						char * startTag = (char *)pCurrNode->ctag_;
						size_t endLength = strlen( tag );

						if (strncmp( startTag, tag, endLength ) == 0)
						{
							if (isWhiteSpace( startTag[ endLength ] ))
							{
								startTag[ endLength ] = '\0';
								matched = true;
							}
						}
					}

					if (matched)
					{
						// We have finished with this tag and we are back in
						// the scope of the parent node.

#if ENABLE_RESOURCE_COUNTERS
						RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
							pCurrNode->children_.capacity() * sizeof(pCurrNode->children_[0]), 0 )
#endif
						pCurrNode->children_.assign(
							s_parseKids->end() - pCurrNode->numChildren_,
							s_parseKids->end() );
#if ENABLE_RESOURCE_COUNTERS
						RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
							pCurrNode->children_.capacity() * sizeof(pCurrNode->children_[0]), 0 )
#endif
						s_parseKids->reduceTo(
							(int)(s_parseKids->size()) - pCurrNode->numChildren_ );

						pCurrNode->block_ = pBlock;

						if(!s_parseStack->empty())
						{
							pCurrNode = s_parseStack->back();
							s_parseStack->pop_back();
						}
						else
						{
							pCurrNode = NULL;
						}
					}
					else
					{
						ERROR_MSG( "XMLSection::createFromStream: "
									"Tags do not match\n"
									"\tStart tag = \"%s\"\n"
									"\tEnd tag   = \"%s\"\n",
								pCurrNode->sectionName().c_str(),
								tag );

						isInError = true;
					}
				}
				else
				{
					//uint64 st = timestamp();

					// Strip off any '/' at the end then move it back on
					// after creating the XMLSection:

					char * pCurr = tag;
					while (*pCurr && *pCurr != '/' 
						&& !isWhiteSpace( *pCurr ))
					{
						++pCurr;
					}

					bool isEmptyTag = (*(pTagEnd-1) == '/');
					*pCurr = '\0';

					XMLSection * pNewNode = new XMLSection( tag, true );

					//addnodeTime += timestamp() - st;
					//nnodes++;


					if (pCurrNode == NULL)
					{
						pRootNode = pNewNode;
					}
					else
					{
						// add this to its list of children ... later
						s_parseKids->push_back( pNewNode );

						pCurrNode->numChildren_++;
					}

					// Check for empty tag shorthand.
					if (isEmptyTag)
					{
						pNewNode->block_ = pBlock;
					}
					else
					{
						if (pRootNode != pNewNode)
						{
							// Remember the old node.
							s_parseStack->push_back( pCurrNode );
						}
						pCurrNode = pNewNode;
					}

					if (s_shouldReadXMLAttributes)
					{
						pCurr++;
						// Get attributes
						while (pCurr < pTagEnd)
						{
							//
							// Get Name:
							//

							// Skip white spaces
							while (pCurr < pTagEnd && isWhiteSpace( *pCurr ))
							{
								++pCurr;
							}

							char *pName = pCurr;
							// Looking for name
							while (pCurr < pTagEnd && *pCurr &&
								!isWhiteSpace( *pCurr ) && *pCurr != '=')
							{
								++pCurr;
							}

							// Reached end
							if (pCurr >= pTagEnd || *pCurr == 0)
							{
								break;
							}

							char nextChar = *pCurr;

							*pCurr = 0;
							pCurr++;

							//
							// Get Equals:
							//

							if (nextChar != '=')
							{
								// Skip white spaces
								while (pCurr < pTagEnd && isWhiteSpace( *pCurr ))
								{
									++pCurr;
								}

								if (*pCurr != '=')
								{
									ERROR_MSG("XMLSection::createFromStream:"
										" Attributes must have an equals sign"
										" please see tag %s\n",
										tag);
									break;
								}
								++pCurr;
							}

							//
							// Get Value:
							//

							// Skip white spaces
							while (pCurr < pTagEnd && isWhiteSpace( *pCurr ))
							{
								++pCurr;
							}

							char quoteChar = *pCurr;

							if (*pCurr != '"' && *pCurr != '\'')
							{
								ERROR_MSG("XMLSection::createFromStream: "
									"Missing quotes for attribute %s on tag %s\n",
									pName, tag);
								break;
							}

							// Skip quote mark
							++pCurr;

							char *pValue = pCurr;

							while (pCurr < pTagEnd && *pCurr 
								&& *pCurr != quoteChar)
							{
								++pCurr;
							}

							if (*pCurr != quoteChar)
							{
								ERROR_MSG("XMLSection::createFromStream: Unable"
									" to find end of attribute %s on tag %s\n",
									pName, tag);
								break;
							}

							*pCurr = 0;
							pCurr++;


							XMLSection * attribNode;
							if (isEmptyTag)
							{
								attribNode = pNewNode->addChild( pName );
							}
							else
							{
								attribNode = new XMLSection( pName, true );
								s_parseKids->push_back( attribNode );

								pCurrNode->numChildren_++;
							}
							attribNode->block_ = pBlock;
							attribNode->isAttribute_ = true;
						#ifndef NO_XML_ESCAPE_SEQUENCE
							XmlSpecialChars::reduce(pValue);
						#endif // NO_XML_ESCAPE_SEQUENCE
							attribNode->setString(pValue);
						}
					}
				}
			}

			lastPeek = stream.peek();
		}
		else if (!stream)
		{
			ERROR_MSG( "XMLSection::createFromStream: Unterminated tag %s\n",
				(pCurrNode != NULL) ? pCurrNode->sectionName().c_str() : "NULL" );
			isInError = true;
		}
		else if (pCurrNode == NULL)
		{
			ERROR_MSG( "XMLSection::createFromStream: "
					"File must start with a tag\n" );
			isInError = true;
		}
		else
		{
			// Read in the value for the current section
			int index = 0;
			//char * pStringEnd = value;
			int valueBeg = stream.tellg();
			int valueEnd = valueBeg;

			while (stream && stream.peek() != '<')
			{
				currChar = stream.get();
				//value[ index++ ] = currChar;
				index++;

				// This is used to trim trailing white space.
				if (!isWhiteSpace( currChar ))
				//	pStringEnd = value + index;	// index already incremented
					valueEnd = valueBeg + index;
			}

			// Need to set here because we're about to set it to '\0'.
			lastPeek = stream.peek();

			if (stream)
			{
				bigbuf[ valueEnd ] = 0;

				if (pCurrNode->cval_ != NULL)
				{
					ERROR_MSG( "XMLSection::createFromStream: "
							"Trying to set the value twice\n"
							"Tag = \"%s\"\n"
							"Old value = \"%s\"\n"
							"New value = \"%s\"\n",
							pCurrNode->sectionName().c_str(),
							pCurrNode->asString().c_str(),
							bigbuf+valueBeg );

					isInError = true;
				}
				else
				{
					//uint64 st = timestamp();
					//pCurrNode->value( value );

#ifndef NO_XML_ESCAPE_SEQUENCE
					XmlSpecialChars::reduce( bigbuf + valueBeg );
#endif // NO_XML_ESCAPE_SEQUENCE
					pCurrNode->cval_ = bigbuf + valueBeg;

					//vsetTime += timestamp() - st;
				}
			}
			else
			{
				ERROR_MSG( "XMLSection::createFromStream: "
						"Unterminated tag %s\n",
						pCurrNode ? pCurrNode->sectionName().c_str() : "NULL" );
				isInError = true;
			}
		}
	}
	while (((pCurrNode != NULL) ||(pRootNode == NULL)) && !isInError);

	if (isInError)
	{

		while (!s_parseStack->empty())
		{
			s_parseStack->pop_back();
		}

		while (!s_parseKids->empty())
		{
			delete s_parseKids->back();
			s_parseKids->pop_back();
		}

		pRootNode = (XMLSection*)NULL;
	}

	if (pRootNode)
	{
		if (!rootTag.empty())
		{
			pRootNode->ctag_ = NULL;
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				pRootNode->tag_.capacity(), 0 )
#endif
			pRootNode->tag_.assign( rootTag.data(), rootTag.size() );
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::MP_SYSTEM, ResourceCounters::RT_OTHER),
				pRootNode->tag_.capacity(), 0 )
#endif
		}
	}

	/*
	fullTime = timestamp() - fullTime;
	if (fullTime > stampsPerSecond() / 20)
	{
		std::strstream ss( g_longTime, sizeof(g_longTime) );
		ss << "full " << NiceTime( fullTime );
		ss << " add node " << NiceTime( addnodeTime );
		ss << " set value " << NiceTime( vsetTime );
		ss << " nnodes " << nnodes;
		ss.str();
	}
	*/

	return pRootNode;
}

/**
 * This method saves the tree rooted at this node to the input ostream.
 *
 * @param stream		The output stream to write to.
 * @param level	The indentation level to use.
 *
 * @return		True if successful, otherwise false.
 */
bool XMLSection::writeToStream( std::ostream & stream, int level ) const
{
	BW_GUARD;
	// Print the opening tag.
	for (int i = 0; i < level; i++)
		stream << '\t';

	BW::string sectionTag;
	if (level == 0 && !this->isValidSectionName())
	{
		sectionTag = "root";
	}
	else
	{
		sectionTag = this->sectionName();
	}

	stream << '<' << sectionTag;

	Children::const_iterator iter = children_.begin();
	bool hasNormalChildren = false;

	typedef BW::set<BW::string> AttribSetType;
	AttribSetType attributeMap;

	if (s_shouldWriteXMLAttributes)
	{
		while (iter != children_.end())
		{
			if ((*iter)->isAttribute_)
			{
				std::pair<AttribSetType::iterator, bool> insertResult = 
					attributeMap.insert( (*iter)->sectionName() );

				if (!insertResult.second)
				{
					ERROR_MSG( "XMLSection::writeToStream: "
						"The attribute \"%s\" can not be saved as an attribute "
						"because an attribute already exists with its name.",
							(*iter)->sectionName().c_str() );
					(*iter)->isAttribute_ = false;
					hasNormalChildren = true;
				}
				else
				{
					stream << '\n';
					for (int i = 0; i < level + 2; i++)
					{
						stream << '\t';
					}

					stream << (*iter)->sectionName();
					BW::string attribValue = (*iter)->cval_ ?
						BW::string((*iter)->cval_) : (*iter)->value_;
					#ifndef NO_XML_ESCAPE_SEQUENCE
					if (!noXMLEscapeSequence_)
					{
						attribValue = XmlSpecialChars::expand( attribValue.c_str() );
					}
					#endif // NO_XML_ESCAPE_SEQUENCE
					stream << "=\"" << attribValue.c_str() << '"';
				}
			}
			else
			{
				hasNormalChildren = true;
			}
			iter++;
		}
	}
	else
	{
		hasNormalChildren = !children_.empty();
	}

	BW::string value = cval_ ? BW::string(cval_) : value_;

	if (!hasNormalChildren && value.empty())
	{
		// Using empty tag
		stream << " />" << std::endl;
		return true;
	}

	stream << '>';

	// Print the value of this section.
	//

	if (value != "")
	{
#ifndef NO_XML_ESCAPE_SEQUENCE
		if (!noXMLEscapeSequence_)
		{
			value = XmlSpecialChars::expand( value.c_str() );
		}
#endif // NO_XML_ESCAPE_SEQUENCE
		stream << '\t' << value.c_str();
	}

	// Print the sub section children.
	iter = children_.begin();
	if (iter != children_.end())
	{
		stream << std::endl;

		while (iter != children_.end())
		{
			if (!(*iter)->isAttribute() || !s_shouldWriteXMLAttributes)
			{
				(*iter)->writeToStream( stream, level + 1 );
			}

			iter++;
		}

		for (int i = 0; i < level; i++)
		{
			stream << '\t';
		}
	}
	else
	{
		stream << '\t';
	}

	// Print the closing tag.
	stream << "</" << sectionTag << '>' << std::endl;

	return true;
}

/**
 * 	This method saves the given section to a file.
 */
bool XMLSection::save( const BW::StringRef & saveAsFileName )
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
		ERROR_MSG( "XMLSection: Can't save a section without a parent.\n" );
		return false;
	}

	return parent_->saveChild( this, false );
}


/**
 * 	This method saves the given section to a file only if it already exists.
 */
bool XMLSection::saveIfExists( const BW::StringRef & saveAsFileName )
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
		ERROR_MSG( "XMLSection: Can't save a section without a parent.\n" );
		return false;
	}

	return parent_->saveChildIfExists( this, false );
}


/**
 *	This static method replaces space characters with periods.
 *
 *	@param val				The string to sanitise.
 *	@return					The sanitised string.
 */
BW::string XMLSection::sanitise( const BW::StringRef & val ) const
{
	BW_GUARD;
	return SanitiseHelper::substringReplace(val);
}


/**
 *	This static method replaces period characters with spaces.
 *
 *	@param val				The string to sanitise.
 *	@return					The sanitised string.
 */
BW::string XMLSection::unsanitise( const BW::StringRef & val ) const
{
	BW_GUARD;
	return SanitiseHelper::substringReplace(
		val, SanitiseHelper::SANITISING_TOKEN, SanitiseHelper::SPACE_TOKEN);
}



/**
 *	This function sanitises the sectionName.
 */
bool XMLSection::sanitiseSectionName()
{
	BW_GUARD;
	// Switch to using a BW::string if necessary
	if (ctag_ != NULL)
	{
		tag_ = ctag_;
		ctag_ = NULL;
	}
	BW::string newName = sanitise(tag_);
	if (newName[0] != '\0' && newName[0]>=0 && ::isdigit(newName[0]))
		newName = "id." + newName;
	if (newName != tag_)
	{
		tag_ = newName;
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	This function returns whether the section name is valid.
 */
bool XMLSection::isValidSectionName() const
{
	BW_GUARD;
	if (ctag_ != NULL)
		return isValidXMLTag(ctag_);
	else
		return isValidXMLTag(tag_.c_str());
}


/**
 *	This static method decodes a wide string that was encoded using
 *	XMLSection::encodeWideString()
 *
 *	@see XMLSection::encodeWideString
 *	@param	val				The encoded representation of the wide string.
 *	@return	BW::wstring	The decoded wide string.
 */
BW::wstring XMLSection::decodeWideString( const BW::string& val )
{
	BW_GUARD;
	if ( !val.size() )
		return L"";

	return bw_utf8tow( val );
}


/**
 *	This static method encodes a wide string into a string that is xml-friendly.
 *	This string can be decoded using XMLSection::decodeWideString
 *
 *	@see XMLSection::decodeWideString
 *	@param	val				The encoded representation of the wide string.
 *	@return	BW::wstring	The decoded wide string.
 */
BW::string XMLSection::encodeWideString( const BW::wstring& val )
{
	BW_GUARD;
	return bw_wtoutf8( val );
}


/**
 *	This method sets if the XMLSection is an attribute or a standard subsection.
 *
 *	@param	isAttribute			True if the XMLSection should be an attribute,
 *								false otherwise
 *	@return	bool				True if the value was set, false otherwise
 */
bool XMLSection::isAttribute( bool isAttribute )
{
	if (this->countChildren() != 0)
	{
		return false;
	}

	isAttribute_ = isAttribute;
	return true;
}


bool XMLSection::noXMLEscapeSequence( bool noXMLEscapeSequence )
{
	noXMLEscapeSequence_ = noXMLEscapeSequence;
	return true;
}


XMLSectionTagCheckingStopper::XMLSectionTagCheckingStopper()
{
	BW_GUARD;
	s_shouldCheckXMLTag = false;
}

XMLSectionTagCheckingStopper::~XMLSectionTagCheckingStopper()
{
	BW_GUARD;
	s_shouldCheckXMLTag = true;
}

BW_END_NAMESPACE

// xml_section.cpp

