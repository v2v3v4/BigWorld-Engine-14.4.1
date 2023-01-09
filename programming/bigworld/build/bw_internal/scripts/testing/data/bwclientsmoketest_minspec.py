import BigWorld
import FantasyDemo
import MenuScreenSpace
from Helpers.BWCoroutine import *

def run():
	FantasyDemo.coExploreOfflineSpace( "spaces/minspec" ).run()
	coExitAfterLoadSpace().run()

@BWCoroutine
def coExitAfterLoadSpace():
	yield BWWaitForCoroutine( MenuScreenSpace.fini() )
	yield BWWaitForCondition( lambda: BigWorld.spaceLoadStatus() == 1.0 )

	BigWorld.callback(30, BigWorld.exit)