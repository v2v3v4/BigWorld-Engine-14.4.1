#ifndef PERSONALITY_HPP
#define PERSONALITY_HPP

#include "pyobject_plus.hpp"

#define DEFAULT_PERSONALITY_NAME "BWPersonality"

BW_BEGIN_NAMESPACE

/**
 *	This namespace manages the personality script.
 */
namespace Personality
{

extern const char *DEFAULT_NAME;
ScriptModule import( const BW::string &name );
ScriptModule instance();

bool callOnInit( bool isReload = false );

ScriptObject getMember( const char * name );
ScriptObject getMember( const char * currentName, const char * deprecatedName );

} // namespace Personality

BW_END_NAMESPACE

#endif // PERSONALITY_HPP
