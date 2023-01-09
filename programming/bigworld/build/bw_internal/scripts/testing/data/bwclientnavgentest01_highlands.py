import BigWorld
import sys
import FantasyDemo
import PlayerAvatar
import Avatar
import MenuScreenSpace
from Helpers.BWCoroutine import *
from bwdebug import *

# ---------------------------
# a simple straight line test
# ---------------------------

def run():
	FantasyDemo.coExploreOfflineSpace( "spaces/highlands" ).run()	
	coExitAfterLoadSpace().run()

def _onSeekFinished( success ):
	if not success:
		BigWorld.criticalExit( "Unable to get to destination" )
	
@BWCoroutine
def coExitAfterLoadSpace():
	yield BWWaitForCoroutine( MenuScreenSpace.fini() )
	yield BWWaitForCondition( lambda: BigWorld.spaceLoadStatus() == 1.0)

	avatarPlayer = BigWorld.player()
		
	target = (612.677, 209.209, -199.762)
	
	try:
		# Calculate path path
		path = BigWorld.navigatePathPoints(
			avatarPlayer.position, target )
	except ValueError, e:
		BigWorld.criticalExit( "Unable to calculate the Path" )
		
	targetPosAndYaw = avatarPlayer.seeker.getPosAndYawToTarget( 
		avatarPlayer.position, target )
	avatarPlayer.seeker.seekPath( avatarPlayer, targetPosAndYaw, 
		avatarPlayer.runFwdSpeed, callback=_onSeekFinished )

	#wait for avatar to stop moving
	yield BWWaitForCondition( lambda: BigWorld.player().isMovingToDest == False )
	
	BigWorld.callback( 5, BigWorld.exit )
	


	

	
