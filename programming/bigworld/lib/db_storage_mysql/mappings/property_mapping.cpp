#include "script/first_include.hpp"

#include "property_mapping.hpp"

#include "blobbed_sequence_mapping.hpp"
#include "blob_mapping.hpp"
#include "class_mapping.hpp"
#include "num_mapping.hpp"
#include "python_mapping.hpp"
#include "sequence_mapping.hpp"
#include "string_mapping.hpp"
#include "udo_ref_mapping.hpp"
#include "unicode_string_mapping.hpp"
#include "unique_id_mapping.hpp"
#include "user_type_mapping.hpp"
#include "vector_mapping.hpp"

#include "../namer.hpp"

#include "entitydef/data_types/class_data_type.hpp"
#include "entitydef/data_types/fixed_dict_data_type.hpp"
#include "entitydef/data_types/sequence_data_type.hpp"
#include "entitydef/data_types/user_data_type.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "DBEngine", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Helper functions
// -----------------------------------------------------------------------------

namespace
{

/**
 *	This method gets the default value section for the child type based on
 * 	the parent type's default value section. If it cannot find the section,
 * 	then it uses the child's default section.
 */
DataSectionPtr getChildDefaultSection( const DataType & childType,
		const BW::string childName, DataSectionPtr pParentDefault )
{
	DataSectionPtr pChildDefault =
					pParentDefault ?
						pParentDefault->openSection( childName ) :
						DataSectionPtr( NULL );

	if (!pChildDefault)
	{
		pChildDefault = childType.pDefaultSection();
	}

	return pChildDefault;
}


/**
 *	This method creates a ClassMapping for a CLASS or FIXED_DICT property.
 */
template <class DATATYPE>
ClassMapping * createClassTypeMapping( const Namer & classNamer,
		const BW::string & propName, const DATATYPE& type,
		int databaseLength, DataSectionPtr pDefaultValue )
{
	ClassMapping * pClassMapping =
		new ClassMapping( classNamer, propName, type.allowNone() );

	Namer childNamer( classNamer, propName, false );
	// Don't add extra level of naming if we are inside a sequence.
	const Namer & namer = (propName.empty()) ? classNamer : childNamer;

	const ClassDataType::Fields & fields = type.getFields();

	for ( ClassDataType::Fields::const_iterator i = fields.begin();
			i < fields.end(); ++i )
	{
		if (i->isPersistent_)
		{
			DataSectionPtr pPropDefault = getChildDefaultSection(
					*(i->type_), i->name_, pDefaultValue );
			PropertyMappingPtr pMemMapping =
				PropertyMapping::create( namer, i->name_, *(i->type_),
										i->dbLen_, pPropDefault );
			if (pMemMapping.exists())
			{
				pClassMapping->addChild( pMemMapping );
			}
		}
	}

	return pClassMapping;
}



} // anonymous namespace


/**
 *	Get the MySQL column index type for this DatabaseIndexingType enum value.
 *
 *	@returns 	The MySQL column index type for this DatabaseIndexingType enum
 *				value.
 */
ColumnIndexType PropertyMapping::getColumnIndexType( 
		DatabaseIndexingType indexingType )
{
	switch (indexingType)
	{
	case DATABASE_INDEXING_UNIQUE:
		return INDEX_TYPE_UNIQUE;

	case DATABASE_INDEXING_NON_UNIQUE:
		return INDEX_TYPE_NON_UNIQUE;

	default:
		return INDEX_TYPE_NONE;
	}
}

// -----------------------------------------------------------------------------
// Section: PropertyMapping
// -----------------------------------------------------------------------------

/**
 *	This method creates the correct PropertyMapping-derived class for a
 * 	property.
 */
PropertyMappingPtr PropertyMapping::create( const Namer & namer,
		const BW::string & propName, const DataType & type,
		int databaseLength, DataSectionPtr pDefaultValue,
		DatabaseIndexingType indexingType )
{
	PropertyMappingPtr pResult;

	const SequenceDataType * pSeqType;
	const ClassDataType * pClassType;
	const UserDataType * pUserType;
	const FixedDictDataType * pFixedDictType;

	const bool isIndexed = (indexingType != DATABASE_INDEXING_NONE);

	const ColumnIndexType mysqlIndexType = 
		PropertyMapping::getColumnIndexType( indexingType );

	if (!isIndexed && 
			(pSeqType = dynamic_cast<const SequenceDataType *>(&type)))
	{
		// TODO: Is it possible to specify the default for an ARRAY or TUPLE
		// to have more than one element:
		//		<Default>
		//			<item> 1 </item>
		//			<item> 2 </item>
		//			<item> 3 </item>
		//		</Default>
		// Currently, when adding a new ARRAY/TUPLE to an entity, all
		// existing entities in the database will default to having no
		// elements. When creating a new entity, it will default to the
		// specified default.
		// TODO: For ARRAY/TUPLE of FIXED_DICT, there is an additional
		// case where a new property is added to the FIXED_DICT. Then
		// all existing elements in the database will need a default value
		// for the new property. Currently we use the default value for the
		// child type (as opposed to array type) so we don't have to handle
		// complicated cases where default value for the array doesn't
		// have the same number of elements as the existing arrays in the
		// database.

		// Use the type default value for the child. This is
		// mainly useful when adding new properties to an ARRAY of
		// FIXED_DICT. The new column will have the specified default value.
		const DataType & childType = pSeqType->getElemType();
		DataSectionPtr 	pChildDefault = childType.pDefaultSection();

		PropertyMappingPtr childMapping =
			PropertyMapping::create( Namer( namer, propName, true ),
									BW::string(), childType,
									databaseLength, pChildDefault );

		if (childMapping.exists())
		{
			if (pSeqType->dbLen() > 0)
			{
				pResult = new BlobbedSequenceMapping( namer, propName,
									childMapping, pSeqType->getSize(),
									pSeqType->dbLen() );
			}
			else
			{
				pResult = new SequenceMapping( namer, propName,
										childMapping, pSeqType->getSize() );
			}
		}
	}
	else if (!isIndexed &&
			(pFixedDictType = dynamic_cast<const FixedDictDataType *>(&type)))
	{
		pResult = createClassTypeMapping( namer, propName,
						*pFixedDictType, databaseLength, pDefaultValue );
	}
	else if (!isIndexed && 
			(pClassType = dynamic_cast<const ClassDataType *>(&type)))
	{
		pResult = createClassTypeMapping( namer, propName, *pClassType,
									databaseLength, pDefaultValue );
	}
	else if (!isIndexed && 
			(pUserType = dynamic_cast<const UserDataType *>(&type)))
	{
		pResult = UserTypeMapping::create( namer, propName, *pUserType,
										pDefaultValue );
		if (!pResult.exists())
		{
			// Treat same as parse error i.e. stop DBApp. This is to prevent
			// altering tables (particularly dropping columns) due to
			// a simple scripting error.
			BW::stringstream errorMsg;
			errorMsg << "Unable to bind USER_TYPE property '" << propName <<
						"' to database.";
			throw std::runtime_error( errorMsg.str().c_str() );
		}
	}
	else
	{
		const MetaDataType * pMetaType = type.pMetaDataType();
		MF_ASSERT(pMetaType);
		const char * metaName = pMetaType->name();
		if (strcmp( metaName, "UINT8" ) == 0)
		{
			pResult = new NumMapping< uint8 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "UINT16" ) == 0)
		{
			pResult = new NumMapping< uint16 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "UINT32" ) == 0)
		{
			pResult = new NumMapping< uint32 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "UINT64" ) == 0)
		{
			pResult = new NumMapping< uint64 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "INT8" ) == 0)
		{
			pResult = new NumMapping< int8 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "INT16" ) == 0)
		{
			pResult = new NumMapping< int16 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "INT32" ) == 0)
		{
			pResult = new NumMapping< int32 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "INT64" ) == 0)
		{
			pResult = new NumMapping< int64 >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (strcmp( metaName, "FLOAT32" ) == 0)
		{
			pResult = new NumMapping< float >( namer, propName, pDefaultValue, 
				mysqlIndexType );
		}
		else if (strcmp( metaName, "FLOAT64" ) == 0)
		{
			pResult = new NumMapping< double >( namer, propName, pDefaultValue,
				mysqlIndexType );
		}
		else if (!isIndexed && strcmp( metaName, "VECTOR2" ) == 0)
		{
			pResult = new VectorMapping<Vector2,2>( namer, propName, 
				pDefaultValue );
		}
		else if (!isIndexed && strcmp( metaName, "VECTOR3" ) == 0)
		{
			pResult = new VectorMapping<Vector3,3>( namer, propName, 
				pDefaultValue );
		}
		else if (!isIndexed && strcmp( metaName, "VECTOR4" ) == 0)
		{
			pResult = new VectorMapping<Vector4,4>( namer, propName, 
				pDefaultValue );
		}
		else if (strcmp( metaName, "STRING" ) == 0)
		{
			pResult = new StringMapping( namer, propName, mysqlIndexType,
				databaseLength,	pDefaultValue );
		}
		else if (strcmp( metaName, "UNICODE_STRING" ) == 0)
		{
			pResult = new UnicodeStringMapping( namer, propName, 
				mysqlIndexType, databaseLength, pDefaultValue );
		}
		else if (!isIndexed && strcmp( metaName, "PYTHON" ) == 0)
		{
			pResult = new PythonMapping( namer, propName, 
				databaseLength, pDefaultValue );
		}
		else if (strcmp( metaName, "BLOB" ) == 0)
		{
			pResult = new BlobMapping( namer, propName, mysqlIndexType,
				databaseLength, pDefaultValue );
		}
		else if (!isIndexed && strcmp( metaName, "PATROL_PATH" ) == 0)
		{
			pResult = new UniqueIDMapping( namer, propName, pDefaultValue );
		}
		else if (!isIndexed && strcmp( metaName, "UDO_REF" ) == 0)
		{
			pResult = new UDORefMapping( namer, propName, pDefaultValue );
		}
	}

	if (!pResult.exists())
	{
		ERROR_MSG( "PropertyMapping::create: Property %s (%s index) of type %s "
				"cannot be persistent.\n",
			propName.c_str(), 
			isIndexed ? "with" : "without",
			type.typeName().c_str() );
	}

	return pResult;
}

BW_END_NAMESPACE

// property_mapping.cpp
