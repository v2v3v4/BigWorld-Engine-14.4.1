// TODO: Currently not supported
#include "pch.hpp"

#if 0
// TODO: Should consider whether we want this datatype. The main issue with it
// at the moment is similar to the list. If a modification is made to it, it
// is not propagated. Only when the top level property is set to a new
// dictionary.

// -----------------------------------------------------------------------------
// Section: DictionaryDataType
// -----------------------------------------------------------------------------

/**
 *	This class is used for dictionary data types.
 */
class DictionaryDataType : public DataType
{
public:
	DictionaryDataType( DataType & keyType, DataType & valueType ) :
		keyType_( keyType ),
		valueType_( valueType )
	{
		this->typeName( "DICT: " +
				keyType_.typeName() + valueType_.typeName() );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::isSameType
	 */
	virtual bool isSameType( PyObject * pValue )
	{
		if (!PyDict_Check( pValue ))
		{
			ERROR_MSG( "DictionaryDataType::isSameType: Not a dictionary.\n" );
			if (pValue != NULL)
			{
				PyObject * pAsStr = PyObject_Str( pValue );

				if (pAsStr)
				{
					ERROR_MSG( "\tpValue = %s\n",
							PyString_AsString( pAsStr ) );
					Py_DECREF( pAsStr );
				}
			}

			return false;
		}

		int pos = 0;
		PyObject * pKey;
		PyObject * pEntry;

		while (PyDict_Next( pValue, &pos, &pKey, &pEntry ))
		{
			if (pKey && pValue)
			{
				if (!keyType_.isSameType( pKey ))
				{
					ERROR_MSG( "DictionaryDataType::isSameType: Bad key.\n" );
					return false;
				}

				if (!valueType_.isSameType( pEntry ))
				{
					ERROR_MSG( "DictionaryDataType::isSameType: Bad value.\n" );
					return false;
				}
			}
		}

		return true;
	}

	virtual void reloadScript()
	{
		keyType_.reloadScript();
		valueType_.reloadScript();
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::pDefaultValue
	 */
	virtual PyObjectPtr pDefaultValue()
	{
		return PyObjectPtr( PyDict_New(), PyObjectPtr::STEAL_REFERENCE );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::addToStream
	 */
	virtual void addToStream( PyObject * pNewValue,
			BinaryOStream & stream, bool isPersistentOnly )
	{
		int size = PyDict_Size( pNewValue );
		stream << size;

		int pos = 0;
		PyObject * pKey;
		PyObject * pValue;

		while (PyDict_Next( pNewValue, &pos, &pKey, &pValue ))
		{
			if (pKey && pValue)
			{
				keyType_.addToStream( pKey, stream, isPersistentOnly );
				valueType_.addToStream( pValue, stream, isPersistentOnly );
			}
		}
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::createFromStream
	 */
	virtual PyObjectPtr createFromStream( BinaryIStream & stream,
			bool isPersistentOnly ) const
	{
		int size;
		stream >> size;

		if (size > stream.remainingLength())
		{
			ERROR_MSG( "DictionaryDataType::createFromStream: "
					   "Size (%d) is greater than remainingLength() (%d)\n",
					   size, stream.remainingLength() );
			return NULL;
		}

		PyObject * pDict = PyDict_New();

		for (int i = 0; i < size; i++)
		{
			PyObject * pKey =
				keyType_.createFromStream( stream, isPersistentOnly );

			if (pKey)
			{
				PyObject * pValue =
					valueType_.createFromStream( stream, isPersistentOnly );

				if (pValue)
				{
					if (PyDict_SetItem( pDict, pKey, pValue ) == -1)
					{
						ERROR_MSG( "DictionaryDataType::createFromStream: "
								"Insert failed.\n" );
						return NULL;
					}

					Py_DECREF( pValue );
				}
				else
				{
					ERROR_MSG( "DictionaryDataType::createFromStream: "
							"Bad key.\n" );
					return NULL;
				}

				Py_DECREF( pKey );
			}
			else
			{
				ERROR_MSG( "DictionaryDataType::createFromStream: Bad key.\n" );
				return NULL;
			}
		}

		return PyObjectPtr( pDict, PyObjectPtr::STEAL_REFERENCE );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::addToSection
	 */
	virtual bool addToSection( PyObject * pNewValue, DataSectionPtr pSection )
		const
	{
		int pos = 0;
		PyObject * pKey;
		PyObject * pValue;

		while (PyDict_Next( pNewValue, &pos, &pKey, &pValue ))
		{
			if (pKey && pValue)
			{
				DataSectionPtr pItemSection = pSection->newSection( "item" );
				DataSectionPtr pKeySection = pItemSection->newSection( "key" );
				if (!keyType_.addToSection( pKey, pKeySection ))
				{
					return false;
				}

				DataSectionPtr pValueSection =
					pItemSection->newSection( "value" );
				if (!valueType_.addToSection( pValue, pValueSection ))
				{
					return false;
				}
			}
		}
		return true;
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::createFromSection
	 */
	virtual PyObjectPtr createFromSection( DataSectionPtr pSection )
	{
		if (!pSection)
		{
			ERROR_MSG( "DictionaryDataType::createFromSection: "
					"Section is NULL.\n" );
			return NULL;
		}

		PyObject * pDict = PyDict_New();

		DataSection::iterator iter = pSection->begin();

		while (iter != pSection->end())
		{
			DataSectionPtr pItem = *iter;

			if (pItem->sectionName() == "item")
			{
				DataSectionPtr pKeySection = pItem->findChild( "key" );

				if (pKeySection)
				{
					DataSectionPtr pValueSection = pItem->findChild( "value" );

					if (pValueSection)
					{
						PyObject * pKey =
							keyType_.createFromSection( pKeySection );
						PyObject * pValue =
							valueType_.createFromSection( pValueSection );

						if (pKey && pValue)
						{
							PyDict_SetItem( pDict, pKey, pValue );
						}

						Py_XDECREF( pKey );
						Py_XDECREF( pValue );
					}
					else
					{
						ERROR_MSG( "DictionaryDataType::createFromSection: "
								"No value section\n" );
					}
				}
				else
				{
					ERROR_MSG( "DictionaryDataType::createFromSection: "
							"No key section\n" );
				}
			}
			else
			{
				ERROR_MSG( "DictionaryDataType::createFromSection: "
						"Bad section '%s'\n", (*iter)->sectionName().c_str() );
			}

			++iter;
		}

		return PyObjectPtr( pDict, PyObjectPtr::STEAL_REFERENCE );
	}


	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool isPersistentOnly )
	{
		int size;
		stream >> size;

		for (int i = 0; i < size; i++)
		{
			DataSectionPtr pItemSection = pSection->newSection( "item" );
			DataSectionPtr pKeySection = pItemSection->newSection( "key" );
			if (!keyType_.fromStreamToSection( stream, pKeySection,
											isPersistentOnly ))
			{
				return false;
			}

			DataSectionPtr pValueSection = pItemSection->newSection( "value" );
			if (!valueType_.fromStreamToSection( stream, pValueSection,
											isPersistentOnly ))
			{
				return false;
			}
		}

		return true;
	}

	virtual void addToMD5( MD5 & md5 ) const
	{
		md5.append( "Dict", sizeof( "Dict" ) );
		keyType_.addToMD5( md5 );
		valueType_.addToMD5( md5 );
	}

private:
	DataType & keyType_;
	DataType & valueType_;
};

class DictionaryMetaDataType : public MetaDataType
{
public:
	DictionaryMetaDataType() : MetaDataType( "DICTIONARY" ) {}

	virtual DataTypePtr getType( DataSectionPtr pSection )
	{
		DataType * pKeyType =
			DataType::fromDataSection( pSection->openSection( "key" ) );

		if (!pKeyType)
		{
			ERROR_MSG( "DictionaryMetaDataType::getType: Bad key type.\n" );
			return NULL;
		}

		DataType * pValueType =
			DataType::fromDataSection( pSection->openSection( "value" ) );

		if (!pValueType)
		{
			ERROR_MSG( "DictionaryMetaDataType::getType: Bad value type.\n" );
			return NULL;
		}

		return new DictionaryDataType( *pKeyType, *pValueType );
	}
};

static DictionaryMetaDataType s_DICT_metaDataType;

#endif

// dictionary_data_type.cpp
