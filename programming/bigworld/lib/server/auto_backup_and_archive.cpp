#include "script/first_include.hpp"

#include "auto_backup_and_archive.hpp"

BW_BEGIN_NAMESPACE

namespace Script
{

int setData( PyObject * pObj,
		AutoBackupAndArchive::Policy & value, 
		const char * varName )
{
	int intVal = int(value);

	int result = Script::setData( pObj, intVal, varName );

	if ((result == 0) &&
			(0 <= intVal) && (intVal <= 2))
	{
		value = AutoBackupAndArchive::Policy( intVal );
		return 0;
	}
	else
	{
		PyErr_Format( PyExc_ValueError,
				"%s must be set to an integer between 0 and 2", varName );
		return -1;
	}
}

} // namespace Script

void AutoBackupAndArchive::addNextOnlyConstant( PyObject * pModule )
{

	PyObjectPtr pNextOnly( Script::getData( AutoBackupAndArchive::NEXT_ONLY ),
		PyObjectPtr::STEAL_REFERENCE );
	PyObject_SetAttrString( pModule, "NEXT_ONLY", pNextOnly.get() );
}


const char * AutoBackupAndArchive::policyAsString(
		AutoBackupAndArchive::Policy policy )
{
	switch (policy)
	{
		case NO:
			return "False";
			break;
		case YES:
			return "True";
			break;
		default:
			return "BigWorld.NEXT_ONLY";
			break;
	}
}



BW_END_NAMESPACE

// auto_backup_and_archive.cpp
