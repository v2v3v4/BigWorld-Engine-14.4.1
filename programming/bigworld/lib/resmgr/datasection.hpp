#ifndef DATA_SECTION_HPP
#define DATA_SECTION_HPP

// Standard Library Headers.
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_vector.hpp"
#include <sstream>
#include <limits.h>

// Standard MF Library Headers.
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"
#include "math/matrix.hpp"
#include "cstdmf/smartpointer.hpp"

// Application Specific Headers.
#include "binary_block.hpp"
#include "data_section_census.hpp"

BW_BEGIN_NAMESPACE

/// @name Flag values for readString.
//@{
const int DS_IncludeWhitespace		= 0x0000;
const int DS_TrimWhitespace			= 0x0001;

const int DS_AppendVector			= 0x0000;
const int DS_OverwriteVector		= 0x0001;
//@}


/// @name Smart Pointers for DataSection.
//@{
class DataSection;

typedef SmartPointer<DataSection> DataSectionPtr;
//@}

/**
 *	This struct determines the default values for types returned by
 *	DataSection access methods, in the case that no good value exists.
 *	It is necessary because the default constructor for some types
 *	does not initialise its members (for speed reasons).
 */
/**
 * TODO: to be documented.
 * note: the above comment is mapping to each templated type below
 *       in doxygen.
 */
namespace Datatype {
	template <class C> struct DefaultValue
		{ static inline C val() { return C(); } };
	// BC: added specialisations for built-in
	// types to avoid uninitialised memory reads
	template <> struct DefaultValue<bool>
		{ static inline bool val() { return false; } };
	template <> struct DefaultValue<int>
		{ static inline int val() { return 0; } };
	template <> struct DefaultValue<long>
		{ static inline long val() { return 0; } };
	template <> struct DefaultValue<float>
		{ static inline float val() { return 0; } };
	template <> struct DefaultValue<double>
		{ static inline double val() { return 0; } };
	template <> struct DefaultValue<Vector2>
		{ static inline Vector2 val() { return Vector2::zero(); } };
	template <> struct DefaultValue<Vector3>
		{ static inline Vector3 val() { return Vector3::zero(); } };
	template <> struct DefaultValue<Vector4>
		{ static inline Vector4 val() { return Vector4::zero(); } };
}
/**
 *	This class is used for iterating over the contents of a data section.
 *	It works the same way as an STL iterator.
 */
class DataSectionIterator
{
public:
	DataSectionIterator();
	DataSectionIterator(const DataSectionPtr & dataSection, int index);

	bool operator==(const DataSectionIterator & dataSection) const;
	bool operator!=(const DataSectionIterator & dataSection) const;

	DataSectionPtr			operator*();
	DataSectionIterator		operator++(int);
	const DataSectionIterator &	operator++();
	// No longer supported since its definition is inconsistent with operator*
	// and it is also error prone when using with PackedSection.
	// DataSectionPtr			operator->();

	BW::string tag();

private:
	DataSectionPtr	dataSection_;
	int				index_;
};


/**
 *	This struct allows arbitrary types to incorporate their DataSection
 *	storage functions into the standard as/be mechanism in DataSection.
 *
 *	A type can either make an explicit specialisation of this struct or
 *	implement the DataSection_as/be methods itself.
 */
template <class C> struct DataSectionTypeConverter
{
	static C as( DataSectionPtr pSection,
			const C & defaultVal = Datatype::DefaultValue<C>::val() )
		{ return C::DataSection_as( pSection, defaultVal ); }

	static bool be( DataSectionPtr pSection, const C & value )
		{ return C::DataSection_be( pSection, value ); }
};

/**
 *	This class is used to allow more flexible creation of data sections.
 *	This interface is implemented throughout the various data section
 *	types (e.g. BinSection::creator()). They can be used to control the
 *	type of sections being created / loaded. For example the following
 *	will created a new zip section.:
 *
 *	DataSectionPtr pDS = BWResource::openSection( "new_file.zip", true,
 *												ZipSection::creator() );
 *
 *
 *	and the following will load an xml section as a binary section:
 *
 *	DataSectionPtr pDS = BWResource::openSection( "existing_file.xml", false,
 *												BinSection::creator() );
 *
 *	The default operation of data section creation / loading has not been changed,
 *	the creators allow more flexibility. The majority of the data section creation
 *	methods will now take an optional DataSectionCreator*.
 *
 */
class DataSectionCreator
{
public:
	virtual DataSectionPtr create(DataSectionPtr pSection,
									const BW::StringRef& tag) = 0;

	virtual DataSectionPtr load(DataSectionPtr pSection,
									const BW::StringRef& tag,
									BinaryPtr pBinary = NULL) = 0;
};

/**
 *	A class that encapsulates a document into hierarchical sections. This
 *	is an interface that client code can use to read documents that have a
 *	hierarchical structure. The actual implementation of the class is handled
 *	by derived classes specialised for each type of document format. An
 *	instance of DataSection can access any tags at its top level and/or open
 *	access to other sections at its level. It cannot however, access sections
 *	above it.
 *
 *	The main tasks of a DataSection is to read in values specified by
 *	identifiers (known as tags,) and to recursively descend the document
 *	structure.
 *
 *	A section can be specified also by a path. For example, you can open a
 *	section "A", followed by opening a section "B" from sectionA, and open
 *	section "C" from sectionB. Or you can simply open "A/B/C" via the section
 *	path and the result is still sectionC being created. The path applies to
 *	the read methods as well. You can open all the relevant sections from A to
 *	C in turn to access a tag "M001" in sectionC, or you can read "A/B/C/M001"
 *	in a single read call.
 */
class DataSection : public SafeReferenceCount
{
protected:

	friend void DataSectionCensus::tryDestroy( const DataSection * );

	DataSection() {};
	virtual ~DataSection();

private:
	DataSection( const DataSection & other );
	DataSection & operator=( const DataSection & );

	virtual void destroy() const;

public:
	/// This type is the iterator for DataSection.
	typedef DataSectionIterator iterator;
	/// This type is the const iterator for DataSection.
	typedef const DataSectionIterator const_iterator;

	/**
	 * This iterator is used to iterate through child sections that matches
	 * a particular section name.
	 */
	class SearchIterator
	{
		friend class DataSection;

	public:
		// This is a bit weird. It should return a DataSection& but potentially
		// the DataSection is created only for the openChild() call so
		// we really need a DataSectionPtr to keep it alive.
		DataSectionPtr operator*()
		{ return pDataSection_->openChild( index_ ); }

		SearchIterator& operator++()
		{
			index_++;
			this->findNext();
			return *this;
		}
		SearchIterator operator++( int )
		{
			SearchIterator copy( *this );
			++(*this);
			return copy;
		}

		bool operator==( const SearchIterator& other ) const
		{
			return (index_ == other.index_);
		}

		bool operator!=( const SearchIterator& other ) const
		{
			return !this->operator==( other );
		}

	private:
		SearchIterator( DataSection& ds, const BW::StringRef& sectionName ) :
			pDataSection_( &ds ), index_( 0 ), sectionName_( sectionName.data(), sectionName.size() )
		{
			this->findNext();
		}

		// This is the end iterator
		SearchIterator() :
			pDataSection_( NULL ), index_( INT_MAX ), sectionName_()
		{}

		void findNext()
		{
			if (pDataSection_)
			{
				int numChildren = pDataSection_->countChildren();
				while (index_ < numChildren)
				{
					if (pDataSection_->childSectionName( index_ ) ==
							sectionName_)
					{
						return;
					}
					++index_;
				}
			}
			index_ = INT_MAX;
		}

		DataSection* 	pDataSection_;
		int				index_;
		BW::string		sectionName_;
	};

	// Returns the first iterator that points to a section matching the
	// sectionName.
	SearchIterator beginSearch( const BW::StringRef& sectionName )
	{
		return SearchIterator( *this, sectionName );
	}

	static const SearchIterator& endOfSearch()
	{
		static SearchIterator endOfSearchIterator;
		return endOfSearchIterator;
	}

	///	@name Pure virtual section access functions
	//@{
	/// This methods returns the number of sections under this one.
	virtual int countChildren() = 0;

	/// This method returns the index of the DataSectionPtr (-1 if not found).
	/// Only relevant for XMLSections.
	virtual int getIndex( DataSectionPtr data );

	/// This method opens a section with the given index. Note that it
	/// may return NULL for a valid index if there is an access error.
	virtual DataSectionPtr openChild( int index ) = 0;

	/// This method returns the name of the child with the given index.
	virtual BW::string childSectionName( int index );

	///	This method creates a new section directly under the current section.
	/// It should be the same type as the parent section, or NULL if not
	/// appropriate.
	virtual DataSectionPtr newSection( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL ) = 0;

	/// This method allows a new data section to be created at the index specified.
	/// It should be the same type as the parent section, or NULL if not
	/// appropriate. It is only relevant for XMLSections.
	virtual DataSectionPtr insertSection( const BW::StringRef & /* tag */,
			int /* index */ )
		{ return NULL; }

	/// This method searches for a new section directly under the current
	/// section.
	virtual DataSectionPtr findChild( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL ) = 0;

	/// This method deletes a child directly under the current section.
	virtual void delChild (const BW::StringRef &tag ) = 0;

	/// This method deletes a child directly under the current section.
	virtual void delChild (DataSectionPtr pSection) = 0;

	/// This method deletes all children.
	virtual void delChildren() = 0;

	///	This method returns the section's name.
	virtual BW::string sectionName() const = 0;

	/// This method returns the approx size in bytes used by this section
	virtual int bytes() const = 0;

	/// This method saves the datasection.
	virtual bool save( const BW::StringRef & saveAsFileName = BW::StringRef() ) = 0;

	/// This method saves the datasection only if it already exists.
	virtual bool saveIfExists( const BW::StringRef & saveAsFileName = BW::StringRef() )
		{ return false; }

	/// This method saves the given child of this data section.
	virtual bool saveChild( DataSectionPtr /*pChild*/, bool /*isBinary*/ )
		{ return false; }

	/// This method saves the given child of this data section, only if it already exists.
	virtual bool saveChildIfExists( DataSectionPtr /*pChild*/, bool /*isBinary*/ )
		{ return false; }

	/// This method sets a DataSection's parent section.
	/// This is only set on DataSections that want to allow a 'save'
	/// call without arguments to work on them
	virtual void setParent( DataSectionPtr /*pParent*/ ) { }

	// This method returns a copy of the data section, converted to zip
	virtual DataSectionPtr convertToZip(
			const BW::StringRef & /* saveAs */ = BW::StringRef(),
			DataSectionPtr /* pRoot */ = NULL,
			BW::vector< DataSectionPtr > * /* children */ = NULL )
		{ return this; }

	virtual bool canPack() const	{ return true; }
	virtual bool isPacked() const	{ return false; }

	/// This method can only be used on XMLSections, it is used to check if
	/// the DataSection is an attribute.
	virtual bool isAttribute() const
		{ return false; }

	/// This method can only be used on XMLSections, it is used to change
	/// a DataSection to an attribute.
	virtual bool isAttribute( bool isAttribute )
		{ return false; }

	/// This method can only be used on XMLSections, it is used to check if
	/// the DataSection should encode escape sequence characters
	virtual bool noXMLEscapeSequence() const
		{ return false; }

	/// This method can only be used on XMLSections, it is used to change
	/// whether a DataSection should encode escape sequence characters
	virtual bool noXMLEscapeSequence( bool noXMLEscapeSequence )
		{ return false; }

	//@}

	///	@name Non-virtual section access functions.
	//@{
	///	This method opens a section with the name tag specified.
	DataSectionPtr openSection( const BW::StringRef &tagPath,
									bool makeNewSection = false,
									DataSectionCreator* creator = NULL);

	///	This method opens the first sub-section within the section.
	DataSectionPtr openFirstSection( void );

	///	This method opens a vector of sections with the name tag specified.
	void openSections( const BW::StringRef &tagPath,
		BW::vector<DataSectionPtr> &dest,
		DataSectionCreator* creator = NULL);

	///	This method deletes the specified section under the current section.
	bool deleteSection( const BW::StringRef &tagPath );

	/// This methods deletes all sections with the specified name under the
	/// current section
	void deleteSections( const BW::StringRef &tagPath );

	/// This method returns an iterator pointing to the first element.
	DataSectionIterator begin();

	/// This method returns an iterator pointing after the last element.
	DataSectionIterator end();
	//@}

	/// @name Methods for reading the value of the DataSection
	//@{
	template <class C> C as()
		{ return DataSectionTypeConverter<C>::as( this ); }
	template <class C> C as( const C & defaultVal )
		{ return DataSectionTypeConverter<C>::as( this, defaultVal ); }

	/// This method reads in the value of the DataSection as a boolean.
	virtual bool asBool( bool defaultVal = Datatype::DefaultValue<bool>::val() );

	/// This method reads in the value of the DataSection as an integer.
	virtual int asInt( int defaultVal = Datatype::DefaultValue<int>::val() );

	/// This method reads in the value of the DataSection as an unsigned integer.
	virtual unsigned int asUInt( unsigned int defaultVal = Datatype::DefaultValue<unsigned int>::val() );

	/// This method reads in the value of the DataSection as a long integer.
	virtual long asLong( long defaultVal = Datatype::DefaultValue<long>::val() );

	/// This method reads in the value of the DataSection as a 64 bits integer.
	virtual int64 asInt64( int64 defaultVal = Datatype::DefaultValue<int64>::val() );

	/// This method reads in the value of the DataSection as an unsigned 64 bits integer.
	virtual uint64 asUInt64( uint64 defaultVal = Datatype::DefaultValue<uint64>::val() );

	/// This method reads in the value of the DataSection as a float.
	virtual float asFloat( float defaultVal = Datatype::DefaultValue<float>::val() );

	/// This method reads in the value of the DataSection as a double.
	virtual double asDouble( double defaultVal = Datatype::DefaultValue<double>::val() );

	/// This method reads in the value of the DataSection as a string.
	virtual BW::string asString(
			const BW::StringRef &defaultVal = Datatype::DefaultValue<BW::StringRef>::val(),
			int flags = DS_IncludeWhitespace );

	/// This method reads in the value of the DataSection as a wide string.
	virtual BW::wstring asWideString(
			const BW::WStringRef &defaultVal = Datatype::DefaultValue<BW::WStringRef>::val(),
			int flags = DS_IncludeWhitespace );

	/// This method reads in the value of the DataSection as a 2D vector.
	virtual Vector2 asVector2( const Vector2 &defaultVal =
			Datatype::DefaultValue<Vector2>::val() );

	/// This method reads in the value of the DataSection as a 3D vector.
	virtual Vector3 asVector3( const Vector3 &defaultVal =
			Datatype::DefaultValue<Vector3>::val() );

	/// This method reads in the value of the DataSection as a 4D vector.
	virtual Vector4 asVector4( const Vector4 &defaultVal =
			Datatype::DefaultValue<Vector4>::val() );

	/// This method reads in the value of the DataSection as a 3x4 matrix.
	virtual Matrix asMatrix34( const Matrix &defaultVal =
			Datatype::DefaultValue<Matrix>::val() );

	/// This method returns the value of the DataSection as binary data.
	virtual BinaryPtr asBinary();

	/// This method reads in the value of the DataSection as a BLOB.
	virtual BW::string asBlob(
			const BW::StringRef &defaultVal = Datatype::DefaultValue<BW::StringRef>::val() );
	//@}

	///	@name Methods for setting the value of the DataSection
	//@{
	template <class C> bool be( const C & value )
		{ return DataSectionTypeConverter<C>::be( this, value ); }

	/// This method sets the datasection to a boolean value.
	virtual bool setBool( bool value );

	/// This method sets the datasection to an integer value.
	virtual bool setInt( int value );

	/// This method sets the datasection to an uint value.
	virtual bool setUInt( unsigned int value );

	/// This method sets the datasection to a long value.
	virtual bool setLong( long value );

	/// This method sets the datasection to an int64 value.
	virtual bool setInt64( int64 value );

	/// This method sets the datasection to an uint64 value.
	virtual bool setUInt64( uint64 value );

	/// This method sets the datasection to a floating-point value.
	virtual bool setFloat( float value );

	/// This method sets the datasection to a double value.
	virtual bool setDouble( double value );

	/// This method sets the datasection to a string value.
	virtual bool setString( const BW::StringRef &value );

	/// This method sets the datasection to a wide string value.
	virtual bool setWideString( const BW::WStringRef &value );

	/// This method sets the datasection to a Vector2 value.
	virtual bool setVector2( const Vector2 &value );

	/// This method sets the datasection to a Vector3 value.
	virtual bool setVector3( const Vector3 &value );

	/// This method sets the datasection to a Vector4 value.
	virtual bool setVector4( const Vector4 &value );

	/// This method sets the datasection to a Matrix34 value.
	virtual bool setMatrix34( const Matrix &value );

	/// This methods stores binary data in the given datasection.
	virtual bool setBinary( BinaryPtr pData );

	/// This method sets the datasection to a BLOB value.
	virtual bool setBlob( const BW::StringRef &value );
	//@}


	///	@name Methods of reading in values of document elements.
	//@{
	template <class C> C read( const BW::StringRef & tagPath )
	{
		DataSectionPtr pSection = this->openSection( tagPath, false );

		if (!pSection) return Datatype::DefaultValue<C>::val();
		return pSection->as<C>();
	}

	template <class C> C read( const BW::StringRef & tagPath,
		const C & defaultVal )
	{
		DataSectionPtr pSection = this->openSection( tagPath, false );

		if (!pSection) return defaultVal;
		return pSection->as<C>( defaultVal );
	}

	/// This method reads in the value of the specified tag as a boolean value.
	bool readBool( const BW::StringRef &tagPath,
			bool defaultVal = Datatype::DefaultValue<bool>::val() );

	/// This method reads in the value of the specified tag as an integer
	///	value.
	int readInt( const BW::StringRef &tagPath,
			int defaultVal = Datatype::DefaultValue<int>::val() );

	/// This method reads in the value of the specified tag as a uint value.
	unsigned int readUInt( const BW::StringRef &tagPath,
			unsigned int defaultVal = Datatype::DefaultValue<unsigned int>::val() );

	/// This method reads in the value of the specified tag as a long integer
	///	value.
	long readLong( const BW::StringRef &tagPath,
			long defaultVal = Datatype::DefaultValue<long>::val() );

	/// This method reads in the value of the specified tag as a 64 bits integer
	///	value.
	int64 readInt64( const BW::StringRef &tagPath,
			int64 defaultVal = Datatype::DefaultValue<int64>::val() );

	/// This method reads in the value of the specified tag as a 64 bits uint
	///	value.
	uint64 readUInt64( const BW::StringRef &tagPath,
			uint64 defaultVal = Datatype::DefaultValue<uint64>::val() );

	/// This method reads in the value of the specified tag as a
	///	floating-point value.
	float readFloat( const BW::StringRef &tagPath,
			float defaultVal = Datatype::DefaultValue<float>::val() );

	/// This method reads in the value of the specified tag as a double
	/// floating-point value.
	double readDouble( const BW::StringRef &tagPath,
			double defaultVal = Datatype::DefaultValue<double>::val() );

	/// This method reads in the value of the specified tag as a string.
	BW::string readString( const BW::StringRef &tagPath,
			const BW::StringRef &defaultVal = Datatype::DefaultValue<BW::StringRef>::val(),
			int flags = DS_IncludeWhitespace );

	/// This method reads in the value of the specified tag as a wide string.
	BW::wstring readWideString( const BW::StringRef &tagPath,
			const BW::WStringRef &defaultVal = Datatype::DefaultValue<BW::WStringRef>::val(),
			int flags = DS_IncludeWhitespace );

	/// This method reads in the value of the specified tag as a 2D vector.
	Vector2 readVector2( const BW::StringRef &tag,
			const Vector2 &defaultVal = Datatype::DefaultValue<Vector2>::val() );

	/// This method reads in the value of the specified tag as a 3D vector.
	Vector3 readVector3( const BW::StringRef &tag,
			const Vector3 &defaultVal = Datatype::DefaultValue<Vector3>::val() );

	/// This method reads in the value of the specified tag as a 4D vector.
	Vector4 readVector4( const BW::StringRef &tag,
			const Vector4 &defaultVal = Datatype::DefaultValue<Vector4>::val() );

	/// This method reads in the value of the specified tag as a 3x4 matrix.
	Matrix readMatrix34( const BW::StringRef &tag,
			const Matrix &defaultVal = Datatype::DefaultValue<Matrix>::val() );

	/// This method reads in the value of a tag as binary data.
	BinaryPtr readBinary(const BW::StringRef &tag);

	/// This method reads in the value of the specified tag as a Blob
	BW::string readBlob( const BW::StringRef &tagPath,
			const BW::StringRef &defaultVal = Datatype::DefaultValue<BW::StringRef>::val() );
	//@}

	///	@name Methods of writing out values of document elements.
	//@{
	template <class C> bool write( const BW::StringRef & tagPath, const C & value)
	{
		DataSectionPtr pSection = this->openSection( tagPath, true );

		if (!pSection) return false;
		return pSection->be( value );
	}

	/// This method writes a boolean value to the tag specified.
	bool writeBool( const BW::StringRef &tagPath, bool value );

	/// This method writes an integer value to the tag specified.
	bool writeInt( const BW::StringRef &tagPath, int value );

	/// This method writes an unsigned integer value to the tag specified.
	bool writeUInt( const BW::StringRef &tagPath, unsigned int value );

	/// This method writes a long value to the tag specified.
	bool writeLong( const BW::StringRef &tagPath, long value );

	/// This method writes an int64 value to the tag specified.
	bool writeInt64( const BW::StringRef &tagPath, int64 value );

	/// This method writes an int64 value to the tag specified.
	bool writeUInt64( const BW::StringRef &tagPath, uint64 value );

	/// This method writes a floating-point value to the tag specified.
	bool writeFloat( const BW::StringRef &tagPath, float value );

	/// This method writes a double floating-point value to the tag specified.
	bool writeDouble( const BW::StringRef &tagPath, double value );

	/// This method writes a string value to the tag specified.
	bool writeString( const BW::StringRef &tagPath, const BW::StringRef &value );

	/// This method writes a wide string value to the tag specified.
	bool writeWideString(
			const BW::StringRef &tagPath, const BW::WStringRef &value );

	/// This method writes a Vector2 value to the tag specified.
	bool writeVector2( const BW::StringRef &tagPath, const Vector2 &value );

	/// This method writes a Vector3 value to the tag specified.
	bool writeVector3( const BW::StringRef &tagPath, const Vector3 &value );

	/// This method writes a Vector5 value to the tag specified.
	bool writeVector4( const BW::StringRef &tagPath, const Vector4 &value );

	/// This method writes a Matrix34 value to the tag specified.
	bool writeMatrix34( const BW::StringRef &tagPath, const Matrix &value );

	/// This method write binary data to the tag specified.
	bool writeBinary( const BW::StringRef& tagPath, BinaryPtr pBinary );

	/// This method writes a BLOB value to the tag specified.
	bool writeBlob( const BW::StringRef &tagPath, const BW::StringRef &value );

	//@}

	///	@name Methods of reading whole vectors of elements with the same tag.
	//@{
	template <class SEQ> void readSeq( const BW::StringRef & tagPath, SEQ & dest )
	{
		BW::StringRef tag;
		DataSectionPtr pSrc = this->splitTagPath( tagPath, tag, false );
		if (!pSrc) return;

		uint numChildren = pSrc->countChildren();
		for (uint i = 0; i < numChildren; i++)
		{
			typedef typename SEQ::value_type T;
			if (pSrc->childSectionName( i ) == tag)
				dest.push_back( pSrc->openChild( i )->as<T>() );
		}
	}

	template <class SEQ> void readSeq( const BW::StringRef & tagPath, SEQ & dest,
		bool replaceExisting )
	{
		if (replaceExisting) dest.clear();

		BW::StringRef tag;
		DataSectionPtr pSrc = this->splitTagPath( tagPath, tag, false );
		if (!pSrc) return;

		uint numChildren = pSrc->countChildren();
		for (uint i = 0; i < numChildren; i++)
		{
			typedef typename SEQ::value_type T;
			if (pSrc->childSectionName( i ) == tag)
				dest.push_back( pSrc->openChild( i )->as<T>() );
		}
	}

	/// This method reads in a vector of bools under the specified tag.
	void readBools( const BW::StringRef &tagPath, BW::vector<bool> &dest );

	/// This method reads in a vector of ints under the specified tag.
	void readInts( const BW::StringRef &tagPath, BW::vector<int> &dest );

	/// This method reads in a vector of longs under the specified tag.
	void readLongs( const BW::StringRef &tagPath, BW::vector<long> &dest );

	/// This method reads in a vector of floats under the specified tag.
	void readFloats( const BW::StringRef &tagPath, BW::vector<float> &dest );

	/// This method reads in a vector of doubles under the specified tag.
	void readDoubles( const BW::StringRef &tagPath, BW::vector<double> &dest );

	/// This method reads in a vector of strings under the specified tag.
	void readStrings( const BW::StringRef &tagPath,
			BW::vector<BW::string> &dest,
			int flags = DS_IncludeWhitespace );

	/// This method reads in a vector of wide strings under the specified tag.
	void readWideStrings( const BW::StringRef &tagPath,
			BW::vector<BW::wstring> &dest,
			int flags = DS_IncludeWhitespace );

	/// This method reads in a vector of Vector2s under the specified tag.
	void readVector2s( const BW::StringRef &tagPath, BW::vector<Vector2> &dest );

	/// This method reads in a vector of Vector3s under the specified tag.
	void readVector3s( const BW::StringRef &tagPath, BW::vector<Vector3> &dest );

	/// This method reads in a vector of Vector4s under the specified tag.
	void readVector4s( const BW::StringRef &tagPath, BW::vector<Vector4> &dest );

	/// This method reads in a vector of Matrix34s under the specified tag.
	void readMatrix34s(
			const BW::StringRef &tagPath, BW::vector<Matrix> &dest );
	//@}

	///	@name Methods of writing whole vectors of elements with the same tag.
	//@{
	template <class SEQ> void writeSeq( const BW::StringRef & tagPath, SEQ & src )
	{
		BW::StringRef tag;
		DataSectionPtr pDest = this->splitTagPath( tagPath, tag, false );
		if (!pDest) return;

		for (typename SEQ::iterator it = src.begin(); it != src.end(); it++)
		{
			pDest->newSection( tag )->be( *it );
		}
	}

	template <class SEQ> void writeSeq( const BW::StringRef & tagPath, SEQ & src,
		bool replaceExisting )
	{
		BW::StringRef tag;
		DataSectionPtr pDest = this->splitTagPath( tagPath, tag, false );
		if (!pDest) return;

		if (replaceExisting)
		{
			DataSectionPtr pOldChild;
			while ((pOldChild = pDest->findChild( tag )))
				pDest->delChild( pOldChild );
		}

		for (typename SEQ::iterator it = src.begin(); it != src.end(); it++)
		{
			pDest->newSection( tag )->be( *it );
		}
	}

	/// This method writes in a vector of bools under the specified tag.
	void writeBools( const BW::StringRef &tagPath,
			BW::vector<bool> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of ints under the specified tag.
	void writeInts( const BW::StringRef &tagPath,
			BW::vector<int> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of longs under the specified tag.
	void writeLongs( const BW::StringRef &tagPath,
			BW::vector<long> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of floats under the specified tag.
	void writeFloats( const BW::StringRef &tagPath,
			BW::vector<float> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of doubles under the specified tag.
	void writeDoubles( const BW::StringRef &tagPath,
			BW::vector<double> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of strings under the specified tag.
	void writeStrings( const BW::StringRef &tagPath,
			BW::vector<BW::string> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of wide strings under the specified tag.
	void writeWideStrings( const BW::StringRef &tagPath,
			BW::vector<BW::wstring> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of Vector2s under the specified tag.
	void writeVector2s( const BW::StringRef &tagPath,
			BW::vector<Vector2> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of Vector3s under the specified tag.
	void writeVector3s( const BW::StringRef &tagPath,
			BW::vector<Vector3> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of Vector4s under the specified tag.
	void writeVector4s( const BW::StringRef &tagPath,
			BW::vector<Vector4> &src,
			int flags = DS_OverwriteVector );

	/// This method writes in a vector of Matrix34s under the specified tag.
	void writeMatrix34s( const BW::StringRef &tagPath,
			BW::vector<Matrix> &src,
			int flags = DS_OverwriteVector );
	//@}

	void setWatcherValues( const BW::StringRef& path = "" );

	/// recursively copys everything in pSection.
	/// by default, also clear the current section and set the section
	/// name to that of the passed in section
	void copy( const DataSectionPtr pSection,
		bool modifyCurrent = true );

	/// Copies all matching sections from pSection, without clearing the
	/// current section first
	void copySections( const DataSectionPtr pSection, const BW::StringRef &tag );

	/// Copies all sections from pSection, without clearing the
	/// current section first
	void copySections( const DataSectionPtr pSection );

	/// Compares this DataSection with another. Returns 0 if equal, > 0 if
	/// this DataSection is "greater than" the other and < 0 if this
	/// DataSection is "less than" the other.
	/// This function use asString to compare internally
	int compare( DataSectionPtr pSection );

	/// Compares this DataSection with another. Returns 0 if equal, > 0 if
	/// this DataSection is "greater than" the other and < 0 if this
	/// DataSection is "less than" the other.
	/// This function use BinaryData to compare internally
	int binaryCompare( DataSectionPtr pSection );

	/// Convert a section name to a sanitised form.
	virtual BW::string sanitise( const BW::StringRef & str ) const;

	/// Undo whatever sanitise did to a section name
	virtual BW::string unsanitise( const BW::StringRef & str ) const;

	/// Sanitise the sectionName.
	virtual bool sanitiseSectionName();

	/// Is the sectionName valid?
	virtual bool isValidSectionName() const;

	/// Helper method to create a section for the given data and/or tag
	static DataSectionPtr createAppropriateSection(
		const BW::StringRef& tag, BinaryPtr pData, bool allowModifyData = true,
		DataSectionCreator* creator=NULL );

protected:

	/// Helper method to split up a save as filename into parent section and tag
	static bool splitSaveAsFileName( const BW::StringRef & fileName,
		DataSectionPtr & parent, BW::StringRef & tag );

private:

	/// Helper function used by the array read/write methods.
	DataSectionPtr splitTagPath( const BW::StringRef& tagPath, BW::StringRef& tag,
			bool makeNewSection,
			DataSectionCreator* creator = NULL );
};

// specialisations for things that already have member functions
#define BUILTIN_TEMPLATES( TYPE, AS_FUNCTION, BE_FUNCTION )					\
	/**																		\
	 * TODO: to be documented.												\
	 */																		\
	template <> struct DataSectionTypeConverter<TYPE>						\
	{																		\
		static TYPE as( DataSectionPtr pSection, const TYPE & defaultVal )	\
			{ return pSection->AS_FUNCTION( defaultVal ); }					\
		static TYPE as( DataSectionPtr pSection )							\
			{ return pSection->AS_FUNCTION(); }								\
		static bool be( DataSectionPtr pSection, const TYPE & value )		\
			{ return pSection->BE_FUNCTION( value ); }						\
	};

BUILTIN_TEMPLATES( bool, asBool, setBool )
BUILTIN_TEMPLATES( int, asInt, setInt )
BUILTIN_TEMPLATES( uint32, asUInt, setUInt )
#ifndef _LP64
// In the 64-bit server, int64 = long, so just use "long" in 32-bit OSes.
BUILTIN_TEMPLATES( long, asLong, setLong )
#endif
BUILTIN_TEMPLATES( int64, asInt64, setInt64 )
BUILTIN_TEMPLATES( uint64, asUInt64, setUInt64 )
BUILTIN_TEMPLATES( float, asFloat, setFloat )
BUILTIN_TEMPLATES( double, asDouble, setDouble )
BUILTIN_TEMPLATES( BW::string, asString, setString )
BUILTIN_TEMPLATES( BW::wstring, asWideString, setWideString )
BUILTIN_TEMPLATES( Vector2, asVector2, setVector2 )
BUILTIN_TEMPLATES( Vector3, asVector3, setVector3 )
BUILTIN_TEMPLATES( Vector4, asVector4, setVector4 )
BUILTIN_TEMPLATES( Matrix, asMatrix34, setMatrix34 )

// specialisations for signed/unsigned integers of different sizes by using
// functions of a larger integer size.
#define BUILTIN_TEMPLATES_PROMOTE( TYPE, DSTYPE, AS_FUNCTION, BE_FUNCTION )	\
	template <> struct DataSectionTypeConverter<TYPE>						\
	{																		\
		static TYPE as( DataSectionPtr pSection, const TYPE& defaultVal )	\
			{ return TYPE( pSection->AS_FUNCTION( defaultVal ) ); }			\
		static TYPE as( DataSectionPtr pSection )							\
			{ return TYPE( pSection->AS_FUNCTION() ); }						\
		static bool be( DataSectionPtr pSection, const TYPE& value )		\
			{ return pSection->BE_FUNCTION( value ); }						\
	};
BUILTIN_TEMPLATES_PROMOTE( int8, int, asInt, setInt )
BUILTIN_TEMPLATES_PROMOTE( int16, int, asInt, setInt )
BUILTIN_TEMPLATES_PROMOTE( uint8, int, asInt, setInt )
BUILTIN_TEMPLATES_PROMOTE( uint16, int, asInt, setInt )


/**
 * TODO: to be documented.
 */
template <> struct DataSectionTypeConverter<BinaryPtr>
{
	static BinaryPtr as( DataSectionPtr pSection )
		{ return pSection->asBinary(); }
	static bool be( DataSectionPtr pSection, const BinaryPtr& value )
		{ return pSection->setBinary(value); }
};


/**
 *	macro interface to DataSection write and read for member variables
 */
#define SERIALISE(DS, var, Type, load)						\
		{													\
			if (load)										\
			{												\
				var = DS->read ## Type( #var, var );		\
			}												\
			else											\
			{												\
				DS->write ## Type( #var, var );				\
			}												\
		}


#define SERIALISE_ENUM(DS, var, enumType, load)				\
		{													\
			if (load)										\
			{												\
				var = (enumType)(DS->readInt( #var, var ));	\
			}												\
			else											\
			{												\
				DS->writeInt( #var, (int)(var) );			\
			}												\
		}


/**
 *	macro interface to DataSection write and read for member functions
 */
#define SERIALISE_TO_FUNCTION(DS, func, Type, load)			\
		{													\
			if (load)										\
			{												\
				func(DS->read ## Type( #func, func() ));	\
			}												\
			else											\
			{												\
				DS->write ## Type( #func, func() );			\
			}												\
		}

BW_END_NAMESPACE

#endif // DATA_SECTION_HPP

