#include "pch.hpp"

#if ENABLE_CONSOLES

#include "pyscript/script.hpp"
#include "console_manager.hpp"
#include "xconsole.hpp"
#include "moo/geometrics.hpp"

#include "input/ime.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "console_manager.ipp"
#endif

ConsoleManager* ConsoleManager::s_instance_ = NULL;


// -----------------------------------------------------------------------------
// Section: ConsoleManager
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ConsoleManager::ConsoleManager() : 
	pActiveConsole_( NULL ),
	darkenBackground_(true)
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
ConsoleManager::~ConsoleManager()
{
	BW_GUARD;
	fini();
}

/**
 *	Free allocated data.
 */
void ConsoleManager::fini()
{
	BW_GUARD;
	// TODO:PM We may need to be careful about consoles that are added twice
	// with different key bindings.

	ConsoleSet::iterator iter = consoleSet_.begin();

	while (iter != consoleSet_.end())
	{
		XConsole* console = *iter;
		bw_safe_delete( console );
		iter++;
	}
	consoleSet_.clear();
}


/**
 *	Create instance of this object.
 */
/*static*/ void ConsoleManager::createInstance()
{
	BW_GUARD;
	if (s_instance_ == NULL)
	{
		s_instance_ = new ConsoleManager();
		REGISTER_SINGLETON( ConsoleManager )
	}
}


/**
 *	Destroy the instance of this object.
 */
/*static*/ void ConsoleManager::deleteInstance()
{
	BW_GUARD;
	bw_safe_delete(s_instance_);
}


/**
 *	This method adds a console to be managed. The console manager owns the input
 *	console and is responsible for deleting it.
 *
 *	@param pConsole		The console to be managed.
 *	@param label		The label to associate with the console.
 *	@param key			The keyboard key to associate with the console.
 *	@param modifiers	The modifiers that must be pressed with 'key'.
 */
void ConsoleManager::add( XConsole * pConsole,
		const char * label,
		KeyCode::Key key,
		uint32 modifiers )
{
	BW_GUARD;
	if (key != KeyCode::KEY_NONE)
	{
		keyedConsoles_[ key | (modifiers << 24) ] = pConsole;
	}

	if (label != NULL)
	{
		labelledConsoles_[ label ] = pConsole;
	}

	// Consoles should currently only be added once.
	MF_ASSERT( consoleSet_.find( pConsole ) == consoleSet_.end() );

	consoleSet_.insert( pConsole );

	pConsole->init();
}


/**
 *	This method activates the console with the associated label.
 */
void ConsoleManager::activate( const char * label )
{
	BW_GUARD;
	LabelledConsoles::iterator iter = labelledConsoles_.find( label );

	MF_ASSERT( iter != labelledConsoles_.end() );

	if (iter != labelledConsoles_.end())
	{
		if (pActiveConsole_ != iter->second )
		{
			this->deactivate();
			this->activate( iter->second );
			pActiveConsole_->activate( false );
		}
	}
}

/**
 *	This method sets the current active console.
 */
void ConsoleManager::activate( XConsole * pConsole )
{
	BW_GUARD;
	pActiveConsole_ = pConsole;

	if (IME::pInstance())
	{
		bool allowIME = pActiveConsole_ == NULL && !InputDevices::isKeyDown( KeyCode::KEY_DEBUG );
		IME::instance().allowEnable( allowIME );
	}
}


/**
 *	This method toggles the activation of the console with the associated
 *	label.
 */
void ConsoleManager::toggle( const char * label )
{
	BW_GUARD;
	LabelledConsoles::iterator iter = labelledConsoles_.find( label );

	MF_ASSERT( iter != labelledConsoles_.end() );

	if (iter != labelledConsoles_.end())
	{
		if (pActiveConsole_ == iter->second )
		{
			pActiveConsole_->activate( true );
			this->deactivate();
		}
		else
		{
			this->deactivate();
			this->activate( iter->second );
			pActiveConsole_->activate( pActiveConsole_ != iter->second );
		}
	}
}


/**
 *	This method draws the active console, if necessary.
 */
void ConsoleManager::draw( float dTime )
{
	BW_GUARD;
	if (pActiveConsole_ != NULL)
	{
		if (this->darkenBackground() && pActiveConsole_->allowDarkBackground())
		{
			Geometrics::drawRect( 
				Vector2::zero(), 
				Vector2( Moo::rc().screenWidth(), Moo::rc().screenHeight() ),
				0xc0202020 );
		}
		pActiveConsole_->draw( dTime );
	}
}


/**
 *	This method returns the console associated with the input label.
 */
XConsole * ConsoleManager::find( const char * name ) const
{
	BW_GUARD;
	LabelledConsoles::const_iterator iter = labelledConsoles_.find( name );

	return iter != labelledConsoles_.end() ? iter->second : NULL;
}

/**
 *	Given a console, this determines the name of that console. Returns "" if not found.
 */
const BW::string & ConsoleManager::consoleName( XConsole * console ) const
{
	BW_GUARD;
	for (LabelledConsoles::const_iterator iter = labelledConsoles_.begin();
		iter != labelledConsoles_.end(); iter++)
	{
		if (iter->second == console)
		{
			return iter->first;
		}
	}

	static BW::string s_emptyString = "";
	return s_emptyString;
}


// -----------------------------------------------------------------------------
// Section: InputHandler overrides
// -----------------------------------------------------------------------------

/**
 *	This method handles key events.
 */
bool ConsoleManager::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = false;

	KeyedConsoles::iterator iter =
		keyedConsoles_.find( event.key() | (event.modifiers() << 24) );

	if (iter != keyedConsoles_.end())
	{
		if (event.isKeyDown())
		{
			bool isReactivate = (iter->second == pActiveConsole_);

			if( !isReactivate )
				this->deactivate();

			pActiveConsole_ = iter->second;

			pActiveConsole_->activate( isReactivate );
		}

		handled = true;
	}
	else if (pActiveConsole_ != NULL)
	{
		InputHandler * pHandler = pActiveConsole_->asInputHandler();

		if (pHandler)
		{
			handled = pHandler->handleKeyEvent( event );
		}

		if (!handled &&
			event.key() == KeyCode::KEY_ESCAPE &&
			event.isKeyDown())
		{
			this->deactivate();
			handled = true;
		}
	}

	return handled;
}


/**
 *	This method handles axis events.
 */
bool ConsoleManager::handleAxisEvent( const AxisEvent & event )
{
	BW_GUARD;
	bool handled = false;

	if (pActiveConsole_ != NULL)
	{
		InputHandler * pHandler = pActiveConsole_->asInputHandler();

		if (pHandler)
		{
			handled = pHandler->handleAxisEvent( event );
		}
	}

	return handled;
}


/**
 *	This method clears the active console.
 */
void ConsoleManager::deactivate()
{
	BW_GUARD;
	if (pActiveConsole_ != NULL) 
	{
		pActiveConsole_->deactivate();
		pActiveConsole_ = NULL;
	}

	if (IME::pInstance())
	{
		IME::instance().allowEnable( !InputDevices::isKeyDown( KeyCode::KEY_DEBUG ) );
	}
}


/**
 *	Returns true if the a dark background is to be 
 *	drawn underneat the current active console.
 */
bool ConsoleManager::darkenBackground() const
{
	BW_GUARD;
	return this->darkenBackground_;
}


/**
 *	Sets wether a dark background is to be 
 *	drawn underneat the current active console.
 */
void ConsoleManager::setDarkenBackground(bool enable)
{
	BW_GUARD;
	this->darkenBackground_ = enable;
}


/*~	function BigWorld.darkenConsoleBackground
 *	@components{ client, tools }
 *
 *	Sets the dark-console-background flag. When true, a dark background will be 
 *	rendered underneat the current active console (if any), making the console 
 *	text easier to read. If false, console text will be rendered directly over
 *	the current contents of the color buffer. Some console may inhibit the 
 *	renderting of the dark background (e.g. the histogram console).
 *	
 *	@param	enable	(optional) new value to be assigned to the 
 *					dark-console-background flag.
 *
 *	@return			the current value (before the call is processed).
 *
 */
PyObject * py_darkenConsoleBackground(PyObject * args)
{
	BW_GUARD;
	bool currentValue = ConsoleManager::instance().darkenBackground();
	if (PyTuple_Size(args) > 0)
	{
		bool enable = false;
		if (!PyArg_ParseTuple(args, "b", &enable))
		{
			PyErr_SetString( PyExc_TypeError, "BigWorld.darkenConsoleBackground: "
				"Argument parsing error: Expected a boolean" );
			return NULL;
		}
		ConsoleManager::instance().setDarkenBackground(enable);
	}
	return Script::getData(currentValue);
}
PY_MODULE_FUNCTION( darkenConsoleBackground, BigWorld )

BW_END_NAMESPACE

// console_manager.cpp
#endif // ENABLE_CONSOLES
