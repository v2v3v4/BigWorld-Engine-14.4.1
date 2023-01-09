#ifndef CONSOLE_MANAGER_HPP
#define CONSOLE_MANAGER_HPP

#include "cstdmf/config.hpp"

#if ENABLE_CONSOLES

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"

#include "input/input.hpp"


BW_BEGIN_NAMESPACE

class XConsole;

/**
 *	This class is the base class for all consoles.
 */
class ConsoleManager : public InputHandler
{
public:
	///	@name Constructors and Destructors.
	//@{
	ConsoleManager();
	~ConsoleManager();
	//@}

	void add( XConsole * pConsole, const char * label,
		KeyCode::Key key = KeyCode::KEY_NONE, uint32 modifiers = 0 );
	void draw( float dTime );

	static ConsoleManager & instance();

	static void createInstance();
	static void deleteInstance();

	void fini();

	/// @name Accessors
	//@{
	XConsole * pActiveConsole() const;
	XConsole * find( const char * name ) const;
	const BW::string & consoleName( XConsole * console ) const;
	//@}

	bool handleKeyEvent( const KeyEvent & event );
	bool handleAxisEvent( const AxisEvent & event );

	void activate( XConsole * pConsole );
	void activate( const char * label );
	void toggle( const char * label );
	void deactivate();

	bool darkenBackground() const;
	void setDarkenBackground(bool enable);

private:
	ConsoleManager(const ConsoleManager&);
	ConsoleManager& operator=(const ConsoleManager&);

	typedef BW::map< uint32, XConsole *> KeyedConsoles;
	typedef StringHashMap< XConsole *> LabelledConsoles;
	typedef BW::set< XConsole * > ConsoleSet;

	XConsole * pActiveConsole_;
	KeyedConsoles keyedConsoles_;
	LabelledConsoles labelledConsoles_;
	ConsoleSet consoleSet_;

	bool darkenBackground_;

	static ConsoleManager* s_instance_;
};


#ifdef CODE_INLINE
	#include "console_manager.ipp"
#endif

BW_END_NAMESPACE

#endif // CONSOLE_MANAGER_HPP
#endif // #if ENABLE_CONSOLES
