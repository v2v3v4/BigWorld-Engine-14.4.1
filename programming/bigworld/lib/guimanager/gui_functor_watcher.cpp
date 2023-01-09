#include "pch.hpp"
#include "gui_functor_watcher.hpp"
#include "gui_item.hpp"

#include "cstdmf/watcher.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

const BW::string& WatcherFunctor::name() const
{
	static BW::string name = "Watcher";
	return name;
}

bool WatcherFunctor::text( const BW::string& textor, ItemPtr item, BW::string& result )
{
	BW_GUARD;

#if ENABLE_WATCHERS
	BW::string desc;
	Watcher::Mode mode;

	if( Watcher::rootWatcher().getAsString( NULL, textor.c_str(), result, desc, mode ) )
	{
		return true;
	}
#endif //ENABLE_WATCHERS

	return false;
}

bool WatcherFunctor::update( const BW::string& updater, ItemPtr item, unsigned int& result )
{
	BW_GUARD;

#if ENABLE_WATCHERS
	if( updater.find( '=' ) != updater.npos )
	{
		BW::string name = updater.substr( 0, updater.find( '=' ) );
		BW::string value = updater.substr( updater.find( '=' ) + 1 );
		if( !value.empty() && value[0] == '=' )
			value.erase( value.begin() );
		FunctorManager::trim( name );
		FunctorManager::trim( value );

		BW::string watcherValue;
		BW::string desc;
		Watcher::Mode mode;

		if( Watcher::rootWatcher().getAsString( NULL, name.c_str(), watcherValue, desc, mode ) )
		{
			if( watcherValue == value )
				result = 1;
			else
				result = 0;
			return true;
		}
	}
#endif//ENABLE_WATCHERS

	return false;
}

bool WatcherFunctor::act( const BW::string& action, ItemPtr item, bool& result )
{
	BW_GUARD;

#if ENABLE_WATCHERS
	if( action.find( '=' ) != action.npos )
	{
		BW::string name = action.substr( 0, action.find( '=' ) );
		BW::string value = action.substr( action.find( '=' ) + 1 );
		FunctorManager::trim( name );
		FunctorManager::trim( value );

		if( Watcher::rootWatcher().setFromString( NULL, name.c_str(), value.c_str() ) )
		{
			result = true;
			return true;
		}
	}
#endif//ENABLE_WATCHERS

	return false;
}


END_GUI_NAMESPACE
BW_END_NAMESPACE

