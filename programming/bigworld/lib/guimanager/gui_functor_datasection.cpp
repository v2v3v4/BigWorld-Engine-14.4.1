#include "pch.hpp"
#include "gui_functor_datasection.hpp"
#include "gui_item.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

void DataSectionFunctor::setRoot( DataSectionPtr root )
{
	BW_GUARD;

	ds_ = root;
}

const BW::string& DataSectionFunctor::name() const
{
	static BW::string name = "DataSection";
	return name;
}

bool DataSectionFunctor::text( const BW::string& textor, ItemPtr item, BW::string& result )
{
	BW_GUARD;

	DataSectionPtr section;
	if( ds_ && ( section = ds_->openSection( textor, false ) ) )
	{
		result = section->asString();
		return true;
	}
	return false;
}

bool DataSectionFunctor::update( const BW::string& updater, ItemPtr item, unsigned int& result )
{
	BW_GUARD;

	if( ds_ && updater.find( '=' ) != updater.npos )
	{
		BW::string name = updater.substr( 0, updater.find( '=' ) );
		BW::string value = updater.substr( updater.find( '=' ) + 1 );
		if( !value.empty() && value[0] == '=' )
			value.erase( value.begin() );
		FunctorManager::trim( name );
		FunctorManager::trim( value );
		DataSectionPtr section = ds_->openSection( name, false );
		if( section )
		{
			if( section->asString() == value )
				result = 1;
			else
				result = 0;
			return true;
		}
	}
	return false;
}

bool DataSectionFunctor::act( const BW::string& action, ItemPtr item, bool& result )
{
	BW_GUARD;

	if( ds_ && action.find( '=' ) != action.npos )
	{
		BW::string name = action.substr( 0, action.find( '=' ) );
		BW::string value = action.substr( action.find( '=' ) + 1 );
		FunctorManager::trim( name );
		FunctorManager::trim( value );
		DataSectionPtr section = ds_->openSection( name, false );
		if( section )
		{
			section->be( value );
			result = true;
			return true;
		}
	}
	return false;
}


END_GUI_NAMESPACE
BW_END_NAMESPACE

