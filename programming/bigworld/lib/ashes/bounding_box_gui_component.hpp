#ifndef BOUNDING_BOX_GUI_COMPONENT_HPP
#define BOUNDING_BOX_GUI_COMPONENT_HPP

#include "ashes/simple_gui_component.hpp"

#include "math/boundbox.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

/*~ class GUI.BoundingBoxGUIComponent
 *	@components{ client, tools }
 *
 *	A BoundingBoxGUIComponent is used to render a bounding image on the hud
 *  that surounds a Model displayed in the world.  It moves across the hud to
 *  stay bounding the model as either the camera or the model moves.
 *
 *	BoundingBoxGUIComponent inherits from SimpleGUIComponent.  It is created by
 *  the GUI.BoundingBox function, which takes the filepath to the image to use
 *  to render the corner of the bounding box.  The image is used unflipped in
 *  the bottom right hand corner of the bounding box.  It is flipped
 *  horizontally to give the bottom left corner. It is flipped vertically to
 *  give the the top right corner.  It is flipped horizontally and vertically
 *	to give the top left corner.
 *
 *	The outside of the corners line up with the outside of the bounding
 *	rectangle.  The images can be scaled using the functionality of 
 *	SimpleGUIComponent.  If they are too big, then they overlap.
 *
 *	The bounding components are also semi-constrained by the screen.
 *	For example, if the source matrix goes gradually off the left of the screen, then the
 *	two left hand corners will remain on the left hand edge of the screen until
 *	the right hand corners have followed the source off.  At this point, the left
 *	hand corners will gradually move off screen.  The same logic applies to the other
 *	screen edges.
 *
 *	In order to make the BoundingBoxGUIComponent appear, it needs both to be
 *  added to the GUI tree, and it needs	to have its source attribute assigned.
 *  This is normally set to the bounds attribute of the model the box is
 *	to surround.
 *
 *	The following example draws a bounding box around the player model:
 *
 *	@{
 *	bb = GUI.BoundingBox( "maps/gui/select.dds" )
 *	model = BigWorld.player().model
 *	bb.source = model.bounds
 *	@}
 */
/**
 * This class creates and manages 4 components at the root level,
 * corresponding to selection brackets around the corners of
 * the bounding box held by the model specified in preDraw()
 *
 * You can also add child objects to this component.  They will
 * be positioned around the bounding box, according to their
 * anchor points and their position.
 *
 * The child components' position should be (0..1, 0..1), which
 * correspond to the anchor positions along the width/height
 * of the bounding box.
 *
 * To use, simply add a BoundingBoxGUIComponent to the GUI graph,
 * and choose a model.
 */
class BoundingBoxGUIComponent : public SimpleGUIComponent
{
	Py_Header( BoundingBoxGUIComponent, SimpleGUIComponent )

public:
	BoundingBoxGUIComponent( const BW::string& textureName,
		PyTypeObject * pType = &s_type_ );
	~BoundingBoxGUIComponent();

	virtual void	width( float w );
	virtual void	height( float h );
	virtual void	colour( uint32 col );
	virtual void	textureName( const BW::string& name );

	///The bounding box component has a different update / shader model
	virtual void	applyShaders( float dTime );
	virtual void	applyShader( GUIShader& shader, float dTime );
	virtual void	internalApplyShaders( float dTime );

	void		source( MatrixProviderPtr mpp )		{ pSource_ = mpp; }

	PY_FACTORY_DECLARE()

	PY_RW_ATTRIBUTE_DECLARE( pSource_, source )
	PY_RW_ATTRIBUTE_DECLARE( clipSpaceSource_, clipSpaceSource )
	PY_RW_ATTRIBUTE_DECLARE( clipToBox_, clipToBox )
	PY_RW_ATTRIBUTE_DECLARE( absoluteSubspace_, absoluteSubspace )
	PY_RW_ATTRIBUTE_DECLARE( offsetSubspace_, offsetSubspace )
	PY_RW_ATTRIBUTE_DECLARE( alwaysDisplayChildren_, alwaysDisplayChildren )

	void		update( float dTime, float relParentWidth, float relParentHeight );
	void		draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay = true );

private:
	BoundingBoxGUIComponent(const BoundingBoxGUIComponent&);
	BoundingBoxGUIComponent& operator=(const BoundingBoxGUIComponent&);

	void		preDraw( float relParentWidth, float relParentHeight );

	bool		calculatePostDivideMinMax( Vector3& min, Vector3& max,
										   BoundingBox& bb,
										   const Matrix& objectToCamera,
										   const Matrix& cameraToClip  );

	SimpleGUIComponent*	corners_[4];
	BoundingBox			bb_;
	MatrixProviderPtr	pSource_;
	float				dTime_;
	bool				onScreen_;

	bool				clipSpaceSource_;
	bool				clipToBox_;
	bool				alwaysDisplayChildren_;
	int					absoluteSubspace_;
	Vector3				offsetSubspace_;

	Vector3				minInClip_;
	Vector3				maxInClip_;

	Matrix				savedView_;
	Matrix				savedProjection_;

	virtual bool load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings );
	virtual void save( DataSectionPtr pSect, SaveBindings & bindings );

	COMPONENT_FACTORY_DECLARE( BoundingBoxGUIComponent("") )
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "bounding_box_gui_component.ipp"
#endif


#endif
/*bounding_box_gui_component.hpp*/
