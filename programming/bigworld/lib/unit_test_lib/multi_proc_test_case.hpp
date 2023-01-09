#ifndef __MULTIPROC_TEST_CASE_HPP__
#define __MULTIPROC_TEST_CASE_HPP__

#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

#include "cstdmf/debug.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class acts as a base class for any object that needs to be able to pass
 *  up a failure message into an outer context.  This is necessitated by the
 *  fact that CppUnitLite2 doesn't support calling any of the assertion macros
 *  (e.g. CHECK(), CHECK_MESSAGE() etc) from anywhere other than a TEST() {}
 *  block.
 */
class TestCase
{
public:
	/**
	 *  This method indicates that this test case has failed.
	 */
	void fail( const BW::string & message )
	{
		BW::string messageWithNewline = message;
		messageWithNewline.push_back( '\n' );
		ERROR_MSG( "%s", messageWithNewline.c_str() );

		failureMsg_ = message;
	}

	/**
	 *  This method returns true if this test case has failed.
	 */
	bool hasFailed() const { return !failureMsg_.empty(); }


	/**
	 *  This method returns the failure message itself.
	 */
	const char * failureMsg() const { return failureMsg_.c_str(); }


private:
	BW::string failureMsg_;
};


// -----------------------------------------------------------------------------
// Section: MultiProcTestCase
// -----------------------------------------------------------------------------

/**
 *	This subclass of TestCase forks multiple processes as part of a
 *	multi-process fixture for unit testing.
 */
class MultiProcTestCase : public TestCase
{
public:
	class ChildProcess;

	MultiProcTestCase( ChildProcess & mainProcess ):
		mainProcess_( mainProcess ),
		statuses_(),
		pids_(),
		hasKilledChildren_( false )
	{}

	virtual ~MultiProcTestCase() {}

	/**
	 *	This class contains a method run() which is invoked n the child
	 *	process. The exit status is returned by run().
	 */
	class ChildProcess : public TestCase
	{
	public:
		virtual ~ChildProcess() {}

		/**
		 *	Run function.
		 *
		 *	@return the child exit status
		 */
		virtual int run() = 0;


		/**
		 *  This method should cause run() to terminate.
		 */
		virtual void stop() = 0;


		/**
		 *  This method stops this child process due to failure.
		 */
		void fail( const char * message )
		{
			this->TestCase::fail( message );
			this->stop();
		}
	};

	/**
	 *	A class for creating ChildProcess objects. Factory instances are passed
	 *	to the MultiProcTestCase for creating new
	 *	MultiProcTestCase::ChildProcess objects for child processes to run.
	 */
	class ChildProcessFactory
	{
	public:
		/**
		 *	Pure virtual factory method for creating
		 *	MultiProcTestCase::ChildProcess instances.
		 *
		 *	The invoker of this method is responsible for destroying the child
		 *	object when it is finished with it - this should be
		 *	MultiProcTestCase itself (see runChildren() implementation).
		 *
		 *	@return child object
		 */
		virtual ChildProcess * create() = 0;
	};

	bool hasRunningChildren();
	int numRunningChildren();

	void runChildren( int num,
		MultiProcTestCase::ChildProcessFactory * pFactory );

	bool runChild( ChildProcess * pChild );
	void killChildren();
	bool checkAllChildrenPass();
	void waitForAll();
	bool updateChildren( bool shouldBlock );

	const ChildProcess & mainProcess() const { return mainProcess_; }

protected:
	/// This isn't strictly a child process.  This is the run()'able thing that
	/// runs in the parent process.  It obeys the same interface as the child
	/// processes.
	ChildProcess & mainProcess_;

	typedef BW::map<int,int> Statuses;
	Statuses statuses_; // map pids -> exit status values

	typedef BW::set<int> ProcessIDs;
	ProcessIDs pids_;	// pids started by parent that are still outstanding

	/// This is true if child processes were terminated early due to a failure
	/// in the main process.
	bool hasKilledChildren_;
};


/**
 *  This macro should be called from within a TEST() block.
 */
#define MULTI_PROC_TEST_CASE_WAIT_FOR_CHILDREN( OBJ )						\
	{																		\
		OBJ.waitForAll();													\
																			\
		/* If the main process failed, display its failure message */		\
		ASSERT_WITH_MESSAGE(												\
			!OBJ.mainProcess().hasFailed(),									\
			OBJ.mainProcess().failureMsg() );								\
																			\
		/* Check that all children have passed OK */						\
		ASSERT_WITH_MESSAGE(												\
			OBJ.checkAllChildrenPass(),										\
			OBJ.failureMsg() );												\
	}																		\

#endif // __MULTIPROC_TEST_CASE_HPP__

BW_END_NAMESPACE

// multiproc_test_case.hpp
