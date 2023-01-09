#ifndef GUI_FUNCTOR_OPTION_HPP__
#define GUI_FUNCTOR_OPTION_HPP__

#include "gui_functor.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class OptionMap
{
public:
	virtual ~OptionMap(){}
	virtual BW::string get( const BW::string& key ) const = 0;
	virtual bool exist( const BW::string& key ) const = 0;
	virtual void set( const BW::string& key, const BW::string& value ) = 0;
};

class OptionFunctor : public Functor
{
	OptionMap* option_;
public:
	OptionFunctor();
	void setOption( OptionMap* option );
	virtual const BW::string& name() const;
	virtual bool text( const BW::string& textor, ItemPtr item, BW::string& result );
	virtual bool update( const BW::string& updater, ItemPtr item, unsigned int& result );
	virtual bool act( const BW::string& action, ItemPtr item, bool& result );
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_FUNCTOR_OPTION_HPP__
