#ifndef SNAPSHOT_HELPER_CONSTANTS_HPP
#define SNAPSHOT_HELPER_CONSTANTS_HPP

#include <stdlib.h>

namespace SnapshotHelper
{
	/*
	 *	Return values used by SnapshotHelper that must be known by the
	 *	TrasferDB process.
	 */
	enum ReturnValues
	{
		SUCCESS = EXIT_SUCCESS,
		FAILURE = EXIT_FAILURE
	};
}

#endif // SNAPSHOT_HELPER_CONSTANTS_HPP
