#define CELLAPP_PROFILE_INTERNAL

#include "profile.hpp"

#include "cstdmf/debug.hpp"
#include "server/bwconfig.hpp"

DECLARE_DEBUG_COMPONENT( 0 )


BW_BEGIN_NAMESPACE

// __kyl__(30/7/2007) Special profile thresholds for T2CN
TimeStamp 	g_profileInitGhostTimeLevel( TimeStamp::fromSeconds( 0.1 ) );	// 100 ms
int			g_profileInitGhostSizeLevel = 5 * 1024;
TimeStamp 	g_profileInitRealTimeLevel( TimeStamp::fromSeconds( 0.1 ) );	// 100 ms
int			g_profileInitRealSizeLevel = 10 * 1024;
TimeStamp 	g_profileOnloadTimeLevel( TimeStamp::fromSeconds( 0.1 ) );	// 100 ms
int			g_profileOnloadSizeLevel = 10 * 1024;
int			g_profileBackupSizeLevel = 10 * 1024;

#ifndef CODE_INLINE
#include "profile.ipp"
#endif


// -----------------------------------------------------------------------------
// Section: Profile
// -----------------------------------------------------------------------------

ProfileVal SCRIPT_CALL_PROFILE( "scriptCall" );

ProfileVal ON_MOVE_PROFILE( "onMove" );
ProfileVal ON_NAVIGATE_PROFILE( "onNavigate" );
ProfileVal SHUFFLE_ENTITY_PROFILE( "shuffleEntity" );
ProfileVal SHUFFLE_TRIGGERS_PROFILE( "shuffleTriggers" );
ProfileVal SHUFFLE_AOI_TRIGGERS_PROFILE( "shuffleAoITriggers" );

ProfileVal CLIENT_UPDATE_PROFILE( "clientUpdate" );
ProfileVal TRANSIENT_LOAD_PROFILE( "transientLoad" );
ProfileVal DROP_POSITION( "setAndDropGlobalPositionAndDirection" );


namespace CellProfileGroup
{

void init()
{
	// make all our profiles get reset when the 'running time' one does
	ProfileGroup & group = ProfileGroup::defaultGroup();
	ProfileGroupResetter::instance().nominateProfileVal( group.pRunningTime() );
	ProfileGroupResetter::instance().addProfileGroup( &group );

	// __kyl__(30/7/2007) Init special T2CN profile thresholds
	double initGhostTime = g_profileInitGhostTimeLevel.inSeconds();
	BWConfig::update( "cellApp/profiles/initGhost/timeWarningLevel",
		initGhostTime );
	g_profileInitGhostTimeLevel.setInSeconds( initGhostTime );
	BWConfig::update( "cellApp/profiles/initGhost/sizeWarningLevel",
		g_profileInitGhostSizeLevel );

	double initRealTime = g_profileInitRealTimeLevel.inSeconds();
	BWConfig::update( "cellApp/profiles/initReal/timeWarningLevel",
		initRealTime );
	g_profileInitRealTimeLevel.setInSeconds( initRealTime );
	BWConfig::update( "cellApp/profiles/initReal/sizeWarningLevel",
		g_profileInitRealSizeLevel );

	double onloadTime = g_profileOnloadTimeLevel.inSeconds();
	BWConfig::update( "cellApp/profiles/onload/timeWarningLevel",
		onloadTime );
	g_profileOnloadTimeLevel.setInSeconds( onloadTime );
	BWConfig::update( "cellApp/profiles/onload/sizeWarningLevel",
		g_profileOnloadSizeLevel );

	BWConfig::update( "cellApp/profiles/backup/sizeWarningLevel",
		g_profileBackupSizeLevel );

	MF_WATCH( "profilesConfig/initGhost/timeWarningLevel",
		g_profileInitGhostTimeLevel );
	MF_WATCH( "profilesConfig/initGhost/sizeWarningLevel",
		g_profileInitGhostSizeLevel );

	MF_WATCH( "profilesConfig/initReal/timeWarningLevel",
		g_profileInitRealTimeLevel );
	MF_WATCH( "profilesConfig/initReal/sizeWarningLevel",
		g_profileInitRealSizeLevel );

	MF_WATCH( "profilesConfig/onload/timeWarningLevel",
		g_profileOnloadTimeLevel );
	MF_WATCH( "profilesConfig/onload/sizeWarningLevel",
		g_profileOnloadSizeLevel );

	MF_WATCH( "profilesConfig/backup/sizeWarningLevel",
		g_profileBackupSizeLevel );
}
}

BW_END_NAMESPACE

// cellapp.cpp
