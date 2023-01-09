#ifndef GLOB_HPP
#define GLOB_HPP

#include "cstdmf/bw_namespace.hpp"

#include <glob.h>
BW_BEGIN_NAMESPACE


/**
 *	This class wraps the glob() function, maintaining a glob_t, that has
 *	globfree() called on it when the object goes out of scope.
 */
class Glob
{
public:
	/**
	 *	Constructor.
	 */
	Glob() :
			context_()
	{
		memset( &context_, '\0', sizeof( context_ ) );
	}


	/** Destructor. */
	~Glob()
	{
		globfree( &context_ );
	}


	/**
	 *	Call glob(), matching against the given pattern.
	 *
	 *	@param pattern 	The glob pattern to match.
	 *	@param flags 	The glob flags.
	 *	@param errfunc 	The error function.
	 *
	 *	@return 		The return value from glob().
	 */
	int match( const char * pattern, int flags = 0,
			int errfunc( const char *, int ) = NULL,
			size_t numSlotsToReserve = 0 )
	{
		if (numSlotsToReserve)
		{
			flags |= GLOB_DOOFFS;
		}
		else
		{
			flags &= ~GLOB_DOOFFS;
		}

		context_.gl_offs = numSlotsToReserve;

		return glob( pattern, flags, errfunc, &context_ );
	}

	
	/**
	 *	This method clears any existing results.
	 */
	void clear()
	{
		globfree( &context_ );
		memset( &context_, '\0', sizeof( context_ ) );
	}

	
	/**
	 *	Return the number of the results returned.
	 */
	size_t size() const { return context_.gl_pathc; }

	/**
	 *	Return the result at the given index.
	 */
	const char * operator[]( size_t index ) const
	{
		return context_.gl_pathv[ index ];
	}


private:
	Glob( const Glob & other );
	Glob & operator=( const Glob & other );

	glob_t context_;
};

BW_END_NAMESPACE

#endif // GLOB_HPP
