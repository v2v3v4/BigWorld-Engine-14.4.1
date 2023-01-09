#include "pch.hpp"
#include "dxenum.hpp"

BW_BEGIN_NAMESPACE

DXEnum::EnumEntry::EnumEntry( DataSectionPtr dataSection )
{
	BW_GUARD;

	BW::vector<DataSectionPtr> entries;
	dataSection->openSections( "ENTRY", entries );
	for( BW::vector<DataSectionPtr>::iterator iter = entries.begin(); iter != entries.end(); ++iter )
	{
		BW::string name = (*iter)->readString( "NAME" );
		DXEnum::EnumType value = (DXEnum::EnumType)(*iter)->readInt( "VALUE" );
		if( name.size() && nameToValueMap_.find( name ) == nameToValueMap_.end() &&
			valueToNameMap_.find( value ) == valueToNameMap_.end() )
		{
			nameToValueMap_[ name ] = value;
			valueToNameMap_[ value ] = name;
		}
		else
		{
			// There is a mistake in parsing Enum Entry for Material
			// Either it has an unnamed entry or the name/value is duplicated
		}
	}
}

DXEnum::EnumEntry::size_type DXEnum::EnumEntry::size() const
{
	return nameToValueMap_.size();
}

const BW::string& DXEnum::EnumEntry::entry( DXEnum::EnumEntry::size_type index ) const
{
	BW_GUARD;

	// no check for index, since no exceptions are allowed here
	BW::map<BW::string, EnumType>::const_iterator iter = nameToValueMap_.begin();
	std::advance( iter, index );
	return iter->first;
}

DXEnum::EnumType DXEnum::EnumEntry::value( const BW::string& name ) const
{
	BW_GUARD;

	return nameToValueMap_.find( name )->second;
}

const BW::string& DXEnum::EnumEntry::name( DXEnum::EnumType value ) const
{
	BW_GUARD;

	if( valueToNameMap_.find( value ) != valueToNameMap_.end() )
		return valueToNameMap_.find( value )->second;
	return valueToNameMap_.begin()->second;
}

DXEnum::DXEnum( const BW::string& resFileName )
{
	BW_GUARD;

	DataSectionPtr enumDef = BWResource::instance().openSection( resFileName );
	if( enumDef )
	{
		for( DataSectionIterator iter = enumDef->begin(); iter != enumDef->end(); ++iter )
		{
			DataSectionPtr section = *iter;
			DataSectionPtr alias = section->openSection( "ALIAS" );
			if( alias )
			{
				if( alias_.find( section->sectionName() ) == alias_.end() )
					alias_[ section->sectionName() ] = alias->asString();
				else
				{
					// alias already existing
				}
			}
			else
			{
				if( enums_.find( section->sectionName() ) == enums_.end() )
				{
					EnumEntry entry( section );
					if( entry.size() )
						enums_[ section->sectionName() ] = entry;
				}
				else
				{
					// enum already existing
				}
				
			}
		}
		bool changed = true;
		while( changed )
		{
			changed = false;
			for( BW::map<BW::string, BW::string>::iterator iter = alias_.begin();
				iter != alias_.end(); ++iter )
			{
				if( enums_.find( iter->first ) != enums_.end() )
				{// enum and alias has same name
					alias_.erase( iter );
					changed = true;
					break;
				}
				BW::string s = iter->second;
				while( alias_.find( s ) != alias_.end() )
					s = alias_.find( s )->second;
				if( enums_.find( s ) == enums_.end() )
				{// alias has no correspond enum
					alias_.erase( iter );
					changed = true;
					break;
				}
				iter->second = s;
			}
		}
	}
}

bool DXEnum::isEnum( const BW::string& typeName ) const
{
	BW_GUARD;

	return enums_.find( typeName ) != enums_.end() ||
		alias_.find( typeName ) != alias_.end();
}

const BW::string& DXEnum::getRealType( const BW::string& typeName ) const
{
	BW_GUARD;

	if( alias_.find( typeName ) != alias_.end() )
		return alias_.find( typeName )->second;
	return typeName;
}

DXEnum::size_type DXEnum::size( const BW::string& typeName ) const
{
	BW_GUARD;

	return enums_.find( getRealType( typeName ) )->second.size();
}

const BW::string& DXEnum::entry( const BW::string& typeName, DXEnum::size_type index ) const
{
	BW_GUARD;

	return enums_.find( getRealType( typeName ) )->second.entry( index );
}

DXEnum::EnumType DXEnum::value( const BW::string& typeName, const BW::string& name ) const
{
	BW_GUARD;

	return enums_.find( getRealType( typeName ) )->second.value( name );
}

const BW::string& DXEnum::name( const BW::string& typeName, DXEnum::EnumType value ) const
{
	BW_GUARD;

	return enums_.find( getRealType( typeName ) )->second.name( value );
}

BW_SINGLETON_STORAGE( DXEnum );
BW_END_NAMESPACE

