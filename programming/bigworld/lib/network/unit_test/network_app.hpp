#ifndef NETWORK_APP_HPP
#define NETWORK_APP_HPP

// #include "unit_test_lib/multi_proc_test_case.hpp"
#include "CppUnitLite2/src/CppUnitLite2.h"

#include "network/event_dispatcher.hpp"
#include "network/interfaces.hpp"
#include "network/network_interface.hpp"
#include "cstdmf/timestamp.hpp"


BW_BEGIN_NAMESPACE

namespace Mercury
{
class NetworkInterface;
}

// -----------------------------------------------------------------------------
// Section: NetworkApp
// -----------------------------------------------------------------------------

/**
 *	This class is used as a common base class for network apps.
 */
class NetworkApp : public TimerHandler
{
public:
	NetworkApp( Mercury::EventDispatcher & mainDispatcher, 
			Mercury::NetworkInterfaceType networkInterfaceType,
			TestResult & result, const char * testName ) :
		mainDispatcher_( mainDispatcher ),
		result_( result ), 
		m_name( testName ),
		interface_( &mainDispatcher_, networkInterfaceType ),
		timerHandle_()
	{
		static const uint NUM_BIND_ATTEMPTS = 5;
		uint numAttempts = 0;
		while (!interface_.isGood() && (numAttempts < NUM_BIND_ATTEMPTS))
		{
			ERROR_MSG( "NetworkApp::NetworkApp: "
					"Interface failed to bind to socket, retrying (%u/%u)\n", 
				numAttempts + 1, NUM_BIND_ATTEMPTS );

			interface_.recreateListeningSocket( 0, NULL );
			++numAttempts;
		}
	}

	virtual ~NetworkApp()
	{
		this->stopTimer();
	}

	Mercury::NetworkInterface & networkInterface()
	{
		return interface_;
	}

	virtual void handleTimeout( TimerHandle handle, void * arg ) {}

	virtual int run()
	{
		// Guarantee a proper random seed in each test app
		srand( (int)timestamp() );

		this->dispatcher().processUntilBreak();

		return 0;
	}

protected:
	Mercury::EventDispatcher & dispatcher()
	{
		return mainDispatcher_;
	}

	void startTimer( uint tickRate, void * arg = 0 )
	{
		if (timerHandle_.isSet())
		{
			WARNING_MSG( "App::startTimer: Already has a timer\n" );
			this->stopTimer();
		}

		timerHandle_ = this->dispatcher().addTimer( tickRate, this, arg );
	}

	void stopTimer()
	{
		timerHandle_.cancel();
	}

	virtual void stop()
	{
		this->dispatcher().breakProcessing();
	}


	Mercury::EventDispatcher & mainDispatcher_;

	// These make this class look like a Test so that we can use the ASSERT
	// macros.
	TestResult & result_;
	const char * m_name;

	Mercury::NetworkInterface interface_;
	TimerHandle timerHandle_;
};

#define TEST_ARGS result_, m_name
#define TEST_ARGS_PROTO TestResult & result_, const char * m_name


#if 0
/**
 *  Assertions that should be used in NetworkApps inside methods that have a
 *  void return type.
 */
#define NETWORK_APP_FAIL( MESSAGE )											\
	{																		\
		this->fail( MESSAGE );												\
		return;																\
	}

#define NETWORK_APP_ASSERT( COND )											\
	if (!(COND))															\
	{																		\
		this->fail( #COND );												\
		return;																\
	}																		\

#define NETWORK_APP_ASSERT_WITH_MESSAGE( COND, MESSAGE )					\
	if (!(COND))															\
	{																		\
		this->fail( MESSAGE );												\
		return;																\
	}																		\


/**
 *  Assertions that should be used in NetworkApps inside methods that have a
 *  non-void return type.
 */
#define NETWORK_APP_FAIL_RET( MESSAGE, RET )								\
	{																		\
		this->fail( MESSAGE );												\
		return RET;															\
	}

#define NETWORK_APP_ASSERT_RET( COND, RET )									\
	if (!(COND))															\
	{																		\
		this->fail( #COND );												\
		return RET;															\
	}																		\

#define NETWORK_APP_ASSERT_WITH_MESSAGE_RET( COND, MESSAGE, RET )			\
	if (!(COND))															\
	{																		\
		this->fail( MESSAGE );												\
		return RET;															\
	}																		\

#endif

BW_END_NAMESPACE

#endif // NETWORK_APP_HPP
