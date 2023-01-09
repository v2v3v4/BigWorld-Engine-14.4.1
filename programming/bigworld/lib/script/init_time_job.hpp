#ifndef SCRIPT_INIT_TIME_JOB_HPP
#define SCRIPT_INIT_TIME_JOB_HPP

#include "cstdmf/bw_namespace.hpp"


BW_BEGIN_NAMESPACE

namespace Script
{

/**
 *	This class is a job that should be run at script init time.
 *	Simply derive from it and implement the init method and your
 *	job will be run immediately after scripts are initialised.
 *
 *	The rung specified in the constructor indicates how early on
 *	you want your job to run. Negative rungs are for 'before
 *	PyInitialise' and positive ones are for after that time.
 *	(although before PyInitialise ones may not actually be run
 *	before then - this is just to reduce possible conflicts).
 *	Rung zero is reserved for module link initialisations.
 *
 *	If an init time job is constructed after scripts have been
 *	initialised then it currently generates a critical error
 *	(it cannot call its init fn because the derived constructor
 *	 hasn't finished)
 */
class InitTimeJob
{
public:
	InitTimeJob( int rung );
	virtual ~InitTimeJob();

	virtual void init() = 0;
};

void runInitTimeJobs();

/**
 *	This class is a job that should be run at script fini time.
 *	Simply derive from it and implement the fini method and your
 *	job will be run immediately after scripts are finalised.
 *
 *	The rung specified in the constructor indicates how early on
 *	you want your job to run. Negative rungs are for 'after
 *	PyFinalize' and positive ones are for before that time.
 */
class FiniTimeJob
{
public:
	FiniTimeJob( int rung = 1 );
	virtual ~FiniTimeJob();

	virtual void fini() = 0;
};

void runFiniTimeJobs();

} // namespace Script

BW_END_NAMESPACE

#endif // SCRIPT_INIT_TIME_JOB_HPP
