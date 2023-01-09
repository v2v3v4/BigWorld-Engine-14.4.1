#ifndef GUI_FUNCTOR_PYTHON_HPP__
#define GUI_FUNCTOR_PYTHON_HPP__

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "gui_functor.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class PythonFunctor : public Functor
{
	BW::string defaultModule_;
	BW::map<BW::string, PyObject*> modules_;

	PyObject* call( BW::string function, ItemPtr item );

public:
	PythonFunctor() : defaultModule_( "UIExt" ) {}

	virtual const BW::string& name() const;
	virtual bool text( const BW::string& textor, ItemPtr item, BW::string& result );
	virtual bool update( const BW::string& updater, ItemPtr item, unsigned int& result );
	virtual bool act( const BW::string& action, ItemPtr item, bool& result );
	virtual void defaultModule( const BW::string& module );
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_FUNCTOR_PYTHON_HPP__
