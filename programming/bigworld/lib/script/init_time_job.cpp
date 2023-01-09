#include "pch.hpp"
#include "init_time_job.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/stdmf.hpp"

#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"


BW_BEGIN_NAMESPACE

namespace Script
{

// -----------------------------------------------------------------------------
// Section: InitTimeJob
// -----------------------------------------------------------------------------

typedef BW::vector< InitTimeJob * >	InitTimeJobsVector;
typedef BW::map< int, InitTimeJobsVector >		InitTimeJobsMap;
static InitTimeJobsMap * s_initTimeJobsMap = NULL;
static int s_initTimeJobsMapLinksCount = 0;
static bool s_initTimeJobsInitted = false;

/**
 *	Constructor
 */
InitTimeJob::InitTimeJob( int rung )
{
	if (s_initTimeJobsInitted)
	{
		CRITICAL_MSG( "Script::InitTimeJob (rung %d) "
			"constructed after script init time!\n", rung );
		return;
	}

	if (s_initTimeJobsMap == NULL)
	{
		s_initTimeJobsMap = new InitTimeJobsMap();
	}
	(*s_initTimeJobsMap)[rung].push_back( this );
	++s_initTimeJobsMapLinksCount;
}


/**
 *	Destructor
 */
InitTimeJob::~InitTimeJob()
{
	--s_initTimeJobsMapLinksCount;

	if (s_initTimeJobsMapLinksCount == 0)
		bw_safe_delete(s_initTimeJobsMap);

	/*
	if (!s_initTimeJobsInitted)
	{
		ERROR_MSG( "Script::InitTimeJob "
			"destructed before script init time!\n" );
	}
	*/
}


/**
 *	This function runs init time jobs
 */
void runInitTimeJobs()
{
	if (s_initTimeJobsInitted)
	{
		ERROR_MSG( "Script::runInitTimeJobs called more than once\n" );
		return;
	}

	s_initTimeJobsInitted = true;

	if (s_initTimeJobsMap != NULL)
	{
		for (InitTimeJobsMap::iterator mit = s_initTimeJobsMap->begin();
			mit != s_initTimeJobsMap->end();
			mit++)
		{
			for (InitTimeJobsVector::iterator vit = mit->second.begin();
				vit != mit->second.end();
				vit++)
			{
				(*vit)->init();
			}
		}

		bw_safe_delete(s_initTimeJobsMap);
	}
}


// -----------------------------------------------------------------------------
// Section: FiniTimeJob
// -----------------------------------------------------------------------------

typedef BW::vector< FiniTimeJob * >	FiniTimeJobsVector;
typedef BW::map< int, FiniTimeJobsVector >		FiniTimeJobsMap;
static FiniTimeJobsMap * s_finiTimeJobsMap = NULL;
static int s_finiTimeJobsMapLinksCount = 0;
static bool s_finiTimeJobsRun = false;

/**
 *	Constructor
 */
FiniTimeJob::FiniTimeJob( int rung )
{
	if (s_finiTimeJobsRun)
	{
		CRITICAL_MSG( "Script::FiniTimeJob (rung %d) "
			"constructed after script finalised time!\n", rung );
		return;
	}

	if (s_finiTimeJobsMap == NULL)
	{
		s_finiTimeJobsMap = new FiniTimeJobsMap();
	}
	(*s_finiTimeJobsMap)[rung].push_back( this );
	++s_finiTimeJobsMapLinksCount;
}


/**
 *	Destructor
 */
FiniTimeJob::~FiniTimeJob()
{
	--s_finiTimeJobsMapLinksCount;

	if (s_finiTimeJobsMapLinksCount == 0)
		bw_safe_delete(s_finiTimeJobsMap);
}


/**
 *	This function runs fini time jobs
 */
void runFiniTimeJobs()
{
	if (s_finiTimeJobsRun)
	{
		ERROR_MSG( "Script::runFiniTimeJobs called more than once\n" );
		return;
	}

	s_finiTimeJobsRun = true;

	if (s_finiTimeJobsMap != NULL)
	{
		for (FiniTimeJobsMap::iterator mit = s_finiTimeJobsMap->begin();
			mit != s_finiTimeJobsMap->end();
			mit++)
		{
			for (FiniTimeJobsVector::iterator vit = mit->second.begin();
				vit != mit->second.end();
				vit++)
			{
				(*vit)->fini();
			}
		}

		bw_safe_delete(s_finiTimeJobsMap);
	}
}

} // namespace Script

BW_END_NAMESPACE

// init_time_job.cpp

