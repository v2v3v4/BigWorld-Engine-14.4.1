import BigWorld
import MenuScreenSpace
from Helpers.BWCoroutine import *

def run():
	coExitAfterLoadMenu().run()

@BWCoroutine
def coExitAfterLoadMenu():
	yield BWWaitForCondition( lambda: MenuScreenSpace.g_loaded )
	BigWorld.callback(30, BigWorld.exit)