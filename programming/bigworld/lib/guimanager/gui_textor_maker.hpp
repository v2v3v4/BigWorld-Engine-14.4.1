#ifndef GUI_TEXTOR_MAKER_HPP__
#define GUI_TEXTOR_MAKER_HPP__


#include "gui_manager.hpp"


BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE


template< typename T, int index = 0 >
struct TextorMaker: public Textor
{
	BW::string (T::*func_)( ItemPtr item );
	TextorMaker( const BW::string& name, BW::string (T::*func)( ItemPtr item ) )
		: func_( func )
	{
		BW_GUARD;

		if (GUI::Manager::pInstance())
		{
			GUI::Manager::instance().cppFunctor().set( name, this );
		}
	}
	~TextorMaker()
	{
		BW_GUARD;

		if (GUI::Manager::pInstance())
		{
			GUI::Manager::instance().cppFunctor().remove( this );
		}
	}
	virtual BW::string text( ItemPtr item )
	{
		BW_GUARD;

		return ( ( ( T* )this ) ->* func_ )( item );
	}
};


END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif //GUI_TEXTOR_MAKER_HPP__
