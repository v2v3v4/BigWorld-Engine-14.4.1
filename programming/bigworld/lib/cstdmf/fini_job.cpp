#include "pch.hpp"

#include "stdmf.hpp"

#include "fini_job.hpp"

#include <cstddef>
#include "cstdmf/bw_list.hpp"

BW_BEGIN_NAMESPACE

namespace
{

typedef BW::list< FiniJob * > FiniJobs;
BW::list< FiniJob * > * s_pFiniJobs = NULL;

class FiniRunner
{
public:
	~FiniRunner()
	{
		FiniJob::runAll();
	}
};

FiniRunner finiRunner;

} // anonymous namespace


/**
 *	This static method runs all of the FiniJob jobs.
 */
bool FiniJob::runAll()
{
	bool result = true;

	if (s_pFiniJobs != NULL)
	{
		FiniJobs::iterator iter = s_pFiniJobs->begin();

		while (iter != s_pFiniJobs->end())
		{
			FiniJob * pJob = *iter;
			result &= pJob->fini();
			bw_safe_delete(pJob);

			++iter;
		}

		bw_safe_delete(s_pFiniJobs);
	}

	return result;
}


/**
 *	Constructor.
 */
FiniJob::FiniJob()
{
	if (s_pFiniJobs == NULL)
	{
		s_pFiniJobs = new FiniJobs;
	}

	s_pFiniJobs->push_back( this );
}

BW_END_NAMESPACE

// fini_job.cpp
