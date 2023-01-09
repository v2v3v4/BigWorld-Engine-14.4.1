#include "pch.hpp"
#include "tool.hpp"
#include "tool_view.hpp"
#include "physics2/worldtri.hpp"
#include "input/py_input.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/dogWatch.hpp"
#include "chunk/chunk_item.hpp"

#ifndef CODE_INLINE
#include "tool.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Tool", 0 );

BW_BEGIN_NAMESPACE

void ToolList::add( Tool* pTool )
{
	s_toolList_.push_back( pTool );
}

void ToolList::remove( Tool* pTool )
{
	for( BW::vector<Tool*>::iterator i = s_toolList_.begin(); 
		i != s_toolList_.end(); ++i )
	{
		if( *i == pTool )
		{
			s_toolList_.erase( i );
			return;
		}
	}
}

void ToolList::clearAll()
{
	for( BW::vector<Tool*>::iterator i = s_toolList_.begin(); 
		i != s_toolList_.end(); ++i )
	{
		(*i)->clear();
	}	
}

BW::vector<Tool*> ToolList::s_toolList_;

static DogWatch s_toolLocate( "tool_locate" );
static DogWatch s_toolView( "tool_draw" );
static DogWatch s_toolFunctor( "tool_apply_function" );

ChunkFinderPtr Tool::s_pChunkFinder_;
bool Tool::s_enableRender_ = true;

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE Tool::

PY_TYPEOBJECT( Tool )

PY_BEGIN_METHODS( Tool )
	PY_METHOD( addView )
	PY_METHOD( delView )
	PY_METHOD( handleKeyEvent )
	PY_METHOD( handleMouseEvent )
	PY_METHOD( handleContextMenu )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Tool )
	PY_ATTRIBUTE( size )
	PY_ATTRIBUTE( strength )
	PY_ATTRIBUTE( locator )
	PY_ATTRIBUTE( functor )
	PY_ATTRIBUTE( applying )
PY_END_ATTRIBUTES()

PY_FACTORY( Tool, WorldEditor )


/**
 *	Constructor.
 */
Tool::Tool( ToolLocatorPtr locator,
			 ToolViewPtr view,
			 ToolFunctorPtr functor,
			 PyTypeObject * pType ):
	PyObjectPlus( pType ),
	size_( 1.f ),
	strength_( 25.f ),
	currentChunk_( NULL ),
	relevantChunks_(),
	view_(),
	locator_( locator ),
	functor_( functor )
{
	BW_GUARD;

	if (view)
	{
		this->addView( "default", view );
	}
	
	ToolList::add( this );
}

Tool::~Tool()
{
	ToolList::remove( this );
}


/**
 *	A callback for when this tool is pushed on the tool stack.
 */
void Tool::onPush()
{
	BW_GUARD;

	if (functor_.exists())
	{
		functor_->onPush( *this );
	}
}

/**
 *	A callback for when this tool is popped off the tool stack.
 */
void Tool::onPop()
{
	if (functor_.exists())
	{
		functor_->onPop( *this );
	}
}

/**
 *	A callback for when this tool becomes the active tool.
 */
void Tool::onBeginUsing()
{
	BW_GUARD;
	MF_ASSERT( currentChunk_ == NULL );
	MF_ASSERT( relevantChunks_.empty() );

	if (functor_.exists())
	{
		functor_->onBeginUsing( *this );
	}
}

/**
 *	A callback for when this tool ceases being the active tool.
 */
void Tool::onEndUsing()
{
	BW_GUARD;

	currentChunk_ = NULL;
	relevantChunks_.clear();

	if (functor_.exists())
	{
		functor_->onEndUsing( *this );
	}
}


/**
 *	This method is the entry point for calculating the position
 *	of the tool.  Most of the work is done in the tool's locator.
 *
 *	@param	worldRay	The camera's world ray.
 */
void Tool::calculatePosition( const Vector3& worldRay )
{
	BW_GUARD;

	s_toolLocate.start();

	relevantChunks_.clear();

	if (locator_.exists())
	{
		locator_->calculatePosition( worldRay, *this );
		this->findRelevantChunks();
	}

	s_toolLocate.stop();
}


/**
 *	This method updates the tool.  It delegates the call
 *	to the tool's functor.
 *
 *	@param	dTime	The change in time since the last frame.
 */
void Tool::update( float dTime )
{
	BW_GUARD;

	s_toolFunctor.start();

	if (functor_.exists())
	{
		functor_->update( dTime, *this );
	}

	s_toolFunctor.stop();
}


/**
 *	This method updates LODs and animations on the tool.
 */
void Tool::updateAnimations()
{
	BW_GUARD;

	s_toolView.start();

	ToolViewPtrs::iterator it = view_.begin();
	ToolViewPtrs::iterator end = view_.end();

	while ( it != end )
	{
		(it++)->second->updateAnimations( *this );
	}

	s_toolView.stop();
}


/**
 *	This method renders the tool.  It delegates the call
 *	to all of the tool's views.
 */
void Tool::render( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if (!s_enableRender_)
		return;

	s_toolView.start();

	ToolViewPtrs::iterator it = view_.begin();
	ToolViewPtrs::iterator end = view_.end();

	while ( it != end )
	{
		(it++)->second->render( drawContext, *this );
	}

	s_toolView.stop();
}


/**
 *	This method overrides InputHandler's method.
 *
 *	@see InputHandler::handleKeyEvent
 */
bool Tool::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;

	if (functor_.exists())
	{
		return functor_->handleKeyEvent( event, *this );
	}
	return false;
}


/**
 *	This method overrides InputHandler's method.
 *
 *	@see InputHandler::handleMouseEvent
 */
bool Tool::handleMouseEvent( const MouseEvent & event )
{
	BW_GUARD;

	if (functor_.exists())
	{
		return functor_->handleMouseEvent( event, *this );
	}
	return false;
}


/**
 *	This method can be called from python to tell the tool to 
 *	display its relevant context menu.
 */
bool Tool::handleContextMenu()
{
	BW_GUARD;

	if (functor_.exists())
	{
		return functor_->handleContextMenu( *this );
	}
	return false;
}


/**
 *	Tell the tool to stop applying if it is currently being applied.
 *	@param saveChanges keep the changes or discard them.
 */
void Tool::stopApplying( bool saveChanges )
{
	BW_GUARD;

	if (functor_.exists())
	{
		functor_->stopApplying( *this, saveChanges );
	}
}


/**
 *	This method sets the tool's locator.
 *
 *	@param	spl		The new locator for the tool.
 */
void Tool::locator( ToolLocatorPtr spl )
{
	locator_ = spl;
}


/**
 *	This method retrieves the vector of views for the tool.
 *
 *	@return	The map of views.
 */
const ToolViewPtrs& Tool::view() const
{
	return view_;
}


/**
 *	This method retrieves the a view for the tool.
 *
 *	@return	The desired view, if it exists.
 */
ToolViewPtr& Tool::view( const BW::string& name )
{
	BW_GUARD;

	return view_[name];
}



/**
 *	This method adds a new view to the tool
 *
 *	@param	spv		The new view for the tool.
 */
void Tool::addView( const BW::string& name, ToolViewPtr spv )
{
	BW_GUARD;

	if ( spv.hasObject() )
	{
		view_[name] = spv;
	}
}


/**
 *	This method deletes a view from the tool
 *
 *	@param	spv		The view to remove.
 */
void Tool::delView( ToolViewPtr spv )
{
	BW_GUARD;

	ToolViewPtrs::iterator it;
	for (it = view_.begin(); it != view_.end(); it++)
	{
		if (it->second.getObject() == spv)
		{
			break;
		}
	}

	if (it != view_.end())
	{
		view_.erase( it );
	}
}


/**
 *	This method deletes a view from the tool, by name.
 *
 *	@param	name	The name of the view to remove.
 */
void Tool::delView( const BW::string & name )
{
	BW_GUARD;

	view_.erase( name );
}


/**
 *	This method sets the tool's functor.
 *
 *	@param	spf		The new functor for the tool.
 */
void Tool::functor( ToolFunctorPtr spf )
{
	functor_ = spf;
}


/**
 *	This method returns the tool's locator.
 *
 *	@return The tool's locator.
 */
const ToolLocatorPtr Tool::locator() const
{
	return locator_;
}


/**
 *	This method returns the tool's functor.
 *
 *	@return The tool's functor.
 */
const ToolFunctorPtr Tool::functor() const
{
	return functor_;
}


/**
 *	Set the chunk finding functor to use.
 *	@param finder the chunk finder to use
 */
void Tool::setChunkFinder( ChunkFinderPtr pFinder )
{
	BW_GUARD;
	s_pChunkFinder_ = pFinder;
}


/**
 *	This method finds all the chunks that a tool covers.
 *
 *	The relevant chunks are stored in the tool's relevantChunks vector.
 */
void Tool::findRelevantChunks( float buffer /* = 0.f */)
{
	BW_GUARD;
	if ( s_pChunkFinder_.exists() )
	{
		(*s_pChunkFinder_)( this, buffer );
	}
}

/**
 *  clear references to external objects that have been given
 */
void Tool::clear()
{
	view_.clear();
	locator_ = NULL;
	functor_ = NULL;
}

/**
 *	Get an attribute for python
 */
ScriptObject Tool::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	// try one of the view names
	ToolViewPtrs::iterator vit = view_.find( attrObj.c_str() );
	if (vit != view_.end())
	{
		return ScriptObjectPtr<ToolView>( vit->second );
	}

	// ask our base class
	return PyObjectPlus::pyGetAttribute( attrObj );
}


/**
 *	Add a view for python
 */
PyObject * Tool::py_addView( PyObject * args )
{
	BW_GUARD;

	char noName[32];

	PyObject	* pView;
	char * name = noName;

	// parse args
	if (!PyArg_ParseTuple( args, "O|s", &pView, &name ) ||
		!ToolView::Check( pView ))
	{
		PyErr_SetString( PyExc_TypeError, "Tool.py_addView() "
			"expects a ToolView and optionally a name" );
		return NULL;
	}

	// make up a name if none was set
	if (name == noName || !name[0])
	{
		name = noName;
		bw_snprintf( noName, sizeof( noName ), "S%p", pView );
	}

	this->addView( name, static_cast<ToolView*>( pView ) );

	Py_RETURN_NONE;
}

/**
 *	Del a view for python
 */
PyObject * Tool::py_delView( PyObject * args )
{
	BW_GUARD;

	if (PyTuple_Size( args ) == 1)
	{
		PyObject * pItem = PyTuple_GetItem( args, 0 );
		if (ToolView::Check( pItem ))
		{
			this->delView( static_cast<ToolView*>( pItem ) );
			Py_RETURN_NONE;
		}
		if (PyString_Check( pItem ))
		{
			this->delView( PyString_AsString( pItem ) );
			Py_RETURN_NONE;
		}
	}

	PyErr_SetString( PyExc_TypeError, "ToolView.py_delView "
		"expects a ToolView or a string" );
	return NULL;
}


/**
 *	Handle a right click call from python
 */
PyObject * Tool::py_handleContextMenu( PyObject * args )
{
	BW_GUARD;

	return Script::getData( this->handleContextMenu() );
}


/**
 *	Python factory method
 */
PyObject * Tool::pyNew( PyObject * args )
{
	BW_GUARD;

	PyObject * l = NULL, * v = NULL, * f = NULL;
	if (!PyArg_ParseTuple( args, "|OOO", &l, &v, &f ) ||
		(l != NULL && !ToolLocator::Check( l )) ||
		(v != NULL && !ToolView::Check( v )) ||
		(f != NULL && !ToolFunctor::Check( f )))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.Tool() "
			"expects an optional ToolLocator, ToolView, and ToolFunctor" );
		return NULL;
	}

	return new Tool(
		static_cast<ToolLocator*>(l),
		static_cast<ToolView*>(v),
		static_cast<ToolFunctor*>(f) );
}

BW_END_NAMESPACE
// tool.cpp
