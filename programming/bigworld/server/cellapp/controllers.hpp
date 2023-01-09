#ifndef CONTROLLERS_HPP
#define CONTROLLERS_HPP

#include "Python.h"

#include "controller.hpp"

#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

class BinaryIStream;
class BinaryOStream;
class Entity;

/**
 *	This abstract class defines an interface to be used when examining a set of
 *	controllers associated with an Entity.
 */ 
class ControllersVisitor
{
public:
	/**
	 *	This method is visited for every controller known by a Controllers
	 *	object.
	 *
	 *	@returns Return true if the visiting should continue, false otherwise.
	 */
	virtual bool visit( ControllerPtr controller ) = 0;
};


class Controllers
{
public:
	Controllers();
	~Controllers();

	void readGhostsFromStream( BinaryIStream & data, Entity * pEntity );
	void readRealsFromStream( BinaryIStream & data, Entity * pEntity );

	void writeGhostsToStream( BinaryOStream & data );
	void writeRealsToStream( BinaryOStream & data );

	void createGhost( BinaryIStream & data, Entity * pEntity );
	void deleteGhost( BinaryIStream & data, Entity * pEntity );
	void updateGhost( BinaryIStream & data );

	ControllerID addController( ControllerPtr pController, int userArg,
			Entity * pEntity );
	bool delController( ControllerID id, Entity * pEntity,
			bool warnOnFailure = true );
	void modController( ControllerPtr pController, Entity * pEntity );

	void startReals();
	void stopReals( bool isFinalStop );

	PyObject * py_cancel( PyObject * args, Entity * pEntity );

	bool visitAll( ControllersVisitor & visitor );

private:
	ControllerID nextControllerID();

	typedef BW::map< ControllerID, ControllerPtr > Container;
	Container container_;

	ControllerID lastAllocatedID_;
};

BW_END_NAMESPACE

#endif // CONTROLLERS_HPP
