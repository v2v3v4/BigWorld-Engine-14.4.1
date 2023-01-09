#include "pch.hpp"

#include "gui_attachment.hpp"
#include "cstdmf/debug.hpp"
#include "moo/render_context.hpp"
#include "moo/visual.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE



// -----------------------------------------------------------------------------
// Section: GuiAttachment
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE GuiAttachment::

PY_TYPEOBJECT( GuiAttachment )

PY_BEGIN_METHODS( GuiAttachment )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( GuiAttachment )

	/*~ attribute GuiAttachment.component
	 *	@components{ client, tools }
	 *
	 * @type SimpleGUIComponent	Stores the GUI component that is to be attached to a 
	 *							PyModel HardPoint through this GuiAttachment object.
	 */
	PY_ATTRIBUTE( component )
	/*~ attribute GuiAttachment.faceCamera
	 *	@components{ client, tools }
	 *
	 * @type Boolean			Turn on to make the GUIAttacment's component use the
	 *							camera direction.  Note this doesn't make the component
	 *							point at the camera position, instead it aligns the
	 *							with the camera plane.
	 */
	 PY_ATTRIBUTE( faceCamera )
	 /*~ attribute GuiAttachment.reflectionVisible
	 *
	 * @type Boolean			Determines whether or not this GUI attachment will be
	 *							rendered in reflection scenes (e.g. water reflections).
	 *							Defaults to True.
	 */
	 PY_ATTRIBUTE( reflectionVisible )

PY_END_ATTRIBUTES()

/*~	function GUI.Attachment
 *	@components{ client, tools }
 *
 *	Creates and returns a new GUIAttachment, which is 
 *	used to display gui elements in the 3D scene.
 *
 *	@return	The new GUIAttachment object
 */
PY_FACTORY_NAMED( GuiAttachment, "Attachment", GUI )


GuiAttachment::GuiAttachment( PyTypeObject * pType ):
	PyAttachment( pType ),
	gui_( NULL ),
	faceCamera_( false ),
	dTime_( 0.0 ),
	tickCookie_( 1 ),
	drawCookie_( 0 ),
	reflectionVisible_( true )
{
	BW_GUARD;	
}


GuiAttachment::~GuiAttachment()
{
	BW_GUARD;	
}


/**
 *	Section - PyAttachment methods
 */

/**
 *	This method implements the PyAttachment::tick interface.
 */
void GuiAttachment::tick( float dTime )
{
	BW_GUARD;
	dTime_ = dTime;
	++tickCookie_;
}


/**
 *	This method implements the PyAttachment::draw interface.  Since
 *	this gui component draws in the world, this is where we do our
 *	actual drawing.
 *
 *	The worldTransform passed in should already be on the Moo::rc().world()
 *	stack.
 */
void GuiAttachment::draw( Moo::DrawContext& drawContext, const Matrix & worldTransform )
{
	BW_GUARD;

	// Have we disabled drawing in reflections?
	if (Moo::rc().reflectionScene() && !reflectionVisible_)
	{
		return;
	}

	//Do not draw the gui attachments while doing draw overrides as:
	//This might cause update to be called with wrong values for text boxes
	//as we are using a different redner target which effects the screen width and height.
	//And in general we probably don't need the attachements in the overrides.
	if ( gui_ )
	{
		// We tick the GUI component here so that it can be up to date with the 
		// current frame (especially the first frame the GuiAttachment gets 
		// attached). This "cookie" check is here so we don't tick the GUI too
		// many times in a single frame (in case we get drawn twice, e.g. into reflection).
		if (drawCookie_ != tickCookie_)
		{
			gui_->update( dTime_, SimpleGUI::instance().screenWidth(), SimpleGUI::instance().screenHeight() );
			gui_->applyShaders( dTime_ );
			drawCookie_ = tickCookie_;			
		}

		Matrix transform( Matrix::identity );

		if (faceCamera_)
		{
			// Grab the individual transforms
			Matrix position, positionInv;
			position.setTranslate( gui_->position() );
			positionInv.setTranslate( -gui_->position() );

			Matrix viewRotInv( Moo::rc().invView() );
			viewRotInv.translation( Vector3(0,0,0) );

			// We only want the position of the node we are attached to.
			Matrix atchWorld;
			atchWorld.setTranslate( worldTransform.applyToOrigin() );

			// Multiply them all together.
			transform.postMultiply( positionInv ); // posiiton is baked into vertices, so undo that.
			transform.postMultiply( viewRotInv ); // rotate around the origin...
			transform.postMultiply( gui_->runTimeTransform() ); // ...and reapply all the transforms.
			transform.postMultiply( atchWorld );
			transform.postMultiply( position );
		}
		else
		{
			transform.postMultiply( gui_->runTimeTransform() );
			transform.postMultiply( worldTransform );
		}

		gui_->addAsSortedDrawItem( drawContext, transform );
	}
}


/**
 *	This accumulates our bounding box into the given matrix
 */
void GuiAttachment::localVisibilityBox( BoundingBox & bb, bool skinny )
{
	BW_GUARD;
	if (!gui_)
		return;

	gui_->localBoundingBox( bb, skinny );	
}


/**
 *	This accumulates our bounding box into the given matrix
 */
void GuiAttachment::localBoundingBox( BoundingBox & bb, bool skinny )
{
	BW_GUARD;
	if (!gui_)
		return;

	gui_->localBoundingBox( bb, skinny );	
}


/**
 *	This accumulates our bounding box into the given matrix
 */
void GuiAttachment::worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny )
{
	BW_GUARD;
	if (!gui_)
		return;

	gui_->worldBoundingBox( bb, world, skinny );	
}


void GuiAttachment::component( SmartPointer<SimpleGUIComponent> component )
{
	BW_GUARD;
	gui_ = component;
}


SmartPointer<SimpleGUIComponent> GuiAttachment::component() const
{
	BW_GUARD;
	return gui_;
}

/**
 *	Factory method
 */
PyObject * GuiAttachment::pyNew( PyObject * args )
{
	BW_GUARD;
	return new GuiAttachment;
}

BW_END_NAMESPACE

// graph_gui_component.cpp
