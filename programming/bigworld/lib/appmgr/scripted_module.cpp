#include "pch.hpp"
#include "scripted_module.hpp"

#include "factory.hpp"
#include "module.hpp"
#include "module_manager.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/mouse_cursor.hpp"
#include "cstdmf/debug.hpp"
#include "pyscript/script.hpp"
#include "input/py_input.hpp"
#include "moo/render_context.hpp"

#ifndef CODE_INLINE
#include "scripted_module.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Module", 0 )

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

typedef ModuleManager ModuleFactory;

IMPLEMENT_CREATOR(ScriptedModule, Module);
int ScriptedModule_token;

/**
 *	Constructor.
 */
ScriptedModule::ScriptedModule() :
	pScriptObject_( NULL )
{
}


/**
 *	Destructor.
 */
ScriptedModule::~ScriptedModule()
{
	Py_XDECREF( pScriptObject_ );
	pScriptObject_ = NULL;
}


/**
 *	This method initialises a scripted module, given a data section.
 *	The data section must contain a nested ;
 *
 *	<script>
 *		<module>	moduleName.py	</module>
 *		<class>		MyClassName		</class>
 *	</script>
 *
 *	@param	pSection	The section to glean information from.
 *	@return True if the python script was loaded.
 */
bool ScriptedModule::init( DataSectionPtr pSection )
{
	bool result = false;

	if (!pSection)
	{
		return false;
	}

	DataSectionPtr pScript = pSection->openSection( "script" );

	if (pScript)
	{
		PyObject * pModule = PyImport_ImportModule( const_cast<char *>(
				pScript->readString( "module", "Modules" ).c_str() ) );

		if (PyErr_Occurred())
		{
			PyErr_Print();
			PyErr_Clear();
		}

		if (pModule != NULL)
		{
			Py_XDECREF( pScriptObject_ );

			BW::string className = pScript->readString( "class" );

			pScriptObject_ = PyObject_CallMethod( pModule, 
				const_cast<char *>( className.c_str() )
				, "" );

			if (PyErr_Occurred())
			{
				PyErr_Print();
				PyErr_Clear();
			}

			if (pScriptObject_ == NULL)
			{
				WARNING_MSG( "No script object %s\n", className.c_str() );
			}
			else
			{
				result = true;
			}

			Py_XDECREF( pModule );
		}
		else
		{
			WARNING_MSG( "Could not get module %s\n",
				pScript->readString( "module", "Modules" ).c_str() );
			PyErr_Print();
		}
	}
	else
	{
		WARNING_MSG( "Could not find 'script' section\n" );
	}

	return result;
}


/**
 *	This method implements the virtual method Module::onStart()
 *
 *	@see Module::onStart()
 */
void ScriptedModule::onStart()
{
	SimpleGUI::instance().mouseCursor().visible( true );

	if (pScriptObject_ != NULL)
	{
		Script::call(
			PyObject_GetAttrString( pScriptObject_, "onStart" ),
			PyTuple_New( 0 ),
			"ScriptedModule::onStart" );
	}
}


/**
 *	This method implements the virtual method Module::onStop()
 *
 *	@see Module::onStop()
 */
int ScriptedModule::onStop()
{
	SimpleGUI::instance().mouseCursor().visible( false );

	int result = -1;

	if (pScriptObject_ != NULL)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_, "onStop" ),
			PyTuple_New( 0 ),
			"ScriptedModule::onStop: " );

		Script::setAnswer( pResult, result, "ScriptedModule::onStop" );
	}

	return result;
}


/**
 *	This method implements the virtual method Module::onPause()
 *
 *	@see Module::onPause()
 */
void ScriptedModule::onPause()
{
	SimpleGUI::instance().mouseCursor().visible( false );

	if (pScriptObject_ != NULL)
	{
		Script::call(
			PyObject_GetAttrString( pScriptObject_, "onPause" ),
			PyTuple_New( 0 ),
			"ScriptedModule::onPause" );
	}
}


/**
 *	This method implements the virtual method FrameworkModule::updateState()
 *
 *	@see Module::updateState()
 */
bool ScriptedModule::updateState( float dTime )
{
	bool handled = false;

	if (pScriptObject_ != NULL)
	{	
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_, "updateState" ),
			Py_BuildValue( "(f)", dTime ),
			"ScriptedModule::updateState: ",
			true );

		Script::setAnswer( pResult, handled, "ScriptedModule::updateState" );
	}

	if (!handled)
	{
		SimpleGUI::instance().update( dTime );
	}

	return true;
}


/**
 *	Updates animated objects in this module.
 */
void ScriptedModule::updateAnimations()
{
	BW_GUARD;

	bool handled = false;

	if (pScriptObject_ != NULL)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_, "updateAnimations" ),
			PyTuple_New( 0 ),
			"ScriptedModule::updateAnimations",
			true );

		Script::setAnswer(
			pResult, handled, "ScriptedModule::updateAnimations" );
	}
}


/**
 *	This method implements the virtual method FrameworkModule::render()
 *
 *	@see Module::render()
 */
void ScriptedModule::render( float dTime )
{
	bool handled = false;

	if (pScriptObject_ != NULL)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_, "render" ),
			Py_BuildValue( "(f)", dTime ),
			"ScriptedModule::render",
			true );

		Script::setAnswer( pResult, handled, "ScriptedModule::render" );
	}

	if (!handled)
	{
		Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			0x202020, 1, 0 );

		SimpleGUI::instance().draw();
		handled = true;
	}
}


/**
 *	This method implements the virtual method Module::onResume()
 *
 *	@see Module::onResume()
 */
void ScriptedModule::onResume( int exitCode )
{
	SimpleGUI::instance().mouseCursor().visible(  true );

	if (pScriptObject_ != NULL)
	{
		Script::call(
			PyObject_GetAttrString( pScriptObject_, "onResume" ),
			Py_BuildValue( "(i)", exitCode ),
			"ScriptedModule::onResume" );
	}
}


/**
 *	This method implements the virtual method InputHandler::handleKeyEvent()
 *
 *	@see InputHandler::handleKeyEvent()
 */
bool ScriptedModule::handleKeyEvent( const KeyEvent & event )
{
	bool handled = false;

	if (pScriptObject_ != NULL)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_, "onKeyEvent" ),
			Py_BuildValue("(N)", Script::getData(event) ), 
			"ScriptedModule::handleKeyEvent: ",
			true );

		Script::setAnswer( pResult, handled, "ScriptedModule::handleKeyEvent" );
	}

	if (!handled)
	{
		handled = SimpleGUI::instance().handleKeyEvent( event );
	}

	return handled;
}


/**
 *	This method implements the virtual method InputHandler::handleMouseEvent()
 *
 *	@see InputHandler::handleMouseEvent()
 */
bool ScriptedModule::handleMouseEvent( const MouseEvent & event )
{
	bool handled = false;

	if (pScriptObject_ != NULL)
	{
		PyObject * pResult = Script::ask(
			PyObject_GetAttrString( pScriptObject_, "onMouseEvent" ),
			Py_BuildValue("(N)", Script::getData(event) ),
			"ScriptedModule::handleMouseEvent: ",
			true );

		Script::setAnswer( pResult, handled,
			"ScriptedModule::handleMouseEvent" );
	}

	if (!handled)
	{
		handled = SimpleGUI::instance().handleMouseEvent( event );
	}

	return handled;
}

BW_END_NAMESPACE

// scripted_module.cpp
