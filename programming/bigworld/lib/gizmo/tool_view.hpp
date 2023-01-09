#ifndef TOOL_VIEW_HPP_
#define TOOL_VIEW_HPP_

#include "cstdmf/named_object.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "moo/render_target.hpp"
#include "moo/visual.hpp"
#include "input/input.hpp"
#include "model/super_model.hpp"
#include "formatter.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class keeps the factory methods for all types of Tool Views
 */
class Tool;
class ToolView;
typedef NamedObject<ToolView * (*)()> ViewFactory;

#define VIEW_FACTORY_DECLARE( CONSTRUCT )									\
	static ViewFactory s_factory;											\
	virtual ViewFactory & factory() { return s_factory; }					\
	static ToolView * s_create() { return new CONSTRUCT; }					\

#define VIEW_FACTORY( CLASS )												\
	ViewFactory CLASS::s_factory( #CLASS, CLASS::s_create );				\

namespace Moo
{
	class DrawContext;
}

/**
 *	The ToolView class displays a tool.
 */
class ToolView : public PyObjectPlus
{
	Py_Header( ToolView, PyObjectPlus )
public:
	ToolView( PyTypeObject * pType = &s_type_ );
	virtual ~ToolView();
	virtual void viewResource( const BW::string& resourceID )
		{ resourceID_ = resourceID; }
	const BW::string& viewResource()	{ return resourceID_; }

	virtual void updateAnimations( const Tool& tool ) = 0;
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool ) = 0;

	//-------------------------------------------------
	//Python Interface
	//-------------------------------------------------
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, viewResource, viewResource )
protected:
	void viewTransform( const Tool& tool, Matrix& result ) const;

	BW::string	resourceID_;
};


typedef SmartPointer<ToolView>				ToolViewPtr;
typedef StringHashMap<ToolViewPtr> 			ToolViewPtrs;


/**
 *	This class implements a toolview that uses a mesh to draw with.
 **/
class MeshToolView : public ToolView
{
	Py_Header( MeshToolView, ToolView )
public:
	MeshToolView( const BW::string& resourceID_ = "res/objects/models/billboard.static.visual",
					PyTypeObject * pType = &s_type_ );

	virtual void viewResource( const BW::string& resourceID );
	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	PY_FACTORY_DECLARE()
protected:
	Moo::VisualPtr	visual_;

	VIEW_FACTORY_DECLARE( MeshToolView() )
};


/**
 *	This class implements a toolview that uses a model to draw with.
 **/
class ModelToolView : public ToolView
{
	Py_Header( ModelToolView, ToolView )
public:
	ModelToolView( const BW::string& resourceID_ = "resources/models/omni_light.model",
					PyTypeObject * pType = &s_type_ );
	~ModelToolView();

	virtual void viewResource( const BW::string& resourceID );
	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	PY_FACTORY_DECLARE()
protected:
	SuperModel*	pModel_;

	VIEW_FACTORY_DECLARE( ModelToolView() )
};

/**
 *	This class visualises a radius at a given point
 */
class MatrixProxy;
class FloatProxy;
typedef SmartPointer<class FloatProxy> FloatProxyPtr;
typedef SmartPointer<class MatrixProxy> MatrixProxyPtr;

class FloatVisualiser : public ToolView
{
	Py_Header( FloatVisualiser, ToolView )
public:
	FloatVisualiser( MatrixProxyPtr pCenter,
			FloatProxyPtr pFloat,
			uint32 colour,
			const BW::string& name,
			LabelFormatter<float>& formatter = DistanceFormatter::s_def,
			PyTypeObject * pType = &s_type_ );
	~FloatVisualiser();

	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	void	colour( uint32 c )	{ colour_ = c; }
	uint32	colour() const		{ return colour_; }

	PY_FACTORY_DECLARE()

private:
	int		colour_;
	MatrixProxyPtr pCenter_;
	FloatProxyPtr pFloat_;
	BW::string name_;
	BW::string units_;
	LabelFormatter<float>&	formatter_;
	class SimpleGUIComponent* backing_;
	class TextGUIComponent*	text_;
};


/**
 *	This class visualises a radius at a given point
 */
class Vector2Visualiser : public ToolView
{
	Py_Header( Vector2Visualiser, ToolView )
public:
	Vector2Visualiser( MatrixProxyPtr pCenter,
			FloatProxyPtr pFloatX,
			FloatProxyPtr pFloatY,
			uint32 colour,
			const BW::string& name,
			LabelFormatter<float>& formatter = DistanceFormatter::s_def,
			PyTypeObject * pType = &s_type_ );
	~Vector2Visualiser();

	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool );

	void	colour( uint32 c )	{ colour_ = c; }
	uint32	colour() const		{ return colour_; }

	PY_FACTORY_DECLARE()

private:
	int		colour_;
	MatrixProxyPtr pCenter_;
	FloatProxyPtr pFloatX_;
	FloatProxyPtr pFloatY_;
	BW::string name_;
	BW::string units_;
	LabelFormatter<float>&	formatter_;
	SimpleGUIComponent* backing_;
	TextGUIComponent*	text_;
};


// -----------------------------------------------------------------------------
// Section: TeeView
// -----------------------------------------------------------------------------

class TeeView : public ToolView
{
	Py_Header( TeeView, ToolView )

private:
	ToolView* f1_;
	ToolView* f2_;
	KeyCode::Key altKey_;

	ToolView* activeView() const {
		if (InputDevices::isKeyDown( altKey_ ))
			return f2_;
		else
			return f1_;
	}

public:
	TeeView( ToolView* f1, ToolView* f2, KeyCode::Key altKey, PyTypeObject * pType = &s_type_ )
		: f1_(f1), f2_(f2), altKey_(altKey) {}

	virtual void updateAnimations( const Tool& tool );
	virtual void render( Moo::DrawContext& drawContext, const Tool& tool )
	{
		activeView()->render( drawContext, tool );
	}

	PY_FACTORY_DECLARE()
};

BW_END_NAMESPACE

#endif