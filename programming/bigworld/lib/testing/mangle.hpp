#ifndef MANGLE_HPP
#define MANGLE_HPP

#include <stdlib.h>

namespace Testing
{

extern float g_mangleProb;

inline float frand()
{
	return ((float)random())/RAND_MAX;
}

inline char& mangle( char &c )
{
	return c = (char)(random() & 0xff);
}

inline void mangleSingleByte( void *data, size_t length )
{
	if (length > 0)
	{
		int offset = random() % length;
		mangle( *((char*)data + offset) );
	}
}
	
template <class T>
T& mangle( T &t )
{
	mangleSingleByte( &t, sizeof( t ) );
	return t;
}

inline bool unlucky() { return frand() < g_mangleProb; }
	
}; // namespace Testing

#endif // MANGLE_HPP
