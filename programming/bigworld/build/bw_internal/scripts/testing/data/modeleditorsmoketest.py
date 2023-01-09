import ModelEditor
import BigWorld

def run():
	ModelEditor.loadModel( "characters/avatars/ranger/ranger_body.model" )
	BigWorld.callback(30, ModelEditor.exit)