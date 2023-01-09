#ifndef GUI_UPDATER_MAKER_HPP__
#define GUI_UPDATER_MAKER_HPP__


#include "gui_manager.hpp"


BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE


template< typename T, int index = 0 >
struct UpdaterMaker: public Updater
{
	unsigned int (T::*func_)( ItemPtr item );
	UpdaterMaker( BW::string names, unsigned int (T::*func)( ItemPtr item ) )
		: func_( func )
	{
		BW_GUARD;

		if (GUI::Manager::pInstance())
		{
			while (!names.empty())
			{
				BW::string::size_type delim = names.find_first_of( GUI_DELIMITER );
				BW::string name = names.substr( 0, delim );

				GUI::Manager::instance().cppFunctor().set( name, this );

				if (delim == names.npos)
				{
					break;
				}

				names = names.substr( delim + 1 );
			}
		}
	}

	virtual ~UpdaterMaker()
	{
		BW_GUARD;

		if (GUI::Manager::pInstance())
		{
			GUI::Manager::instance().cppFunctor().remove( this );
		}
	}
	virtual unsigned int update( ItemPtr item )
	{
		BW_GUARD;

		return ( ( ( T* )this ) ->* func_ )( item );
	}
};


END_GUI_NAMESPACE
BW_END_NAMESPACE


#endif //GUI_UPDATER_MAKER_HPP__
