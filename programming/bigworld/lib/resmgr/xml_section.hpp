/*
 * A class that encapsulates the document into data sections. The class allows
 * manipulation and access to elements by hierarchical data sections.
 */

#ifndef XML_SECTION_HPP
#define XML_SECTION_HPP

#include "datasection.hpp"
#include "dataresource.hpp"

#include "cstdmf/bw_namespace.hpp"

#include "math/vector2.hpp"
#include "math/vector3.hpp"

BW_BEGIN_NAMESPACE

/**
 * We now refer to XMLSection objects using smart pointers.
 */

class XMLSection;
class IFileSystem;
typedef SmartPointer<XMLSection> XMLSectionPtr;

class XMLSectionTagCheckingStopper
{
public:
	XMLSectionTagCheckingStopper();
	~XMLSectionTagCheckingStopper();
};
/**
 *	A specialisation of DataSection that deals with XML files.
 *
 *	Each XMLSection is tied to an XMLResource. By this, all instances of
 *	XMLSection are linked that way - change the values of one XML element and it
 *	will affect all the XMLSection instances linked to the XMLResource.
 *
 *	@see DataSection, XMLResource
 */
class XMLSection : public DataSection
{
public:
	///	@name Constructor(s).
	//@{
	///	Constructor for XMLSection.
	XMLSection( const BW::StringRef & tag );
	XMLSection( const char * tag, bool cstrToken );
	//@}

	///	@name Destructor.
	//@{
	///	Destructor for XMLSection.
	virtual ~XMLSection();
	//@}

	static XMLSectionPtr createFromFile( const char * filename,
			bool returnEmptySection = true );

	static XMLSectionPtr createFromStream(
		const BW::StringRef & tag, std::istream& astream );
	static XMLSectionPtr createFromBinary(
		const BW::StringRef & tag, BinaryPtr pBinary );

	static bool isValidXMLTag(const char *str);

	virtual void setParent( DataSectionPtr pParent );

	uint32 sizeInBytes() const;

	/// This method makes a string with spaces xml-tag friend
	BW::string sanitise( const BW::StringRef & val ) const;
	/// This method reverses what a sanitise does
	BW::string unsanitise( const BW::StringRef & val ) const;
	/// Sanitise the sectionName.
	bool sanitiseSectionName();
	/// Is the sectionName valid?
	bool isValidSectionName() const;

	/// This method reads a wide string from our xml-friendly representation
	static BW::wstring decodeWideString( const BW::string& val );
	/// This method encodes a wide string as our xml-friendly representation
	static BW::string encodeWideString( const BW::wstring& val );

	// creator
	static DataSectionCreator* creator();

	static bool shouldReadXMLAttributes()
		{ return s_shouldWriteXMLAttributes; }
	static void shouldReadXMLAttributes(bool value)
		{ s_shouldWriteXMLAttributes = value; }

	static bool shouldWriteXMLAttributes()
		{ return s_shouldWriteXMLAttributes; }
	static void shouldWriteXMLAttributes(bool value)
		{ s_shouldWriteXMLAttributes = value; }

	/// This method checks if the XMLSection is an attribute or not
	virtual bool isAttribute() const
		{ return isAttribute_; }

	virtual bool isAttribute( bool isAttribute );

	virtual bool noXMLEscapeSequence() const
		{ return noXMLEscapeSequence_; }

	virtual bool noXMLEscapeSequence( bool noXMLEscapeSequence );

protected:

	/// @name General virtual methods from DataSection
	//@{
	virtual DataSectionPtr findChild( const BW::StringRef & tag,
		DataSectionCreator* creator=NULL );
	virtual int countChildren();
	virtual DataSectionPtr openChild( int index );
	virtual DataSectionPtr newSection( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL );
	virtual DataSectionPtr insertSection( const BW::StringRef &tag, int index /* = -1 */ );
	virtual void delChild( const BW::StringRef & tag );
	virtual void delChild( DataSectionPtr pSection );
	virtual void delChildren();
	virtual BW::string sectionName() const;
	virtual int bytes() const;
	virtual bool save( const BW::StringRef & saveAsFileName = BW::StringRef() );
	virtual bool saveIfExists( const BW::StringRef & saveAsFileName = BW::StringRef() );
	//@}

	///	@name Virtual methods for reading this DataSection
	//@{
	virtual bool	asBool(	bool defaultVal = false );
	virtual int		asInt( int defaultVal = 0 );
	virtual unsigned int asUInt( unsigned int defaultVal = 0 );
	virtual long	asLong( long defaultVal = 0 );
	virtual int64	asInt64( int64 defaultVal = 0 );
	virtual uint64	asUInt64( uint64 defaultVal = 0 );
	virtual float	asFloat( float  defaultVal = 0.f );
	virtual double	asDouble( double defaultVal = 0.0 );
	virtual BW::string asString( const BW::StringRef& defaultVal = BW::StringRef(),
			int flags = DS_IncludeWhitespace );
	virtual BW::wstring asWideString( const BW::WStringRef& defaultVal = BW::WStringRef(),
			int flags = DS_IncludeWhitespace );
	virtual Vector2	asVector2( const Vector2& defaultVal = Vector2( 0, 0 ) );
	virtual Vector3	asVector3( const Vector3& defaultVal = Vector3( 0, 0, 0 ) );
	virtual Vector4	asVector4( const Vector4& defaultVal = Vector4( 0, 0, 0, 0 ) );
	virtual Matrix	asMatrix34( const Matrix& defaultVal = Matrix() );
	virtual BinaryPtr	asBinary();
	virtual BW::string asBlob( const BW::StringRef& defaultVal = BW::StringRef() );
	//@}

	///	@name Methods for setting the value of this DataSection.
	//@{
	virtual bool setBool( bool value );
	virtual bool setInt( int value );
	virtual bool setUInt( unsigned int value );
	virtual bool setLong( long value );
	virtual bool setInt64( int64 value );
	virtual bool setUInt64( uint64 value );
	virtual bool setFloat( float value );
	virtual bool setDouble( double value );
	virtual bool setString( const BW::StringRef &value );
	virtual bool setWideString( const BW::WStringRef &value );
	virtual bool setVector2( const Vector2 &value );
	virtual bool setVector3( const Vector3 &value );
	virtual bool setVector4( const Vector4 &value );
	virtual bool setMatrix34( const Matrix &value );
	virtual bool setBlob( const BW::StringRef &value );
	//@}


	bool writeToStream( std::ostream & stream, int level = 0 ) const;

	friend class XMLHandle;


private:
	template <typename T>
	bool checkConversion( T t, const char * str, const char * methodName );

	///	@name Private Helper Methods.
	//@{
	/// This method strips the whitespace from a string.
	char *stripWhitespace( char *inputString );

	/// This method adds a new XMLSection to the current node.
	XMLSection * addChild( const char * tag, int index = -1 );
	//@}

	static bool processBang( class WrapperStream & stream,
			XMLSection * pCurrNode );
	static bool processQuestionMark( class WrapperStream & stream );
	
	const char *cval()
	{
		if (cval_)
			return cval_;
		return value_.c_str();
	}

	typedef BW::vector< XMLSectionPtr > Children;

	const char		* ctag_;
	const char		* cval_;
	BW::string		tag_;
	BW::string		value_;

	int				numChildren_;
	bool			isAttribute_;
	bool			noXMLEscapeSequence_;

	Children		children_;
	DataSectionPtr	parent_;

	BinaryPtr		block_;

	static bool s_shouldReadXMLAttributes;
	static bool s_shouldWriteXMLAttributes;
};

BW_END_NAMESPACE

#endif

// xml_section.hpp
