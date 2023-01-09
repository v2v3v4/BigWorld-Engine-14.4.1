#ifndef MAILBOX_DATA_TYPE_HPP
#define MAILBOX_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"


BW_BEGIN_NAMESPACE

class EntityMailBoxRef;

/**
 *	This class is used to represent a mailbox data type.
 *
 *	@ingroup entity
 */
class MailBoxDataType : public DataType
{
public:
	MailBoxDataType( MetaDataType * pMeta );

protected:
	virtual bool isSameType( ScriptObject pValue );
	virtual void setDefaultValue( DataSectionPtr pSection );
	virtual bool getDefaultValue( DataSink & output ) const;

	virtual int streamSize() const;

	virtual bool addToSection( DataSource & source,
			DataSectionPtr pSection ) const;

	virtual bool createFromSection( DataSectionPtr pSection,
			DataSink & sink ) const;

	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool /*isPersistentOnly*/ ) const;

	virtual bool fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool /*isPersistentOnly*/ ) const;

	virtual void addToMD5( MD5 & md5 ) const;

	virtual StreamElementPtr getStreamElement( size_t index,
		size_t & size, bool & isNone, bool isPersistentOnly ) const;

	virtual int clientSafety() const { return CLIENT_UNUSABLE; }

	virtual bool operator<( const DataType & other ) const;

private:
	/// Helper method from Python
	static void from( ScriptObject pObject, EntityMailBoxRef & mbr );

	/// Helper method to Python
	static void to( const EntityMailBoxRef & mbr, ScriptObject & pObject );

	/// Helper method from DataSource
	static void from( DataSource & source, EntityMailBoxRef & mbr );

	/// Helper method to DataSink
	static void to( const EntityMailBoxRef & mbr, DataSink & sink );

	/// Helper method from Stream
	static void from( BinaryIStream & stream, EntityMailBoxRef & mbr );

	/// Helper method to Stream
	static void to( const EntityMailBoxRef & mbr, BinaryOStream & stream );

	/// Helper method from Section
	static void from( DataSectionPtr pSection, EntityMailBoxRef & mbr );

	/// Helper method to Section
	static void to( const EntityMailBoxRef & mbr, DataSectionPtr pSection );
};

BW_END_NAMESPACE

#endif // MAILBOX_DATA_TYPE_HPP
