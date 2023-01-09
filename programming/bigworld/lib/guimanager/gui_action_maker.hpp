#ifndef GUI_ACTION_MAKER_HPP__
#define GUI_ACTION_MAKER_HPP__


#include "gui_manager.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE


template< typename T, int index = 0 >
struct ActionMaker: public Action
{
	bool (T::*func_)( ItemPtr item);
	ActionMaker( BW::string names, bool (T::*func)( ItemPtr item ) )
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

	virtual ~ActionMaker()
	{
		BW_GUARD;

		if (GUI::Manager::pInstance())
		{
			GUI::Manager::instance().cppFunctor().remove( this );
		}
	}
	virtual bool act( ItemPtr item )
	{
		BW_GUARD;

		return ( ( ( T* )this ) ->* func_ )( item );
	}
};


END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif //GUI_ACTION_MAKER_HPP__
