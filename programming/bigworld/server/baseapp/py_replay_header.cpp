#include "script/first_include.hpp"

#include "py_replay_header.hpp"


BW_BEGIN_NAMESPACE

/*~ class BigWorld.PyReplayHeader
 *	@components{ base }
 *
 *	This class contains a collection of data that is found in the replay file
 *	header.
 */
PY_TYPEOBJECT( PyReplayHeader )

PY_BEGIN_METHODS( PyReplayHeader )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyReplayHeader )
	/*~ attribute PyReplayHeader version 
 	 *	@components{ base }
	 *
	 *	The engine version, as a string.
	 */
	PY_ATTRIBUTE( version )

	/*~ attribute PyReplayHeader digest 
 	 *	@components{ base }
	 *
	 *	The entity defs digest, as a quoted string.
	 */
	PY_ATTRIBUTE( digest )

	/*~ attribute PyReplayHeader numTicks
 	 *	@components{ base }
	 *
	 *	The number of ticks reported in the header.
	 */
	PY_ATTRIBUTE( numTicks )

	/*~ attribute PyReplayHeader gameUpdateFrequency
 	 *	@components{ base }
	 *
	 * 	The game update frequency.
	 */
	PY_ATTRIBUTE( gameUpdateFrequency )

	/*~ attribute PyReplayHeader timestamp
 	 *	@components{ base }
	 *
	 *	The time that the recording was created, in number of seconds after
	 *	epoch.
	 */
	PY_ATTRIBUTE( timestamp )

	/*~ attribute PyReplayHeader reportedSignatureLength
 	 *	@components{ base }
	 *
	 *	The signature length as reported in the header.
	 */
	PY_ATTRIBUTE( reportedSignatureLength )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 *
 *	@param header 	A copy of the header to wrap. A new header instance is
 *					copied from the given value.
 */
PyReplayHeader::PyReplayHeader( const ReplayHeader & header, 
			PyTypeObject * pType ) :
		PyObjectPlus( pType ),
		header_( header )
{
}


/**
 * 	The string representation of this object.
 */
PyObject * PyReplayHeader::pyRepr() const
{
	return PyString_FromFormat( "<ReplayHeader v%s; digest=%s>",
		header_.version().c_str(), header_.digest().quote().c_str() );
}


BW_END_NAMESPACE


// py_replay_header.cpp
