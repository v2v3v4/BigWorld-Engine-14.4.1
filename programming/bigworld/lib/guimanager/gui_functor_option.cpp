#include "pch.hpp"
#include "gui_functor_option.hpp"
#include "gui_item.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

OptionFunctor::OptionFunctor()
	:option_( NULL )
{}

void OptionFunctor::setOption( OptionMap* option )
{
	BW_GUARD;

	option_ = option;
}

const BW::string& OptionFunctor::name() const
{
	static BW::string name = "Option";
	return name;
}

bool OptionFunctor::text( const BW::string& textor, ItemPtr item, BW::string& result )
{
	BW_GUARD;

	if( option_ && option_->exist( textor ) )
	{
		result = option_->get( textor );
		return true;
	}
	return false;
}

bool OptionFunctor::update( const BW::string& updater, ItemPtr item, unsigned int& result )
{
	BW_GUARD;

	if( option_ && updater.find( '=' ) != updater.npos )
	{
		BW::string name = updater.substr( 0, updater.find( '=' ) );
		BW::string value = updater.substr( updater.find( '=' ) + 1 );
		if( !value.empty() && value[0] == '=' )
			value.erase( value.begin() );
		FunctorManager::trim( name );
		FunctorManager::trim( value );
		if( option_->exist( name ) )
		{
			if( option_->get( name ) == value )
				result = 1;
			else
				result = 0;
			return true;
		}
	}
	return false;
}

bool OptionFunctor::act( const BW::string& action, ItemPtr item, bool& result )
{
	BW_GUARD;

	if( option_ && action.find( '=' ) != action.npos )
	{
		BW::string name = action.substr( 0, action.find( '=' ) );
		BW::string value = action.substr( action.find( '=' ) + 1 );
		FunctorManager::trim( name );
		FunctorManager::trim( value );
		if( option_->exist( name ) )
		{
			option_->set( name, value );
			result = true;
			return true;
		}
	}
	return false;
}


END_GUI_NAMESPACE
BW_END_NAMESPACE

