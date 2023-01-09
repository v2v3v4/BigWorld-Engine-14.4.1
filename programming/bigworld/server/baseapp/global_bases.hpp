#ifndef GLOBAL_BASES_HPP
#define GLOBAL_BASES_HPP

#include "Python.h"

#include "cstdmf/binary_stream.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "network/basictypes.hpp"

#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

class Base;
class BaseAppMgrGateway;
class BaseEntityMailBox;
class BinaryIStream;


/*~ class NoModule.GlobalBases
 *  @components{ base }
 *  An instance of this class emulates a dictionary of bases or base mailboxes.
 *
 *  Code Example:
 *  @{
 *  globalBases = BigWorld.globalBases
 *  print "The main mission entity is", globalBases[ "MainMission" ]
 *  print "There are", len( globalBases ), "global bases."
 *  @}
 */
/**
 *	This class is used to expose the collection of global bases.
 */
class GlobalBases : public PyObjectPlus
{
	Py_Header( GlobalBases, PyObjectPlus )

public:
	GlobalBases( PyTypeObject * pType = &GlobalBases::s_type_ );
	~GlobalBases();

	PyObject * 			subscript( PyObject * entityID );
	int					length();

	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )
	PY_METHOD_DECLARE( py_get )

	static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
	static Py_ssize_t	s_length( PyObject * self );

	bool registerRequest( Base * pBase, PyObject * pKey,
		PyObject * pCallback );
	bool deregister( Base * pBase, PyObject * pKey );

	void onBaseDestroyed( Base * pBase );

	void add( BinaryIStream & data );
	void add( Base * pBase, const BW::string & pickledKey );

	void remove( BinaryIStream & data );
	void remove( const BW::string & pickledKey );

	void addLocalsToStream( BinaryOStream & stream ) const;

	void handleBaseAppDeath( const Mercury::Address & deadAddr );

	void resolveInvalidMailboxes();

private:
	void add( BaseEntityMailBox * pMailbox, const BW::string & pickledKey );

	bool removeKeyFromDict( PyObject * pKey );

	PyObject * pMap_;

	typedef BW::multimap< EntityID, BW::string > RegisteredBases;

	RegisteredBases registeredBases_;
};

#ifdef CODE_INLINE
// #include "global_bases.ipp"
#endif

BW_END_NAMESPACE

#endif // GLOBAL_BASES_HPP
