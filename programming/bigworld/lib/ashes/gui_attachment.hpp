#ifndef GUI_ATTACHMENT_HPP
#define GUI_ATTACHMENT_HPP

#include "simple_gui_component.hpp"
#include "duplo/py_attachment.hpp"


BW_BEGIN_NAMESPACE

	/*~ class GUI.GuiAttachment
	 *	@components{ client, tools }
	 *
	 *	This class is a basic adaptor between PyAttachment and SimpleGUIComponent.
	 *
	 *	If you wish to display gui elements in the 3D scene ( and sorted into the scene )
	 *	then use this class as a container for your gui hierarchy, and attach this object
	 *	onto a PyModel or other PyAttachment object.
	 *
	 *	Note that if you wish to receive input like most gui components, then you can still
	 *	use the attached gui component's focus property as per normal.
	 *
	 *		eg,	guiSomeImage = GUI.Simple(imageFile)
	 *			guiAttach = GUI.Attachment()
	 *			guiAttach.component = guiSomeImage
	 *			self.model.rightHand = guiAttach	# Where rightHand is PyModelNode (model hard-point)
	 *			GUI.addRoot(guiSomeImage)
	 *
	 *		Current Model will now run around with the image showing in its right hand...
	 */
/**
 *	This class is a basic adaptor between PyAttachment and SimpleGUIComponent.
 *
 *	If you wish to display gui elements in the 3D scene ( and sorted into the scene )
 *	then use this class as a container for your gui hierarchy, and attach this object
 *	onto a PyModel or other PyAttachment object.
 *
 *	Note that if you wish to receive input like most gui components, then you can still
 *	use the attached gui component's focus property as per normal.
 */
class GuiAttachment : public PyAttachment
{
	Py_Header( GuiAttachment, PyAttachment )

public:
	GuiAttachment( PyTypeObject * pType = &s_type_ );
	virtual ~GuiAttachment();

	//These are PyAttachment methods
	virtual void tick( float dTime );

	virtual bool needsUpdate() const { return false; }
	virtual bool isLodVisible() const { return true; }
#if ENABLE_TRANSFORM_VALIDATION
	virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
	virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const
		{ return false; }
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod ) { /*do nothing*/ }

	virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );
	virtual void localBoundingBox( BoundingBox & bb, bool skinny = false );
	virtual void localVisibilityBox( BoundingBox & bb, bool skinny = false );
	virtual void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false );

	//C++ set the component
	void component( SmartPointer<SimpleGUIComponent> component );
	SmartPointer<SimpleGUIComponent> component() const;

	//This is the python interface
	PY_FACTORY_DECLARE()

	PY_RW_ATTRIBUTE_DECLARE( gui_, component )
	PY_RW_ATTRIBUTE_DECLARE( faceCamera_, faceCamera )
	PY_RW_ATTRIBUTE_DECLARE( reflectionVisible_, reflectionVisible )

private:
	SmartPointer<SimpleGUIComponent>	gui_;
	bool	faceCamera_;
	float	dTime_;
	uint32	tickCookie_;
	uint32	drawCookie_;
	bool reflectionVisible_;
};

BW_END_NAMESPACE

#endif
