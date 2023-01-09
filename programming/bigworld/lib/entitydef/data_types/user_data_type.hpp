#ifndef USER_DATA_TYPE_HPP
#define USER_DATA_TYPE_HPP

#include "entitydef/data_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to represent the data type that is described and
 *	implemented in script.
 *
 *	An example:
 *	<code>
 *	import cPickle
 *
 *	class TestDataType:
 *		def addToStream( self, test ):
 *			return cPickle.dumps( test )
 *
 *		def addToSection( self, test, section ):
 *			for i in test.items():
 *				section.writeInt( i[0], i[1] )
 *
 *		def createFromStream( self, stream ):
 *			return cPickle.loads( stream )
 *
 *		def createFromSection( self, section ):
 *			result = {}
 *			for i in section.values():
 *				result[ i.name ] = i.asInt
 *			return result
 *
 *		def fromStreamToSection( self, stream, section ):	# optional
 *			o = self.createFromStream( stream )
 *			self.addToSection( o, section )
 *
 *		def fromSectionToStream( self, section):			# optional
 *			o = self.createFromSection( section )
 *			return self.addToStream( o )
 *
 *		def defaultValue( self ):
 *			return { "ONE" : 1, "TWO" : 2 }
 *
 *	instance = TestDataType()
 *	</code>
 *
 *	@ingroup entity
 */
class UserDataType : public DataType
{
	public:
		/**
		 *	Constructor.
		 *
		 *	@param pMeta			The meta data type.
		 *	@param moduleName		The name of the module to find the instance.
		 *	@param instanceName	The name of the instance that implements the
		 *							data type.
		 */
		UserDataType( MetaDataType * pMeta,
				const BW::string & moduleName,
				const BW::string & instanceName ) :
			DataType( pMeta, /*isConst:*/false ), // = do not reuse default
			moduleName_( moduleName ),
			instanceName_( instanceName ),
			isInited_( false )
		{
		}

		~UserDataType();

		const BW::string& moduleName() const { return moduleName_; }
		const BW::string& instanceName() const { return instanceName_; }

		bool addObjectToStream( const ScriptObject & object,
			BinaryOStream & stream, bool isPersistentOnly ) const;
		bool createObjectFromStream( ScriptObject & object,
			BinaryIStream & stream, bool isPersistentOnly ) const;

		bool addObjectToSection( const ScriptObject & object,
			DataSectionPtr pSection ) const;
		bool createObjectFromSection( ScriptObject & object,
			DataSectionPtr pSection ) const;

	protected:
		// Overrides the DataType method.
		virtual bool isSameType( ScriptObject /*pValue*/ );
		virtual void reloadScript();
		virtual void clearScript();
		virtual void setDefaultValue( DataSectionPtr pSection );
		virtual bool getDefaultValue( DataSink & output ) const;
		virtual DataSectionPtr pDefaultSection() const { return pDefaultSection_; }
		virtual int streamSize() const;
		virtual bool addToSection( DataSource & source,
				DataSectionPtr pSection ) const;
		virtual bool createFromSection( DataSectionPtr pSection,
				DataSink & sink ) const;
		virtual bool fromStreamToSection( BinaryIStream & stream,
				DataSectionPtr pSection, bool isPersistentOnly ) const;
		virtual bool fromSectionToStream( DataSectionPtr pSection,
				BinaryOStream & stream, bool isPersistentOnly ) const;
		virtual void addToMD5( MD5 & md5 ) const;
		virtual StreamElementPtr getStreamElement( size_t index,
			size_t & size, bool & isNone, bool isPersistentOnly ) const;
		virtual bool operator<( const DataType & other ) const;
		virtual BW::string typeName() const;

	private:
		void init();

		ScriptObject pObject() const
		{
			if (!isInited_)
			{
				const_cast< UserDataType * >( this )->init();
			}

			return pObject_;
		}

		ScriptObject method( const char * name ) const;

		BW::string moduleName_;
		BW::string instanceName_;

		bool isInited_;
		ScriptObject pObject_;
		DataSectionPtr pDefaultSection_;
};

BW_END_NAMESPACE

#endif // USER_DATA_TYPE_HPP
