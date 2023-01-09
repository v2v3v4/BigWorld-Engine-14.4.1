#ifndef DATA_TYPE_HPP
#define DATA_TYPE_HPP

#include "bwentity/bwentity_api.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"
#include "script/script_object.hpp"

#include <memory>

BW_BEGIN_NAMESPACE

class BinaryOStream;
class BinaryIStream;
class DataSection;
class DataSink;
class DataSource;
class DataType;
class MD5;
class MetaDataType;
class PropertyOwnerBase;

typedef SmartPointer< DataSection > DataSectionPtr;
typedef SmartPointer< DataType > DataTypePtr;
typedef ConstSmartPointer< DataType > ConstDataTypePtr;

#ifdef EDITOR_ENABLED
class ChunkItem;
class GeneralProperty;
#endif

/**
 *	Objects derived from this class are used to describe a type of data that can
 *	be used in a data description.
 *	@ingroup entity
 */
class DataType : public ReferenceCount
{
public:
	class StreamElement;
protected:
	typedef std::auto_ptr<StreamElement> StreamElementPtr;

public:
	/**
	 *	Constructor.
	 */
	DataType( MetaDataType * pMetaDataType, bool isConst = true ) :
		pMetaDataType_( pMetaDataType ),
		isConst_( isConst )
	{
	}

	/**
	 *	Destructor.
	 */
	virtual ~DataType();

	/**
	 *	This method causes any stored script objects derived from user script
	 *	to be reloaded.
	 */
	virtual void reloadScript()
	{
	}

	/**
	 *	This method causes any stored script objects derived from user script
	 *	to be reloaded.
	 */
	virtual void clearScript()
	{
	}

	/**
	 *	This method sets the default value associated with this type. This
	 * 	value will be subsequently output by the getDefaultValue() method
	 */
	virtual void setDefaultValue( DataSectionPtr pSection ) = 0;

	/**
	 *	This method outputs the default value associated with this type
	 *	into the given DataSink.
	 */
	virtual bool getDefaultValue( DataSink & output ) const = 0;

	/**
	 *	This method returns the default section for this type as defined
	 *	in alias.xml or the entity definition files.
	 */
	virtual DataSectionPtr pDefaultSection() const;

	/**
	 *	This method returns whether the input object is of this type.
	 */
	virtual bool isSameType( ScriptObject pValue ) = 0;

	BWENTITY_API bool addToStream( DataSource & source,
		BinaryOStream & stream, bool isPersistentOnly )	const;

	BWENTITY_API bool createFromStream( BinaryIStream & stream,
		DataSink & sink, bool isPersistentOnly ) const;

	/**
	 *	This method returns the number of bytes this type needs
	 *	on a stream, or -1 if it is variable.
	 *
	 *	@return			The number of bytes needed to stream an
	 *					object of this type, or -1 if it is variable.
	 */
	BWENTITY_API virtual int streamSize() const = 0;


	/**
	 *	This method adds the DataSource of the appropriate type into the input
	 *	data section.
	 *
	 *	@param source	The source to read the value from.
	 *	@param pSection	The data section to add the value to.
	 *
	 *	@return true if succeeded, false otherwise. In the failure case, the
	 *	section is generally clobbered, and should be discarded.
	 */
	virtual bool addToSection( DataSource & source, DataSectionPtr pSection )
		const = 0;

	/**
	 *	This method outputs an object created from the given
	 *	DataSection into the given DataSink
	 *
	 *	@param pSection	The datasection to use
	 *	@param sink		The DataSink to output to
	 *
	 *	@return			true if succeeded, false otherwise.
	 */
	virtual bool createFromSection( DataSectionPtr pSection, DataSink & sink )
		const = 0;

	// DEPRECATED
	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool isPersistentOnly ) const;
	// DEPRECATED
	virtual bool fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool isPersistentOnly ) const;


	virtual ScriptObject attach( ScriptObject pObject,
		PropertyOwnerBase * pOwner, int ownerRef );
	virtual void detach( ScriptObject pObject );

	virtual PropertyOwnerBase * asOwner( ScriptObject pObject ) const;


	/**
	 *	This method adds this object to the input MD5 object.
	 */
	virtual void addToMD5( MD5 & md5 ) const = 0;


	/**
	 *	This method returns the level of safety for this type
	 *	on the client.
	 */
	BWENTITY_API virtual int clientSafety() const { return CLIENT_SAFE; }

	static DataTypePtr buildDataType( DataSectionPtr pSection );
	BWENTITY_API
	static DataTypePtr buildDataType( const BW::string & typeXML );

	BWENTITY_API enum ClientSafety
	{
		CLIENT_SAFE		= 0,
		CLIENT_UNSAFE	= 0x1,
		CLIENT_UNUSABLE	= 0x2,
	};

	/**
	 *	This class provides an interface for streaming things to and from
	 *	streams. It is expected that DataType implementations will override
	 *	this.
	 */
	class StreamElement
	{
	public:
		BWENTITY_API const DataType & getType() const { return type_; }
		BWENTITY_API bool getSize( size_t & rSize ) const;
		BWENTITY_API bool getChildType( ConstDataTypePtr & rpChildType ) const;
		BWENTITY_API bool getStructureName( BW::string & rStructureName ) const;
		BWENTITY_API bool getFieldName( BW::string & rFieldName ) const;

		BWENTITY_API bool isVariableSizedType() const;
		BWENTITY_API bool isNoneAbleType() const;
		BWENTITY_API bool isCustomStreamingType() const;
		BWENTITY_API bool isSubstreamStart() const;
		BWENTITY_API bool isSubstreamEnd() const;

		BWENTITY_API bool addToStream( DataSource & source,
			BinaryOStream & stream ) const;

		BWENTITY_API bool createFromStream( BinaryIStream & stream,
			DataSink & sink ) const;

	protected:
		StreamElement( const DataType & type );

	private:
		// Methods for subclasses to override, no upcalling required. The
		// default response is always false: "Not supported".
		/**
		 *	This method returns true if this is a fixed size container or
		 *	dictionary, and indicates the number of elements or fields in rSize;
		 *	and returns false otherwise
		 */
		virtual bool containerSize( size_t & rSize ) const
		{
			return false;
		}

		/**
		 *	This method returns true if this is a container item or dictionary
		 *	field, and sets the rpChildType to be the child type of this item or
		 *	field; and returns false otherwise.
		 */
		virtual bool childType( ConstDataTypePtr & rpChildType ) const
		{
			return false;
		}

		/**
		 *	This method returns true if this is a named typed (generally a
		 *	dictionary with fixed structure), and indicates the name of the
		 *	structure in rStructureName; and returns false otherwise.
		 */
		virtual bool dictName( BW::string & rStructureName ) const
		{
			return false;
		}

		/**
		 *	This method returns true if this is a dictionary field, and
		 *	indicates the name of the current field in rFieldName; and returns
		 *	false otherwise.
		 */
		virtual bool fieldName( BW::string & rFieldName ) const
		{
			return false;
		}

		/**
		 *	This method returns true if this is a variable-sized container or
		 *	false otherwise.
		 */
		virtual bool isVariableSizedContainer() const
		{
			return false;
		}

		/**
		 *	This method returns true if this is a type that can be replaced with
		 *	'None' for streaming efficiency, or false otherwise.
		 */
		virtual bool isNoneAble() const
		{
			return false;
		}


		/**
		 *	This method returns true if this is a custom-streaming type, and
		 *	false otherwise.
		 */
		virtual bool isCustomStreamed() const
		{
			return false;
		}


		/**
		 *	This method returns true if this is the start of a range which
		 *	should be streamed in a string and that then streamed in the
		 *	containing stream.
		 */
		virtual bool isStreamAsStringStart() const
		{
			return false;
		}


		/**
		 *	This method returns true if this is the end of a range which
		 *	should be streamed in a string and that then streamed in the
		 *	containing stream.
		 */
		virtual bool isStreamAsStringEnd() const
		{
			return false;
		}


		/**
		 *	This method reads a value of the appropriate type from the supplied
		 *	DataSource and appends it to the supplied BinaryOStream, returning
		 *	true unless an error occurs in which case it returns false.
		 */
		virtual bool fromSourceToStream( DataSource & source,
			BinaryOStream & stream ) const = 0;

		/**
		 *	This method reads a value of the appropriate type from the supplied
		 *	BinaryIStream and writes it to the supplied DataSink, returning
		 *	true, unless an error occurs in which case it returns false.
		 */
		virtual bool fromStreamToSink( BinaryIStream & stream,
			DataSink & sink ) const = 0;

		const DataType & type_;
	};

	/**
	 *	This is an iterator for outputting StreamElements needed to stream
	 *	this particular DataType instance.
	 */
	class const_iterator :
		public std::iterator< std::input_iterator_tag, const StreamElement >
	{
	public:
		// Copy-constructible, copy-assignable and destructible
		BWENTITY_API const_iterator( const const_iterator & other );
		BWENTITY_API const_iterator & operator=( const_iterator other );
		/**
		 *	Destructor
		 */
		BWENTITY_API ~const_iterator() {};

		// Can be compared meaningfully
		BWENTITY_API friend bool operator==(
			const const_iterator & lhs, const const_iterator & rhs );
		BWENTITY_API friend inline bool operator!=(
			const const_iterator & lhs,	const const_iterator & rhs )
		{
			return !(lhs == rhs);
		}

		// Can be dereferenced as an rvalue (if in a dereferenceable state)
		BWENTITY_API pointer operator->() const;
		BWENTITY_API const reference operator*() const
		{
			return *(this->operator->());
		}

		// Can be incremented (if in a dereferenceable state)
		BWENTITY_API const_iterator & operator++();
		BWENTITY_API const_iterator operator++( int );

		BWENTITY_API
		friend void swap( const_iterator & lhs, const_iterator & rhs );

		BWENTITY_API void setSize( size_t size );
		BWENTITY_API void setNone( bool isNone );
	private:
		// Constructor for use by DataType
		friend class DataType;
		const_iterator( const DataType & root, bool isPersistentOnly,
			bool isEnd );

		const DataType & root_;
		bool useChild_;
		std::auto_ptr<const_iterator> pChildIt_;
		size_t index_;
		size_t size_;
		bool isNone_;
		bool isPersistentOnly_;
		StreamElementPtr pCurrent_;
	};

	BWENTITY_API const_iterator begin() const
	{
		return const_iterator( *this, /* isPersistentOnly */ false,
			/* isEnd */ false );
	}
	BWENTITY_API const_iterator end() const
	{
		return const_iterator( *this, /* isPersistentOnly */ false,
			/* isEnd */ true );
	}

	BWENTITY_API const_iterator beginPersistent() const
	{
		return const_iterator( *this, /* isPersistentOnly */ true,
			/* isEnd */ false );
	}
	BWENTITY_API const_iterator endPersistent() const
	{
		return const_iterator( *this, /* isPersistentOnly */ true,
			/* isEnd */ true );
	}


#ifdef EDITOR_ENABLED
	virtual GeneralProperty * createEditorProperty( const BW::string& name,
		ChunkItem* chunkItem, int editorEntityPropertyId )
		{ return NULL; }

	static DataSectionPtr findAliasWidget( const BW::string& name )
	{
		AliasWidgets::iterator i = s_aliasWidgets_.find( name );
		if ( i != s_aliasWidgets_.end() )
			return (*i).second;
		else
			return NULL;
	}

	static void fini()
	{
		s_aliasWidgets_.clear();
	}
#endif

	MetaDataType * pMetaDataType()	{ return pMetaDataType_; }
	const MetaDataType * pMetaDataType() const	{ return pMetaDataType_; }

	/**
	 *	This method indicates whether this DataType can be modified internally,
	 *	and hence whether we can reuse the default value.
	 */
	bool isConst() const			{ return isConst_; }

	bool canIgnoreAssignment( ScriptObject pOldValue,
			ScriptObject pNewValue ) const;
	bool hasChanged( ScriptObject pOldValue, ScriptObject pNewValue ) const;

	// derived class should call this first then do own checks
	virtual bool operator<( const DataType & other ) const
		{ return pMetaDataType_ < other.pMetaDataType_ ; }

	BWENTITY_API virtual BW::string typeName() const;

	static void clearStaticsForReload();

	static void reloadAllScript()
	{
		callOnEach( &DataType::reloadScript );
	}

	static void clearAllScript()
	{
		callOnEach( &DataType::clearScript );
	}
	static void callOnEach( void (DataType::*f)() );

protected:
	MetaDataType * pMetaDataType_;
	bool isConst_;

private:
	/**
	 *	Does this type know how to propagate internal changes?
	 */
	virtual bool isSmart() const	{ return false; }

	// const_iterator-related virtual methods
	/**
	 *	Returns the StreamElement for the given index.
	 *
	 *	@param index	The index in the iterator
	 *	@param size		An input/output size reference, must be set before
	 *					index 1 is read.
	 *	@param isNone	An input/output boolean reference, must be set
	 *					before index 1 is read.
	 *	@param isPersistentOnly	Whether this StreamElement is part of a
	 *					persistent-only iteration.
	 *	NOTE: This may be called with increasing indexes until
	 *	StreamElementPtr() is returned.
	 *	size is not a limit on the index, it will generally be the number of
	 *	direct child data elements, and does not include non-data elements or
	 *	account for children-of-children.
	 *	NOTE: The input/output references may be updated by other calls to this
	 *	StreamElement. For example, StreamElement::addToStream and
	 *	StreamElement::createFromStream will do so according to the
	 *	DataSource/DataSink passed to them.
	 */
	virtual StreamElementPtr getStreamElement( size_t index, size_t & size,
		bool & isNone, bool isPersistentOnly ) const = 0;

	bool canIgnoreSelfAssignment() const
		{ return this->isConst() || this->isSmart(); }

	struct SingletonPtr
	{
		explicit SingletonPtr( DataType * pInst ) : pInst_( pInst ) { }

		DataType * pInst_;

		bool operator<( const SingletonPtr & me ) const
			{ return (*pInst_) < (*me.pInst_); }
	};
	typedef BW::set< SingletonPtr > SingletonMap;
	static SingletonMap * s_singletonMap_;

	static DataTypePtr findOrAddType( DataTypePtr pDT );


	static bool initAliases();

	typedef BW::map< BW::string, DataTypePtr >	Aliases;
	static Aliases s_aliases_;
#ifdef EDITOR_ENABLED
	typedef BW::map< BW::string, DataSectionPtr >	AliasWidgets;
	static AliasWidgets s_aliasWidgets_;
#endif // EDITOR_ENABLED
};

BW_END_NAMESPACE

#endif // DATA_TYPE_HPP
