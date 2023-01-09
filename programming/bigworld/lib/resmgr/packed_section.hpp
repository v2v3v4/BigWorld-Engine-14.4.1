/**
 * An implementation of a packed, binary DataSection.
 */

#ifndef PACKED_SECTION_HPP
#define PACKED_SECTION_HPP

#include "binary_block.hpp"
#include "datasection.hpp"

#include "cstdmf/debug.hpp"

BW_BEGIN_NAMESPACE

namespace PackedSectionData
{
typedef uint8 VersionType;
typedef int16 NumChildrenType;
typedef int32 DataPosType;
typedef int16 KeyPosType;
}

enum SectionType
{
	DATA_POS_MASK = 0x0fffffff,
	TYPE_MASK     = ~DATA_POS_MASK,

	TYPE_DATA_SECTION	= 0x00000000,
	TYPE_STRING			= 0x10000000,
	TYPE_INT			= 0x20000000,
	TYPE_FLOAT			= 0x30000000, // Used for Vector[234] and Matrix too.
	TYPE_BOOL			= 0x40000000,
	TYPE_BLOB			= 0x50000000,
	TYPE_ENCRYPTED_BLOB	= 0x60000000,
	TYPE_ERROR			= 0x70000000,
};



/**
 *	This class is used to represent a file containing packed, binary sections.
 */
class PackedSectionFile : public SafeReferenceCount
{
public:
	PackedSectionFile( const BW::StringRef & name, BinaryPtr pFileData ) :
		storedName_( name.data(), name.size() ),
		pFileData_( pFileData )
	{
	}

	DataSectionPtr createRoot();

	const char * getIndexedString( PackedSectionData::KeyPosType key ) const
	{
		return stringTable_.getString( key );
	}

	BinaryPtr pFileData() const { return pFileData_; }

private:

	/**
	 *	This class is used to store the string table associated with this file.
	 */
	class StringTable
	{
	public:
		int init( const char * pData, int dataLen );
		const char * getString( PackedSectionData::KeyPosType key ) const;

	private:
		BW::vector< const char * > table_;
	};

	BW::string storedName_;
	StringTable	stringTable_;
	BinaryPtr pFileData_;
};

typedef SmartPointer< PackedSectionFile > PackedSectionFilePtr;


/**
 *	This class is used to represent a packed, binary section that has children.
 */
class PackedSection : public DataSection
{
public:
	static DataSectionPtr create( const BW::StringRef & tag, BinaryPtr pFile );

	// Helper methods for res_packer.
	static DataSectionPtr openDataSection( const BW::string & path );
	static bool convert( DataSectionPtr pDS, const BW::string & path );
	static bool convert( const BW::string & inPath,
			const BW::string & outPath,
			BW::vector< BW::string > * pStripStrings = NULL,
			bool shouldEncrypt = false,
			int stripRecursionLevel = 1 );

	PackedSection( const char * name, const char * pData, int dataLen,
			SectionType type, PackedSectionFilePtr pFile );
	~PackedSection();

	///	@name Section access functions.
	//@{
	virtual int countChildren();
	virtual DataSectionPtr openChild( int index );
	virtual DataSectionPtr newSection( const BW::StringRef &tag,
		DataSectionCreator* creator=NULL );
	virtual DataSectionPtr findChild( const BW::StringRef &tag,
		DataSectionCreator* creator );
	virtual void delChild(const BW::StringRef &tag );
	virtual void delChild(DataSectionPtr pSection);
	virtual void delChildren();
	//@}

	virtual BW::string sectionName() const;
	virtual int bytes() const;

	virtual bool isPacked() const	{ return true; }

	virtual bool save( const BW::StringRef& filename = "" );

	// creator
	static DataSectionCreator* creator();
protected:
	// Value access overrides from DataSection.
	virtual bool asBool( bool defaultVal );
	virtual int asInt( int defaultVal );
	virtual unsigned int asUInt( unsigned int defaultVal );
	virtual long asLong( long defaultVal );
	virtual int64	asInt64( int64 defaultVal );
	virtual uint64	asUInt64( uint64 defaultVal );
	virtual float asFloat( float defaultVal );
	virtual double asDouble( double defaultVal );
	virtual BW::string asString( const BW::StringRef &defaultVal, int flags );
	virtual BW::wstring asWideString( const BW::WStringRef &defaultVal, int flags );
	virtual Vector2 asVector2( const Vector2 &defaultVal );
	virtual Vector3 asVector3( const Vector3 &defaultVal );
	virtual Vector4 asVector4( const Vector4 &defaultVal );
	virtual Matrix asMatrix34( const Matrix &defaultVal );
	virtual BW::string asBlob( const BW::StringRef &defaultVal );
	virtual BinaryPtr asBinary();

public:
#pragma pack( push, 1 )
	/**
	 * TODO: to be documented.
	 */
	class ChildRecord
	{
	public:
		typedef PackedSectionData::DataPosType DataPosType;
		typedef PackedSectionData::KeyPosType KeyPosType;

		DataSectionPtr createSection( const PackedSection * pSection ) const;
		bool nameMatches( const PackedSectionFile & parentFile,
				const BW::StringRef & tag ) const;

		const char * getName( const PackedSectionFile & parentFile ) const;

		DataPosType startPos() const	{ return dataPos_ & DATA_POS_MASK; }
		DataPosType endPos() const		{ return (this + 1)->startPos(); }

		SectionType type() const
			{ return SectionType((this + 1)->dataPos_ & TYPE_MASK); }

		void setKeyPos( KeyPosType keyPos )	{ keyPos_ = keyPos; }

		void setEndPosAndType( DataPosType dataPos, SectionType type )
		{
			(this + 1)->dataPos_ = dataPos | type;
		}

	private:
		DataPosType dataPos_;
		KeyPosType keyPos_;
	};
#pragma pack( pop )
    friend class ChildRecord;
private:
	const ChildRecord * pRecords() const;
	const char * getDataBlock() const			{ return pOwnData_; }
	const PackedSectionFilePtr pFile() const	{ return pFile_; }

	bool isMatrix() const
	{
		return (ownDataType_ == TYPE_FLOAT) &&
				(ownDataLen_ == sizeof( float ) * 4 * 3);
	}


protected:
	const char *	name_;

	// Own data is the value of this section.
	int				ownDataLen_;
	const char *	pOwnData_;
	SectionType		ownDataType_;

	// Total data is used if this section has children. It represents all of the
	// data.
	int				totalDataLen_;
	const char *	pTotalData_;
	const PackedSectionFilePtr pFile_;
};

BW_END_NAMESPACE

#endif // PACKED_SECTION_HPP

