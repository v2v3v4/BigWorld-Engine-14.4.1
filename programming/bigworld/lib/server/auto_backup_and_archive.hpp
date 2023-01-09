#ifndef AUTO_BACKUP_AND_ARCHIVE_HPP
#define AUTO_BACKUP_AND_ARCHIVE_HPP

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

namespace AutoBackupAndArchive
{

/**
 *	Enum used for setting auto backup or auto archive settings.
 */
enum Policy
{
	NO = 0,
	YES = 1,
	NEXT_ONLY = 2,
};



void addNextOnlyConstant( PyObject * pModule );


const char * policyAsString( Policy policy );


} // end namespace AutoBackupAndArchive

namespace Script
{

int setData( PyObject * pObj,
		AutoBackupAndArchive::Policy & value, 
		const char * varName = "" );

} // end namespace Script

BW_END_NAMESPACE

#endif // AUTO_BACKUP_AND_ARCHIVE_HPP
