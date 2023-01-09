#ifndef SIMPLE_GUI_COMPONENT_HPP
#define SIMPLE_GUI_COMPONENT_HPP

#include "cstdmf/named_object.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/stringmap.hpp"
#include "math/boundbox.hpp"
#include "moo/forward_declarations.hpp"
#include "moo/effect_material.hpp"
#include "moo/moo_math.hpp"
#include "moo/render_context.hpp"
#include "romp/py_texture_provider.hpp"
#include "input/input.hpp"
#include "input/py_input.hpp"
#include "simple_gui.hpp"
#include "gui_shader.hpp"
#include "gui_vertex_format.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "moo/managed_texture.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{
	class EffectMaterial;
	class DrawContext;
};

class SimpleGUIComponent;
typedef SmartPointer<SimpleGUIComponent> SimpleGUIComponentPtr;
typedef ConstSmartPointer<SimpleGUIComponent> ConstSimpleGUIComponentPtr;



namespace Script
{
	inline PyObject * getReadOnlyData( WeakPyPtr<SimpleGUIComponent> data )
	{
		PyObject * ret = (data.good() ? 
			const_cast<PyObject*>( data.getPyObj() ) : Py_None);
		Py_INCREF( ret );
		return ret;
	}
}

typedef SmartPointer<class DataSection> DataSectionPtr;

class SimpleGUIComponent;

/**
 *	This class keeps the factory methods for all types of SimpleGUIComponents
 */
typedef NamedObject<SimpleGUIComponent * (*)()> GUIComponentFactory;

#define COMPONENT_FACTORY_DECLARE( CONSTRUCT )							\
	static GUIComponentFactory s_factory;								\
	virtual GUIComponentFactory & factory() { return s_factory; }		\
	static SimpleGUIComponent * s_create() { return new CONSTRUCT; }	\

#define COMPONENT_FACTORY( CLASS )										\
	GUIComponentFactory CLASS::s_factory( #CLASS, CLASS::s_create );	\


/*~ class GUI.SimpleGUIComponent
 *	@components{ client, tools }
 *
 *	This class is the base GUI component that by default displays a textured
 *	rectangle.  It can also use more sophisticated PyTextureProviders to
 *	render animations.
 *	All other GUI Components are derived from this class.
 *
 *	A new SimpleGUIComponent is created using the GUI.Simple function.  This
 *	takes a string argument, which is the filepath of a texture to display
 *	on the SimpleGUIComponent.
 *
 *	Once the component is created, it can have its texture attribute set to
 *	any PyTextureProvider, which supplies either a static or dynamic texture
 *	to be mapped across the components rectangle.
 *
 *	The SimpleGUIComponent can have one or more GUIShaders applied to it,
 *	which affect the way it is rendered.  It can also be resized, tiled,
 *	remapped, repositioned and moved.
 *
 *	Children can be added to the component.  However, this isn't a full
 *	windowing system.  The position and orientation of children are specified
 *	relative to the screen, not to the parent.  However, any shaders
 *	which are applied to the parent apply to the children as well.
 *
 *	For example:
 *	@{
 *	c = GUI.Simple( "maps/myPicture.bmp" )
 *	GUI.addRoot( c )
 *	c.size = (2,2)
 *	@}
 *	This example creates a SimpleGUIComponent which covers the entire screen
 *	and displays myPicture.bmp.
 */
/**
 *	This class is a simple GUI component that by default displays a textured
 *	rectangle. The other GUI components are derived from this class.
 */
class SimpleGUIComponent : public PyObjectPlusWithWeakReference
{
	Py_Header( SimpleGUIComponent, PyObjectPlusWithWeakReference )

public:
	SimpleGUIComponent( const BW::string& textureName,
		PyTypeObject * pType = &s_type_ );
	virtual ~SimpleGUIComponent();
	static void init( DataSectionPtr config );
	static void fini();

	typedef enum
	{
		ROT_0 = 0,
		ROT_90,
		ROT_180,
		ROT_270
	} eRotation;

	typedef enum
	{
		NO_FLIP = 0,
		FLIP_X = 1<<0,
		FLIP_Y = 1<<1
	} eFlip;

	typedef enum
	{
		FX_ADD,
		FX_BLEND,
		FX_BLEND_COLOUR,
		FX_BLEND_INVERSE_COLOUR,
		FX_SOLID,
		FX_MODULATE2X,
		FX_ALPHA_TEST,
		FX_BLEND_INVERSE_ALPHA,
		FX_BLEND2X,
		FX_ADD_SIGNED
	} eMaterialFX;
	PY_BEGIN_ENUM_MAP( eMaterialFX, FX_ )
		PY_ENUM_VALUE( FX_ADD )
		PY_ENUM_VALUE( FX_BLEND )
		PY_ENUM_VALUE( FX_BLEND_COLOUR )
		PY_ENUM_VALUE( FX_BLEND_INVERSE_COLOUR )
		PY_ENUM_VALUE( FX_SOLID )
		PY_ENUM_VALUE( FX_MODULATE2X )
		PY_ENUM_VALUE( FX_ALPHA_TEST )
		PY_ENUM_VALUE( FX_BLEND_INVERSE_ALPHA )
		PY_ENUM_VALUE( FX_BLEND2X )
		PY_ENUM_VALUE( FX_ADD_SIGNED )
	PY_END_ENUM_MAP()

	typedef enum
	{
		FT_NONE			= 0,
		FT_POINT		= 1,
		FT_LINEAR		= 2
	} eFilterType;
	PY_BEGIN_ENUM_MAP( eFilterType, FT_ )
		PY_ENUM_VALUE( FT_NONE )
		PY_ENUM_VALUE( FT_POINT )
		PY_ENUM_VALUE( FT_LINEAR )
	PY_END_ENUM_MAP()

	typedef enum
	{
		ANCHOR_H_LEFT,
		ANCHOR_H_CENTER,
		ANCHOR_H_RIGHT
	} eHAnchor;
	PY_BEGIN_ENUM_MAP( eHAnchor, ANCHOR_H_ )
		PY_ENUM_VALUE( ANCHOR_H_LEFT )
		PY_ENUM_VALUE( ANCHOR_H_CENTER )
		PY_ENUM_VALUE( ANCHOR_H_RIGHT )
	PY_END_ENUM_MAP()

	typedef enum
	{
		ANCHOR_V_TOP,
		ANCHOR_V_CENTER,
		ANCHOR_V_BOTTOM
	} eVAnchor;
	PY_BEGIN_ENUM_MAP( eVAnchor, ANCHOR_V_ )
		PY_ENUM_VALUE( ANCHOR_V_TOP )
		PY_ENUM_VALUE( ANCHOR_V_CENTER )
		PY_ENUM_VALUE( ANCHOR_V_BOTTOM )
	PY_END_ENUM_MAP()

	typedef enum
	{
		POSITION_MODE_CLIP,
		POSITION_MODE_PIXEL,
		POSITION_MODE_LEGACY
	} ePositionMode;
	PY_BEGIN_ENUM_MAP( ePositionMode, POSITION_MODE_ )
		PY_ENUM_VALUE( POSITION_MODE_CLIP )
		PY_ENUM_VALUE( POSITION_MODE_PIXEL )
		PY_ENUM_VALUE( POSITION_MODE_LEGACY )
	PY_END_ENUM_MAP()

	typedef enum
	{
		SIZE_MODE_CLIP,
		SIZE_MODE_PIXEL,
		SIZE_MODE_LEGACY
	} eSizeMode;
	PY_BEGIN_ENUM_MAP( eSizeMode, SIZE_MODE_ )
		PY_ENUM_VALUE( SIZE_MODE_CLIP )
		PY_ENUM_VALUE( SIZE_MODE_PIXEL )
		PY_ENUM_VALUE( SIZE_MODE_LEGACY )
	PY_END_ENUM_MAP()

	//-------------------------------------------------
	//Simple GUI component logic
	//-------------------------------------------------
	virtual void		update( float dTime, float relParentWidth, float relParentHeight );
	virtual void		applyShaders( float dTime );
	virtual void		draw( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay = true );
	virtual void		addAsSortedDrawItem( Moo::DrawContext& drawContext, const Matrix& worldTransform );

	//-------------------------------------------------
	//Hierarchy logic
	//-------------------------------------------------
	bool				addChild( const BW::string & name,
							SimpleGUIComponent* child );
	bool				removeChild( SimpleGUIComponent* child );
	bool				removeChild( const BW::string & name );
	SimpleGUIComponentPtr child( const BW::string& name );

	void				reSort();
	void				reSortRecursively();

	bool				hasChild( const SimpleGUIComponent* child, bool deepSearch ) const;
	void				children( BW::set< SimpleGUIComponent * >& returnList ) const;
	SimpleGUIComponentPtr parent();

	//-------------------------------------------------
	//Pluggable python logic
	//-------------------------------------------------
	//bool				scriptObject( PyObject * classObject );
	SmartPointer< PyObject > script()	{ return pScriptObject_; }

	//-------------------------------------------------
	//Input methods
	//-------------------------------------------------

	// TODO: make this pure virtual (good practice: never inherit from a concrete base)
	virtual bool		handleKeyEvent( const KeyEvent & /*event*/ );
	virtual bool		handleMouseButtonEvent( const KeyEvent & /*event*/ );
	virtual bool		handleMouseEvent( const MouseEvent & /*event*/ );
	virtual bool		handleAxisEvent( const AxisEvent & /*event*/ );

	virtual bool		handleMouseEnterEvent( const MouseEvent & /*event*/ );
	virtual bool		handleMouseLeaveEvent( const MouseEvent & /*event*/ );

	virtual bool		handleMouseClickEvent( const KeyEvent & /*event*/ );

	virtual bool		handleDragStartEvent( const KeyEvent & /*event*/ );
	virtual bool		handleDragStopEvent( const KeyEvent & /*event*/ );

	virtual bool		handleDragEnterEvent( 
							SimpleGUIComponent * /*draggedComponen*/, 
							const MouseEvent & /*event*/ );							

	virtual bool		handleDragLeaveEvent( 
							SimpleGUIComponent * /*draggedComponen*/, 
							const MouseEvent & /*event*/ );

	virtual bool		handleDropEvent( 
							SimpleGUIComponent * /*draggedComponen*/, 
							const KeyEvent & /*event*/ );

	bool				hitTest( const Vector2 & mousePosition ) const;

	bool				keyFocused() const;

	void				calcDrawOrder();
	virtual uint32		calcDrawOrderRecursively( uint32 drawOrder,
							uint32 nextDrawOrder );

	virtual void print(const BW::string& indentation);


	//-------------------------------------------------
	//Shaders
	//-------------------------------------------------
	void				addShader( const BW::string & name,
							GUIShader* shader );
	void				removeShader( GUIShader* shader );
	void				removeShader( const BW::string & name );
	GUIShaderPtr		shader( const BW::string& name );

	///accessor, primarily for shaders
	GUIVertex*			vertices( int* numVertices );

	virtual void		applyShader( GUIShader& shader, float dTime );

	//-------------------------------------------------
	//Persistence
	//-------------------------------------------------

	struct LoadBinding
	{
		BW::string	name_;
		int			id_;
	};
	typedef BW::vector<LoadBinding>		LoadBindings;

	struct SaveBindings
	{
		BW::vector<SimpleGUIComponent *>	components_;
		BW::vector<GUIShader *>		shaders_;
	};

	virtual bool		load( DataSectionPtr pSect, const BW::string& ownerName, LoadBindings & bindings );
	virtual void		save( DataSectionPtr pSect, SaveBindings & bindings );
	virtual void		bound();

	//-------------------------------------------------
	//Python Interface
	//-------------------------------------------------
	ScriptObject		pyGetAttribute( const ScriptString & attrObj );
	bool				pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );
	void				pyDel();

	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_addChild )
	PY_METHOD_DECLARE( py_delChild )
	PY_METHOD_DECLARE( py_hasChild )

	PY_METHOD_DECLARE( py_addShader )
	PY_METHOD_DECLARE( py_delShader )

	PY_MODULE_STATIC_METHOD_DECLARE( py_load )
	PY_METHOD_DECLARE( py_save )
	PY_METHOD_DECLARE( py_reSort )

	PY_AUTO_METHOD_DECLARE( RETDATA, handleKeyEvent, ARG( KeyEvent, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, handleMouseEvent, ARG( MouseEvent, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, handleAxisEvent, ARG( AxisEvent, END ) )

	PY_AUTO_METHOD_DECLARE( RETDATA, screenToLocal, ARG( Vector2, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, localToScreen, ARG( Vector2, END ) )

	PY_AUTO_METHOD_DECLARE( RETDATA, hitTest, ARG( Vector2, END ) )

	//-------------------------------------------------
	//Simple GUI component properties
	//-------------------------------------------------
	const Vector3&		position() const;
	virtual void		position( const Vector3& p );

	ePositionMode		horizontalPositionMode() const;
	void				horizontalPositionMode( ePositionMode mode );

	ePositionMode		verticalPositionMode() const;
	void				verticalPositionMode( ePositionMode mode );

	virtual float		width() const;
	virtual void		width( float w );	
	eSizeMode			widthMode() const;
	void				widthMode( eSizeMode mode );

	virtual float		height() const;
	virtual void		height( float h );
	eSizeMode			heightMode() const;
	void				heightMode( eSizeMode mode );

	virtual Vector2		screenToLocal( const Vector2 & screen ) const;
	virtual Vector2		localToScreen( const Vector2 & local ) const;

	///deprecated methods - use width/height mode instead.
	void				widthInClipCoords( bool state );
	bool				widthInClipCoords() const;
	void				heightInClipCoords( bool state );
	bool				heightInClipCoords() const;

	virtual void		clipBounds( Vector2& topLeft,
									 Vector2& topRight,
									 Vector2& botLeft,
									 Vector2& botRight,
									 float* out_relativeParentWidth = NULL,
									 float* out_relativeParentHeight = NULL) const;

	Vector2				size();
	virtual void		size( const Vector2 & size );

	uint32				colour() const;
	virtual void		colour( uint32 col );

	///mapping coordinates
	eRotation			angle() const;
	void				angle( eRotation r );
	int					flip() const;
	void				flip( int f );
	///coordinates from bottom-left, clockwise
	///there must be 4 coordinates in the array
	void				mapping( Vector2* mappingCoords );

	bool				visible() const;
	virtual void		visible( bool v );

	bool				visibleInAppFocusOnly() const;
	virtual void		visibleInAppFocusOnly( bool state );

	bool				momentarilyInvisible() const;
	void				momentarilyInvisible( bool mi );

	eHAnchor			horizontalAnchor() const;
	eVAnchor			verticalAnchor() const;
	void				horizontalAnchor( eHAnchor );
	void				verticalAnchor( eVAnchor );
	void				anchor( eHAnchor h, eVAnchor v );

	const BW::string&	textureName() const;
	virtual void		textureName( const BW::string& name );

	eMaterialFX			materialFX() const;
	void				materialFX( eMaterialFX fx );

	eFilterType			filterType() const;
	void				filterType( eFilterType filter );

	bool				tiled() const;
	void				tiled( bool state );

	int					tileWidth() const;
	void				tileWidth( int w );

	int					tileHeight() const;
	void				tileHeight( int h );

	bool				focus() const;
	virtual void		focus( bool state );

	bool				mouseButtonFocus() const;
	virtual void		mouseButtonFocus( bool state );

	bool				moveFocus() const;
	virtual void		moveFocus( bool state );

	bool				crossFocus() const;
	virtual void		crossFocus( bool state );

	bool				dragFocus() const;
	virtual void		dragFocus( bool state );

	bool				dropFocus() const;
	virtual void		dropFocus( bool state );

	//this is set every frame by the colour / colour / alpha shaders.
	uint32				runTimeColour() const;
	void				runTimeColour( uint32 col );

	//this is set every frame by matrix shaders.
	const Matrix &		runTimeTransform() const;
	void				runTimeTransform( const Matrix& m );

	//adds bounding box in local or world space.  if gui component is drawn in 2D, the
	//bounding box represents clip coordinates, otherwise world coordinates.
	void				localBoundingBox( BoundingBox& bb, bool skinny = false );
	void				worldBoundingBox( BoundingBox& bb, const Matrix& world, bool skinny = false );

	void				drawOrder( uint32 order );
	uint32				drawOrder();

	void				pixelSnap( bool state );
	bool				pixelSnap() const;

	uint32				guiTreeCookie() const	{ return guiTreeCookie_; }
	void				guiTreeCookie(uint32 c)	{ guiTreeCookie_ = c; }
	void				guiTreeCookieRecursive(uint32 c);

	bool				allowDelete() {return allowDelete_;}
	void				allowDelete(bool value) {allowDelete_ = value;}

	/**
	 *	Python attributes
	 */
	PY_RO_ATTRIBUTE_DECLARE( parent_, parent )

	// PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, position, position )
	PyObject * pyGet_position();
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( Vector3, position, position )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, width, width )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, height, height )

	PyObject * pyGet_size();
	PY_WRITABLE_ACCESSOR_ATTRIBUTE_SET( Vector2, size, size )


	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, pixelSnap, pixelSnap )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, visible, visible )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, textureName, textureName )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, tiled, tiled )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, tileWidth, tileWidth )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, tileHeight, tileHeight )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, focus, focus )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, mouseButtonFocus, mouseButtonFocus )	
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, moveFocus, moveFocus )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, crossFocus, crossFocus )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, dragFocus, dragFocus )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, dropFocus, dropFocus )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, widthInClipCoords, widthRelative )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, heightInClipCoords, heightRelative )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, flip, flip )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, visibleInAppFocusOnly, visibleInAppFocusOnly )

	PyObject * pyGet_colour();
	int pySet_colour( PyObject * value );

	PY_DEFERRED_ATTRIBUTE_DECLARE( horizontalAnchor )
	PY_DEFERRED_ATTRIBUTE_DECLARE( verticalAnchor )
	PY_DEFERRED_ATTRIBUTE_DECLARE( horizontalPositionMode )
	PY_DEFERRED_ATTRIBUTE_DECLARE( verticalPositionMode )
	PY_DEFERRED_ATTRIBUTE_DECLARE( widthMode )
	PY_DEFERRED_ATTRIBUTE_DECLARE( heightMode )
	PY_DEFERRED_ATTRIBUTE_DECLARE( materialFX )
	PY_DEFERRED_ATTRIBUTE_DECLARE( filterType )

	PyObject * pyGet_children();
	PY_RO_ATTRIBUTE_SET( children )

	PyObject * pyGet_shaders();
	PY_RO_ATTRIBUTE_SET( shaders )

	PyObject * pyGet_angle();
	int pySet_angle( PyObject * value );

	PyObject * pyGet_mapping();
	int pySet_mapping( PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( pScriptObject_, script )

	PyObject * pyGet_texture();
	int pySet_texture( PyObject * value );

protected:
	float widthInClip( float relativeParentWidth ) const;
	float widthInPixels( float relativeParentWidth ) const;
	float heightInClip( float relativeParentHeight ) const;
	float heightInPixels( float relativeParentHeight ) const;

	void setWidthInScreenClip ( float width );
	void setHeightInScreenClip ( float width );

	void positionInClip( float relativeParentWidth, float relativeParentHeight, float &x, float &y ) const;
	void anchorOffset( float width, float height, float &x, float &y ) const;

	void layout( float relativeParentWidth, float relativeParentHeight, 
				 float& x, float& y, float& w, float& h ) const;

	virtual ConstSimpleGUIComponentPtr nearestRelativeParent( int depth=0 ) const;
	void nearestRelativeDimensions( float &width, float& height ) const;

	virtual Vector2 localToScreenInternalOffset( bool isThis ) const;

	void				tile();

	void				cleanMesh();
	void				cleanMaterial();
	virtual void		buildMesh();

	virtual void		updateChildren( float dTime, float relParentWidth, float relParentHeight );

	virtual void		drawSelf( bool reallyDraw, bool overlay );
	void				drawChildren( Moo::DrawContext& drawContext, bool reallyDraw, bool overlay );

	bool				handlesInput() const;

	void destructComponent();

	///whether or not destructComponent has been already called.
	bool componentDestructed_;

	typedef OrderedStringMap< SmartPointer<SimpleGUIComponent> > ChildRecVector;
	ChildRecVector		children_;

	WeakPyPtr< SimpleGUIComponent > parent_;

	BW::vector<int>	childOrder_;

	bool updatingChildren_;

	///the texture
	Moo::BaseTexturePtr texture_;
	PyTextureProviderPtr	textureProvider_;

	int					nVertices_;
	int					nIndices_;

	GUIVertex*			blueprint_;
	GUIVertex*			vertices_;
	uint16*				indices_;

	///the material to draw this component with.
	Moo::EffectMaterialPtr material_;
	static BW::vector<D3DXHANDLE> s_techniqueTable;
	static bool setupTechniqueTable( Moo::EffectMaterialPtr material );

	///GUI type vertex shaders
	typedef OrderedStringMap<GUIShaderPtr> GUIShaderPtrVector;
	GUIShaderPtrVector	shaders_;

	SmartPointer< SimpleGUIComponent >	pMouseOverChild_;

	///pluggable script event handling
	SmartPointer< PyObject >			pScriptObject_;

	///the position of the component.
	///the z coordinate is ignored
	Vector3				position_;
	ePositionMode		horizontalPositionMode_;
	ePositionMode		verticalPositionMode_;

	///width and height of the component
	///you should always use the getter methods to read these as it sometimes
	///calculated via lazy evaluation.
	float		width_;	
	eSizeMode			widthMode_;
	float		height_;
	eSizeMode			heightMode_;

	///tiled or not?
	bool				tiled_;
	///tile width in pixels
	float				tileWidth_;
	///tile height in pixels
	float				tileHeight_;

	///setup the material.
	virtual bool		buildMaterial();

	///inhibits drawing for just the next frame
	///this is a protected member because of Mesh Adaptors.
	bool				momentarilyInvisible_;

	///this is the dynamic clipping region.  it is calculated at draw time,
	///and kept for hit-testing the next frame;
	Vector4				runTimeClipRegion_;

	///enables/disables pixel snapping in the vertex shader
	bool				pixelSnap_;	

private:
	SimpleGUIComponent(const SimpleGUIComponent&);
	SimpleGUIComponent& operator=(const SimpleGUIComponent&);
	void				calculateAutoMC();

	bool invokeMouseEventHandler( PyObject * pEventHandler_,
			const char * methodName, const Vector2 & mousePosition, 
			SimpleGUIComponent * dragged, const char * callErrorPrefix, 
			const char * returnErrorPrefix );

	bool invokeKeyEventHandler( 
			PyObject * pEventHandler_, const char * methodName, 
			const KeyEvent & event, SimpleGUIComponent * dragged, 
			const char * callErrorPrefix, const char * returnErrorPrefix );	

	///the colour of the component.  Use the alpha
	///part of the colour to fade out the component
	uint32				colour_;
	///this is the dynamic, mutable colour.  it is modified by shaders per frame,
	///without resorting to changing vertex colours.
	uint32				runTimeColour_;
	///this is the dynamic transform. this transform is inherited by 
	///children. it is reset to identity every frame, so the only way 
	///to update it is with a shader. After the component has been draw, 
	///runTimeTransform_ will store the combined world-view-projection 
	///transform used to draw to component.
	Matrix				runTimeTransform_;

	///visible determines whether this component is drawn or not
	bool				visible_;
	bool				visibleInAppFocusOnly_;


	///vertical anchor describes how the position relates to the
	///component.  for example, set this to ANCHOR_V_TOP and
	///the component will hang down below the position
	eVAnchor			verticalAnchor_;
	///horizontal anchor.  see vertical anchor
	eHAnchor			horizontalAnchor_;
	///material effect
	eMaterialFX			materialFX_;
	eFilterType			filterType_;

	///cached rotation enum
	eRotation			cachedAngle_;
	int					flip_;

	///input handling states
	bool				focus_;
	bool				mouseButtonFocus_;
	bool				moveFocus_;
	bool				crossFocus_;
	bool				dragFocus_;
	bool				dropFocus_;

	uint32				drawOrder_;
	uint32				nextDrawOrder_;
	uint32				guiTreeCookie_;
	bool				allowDelete_;
	/**
	 *	This is a helper class to compare the depth of two components
	 */
	class DepthCompare
	{
	public:
		DepthCompare( const ChildRecVector & crv ) : crv_( crv ) { }
		int operator()( const int & aidx, const int & bidx );
	private:
		const ChildRecVector & crv_;
	};

	friend class DepthCompare;
	friend class SimpleGuiSortedDrawItem;

	void				reSortChildren();

	BW::string ownerName_;

	COMPONENT_FACTORY_DECLARE( SimpleGUIComponent("") )
};

PY_SCRIPT_CONVERTERS_DECLARE( SimpleGUIComponent )
PY_ENUM_CONVERTERS_DECLARE( SimpleGUIComponent::eHAnchor )
PY_ENUM_CONVERTERS_DECLARE( SimpleGUIComponent::eVAnchor )
PY_ENUM_CONVERTERS_DECLARE( SimpleGUIComponent::ePositionMode )
PY_ENUM_CONVERTERS_DECLARE( SimpleGUIComponent::eSizeMode )
PY_ENUM_CONVERTERS_DECLARE( SimpleGUIComponent::eMaterialFX )
PY_ENUM_CONVERTERS_DECLARE( SimpleGUIComponent::eFilterType )


#ifdef CODE_INLINE
#include "simple_gui_component.ipp"
#endif

BW_END_NAMESPACE

#endif // SIMPLE_GUI_COMPONENT_HPP
