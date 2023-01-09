#include "pch.hpp"
#include "personality.hpp"
#include "script.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )

BW_BEGIN_NAMESPACE

namespace Personality
{

ScriptModule s_pInstance_;

const char * DEFAULT_NAME = DEFAULT_PERSONALITY_NAME;

BW::string s_personalityName;


/**
 *  Script FiniTimeJob to make sure this module is cleaned up before script
 *  shutdown.
 */
class PersonalityFiniTimeJob : public Script::FiniTimeJob
{
public:
	PersonalityFiniTimeJob( int rung = INT_MAX ) :
		Script::FiniTimeJob( rung )
	{ }

private:
	virtual void fini()
	{
		if (s_pInstance_)
		{
			s_pInstance_.callMethod( "onFini", ScriptErrorPrint( "onFini" ),
				/* allowNullMethod */ true );
			s_pInstance_ = ScriptModule();
		}
		delete this;
	}
};


/**
 *	Import and return the personality module, If the module could be found,
 *	a default ScriptModule() is returned and no Python exception state is set.
 */
ScriptModule import( const BW::string &name )
{
	// Don't do this twice
	if (s_pInstance_)
	{
		WARNING_MSG( "Personality::init: Called twice\n" );
		return s_pInstance_;
	}

	s_personalityName = name;

	BW::string importError( "Personality::import: "
		"Failed to import personality module" );
	importError += name;
	
	s_pInstance_ = ScriptModule::import( name.c_str(),
		ScriptErrorPrint( importError.c_str() ) );

	return s_pInstance_;
}


bool callOnInit( bool isReload )
{
	if (s_pInstance_)
	{
		// Register fini time job to make sure this module is cleaned up
		new PersonalityFiniTimeJob();

		s_pInstance_.callMethod( "onInit", ScriptArgs::create( isReload ),
			ScriptErrorPrint( "onInit" ), /* allowNullMethod */ true );
	}

	return true;
}


/**
 *  Get a borrowed reference to the personality module. You must have
 *  successfully called Personality::import() before doing this.
 */
ScriptModule instance()
{
	return s_pInstance_;
}


/**
 *	This method returns a member of the personality module. If the member
 *	could be found, a default ScriptObject() is returned and no Python exception
 *	state is set.
 */
ScriptObject getMember( const char * name )
{
	if (!s_pInstance_)
	{
		return ScriptObject();
	}

	ScriptObject member = s_pInstance_.getAttribute( name, ScriptErrorClear() );

	return member;
}


/**
 *	This method returns a member of the personality module. If the member
 *	could be found, a default ScriptObject() is returned and no Python exception
 *	state is set.
 */
ScriptObject getMember( const char * currentName, const char * deprecatedName )
{
	ScriptObject member = getMember( currentName );

	if (member)
	{
		return member;
	}

	member = getMember( deprecatedName );

	if (member)
	{
		NOTICE_MSG( "Failed to find %s.%s, using %s.%s instead\n",
				s_personalityName.c_str(), currentName,
				s_personalityName.c_str(), deprecatedName );
	}

	return member;
}

} // namespace Personality

BW_END_NAMESPACE

// personality.cpp
