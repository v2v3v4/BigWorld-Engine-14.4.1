#ifndef PICKLER_HPP
#define PICKLER_HPP

#include "script_object.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE

/**
 * 	This class is a wrapper around the Python pickle methods.
 * 	Essentially, it serialises and deserialises Python objects
 * 	into STL strings.
 *
 * 	@ingroup script
 */
class Pickler
{
public:
	static BW::string 		pickle( ScriptObject pObj );
	static ScriptObject 	unpickle( const BW::string & str );

	static bool			init();
	static void			finalise();
};

BW_END_NAMESPACE

#endif

