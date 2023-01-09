#ifndef MESH_GUI_ADAPTOR_HPP
#define MESH_GUI_ADAPTOR_HPP

#include "simple_gui_component.hpp"
#include "duplo/py_attachment.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class LightContainer;
	typedef SmartPointer<LightContainer>	LightContainerPtr;
};

	/*~ class GUI.MeshGUIAdaptor
	 *	@components{ client, tools }
	 *
	 *	This class is a basic adaptor between SimpleGUIComponent and PyAttachment.
	 *
	 *	If you wish to display pyAttachments in the GUI scene
	 *	then use this class as a container for your attachment, and attach the desired
	 *	object onto it.
	 *	@{
	 *			guiSomeImage = GUI.Simple(imageFile)
	 *			guiSomeAttachment = BigWorld.Model(modelFile)
	 *			guiMesh = GUI.MeshAdaptor()
	 *			guiMesh.attachment = guiSomeAttachment
	 *			guiMesh.transform = someMatrixProvider
	 *			guiSomeImage.addChild(guiMesh)
	 *			GUI.addRoot(guiSomeImage)
	 *	@}
	 *	Model will now be displayed in GUI.  someMatrixProvider can be
	 *	updated to change the orientation of Model, and the model can
	 *	have animations performed on it like any other.
	 *
	 *	Note that this class draws during GUI time, meaning that the 
	 *	Z-Buffer is unavailable.  For this reason, it is only recommended
	 *	to use this class for displaying custom-built meshes, for example
	 *	custom shaped GUI borders or frames that are specifically made for
	 *	this purpose (are flat and/or draw using additive or alpha blending)
	 *
	 *	If you want to render your character in the user interface, then it
	 *	is more appropriate to use a PyModelRenderer or PySceneRenderer.
	 */
/**
 *	This class is a basic adaptor between SimpleGUIComponent and PyAttachment.
 *
 *	If you wish to display pyAttachments in the GUI scene
 *	then use this class as a container for your attachment, and attach the desired
 *	object onto it.
 *
 *	Note that this class draws during GUI time, meaning that the 
 *	Z-Buffer is unavailable.  For this reason, it is only recommended
 *	to use this class for displaying custom-built meshes, for example
 *	custom shaped GUI borders or frames that are specifically made for
 *	this purpose (are flat and/or draw using additive or alpha blending)
 *
 *	If you want to render your character in the user interface, then it
 *	is more appropriate to use a PyModelRenderer or PySceneRenderer.
 */
class MeshGUIAdaptor : public SimpleGUIComponent, MatrixLiaison
{
	Py_Header( MeshGUIAdaptor, SimpleGUIComponent )

public:
	MeshGUIAdaptor( PyTypeObject * pType = &s_type_ );
	virtual ~MeshGUIAdaptor();

	//These are SimpleGUIComponent methods
	virtual void update( float dTime, float relParentWidth, float relParentHeight );
	virtual void draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay = true );
	virtual void addAsSortedDrawItem( Moo::DrawContext& drawContext, const Matrix& worldTransform );

	SmartPointer<PyAttachment>	attachment() const	{ return attachment_; }
	void attachment( SmartPointer<PyAttachment> );

	//These are MatrixLiason Methods
	virtual const Matrix & getMatrix() const;
	virtual bool setMatrix( const Matrix & m )	{ return false; }

	//This is the python interface
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( SmartPointer<PyAttachment>, attachment, attachment )
	PY_RW_ATTRIBUTE_DECLARE( transform_, transform )

protected:
	SmartPointer<PyAttachment>	attachment_;
	MatrixProviderPtr			transform_;
	static Moo::LightContainerPtr	s_lighting_;

	COMPONENT_FACTORY_DECLARE( MeshGUIAdaptor() )
};

#ifdef CODE_INLINE
#include "mesh_gui_adaptor.ipp"
#endif

BW_END_NAMESPACE

#endif
