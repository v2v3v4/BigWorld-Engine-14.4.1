#ifndef TIMER_CONTROLLER_HPP
#define TIMER_CONTROLLER_HPP

#include "controller.hpp"
#include "cstdmf/time_queue.hpp"

#include "pyscript/script.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class implements server objects owned by an entity script, that move
 *	between cells when the script object moves.
 */
class TimerController : public Controller
{
	DECLARE_CONTROLLER_TYPE( TimerController )

public:
	TimerController( GameTime start = 0, GameTime interval = 0 );

	void				writeRealToStream( BinaryOStream & stream );
	bool 				readRealFromStream( BinaryIStream & stream );

	void				handleTimeout();
	void				onHandlerRelease();

	// Controller overrides
	virtual void		startReal( bool isInitialStart );
	virtual void		stopReal( bool isFinalStop );

	static FactoryFnRet New( float initialOffset, float repeatOffset,
		int userArg = 0 );
	PY_AUTO_CONTROLLER_FACTORY_DECLARE( TimerController,
		ARG( float, OPTARG( float, 0.f, OPTARG( int, 0, END ) ) ) )

private:

	/**
	 *	Handler for a timer to go into the global time queue
	 */
	class Handler : public TimerHandler
	{
	public:
		Handler( TimerController * pController );

		void pController( TimerController * pController )
									{ pController_ = pController; }

	private:
		// Overrides from TimerHandler
		virtual void	handleTimeout( TimerHandle handle, void * pUser );
		virtual void	onRelease( TimerHandle handle, void  * pUser );

		TimerController *	pController_;
	};

	Handler *		pHandler_;

	GameTime		start_;
	GameTime		interval_;
	TimerHandle		timerHandle_;
};

BW_END_NAMESPACE

#endif // TIMER_CONTROLLER_HPP
