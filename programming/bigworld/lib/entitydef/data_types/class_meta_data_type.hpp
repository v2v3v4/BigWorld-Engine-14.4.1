#ifndef CLASS_META_DATA_TYPE_HPP
#define CLASS_META_DATA_TYPE_HPP

#include "entitydef/data_description.hpp"
#include "entitydef/meta_data_type.hpp"

#include "class_data_type.hpp"

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements an object that creates class types with properties
 *	like an entity.
 */
class ClassMetaDataType : public MetaDataType
{
public:
	typedef ClassDataType DataType;

	ClassMetaDataType();
	virtual ~ClassMetaDataType();

protected:

	virtual const char * name()	const { return "CLASS"; }

	virtual DataTypePtr getType( DataSectionPtr pSection );

public:
	// This method is used to build both ClassDataType and FixedDictDataType
	template <class METADATATYPE>
	static typename METADATATYPE::DataType* buildType( DataSectionPtr pSection,
		METADATATYPE& metaDataType )
	{
		typename METADATATYPE::DataType::Fields fields;

		DataSectionPtr pParentSect = pSection->openSection( "Parent" );
		if (pParentSect)
		{
			DataTypePtr pParent = DataType::buildDataType( pParentSect );
			if (!pParent || pParent->pMetaDataType() != &metaDataType)
			{
				ERROR_MSG( "<Parent> of %s must also be a %s\n",
							metaDataType.name(), metaDataType.name() );
				return NULL;
			}

			typename METADATATYPE::DataType * pParentClass =
				static_cast<typename METADATATYPE::DataType*>( pParent.get() );
			// just rip its fields out for now
			fields = pParentClass->getFields();
		}

		DataSectionPtr pProps = pSection->openSection( "Properties" );
		if ((!pProps || pProps->countChildren() == 0) && fields.empty())
		{
			ERROR_MSG( "<Properties> section missing or empty in %s type spec\n",
						metaDataType.name() );
			return NULL;
		}

		for (DataSection::iterator it = pProps->begin();
			it != pProps->end();
			++it)
		{
			typename METADATATYPE::DataType::Field f;
			f.name_ = (*it)->sectionName();

			f.type_ = DataType::buildDataType( (*it)->openSection( "Type" ) );
			if (!f.type_)
			{
				ERROR_MSG( "Could not build type for property '%s' in %s type\n",
					f.name_.c_str(), metaDataType.name() );
				return NULL;
			}

			f.dbLen_ =
					(*it)->readInt( "DatabaseLength", DEFAULT_DATABASE_LENGTH );
			f.isPersistent_ = (*it)->readBool( "Persistent", true );

			fields.push_back( f );
		}

		// should have got an error if fields are empty before here
		MF_ASSERT_DEV( !fields.empty() );

		bool allowNone = pSection->readBool( "AllowNone", false );

		return new typename METADATATYPE::DataType( &metaDataType, fields,
														allowNone );
	}
};

BW_END_NAMESPACE

#endif // CLASS_META_DATA_TYPE_HPP
