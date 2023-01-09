#ifndef GLOBAL_EMBODIMENTS_HPP
#define GLOBAL_EMBODIMENTS_HPP

#include "forward_declarations.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class allows any ChunkEmbodiments to be in the world,
 *	without being attached to an entity or another model.
 *	Anything that can be converted to a ChunkEmbodiment is allowed here.
 */
class GlobalEmbodiments
{
public:
	static void tick( float dTime );
	static void fini();	
};

BW_END_NAMESPACE


#endif // GLOBAL_EMBODIMENTS_HPP