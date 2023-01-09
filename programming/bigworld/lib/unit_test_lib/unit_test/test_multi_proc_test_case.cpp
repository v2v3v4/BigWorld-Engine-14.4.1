#include "CppUnitLite2/src/Test.h"
#include "unit_test_lib/multi_proc_test_case.hpp"

#include <sys/types.h>
#include <unistd.h>

BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: Example MultiProc Test cases
// -----------------------------------------------------------------------------

/**
 *	Example test case illustrating how to use the MultiProcTestCase class. This
 *	example will spawn processes that count up to a specified number every
 *	second.
 *
 *	Also refer to test_multi_proc_test_case.cpp for a test case that uses this
 *	example.
 */
class ExampleMultiProcChild: public MultiProcTestCase::ChildProcess
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param id 	the ID of this child, allocated from the factory. This gives
	 *				an easier way of identifying child processes rather than
	 *				from the PID of the child.
	 *	@param num 	the number of iterations
	 */
	ExampleMultiProcChild( int id, int num ):
			MultiProcTestCase::ChildProcess(),
			id_( id ),
			num_( num ),
			running_()
	{}

	/**	Destructor. */
	virtual ~ExampleMultiProcChild() {}

public: // from MultiProcTestCaseChild

	/**
	 *	The run() method. This is basically the main() method of the child
	 *	process.
	 */
	virtual int run()
	{
		printf( "ChildProcess %d (pid=%d) started\n", id_, getpid() );
		running_ = true;
		for (int i = 0; running_ && i < num_; ++i)
		{
			printf( "ChildProcess %d: count %d\n", id_, i );
			usleep( 1000000L );
		}

		int status = 0;
		printf( "ChildProcess %d exiting with status %d\n", id_, status );
		// the return value from this method is the exit status of this child
		// process
		return status;
	}

	virtual void stop()
	{
		running_ = false;
	}

private:
	int 	id_;		// the ID of the child process
	int 	num_;		// number of iterations
	bool	running_;	// running state
};

// -----------------------------------------------------------------------------
// Section: Tests
// -----------------------------------------------------------------------------

/**
 *	Tests the example multi-process child processes.
 */
TEST( multiProc )
{
	// make children count to 15
	const int ITER_MAX = 15;
	ExampleMultiProcChild main( 0, ITER_MAX );

	MultiProcTestCase mp( main );

	// 10 children, iterating 15 times each
	for (int i = 1; i < 10; ++i)
	{
		mp.runChild( new ExampleMultiProcChild( i, ITER_MAX ) );
	}

	// wait for our children to exit
	// and see if they all passed, assert it they don't
	MULTI_PROC_TEST_CASE_WAIT_FOR_CHILDREN( mp );

}

BW_END_NAMESPACE

// test_multi_proc_test_case.cpp
