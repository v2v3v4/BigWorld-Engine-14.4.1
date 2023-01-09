#include "pch.hpp"
#include "tool.hpp"
#include "tool_view.hpp"
#include "cstdmf/debug.hpp"
#include "model/model.hpp"
#include "moo/visual_manager.hpp"
#include "moo/geometrics.hpp"
#include "romp/font_manager.hpp"
#include "appmgr/options.hpp"
#include "resmgr/string_provider.hpp"
#include "chunk/chunk_item.hpp"
#include "material_utility.hpp"

// Required by FloatVisualiser
#include "general_properties.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"

/*~ module View
 *	@components{ tools }
 *
 *	The View Module is a Python module that provides an interface to
 *	the various tool "views" for displaying tools.
 */

DECLARE_DEBUG_COMPONENT2( "ToolView", 0 );

BW_BEGIN_NAMESPACE

//------------------------------------------------------------
//Section : ToolLocator
//------------------------------------------------------------

/// static factory map initialiser
template<> ViewFactory::ObjectMap * ViewFactory::pMap_;


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ToolView::

PY_TYPEOBJECT( ToolView )

PY_BEGIN_METHODS( ToolView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ToolView )
	PY_ATTRIBUTE( viewResource )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
ToolView::ToolView( PyTypeObject * pType ):
	PyObjectPlus( pType )
{
}


/**
 *	Destructor.
 */
/*virtual*/ ToolView::~ToolView()
{
}


/**
 *	This method returns a transform used to draw the tool with,
 *	scaled appropriately with the tool's size and scale.
 *
 *	@param	tool	the tool for this view.
 *	@param	result	the resultant scaled, positioned transform.
 */
void ToolView::viewTransform( const Tool& tool, Matrix& result ) const
{
	BW_GUARD;

	result = tool.locator()->transform();
	Matrix scale;
	scale.setScale( tool.size(), 1.f, tool.size() );
	result.preMultiply( scale );
}


//------------------------------------------------------------
//Section : MeshToolView
//------------------------------------------------------------


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE MeshToolView::

PY_TYPEOBJECT( MeshToolView )

PY_BEGIN_METHODS( MeshToolView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MeshToolView )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( MeshToolView, "MeshToolView", View )

VIEW_FACTORY( MeshToolView )

/**
 *	Constructor
 *
 *	@param	resourceID	the visual resource to use.
 */
MeshToolView::MeshToolView( const BW::string& resourceID, PyTypeObject * pType ):
	ToolView( pType ),
	visual_( NULL )
{
	BW_GUARD;

	this->viewResource( resourceID );
}


/**
 *	This method sets the mesh resource, given a visual name.
 *
 *	@param visualResource	name of the visual to use.
 */
void MeshToolView::viewResource(
	const BW::string& visualResource )
{
	BW_GUARD;

	visual_ = Moo::VisualManager::instance()->get(
			visualResource );
}


/**
 *	This method updates LODs and animations on the tool view.
 *	@param tool the tool.
 */
void MeshToolView::updateAnimations( const Tool& tool )
{
	BW_GUARD;
}


/**
 *	This method is MeshToolView's particular way of drawing a tool.
 *
 *	@param tool		The tool to draw.
 */
void MeshToolView::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	BW_GUARD;

	if ( visual_ )
	{
		Moo::rc().push();

		Matrix transform;
		viewTransform( tool, transform );
		Moo::rc().world( transform );

		visual_->draw( drawContext );

		Moo::rc().pop();
	}
}


/**
 *	Static python factory method
 */
PyObject * MeshToolView::pyNew( PyObject * args )
{
	BW_GUARD;

	char * visualName;
	if (!PyArg_ParseTuple( args, "|s", &visualName ))
	{
		PyErr_SetString( PyExc_TypeError, "View.MeshToolView: "
			"Argument parsing error: Expected an optional visual name" );
		return NULL;
	}

	if ( visualName != NULL )
		return new MeshToolView( visualName );
	else
		return new MeshToolView( "res/objects/models/billboard.static.visual" );
}


//------------------------------------------------------------
//Section : ModelToolView
//------------------------------------------------------------


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE ModelToolView::

PY_TYPEOBJECT( ModelToolView )

PY_BEGIN_METHODS( ModelToolView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ModelToolView )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( ModelToolView, "ModelToolView", View )

VIEW_FACTORY( ModelToolView )

/**
 *	Constructor
 *
 *	@param	resourceID	the visual resource to use.
 */
ModelToolView::ModelToolView( const BW::string& resourceID, PyTypeObject * pType ):
	ToolView( pType ),
	pModel_( NULL )
{
	BW_GUARD;

	this->viewResource( resourceID );
}


ModelToolView::~ModelToolView()
{
	BW_GUARD;

	bw_safe_delete(pModel_);
}


/**
 *	This method sets the mesh resource, given a model name.
 *
 *	@param visualResource	name of the visual to use.
 */
void ModelToolView::viewResource(
	const BW::string& resource )
{
	BW_GUARD;

	BW::vector<BW::string>	modelNames;
	modelNames.push_back( resource );

	pModel_ = new SuperModel( modelNames );
}


/**
 *	This method updates LODs and animations on the tool view.
 *	@param tool the tool to update.
 */
void ModelToolView::updateAnimations( const Tool& tool )
{
	BW_GUARD;

	if (!pModel_)
	{
		return;
	}


	Matrix transform;
	this->viewTransform( tool, transform );

	pModel_->updateAnimations( transform,
		NULL, // pPreFashions
		NULL, // pPostFashions
		Model::LOD_AUTO_CALCULATE );
}


/**
 *	This method is ModelToolView's particular way of drawing a tool.
 *
 *	@param tool		The tool to draw.
 */
void ModelToolView::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	BW_GUARD;

	if (!pModel_)
	{
		return;
	}

	Moo::rc().push();

	Matrix transform;
	this->viewTransform( tool, transform );
	Moo::rc().world( transform );

	pModel_->draw( drawContext, transform, NULL );

	Moo::rc().pop();
}


/**
 *	Static python factory method
 */
PyObject * ModelToolView::pyNew( PyObject * args )
{
	BW_GUARD;

	char * modelName;
	if (!PyArg_ParseTuple( args, "|s", &modelName ))
	{
		PyErr_SetString( PyExc_TypeError, "View.ModelToolView: "
			"Argument parsing error: Expected an optional model name" );
		return NULL;
	}

	if ( modelName != NULL )
		return new ModelToolView( modelName );
	else
		return new ModelToolView;
}




// -----------------------------------------------------------------------------
// Section: FloatVisualiser
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( FloatVisualiser )

PY_BEGIN_METHODS( FloatVisualiser )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( FloatVisualiser )
PY_END_ATTRIBUTES()

PY_FACTORY( FloatVisualiser, View )

/**
 *	Constructor.
 */
FloatVisualiser::FloatVisualiser(
		MatrixProxyPtr pCenter,
		FloatProxyPtr pFloat,
		uint32 colour,
		const BW::string& name,
		LabelFormatter<float>& formatter,
		PyTypeObject * pType ) :
	ToolView( pType ),
	pCenter_( pCenter ),
	pFloat_( pFloat ),
	name_( LocaliseUTF8(name.c_str()) ),
	formatter_( formatter ),
	colour_( colour )
{
	BW_GUARD;

	backing_ = new SimpleGUIComponent( "resources/maps/gui/background.dds" );
	backing_->materialFX( SimpleGUIComponent::FX_BLEND );
	backing_->colour( 0x80000000 );
	backing_->widthMode( SimpleGUIComponent::SIZE_MODE_PIXEL );
	backing_->heightMode( SimpleGUIComponent::SIZE_MODE_PIXEL );
	SimpleGUI::instance().addSimpleComponent( *backing_ );

	text_ = new TextGUIComponent( FontManager::instance().getCachedFont("default_small.font") );
	text_->filterType( SimpleGUIComponent::FT_LINEAR );
	text_->colour( colour_ );
	BW::string label = name_ + " : " + formatter_.format( pFloat_->get() );
	text_->slimLabel( label );
	SimpleGUI::instance().addSimpleComponent( *text_ );
}


FloatVisualiser::~FloatVisualiser()
{
	BW_GUARD;

	SimpleGUI::instance().removeSimpleComponent( *backing_ );
	Py_XDECREF( backing_ );
	backing_ = NULL;

	SimpleGUI::instance().removeSimpleComponent( *text_ );
	Py_XDECREF( text_ );
	text_ = NULL;
}


/**
 *	This method updates LODs and animations on the tool view.
 *	@param tool the tool.
 */
void FloatVisualiser::updateAnimations( const Tool& tool )
{
	BW_GUARD;
}


/**
 *	This method renders the float visualisation.
 */
void FloatVisualiser::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	BW_GUARD;

	bool drawText = !name_.empty();
	if (drawText)
	{
		BW::string label = name_ + " : " + formatter_.format( pFloat_->get() );
		text_->slimLabel( label );
	}

	backing_->width( text_->SimpleGUIComponent::width() );

	//find clip position of center.
	Matrix m;
	pCenter_->getMatrix( m );
	Vector3 pos( m.applyToOrigin() );
	Vector4 projPos( pos.x, pos.y, pos.z, 1.f );
	Moo::rc().viewProjection().applyPoint( projPos, projPos );

	if (drawText && projPos.w > 0.f)
	{
		float oow = 1.f / projPos.w;
		projPos.x *= oow;
		projPos.y *= oow;
		text_->position( Vector3( projPos.x, projPos.y, 1.f ) );
		text_->visible( true );
		text_->colour( colour_ );
	}
	else
	{
		text_->visible( false );
	}

	backing_->position( text_->position() );
	backing_->visible( text_->visible() );
	//backing_->width( TextGUIComponent::s_defFont().fontWidth() * strlen( buf ) );
	//backing_->height( TextGUIComponent::s_defFont().fontHeight() );
}


/**
 *	Python factory method
 */
PyObject * FloatVisualiser::pyNew( PyObject * args )
{
	BW_GUARD;

#if 0
	PyObject * pFloat = NULL;

	if (!PyArg_ParseTuple( args, "|f", &pFloat ))
	{
		PyErr_SetString( PyExc_TypeError, "FloatVisualiser() "
			"expects a a float argument" );
		return NULL;
	}

	FloatVisualiser * rv = new FloatVisualiser();
	return rv;
#endif

	PyErr_SetString( PyExc_NotImplementedError,
		"FloatVisualiser: not supported" );
	return NULL;	//Too bad
}



// -----------------------------------------------------------------------------
// Section: Vector2Visualiser
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( Vector2Visualiser )

PY_BEGIN_METHODS( Vector2Visualiser )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector2Visualiser )
PY_END_ATTRIBUTES()

PY_FACTORY( Vector2Visualiser, View )

/**
 *	Constructor.
 */
Vector2Visualiser::Vector2Visualiser(
		MatrixProxyPtr pCenter,
		FloatProxyPtr pFloatX,
		FloatProxyPtr pFloatY,
		uint32 colour,
		const BW::string& name,
		LabelFormatter<float>& formatter,
		PyTypeObject * pType ) :
	ToolView( pType ),
	pCenter_( pCenter ),
	pFloatX_( pFloatX ),
	pFloatY_( pFloatY ),
	name_( LocaliseUTF8(name.c_str()) ),
	formatter_( formatter ),
	colour_( colour )
{
	BW_GUARD;

	backing_ = new SimpleGUIComponent( "resources/maps/gui/background.dds" );
	backing_->materialFX( SimpleGUIComponent::FX_BLEND );
	backing_->colour( 0x80000000 );
	backing_->widthMode( SimpleGUIComponent::SIZE_MODE_PIXEL );
	backing_->heightMode( SimpleGUIComponent::SIZE_MODE_PIXEL );
	SimpleGUI::instance().addSimpleComponent( *backing_ );

	text_ = new TextGUIComponent( FontManager::instance().getCachedFont("default_medium.font") );
	text_->filterType( SimpleGUIComponent::FT_LINEAR );
	text_->colour( colour_ );
	BW::string label =
		name_ + " : " +
		formatter_.format( pFloatX_->get() ) + ", " +
		formatter_.format( pFloatY_->get() );
	text_->slimLabel( label );
	SimpleGUI::instance().addSimpleComponent( *text_ );
}


Vector2Visualiser::~Vector2Visualiser()
{
	BW_GUARD;

	SimpleGUI::instance().removeSimpleComponent( *backing_ );
	Py_XDECREF( backing_ );
	backing_ = NULL;

	SimpleGUI::instance().removeSimpleComponent( *text_ );
	Py_XDECREF( text_ );
	text_ = NULL;
}


/**
 *	This method updates LODs and animations on the tool view.
 *	@param tool the tool.
 */
void Vector2Visualiser::updateAnimations( const Tool& tool )
{
	BW_GUARD;
}


/**
 *	This method renders the float visualisation.
 */
void Vector2Visualiser::render( Moo::DrawContext& drawContext, const Tool& tool )
{
	BW_GUARD;

	bool drawText = !name_.empty();

	if (drawText)
	{
		BW::string label =
			name_ + " : " +
			formatter_.format( pFloatX_->get() ) + ", " +
			formatter_.format( pFloatY_->get() );
		text_->slimLabel( label );
	}

	backing_->width( text_->SimpleGUIComponent::width() );

	//find clip position of center.
	Matrix m;
	pCenter_->getMatrix( m );
	Vector3 pos( m.applyToOrigin() );
	Vector4 projPos( pos.x, pos.y, pos.z, 1.f );
	Moo::rc().viewProjection().applyPoint( projPos, projPos );

	if (drawText && projPos.w > 0.f)
	{
		float oow = 1.f / projPos.w;
		projPos.x *= oow;
		projPos.y *= oow;
		text_->position( Vector3( projPos.x, projPos.y, 1.f ) );
		text_->visible( true );
		text_->colour( colour_ );
	}
	else
	{
		text_->visible( false );
	}

	backing_->position( text_->position() );
	backing_->visible( text_->visible() );
}


/**
 *	Python factory method
 */
PyObject * Vector2Visualiser::pyNew( PyObject * args )
{
	// Not yet implemented.
	return NULL;
}



PY_TYPEOBJECT( TeeView )

PY_BEGIN_METHODS( TeeView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TeeView )
PY_END_ATTRIBUTES()

PY_FACTORY( TeeView, View )


/**
 *	This method updates LODs and animations on the tool view.
 *	@param tool the tool.
 */
void TeeView::updateAnimations( const Tool& tool )
{
	BW_GUARD;
	this->activeView()->updateAnimations( tool );
}


PyObject * TeeView::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject* f1;
	PyObject* f2;
	int i;
	if (!PyArg_ParseTuple( args, "OOi", &f1, &f2, &i ) ||
		!ToolView::Check( f1 ) ||
		!ToolView::Check( f2 ))
	{
		PyErr_SetString( PyExc_TypeError, "TeeView() "
			"expects two ToolViews and a key" );
		return NULL;
	}

	return new TeeView( 
		static_cast<ToolView*>( f1 ),
		static_cast<ToolView*>( f2 ),
		static_cast<KeyCode::Key>( i )
		);
}
BW_END_NAMESPACE

