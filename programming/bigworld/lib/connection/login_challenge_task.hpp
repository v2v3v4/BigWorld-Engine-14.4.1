#ifndef LOGIN_CHALLENGE_TASK_HPP
#define LOGIN_CHALLENGE_TASK_HPP

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE

class LoginHandler;
typedef SmartPointer< LoginHandler > LoginHandlerPtr;
class LoginChallenge;
typedef SmartPointer< LoginChallenge > LoginChallengePtr;


/**
 *	This background task is used to perform a login challenge.
 */
class LoginChallengeTask : public BackgroundTask
{
public:
	LoginChallengeTask( LoginHandler & handler,	LoginChallengePtr pChallenge );

	~LoginChallengeTask();

	bool isFinished() const { return isFinished_; }
	void disassociate() { pHandler_ = NULL; }
	void perform();
	MemoryOStream & data() { return response_; }
	float calculationDuration() { return calculationDuration_; }

protected:
	void doBackgroundTask( TaskManager & mgr ) /* override */;
	void doMainThreadTask( TaskManager & mgr ) /* override */;

private:
	LoginHandlerPtr 	pHandler_;
	LoginChallengePtr 	pChallenge_;

	float 				calculationDuration_;
	MemoryOStream 		response_;
	bool				isFinished_;
};


BW_END_NAMESPACE


#endif // LOGIN_CHALLENGE_TASK_HPP
