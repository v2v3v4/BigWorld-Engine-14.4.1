#ifndef PY_BILLING_SYSTEM_HPP
#define PY_BILLING_SYSTEM_HPP

#include <Python.h>
#include "pyscript/pyobject_pointer.hpp"

#include "db_storage/billing_system.hpp"


BW_BEGIN_NAMESPACE

class EntityDefs;


/**
 *	This class is used to integrate with a billing system via Python.
 */
class PyBillingSystem : public BillingSystem
{
public:
	PyBillingSystem( PyObject * pPyObject, const EntityDefs & entityDefs );
	~PyBillingSystem();

	virtual void getEntityKeyForAccount(
		const BW::string & username, const BW::string & password,
		const Mercury::Address & clientAddr,
		IGetEntityKeyForAccountHandler & handler );

	virtual void setEntityKeyForAccount( const BW::string & username,
		const BW::string & password, const EntityKey & ekey,
		ISetEntityKeyForAccountHandler & handler );

	virtual bool isOkay() const
	{
		return getEntityKeyFunc_;
	}

private:
	static PyObjectPtr getMember( PyObject * pPyObject, const char * member );

	const EntityDefs & entityDefs_;

	PyObjectPtr getEntityKeyFunc_;
	PyObjectPtr setEntityKeyFunc_;
};

BW_END_NAMESPACE

#endif // PY_BILLING_SYSTEM_HPP
