#ifndef TOOL_HPP_
#define TOOL_HPP_

#include "cstdmf/bw_vector.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "input/input.hpp"
#include "input/py_input.hpp"
#include "tool_view.hpp"
#include "tool_locator.hpp"
#include "tool_functor.hpp"

BW_BEGIN_NAMESPACE

//this kind of vector is only good for one frame ( not even ?),
//since we can't guarantee the existence of the chunk.
class Chunk;
typedef BW::vector< Chunk* >	ChunkPtrVector;
typedef Chunk*	ChunkPtr;

class Tool;
typedef SmartPointer<Tool>	ToolPtr;


/**
 *	Default implementation for Tool::findRelevantChunks.
 */
class ChunkFinder : public SafeReferenceCount
{
public:
	virtual void operator()( ToolPtr tool, float buffer = 0.f ) = 0;
};

typedef SmartPointer<ChunkFinder> ChunkFinderPtr;

namespace Moo
{
	class DrawContext;
}

/**
 *	The list of Tools that have been created.
 *  This was originally created to solve a problem with order of destruction 
 *  in WorldEditor. Because a Tool holds references to external objects it was
 *  delaying the destruction of these objects until scripting was destroyed.
 *  These objects had rendering resources associated with them and need
 *  to be destroyed before ChunkManager is destroyed. 
 */
class ToolList
{
public:

	// Go through all the tools in the list and clear them. This allows
	// all the references to objects they have to be released.
	static void clearAll();

private:
	friend class Tool;
	static void add( Tool* tool );
	static void remove( Tool* tool );

	static BW::vector<Tool*> s_toolList_;
};


/**
 *	The Tool class is an amalgamation of View, Locator and Functor.
 *	This class exposes common Tool properties, as well as these
 *	plugin logic units.  All methods are delegated through the
 *	plugin units.
 */
class Tool : public InputHandler, public PyObjectPlus
{
	Py_Header( Tool, PyObjectPlus )

public:
	Tool( ToolLocatorPtr locator,
		ToolViewPtr view,
		ToolFunctorPtr functor,
		PyTypeObject * pType = &s_type_ );

	virtual ~Tool();

	virtual void onPush();
	virtual void onPop();

	virtual void onBeginUsing();
	virtual void onEndUsing();

	virtual void size( float s );
	virtual float size() const;

	virtual void strength( float s );
	virtual float strength() const;

	virtual const ToolLocatorPtr locator() const;
	virtual void locator( ToolLocatorPtr spl );

	virtual const ToolViewPtrs& view() const;
	virtual ToolViewPtr& view( const BW::string& name );
	virtual void addView( const BW::string& name, ToolViewPtr spv );
	virtual void delView( ToolViewPtr spv );
	virtual void delView( const BW::string& name );

	virtual const ToolFunctorPtr functor() const;
	virtual void functor( ToolFunctorPtr spf );

	virtual ChunkPtrVector& relevantChunks();
	virtual const ChunkPtrVector& relevantChunks() const;

	virtual ChunkPtr& currentChunk();
	virtual const ChunkPtr& currentChunk() const;

	virtual void calculatePosition( const Vector3& worldRay );
	virtual void update( float dTime );
	virtual void updateAnimations();
	virtual void render( Moo::DrawContext& drawContext );
	virtual bool handleKeyEvent( const KeyEvent & event );
	virtual bool handleMouseEvent( const MouseEvent & event );
	virtual bool handleContextMenu();
	virtual bool applying() const;
	virtual void stopApplying( bool saveChanges );

	static void setChunkFinder( ChunkFinderPtr pFinder );
	void findRelevantChunks( float buffer = 0.f );
	void clear();

	static bool enableRenderTools() { return s_enableRender_; }
	static void enableRenderTools( bool enable ) { s_enableRender_ = enable; }

	//-------------------------------------------------
	//Python Interface
	//-------------------------------------------------
	ScriptObject		pyGetAttribute( const ScriptString & attrObj );

	PY_METHOD_DECLARE( py_addView )
	PY_METHOD_DECLARE( py_delView )

	PY_AUTO_METHOD_DECLARE( RETDATA, handleKeyEvent, ARG( KeyEvent, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, handleMouseEvent, ARG( MouseEvent, END ) )
	PY_METHOD_DECLARE( py_handleContextMenu )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, size, size )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, strength, strength )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( ToolLocatorPtr, locator, locator )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( ToolFunctorPtr, functor, functor )

	PY_RO_ATTRIBUTE_DECLARE( applying(), applying )

	PY_FACTORY_DECLARE()

private:
	float			size_;
	float			strength_;
	ChunkPtr		currentChunk_;
	ChunkPtrVector	relevantChunks_;

	ToolViewPtrs	view_;
	ToolLocatorPtr	locator_;
	ToolFunctorPtr	functor_;

	static ChunkFinderPtr s_pChunkFinder_;
	static bool s_enableRender_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "tool.ipp"
#endif

#endif
