#include "pch.hpp"
#include "gui_progress.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/simple_gui_component.hpp"
#include "ashes/clip_gui_shader.hpp"


BW_BEGIN_NAMESPACE

static float lastTime_ = 0.f;
static float MIN_PROGRESS_STEP = 0.05f;

/*
"""
This classes implements the interface to GUI Progress Bars.
"""

Your gui script must have the following interface :

	def setProgress( self, value ):
		pass

	def addMessage( self, str ):
		pass

	def appendMessage( self, str ):
		pass
*/

GUIProgressDisplay::GUIProgressDisplay( const BW::string& guiName,
							ProgressCallback pCallback/* = NULL*/ ):
	ProgressDisplay( 0, pCallback )
{
	BW_GUARD;
	lastTime_ = -1.f;
	PyObject* params = PyTuple_New(1);
	PyTuple_SET_ITEM( params, 0, PyString_FromString(guiName.c_str()) );
	PyObject * gui = SimpleGUIComponent::py_load( params );

	Py_DECREF(params);
	if (gui != NULL)
	{
		SimpleGUIComponent* g = static_cast<SimpleGUIComponent*>(gui);
		this->gui_ = SmartPointer< SimpleGUIComponent >(g, true);
	}
}


GUIProgressDisplay::~GUIProgressDisplay()
{	
	BW_GUARD;
	gui_ = NULL;
}

SmartPointer< SimpleGUIComponent > GUIProgressDisplay::gui()
{
	return gui_;
}


/**
 *	Method to add a static message string
 */
void GUIProgressDisplay::add( const BW::string & str )
{
	BW_GUARD;
	SmartPointer< PyObject > fn;

	if (this->gui_->script().exists())
	{
		fn = PyObject_GetAttrString( this->gui_->script().getObject(), "addMessage" );
	}

	if (fn)
	{
		Script::call(
			&*fn,
			Py_BuildValue( "(s)", str.c_str() ),
			"GUIProgress add: " );
	}

	ProgressDisplay::add( str );
}


/**
 *	Appends a string to the last line in the progress display
 */
void GUIProgressDisplay::append( const BW::string & str )
{
	BW_GUARD;
	SmartPointer< PyObject > fn;

	if (this->gui_->script().exists())
	{
		fn = PyObject_GetAttrString( this->gui_->script().getObject(), "appendMessage" );
	}

	if (fn)
	{
		Script::call(
			&*fn,
			Py_BuildValue( "(s)", str.c_str() ),
			"GUIProgress append: " );
	}

	ProgressDisplay::append( str );
}


void GUIProgressDisplay::add( ProgressTask & task, const BW::string & name )
{
	BW_GUARD;
	this->add( name );
	ProgressDisplay::add( task, name );
}


bool GUIProgressDisplay::draw( bool force )
{
	BW_GUARD;
	// run the callback
	bool ret = true;
	if (pCallback_)
		ret = (*pCallback_)();

	if (!ret)
		return false;

	float percentDone = 0.5f;

	int row = 0;

	// now traverse the tree, working out the current progress total
	BW::vector<int>	ixStack = roots_;

	while( ixStack.size() > 0)
	{
		int i = ixStack.back();
		ixStack.pop_back();

		ProgressNode & node = tasks_[i];

		// draw the bar
		if (node.task != NULL)
		{
			float usedone = node.task->done_;

			if (usedone < 0.f) usedone = 0.f;

			float uselen = node.task->length_;

			if ( uselen == 0.f ) uselen = 1000000.f;
			if ( uselen > 0.f && uselen < usedone ) uselen = usedone;

			percentDone = usedone / uselen;
		}

		// now push on all our children in reverse order
		const int numChildren = static_cast<int>( node.children.size() );
		for (int j = numChildren-1; j >= 0; j--)
		{
			ixStack.push_back( node.children[j] );
		}

		row++;
	}

	static float s_lastPercent = 0;
	if (percentDone - s_lastPercent > MIN_PROGRESS_STEP || force)
	{
		s_lastPercent = percentDone;

		// Set the progress amount, either via the 
		// GUI script or using the clipper shader	
		SmartPointer< PyObject > fn;

		if (this->gui_->script().exists())
		{
			fn = PyObject_GetAttrString( this->gui_->script().getObject(), "setProgress" );
		}

		if (fn)
		{
			Script::call(
				&*fn,
				Py_BuildValue( "(f)", percentDone ),
				"GUIProgress draw: " );
		}
		else
		{
			SimpleGUIComponentPtr bar = gui_->child( "bar" );
			if (bar)
			{
				GUIShaderPtr shader = bar->shader( "clipper" );
				if (shader)
				{
					ClipGUIShader* clipper = static_cast<ClipGUIShader*>(&*shader);	
					clipper->value( percentDone );
				}
			}
		}

		float aimTime = percentDone * 3.3333f;
		float currentTime = lastTime_ < 0.f
			? currentTime = 0.f
			: currentTime = lastTime_;

		float dTime = (aimTime - currentTime) / 10.f;

		while ( currentTime < aimTime || force )
		{
			force = false;
			currentTime += dTime;

			SimpleGUI::instance().update(dTime);

			// just draw the scene
			Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1, 0 );
			Moo::rc().beginScene();
			if (Moo::rc().mixedVertexProcessing())
				Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
			Moo::rc().nextFrame();
			if (Moo::rc().mixedVertexProcessing())
				Moo::rc().device()->SetSoftwareVertexProcessing(TRUE);

			Moo::rc().push();
			Matrix oldView = Moo::rc().view();

			//Matrix transform;		
			Moo::rc().world( Matrix::identity );
			Moo::rc().view( Matrix::identity );
			Moo::rc().updateViewTransforms();

			SimpleGUI::instance().draw();

			Moo::rc().pop();
			Moo::rc().view( oldView );

			Moo::rc().endScene();
			Moo::rc().present();
		}

		lastTime_ = aimTime;
	}
	return true;
}

BW_END_NAMESPACE

// gui_progress.cpp
