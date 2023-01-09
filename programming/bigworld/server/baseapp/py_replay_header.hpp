#ifndef PY_REPLAY_HEADER_HPP
#define PY_REPLAY_HEADER_HPP

#include "pyscript/pyobject_plus.hpp"

#include "connection/replay_header.hpp"


BW_BEGIN_NAMESPACE


/**
 *	This class wraps a ReplayHeader object for exposure to Python.
 */
class PyReplayHeader : public PyObjectPlus
{
	Py_Header( PyReplayHeader, PyObjectPlus )

public:
	PyReplayHeader( const ReplayHeader & header,
		PyTypeObject * pType = &s_type_);
	
	PY_RO_ATTRIBUTE_DECLARE( header_.version().c_str(), version );
	PY_RO_ATTRIBUTE_DECLARE( header_.digest().quote(), digest );
	PY_RO_ATTRIBUTE_DECLARE( header_.numTicks(), numTicks );
	PY_RO_ATTRIBUTE_DECLARE( header_.gameUpdateFrequency(), 
		gameUpdateFrequency );
	PY_RO_ATTRIBUTE_DECLARE( header_.timestamp(), timestamp );
	PY_RO_ATTRIBUTE_DECLARE( header_.reportedSignatureLength(), 
		reportedSignatureLength );

	PyObject * pyRepr() const;

private:
	ReplayHeader header_;
};


BW_END_NAMESPACE


#endif // PY_REPLAY_HEADER_HPP
