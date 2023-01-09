import WorldEditor
import BigWorld

def run():
	WorldEditor.enterPlayerPreviewMode()
	BigWorld.callback(30, WorldEditor.exit)