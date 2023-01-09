#include "pch.hpp"
#include "voip_app.hpp"

#include "bwvoip.hpp"
#include "voip.hpp"
#include "script_player.hpp"
#include "app.hpp"


BW_BEGIN_NAMESPACE

///////////////////////////////////////////////////////////////////////////////
///  Member Documentation for VOIPApp
///////////////////////////////////////////////////////////////////////////////


/**
 *	@property	VOIPClient* VOIPApp::voipClient_
 *
 *	The pointer to VOIPClient instance. This member is set in VOIPApp::init().
 */


///////////////////////////////////////////////////////////////////////////////
///  End Member Documentation for VOIPApp
///////////////////////////////////////////////////////////////////////////////


/**
 *	The singleton instance of VOIPApp
 */
VOIPApp VOIPApp::s_instance_;

int VOIPApp_token = 1;


/**
 *	Constructor for VOIPApp
 *
 *	The VOIPApp adds its self to the pool of main loop tasks under 'VOIP/App'.
 */
VOIPApp::VOIPApp() :
	voipClient_( NULL )
{
	MainLoopTasks::root().add( this, "VOIP/App", NULL );
}


/**
 *	Destructor
 */
VOIPApp::~VOIPApp()
{
}


/**
 *	This method initalises the VOIPApp by creating the VOIPClient from
 *	the voip.dll.
 *
 *	@note	The VOIPClient is not initialised in this method but instead
 *			intended to be initialised through python.
 */
bool VOIPApp::init()
{
	BW_GUARD;
	if (App::instance().isQuiting())
	{
		return false;
	}
	voipClient_ = VOIPClient::createVOIPClient();

	MF_ASSERT_DEV( voipClient_ );

	return true;
}


/**
 *	This method cleans up the VOIPApp by destroying the previously created voip
 *	client.
 *
 *	@note	The python module BigWorld.VOIP must not be used after this point.
 */
void VOIPApp::fini()
{
	BW_GUARD;
	VOIPClient::destroyVOIPClient( voipClient_ );
	voipClient_ = NULL;
}


/**
 *	This method ticks the VOIPClient each frame. Passing in the current
 *	position and rotation of the player.
 */
void VOIPApp::tick( float /* dGameTime */, float /* dRenderTime */ )
{
	BW_GUARD_PROFILER( AppTick_VOIP );

	const Entity * playerEntity = ScriptPlayer::entity();

	if( !playerEntity || !playerEntity->isInWorld() )
	{
		return;
	}

	const Position3D playerPosition = playerEntity->position();

	const Direction3D playerHeadRotation = playerEntity->direction();

	const SpaceID & playerSpace = playerEntity->spaceID();

	voipClient_->tick(	playerPosition.x,
						playerPosition.y,
						playerPosition.z,
						playerHeadRotation.yaw,
						playerHeadRotation.pitch,
						playerHeadRotation.roll,
						playerSpace );
}


/**
 *	This method currently does nothing.
 */
void VOIPApp::draw()
{

}



/**
 *	This function retrieves the VOIPClient possessed by the VOIPApp.
 *
 *	@return	The VOIPClient owned by the VOIPApp instance
 *
 *	@note	This function must not be called before the VOIPApp has been
 *			initialised.
 */
VOIPClient & VOIPApp::getVOIPClient()
{
	BW_GUARD;
	MF_ASSERT_DEV( VOIPApp::s_instance_.voipClient_ != NULL );

	return *( VOIPApp::s_instance_.voipClient_ );
}

BW_END_NAMESPACE

// voip_app.cpp
