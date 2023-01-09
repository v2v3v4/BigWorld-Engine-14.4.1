#ifndef EGEXTRA_HPP
#define EGEXTRA_HPP

#include "cellapp/entity_extra.hpp"


BW_BEGIN_NAMESPACE

/**
 * Simple example entity extra... can print a message to the screen.
 */

class EgExtra : public EntityExtra
{
	Py_EntityExtraHeader( EgExtra );

public:
	EgExtra( Entity & e );
	~EgExtra();

	PY_AUTO_METHOD_DECLARE( RETVOID, helloWorld, END );
	void helloWorld();

	PY_AUTO_METHOD_DECLARE( RETOWN, addEgController, ARG( int, END ) );
	PyObject * addEgController( int userArg );

	static const Instance<EgExtra> instance;
};

BW_END_NAMESPACE

#endif // EGEXTRA_HPP
