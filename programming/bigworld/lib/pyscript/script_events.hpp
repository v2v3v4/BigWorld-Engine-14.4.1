#ifndef SCRIPT_EVENTS_HPP
#define SCRIPT_EVENTS_HPP

#include "Python.h"

#include "pyobject_pointer.hpp"

#include "script/script_object.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is a helper that is used by ScriptEvents.
 */
class ScriptEventList
{
public:
	bool triggerEvent( PyObject * pArgs, ScriptList resultsList );

	//TODO: make ScriptEventList use ScriptObject
	bool add( PyObject * pListener, int level );
	bool remove( PyObject * pListener );

private:
	class Element
	{
	public:
		Element( PyObject * pFunc, int level ) :
			pFunc_( pFunc ),
			level_( level )
		{
		}

		PyObject * call( PyObject * pArgs );

		int level() const	{ return level_; }
		PyObject * pFunc() const	{ return pFunc_.get(); }
		bool matches( PyObject * pFunc ) const
			{ return pFunc_.get() == pFunc; }

	private:
		PyObjectPtr pFunc_;
		int level_;
	};

	typedef BW::vector< Element > Container;
	Container container_;
};


/**
 *	This class maintains a collection of events and the listeners associated
 *	with those events.
 */
class ScriptEvents
{
public:
	ScriptEvents();
	~ScriptEvents();

	void initFromPersonality( ScriptModule personality );

	void createEventType( const char * eventName );
	void clear();
	
	bool triggerEvent( const char * eventName, PyObject * pArgs,
			ScriptList resultsList = ScriptList() );
	bool triggerTwoEvents( const char * event1, const char * event2,
			PyObject * pArgs );

	bool addEventListener( const char * eventName,
			PyObject * pListener, int level );
	bool removeEventListener( const char * eventName, PyObject * pListener );

private:
	typedef BW::map< BW::string, ScriptEventList > Container;
	Container container_;
};

BW_END_NAMESPACE

#endif // SCRIPT_EVENTS_HPP
