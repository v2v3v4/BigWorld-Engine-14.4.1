#include "pch.hpp"
#include "meta_data.hpp"
#include "meta_data_define.hpp"
#include "cstdmf/string_utils.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/bwresource.hpp"


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE(MetaData::Environment)

namespace MetaData
{

PropertyType::PropertyType( const BW::string& name ) : name_( name )
{
	BW_GUARD;

	registerType();
}


PropertyType::TypeMap& PropertyType::typeMap()
{
	static TypeMap s_typeMap;
	return s_typeMap;
}


void PropertyType::registerType()
{
	BW_GUARD;

	typeMap()[ name_ ] = this;
}


const PropertyType* PropertyType::get( const BW::string& name )
{
	BW_GUARD;

	TypeMap::iterator iter = typeMap().find( name );
	if (iter != typeMap().end())
		return iter->second;
	return NULL;
}


Desc::Desc( const BW::string& configFile )
{
	BW_GUARD;

	load( BWResource::openSection( configFile ) );
}


bool Desc::load( DataSectionPtr ds )
{
	BW_GUARD;

	propDescs_.clear();
	addDefaultDesc();
	return internalLoad( ds );
}


bool Desc::load( DataSectionPtr ds, MetaData& metaData ) const
{
	BW_GUARD;

	metaData.clearProperties();

	for (PropDescs::const_iterator iter = propDescs_.begin();
		iter != propDescs_.end(); ++iter)
	{
		metaData.addProperty( iter->second->create( metaData, ds ) );
	}

	return true;
}


void Desc::addPropDesc( PropertyDescPtr propDesc )
{
	BW_GUARD;

	propDescs_[ propDesc->name() ] = propDesc;
}


bool Desc::internalLoad( DataSectionPtr ds )
{
	BW_GUARD;

	if (ds)
	{
		for (int i = 0; i < ds->countChildren(); ++i)
		{
			DataSectionPtr propDesc = ds->openChild( i );
			const PropertyType* type =
				PropertyType::get( propDesc->readString( "type" ) );

			if (type)
			{
				BW::string name = propDesc->sectionName();
				BW::string description;
				DataSectionPtr desc = propDesc->openSection( "description" );

				if (desc)
				{
					description = desc->asString();
				}
				else
				{
					description = name;
				}

				addPropDesc( new PropertyDesc(
					name, description, type, propDesc ) );
			}
			else
			{
				ERROR_MSG( "Desc::internalLoad failed to load property %s",
					propDesc->sectionName().c_str() );
			}
		}
	}
	return true;
}


void Desc::addDefaultDesc()
{
	BW_GUARD;

	BW::string defaultDesc =
		"<root>															\
			<description>												\
				<type>	STRING	</type>									\
			</description>												\
			<comments>													\
				<type>	COMMENTS	</type>								\
			</comments>													\
			<" METADATA_CREATED_ON ">									\
				<description>	created on	</description>				\
				<type>	DATETIME	</type>								\
			</" METADATA_CREATED_ON ">									\
			<" METADATA_CREATED_BY ">									\
				<description>	created by	</description>				\
				<type>	STRING	</type>									\
				<readonly>	true	</readonly>							\
			</" METADATA_CREATED_BY ">									\
			<" METADATA_MODIFIED_ON ">									\
				<description>	modified on	</description>				\
				<type>	DATETIME	</type>								\
			</" METADATA_MODIFIED_ON ">									\
			<" METADATA_MODIFIED_BY ">									\
				<description>	modified by	</description>				\
				<type>	STRING	</type>									\
				<readonly>	true	</readonly>							\
			</" METADATA_MODIFIED_BY ">									\
		</root>";
	BW::istringstream iss( defaultDesc ) ;

	internalLoad( XMLSection::createFromStream( "", iss ) );
}


MetaData::MetaData( const MetaData& that )
{
	BW_GUARD;

	*this = that;
}

MetaData& MetaData::operator =( const MetaData& that )
{
	BW_GUARD;

	bool same = true;

	if (properties_.size() != that.properties_.size())
	{
		same = false;
	}
	else
	{
		Properties::iterator iter = properties_.begin();
		Properties::const_iterator jter = that.properties_.begin();

		for (;iter != properties_.end(); ++iter, ++jter)
		{
			if (&(*iter)->desc() != &(*jter)->desc())
			{
				same = false;
				break;
			}
		}
	}

	if (same)
	{
		Properties::iterator iter = properties_.begin();
		Properties::const_iterator jter = that.properties_.begin();

		for (;iter != properties_.end(); ++iter, ++jter)
		{
			DataSectionPtr temp = new XMLSection( "temp" );

			(*jter)->save( temp );
			(*iter)->load( temp );
		}
	}
	else
	{
		properties_.clear();

		Properties::const_iterator jter = that.properties_.begin();

		for (;jter != that.properties_.end(); ++jter)
		{
			properties_.push_back( (*jter)->clone() );
			properties_.back()->metaData( *this );
		}
	}

	owner_ = that.owner_;

	return *this;
}

bool MetaData::load( DataSectionPtr ds, const Desc& desc )
{
	BW_GUARD;

	ds = ds ? ds->openSection( METADATA_SECTION_NAME, false ) : ds;

	return desc.load( ds, *this );
}


bool MetaData::save( DataSectionPtr ds ) const
{
	BW_GUARD;

	ds = ds->openSection( METADATA_SECTION_NAME, true );

	for (Properties::const_iterator iter = properties_.begin();
		iter != properties_.end(); ++iter)
	{
		(*iter)->save( ds );
	}

	return true;
}


void MetaData::edit( GeneralEditor& editor, const Name& group, bool readOnly )
{
	BW_GUARD;

	for (Properties::iterator iter = properties_.begin();
		iter != properties_.end(); ++iter)
	{
		GeneralProperty* prop = (*iter)->createProperty( readOnly, editor.useFullDateFormat() );

		if (!group.empty())
		{
			prop->setGroup( group.str() + '/' + prop->getGroup().c_str() );
		}

		editor.addProperty( prop );
	}
}


void MetaData::clearProperties()
{
	BW_GUARD;

	properties_.clear();
}


void MetaData::addProperty( PropertyPtr property )
{
	BW_GUARD;

	properties_.push_back( property );
}


PropertyPtr MetaData::operator[]( const BW::string& name )
{
	BW_GUARD;

	for (Properties::iterator iter = properties_.begin();
		iter != properties_.end(); ++iter)
	{
		if ((*iter)->desc().name() == name)
		{
			return *iter;
		}
	}

	return NULL;
}


Restore::Restore( MetaData& metaData )
	: metaData_( metaData ), saved_( metaData )
{
}


void Restore::restore()
{
	BW_GUARD;

	GeneralEditor::Editors::const_iterator iter
		= GeneralEditor::currentEditors().begin();
	GeneralEditor::Editors::const_iterator end
		= GeneralEditor::currentEditors().end();

	while (iter != end)
	{
		(*iter)->expel();
		++iter;
	}

	metaData_ = saved_;
	Environment::instance().changed( metaData_.owner() );

	iter = GeneralEditor::currentEditors().begin();
	end = GeneralEditor::currentEditors().end();

	GeneralProperty::View::pLastElected( NULL );

	while (iter != end)
	{
		(*iter)->elect();
		++iter;
	}

	if (GeneralProperty::View::pLastElected())
	{
		GeneralProperty::View::pLastElected()->lastElected();
	}
}


BW::string Environment::username() const
{
	BW_GUARD;

	static wchar_t username[ 1024 ] = { 0 };

	if (!username[0])
	{
		DWORD size = sizeof( username ) / sizeof( username[0] );

		GetUserName( username, &size );
	}

	return bw_wtoutf8( username );
}


time_t Environment::time() const
{
	BW_GUARD;

	return ::time( NULL );
}

}//namespace MetaData

extern int metaDataGeneralPropertyTypeToken;
extern int metaDataDateTimePropertyTypeToken;
extern int metaDataCommentsPropertyTypeToken;
static int metaDataToken =
	metaDataGeneralPropertyTypeToken |
	metaDataDateTimePropertyTypeToken |
	metaDataCommentsPropertyTypeToken;
BW_END_NAMESPACE

