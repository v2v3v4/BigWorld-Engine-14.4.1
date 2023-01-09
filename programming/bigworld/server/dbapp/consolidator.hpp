#ifndef CONSOLIDATOR_HPP
#define CONSOLIDATOR_HPP

#include "network/interfaces.hpp"

#include "server/child_process.hpp"


BW_BEGIN_NAMESPACE

class DBApp;

namespace Mercury
{
class EventDispatcher;
}


/**
 *	This class wraps the functionality of calling the ConsolidateDBs process,
 *	and managing the child process events and notifying the DBApp on completion.
 */
class Consolidator : public IChildProcess
{
public:
	Consolidator( DBApp & dbApp );
	virtual ~Consolidator();

	bool startConsolidation();


private:
	Mercury::EventDispatcher & dispatcher();

	void outputErrorLogs() const;

	// IChildProcess interface
	virtual void onChildComplete( int status, ChildProcess * process );
	virtual void onChildAboutToExec();
	virtual void onExecFailure( int result );


	DBApp & dbApp_;

	ChildProcess * pChildProcess_;
};

BW_END_NAMESPACE

#endif // CONSOLIDATOR_HPP
