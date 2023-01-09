#include "pch.hpp"

#include "general_editor.hpp"
#include "tool_manager.hpp"
#include "appmgr/options.hpp"
#include "current_general_properties.hpp"

#include "cstdmf/bw_set.hpp"


// -----------------------------------------------------------------------------
// Section: GeneralEditor
// -----------------------------------------------------------------------------

DECLARE_DEBUG_COMPONENT2( "Gizmo", 0 )

BW_BEGIN_NAMESPACE

// make the standard python stuff
PY_TYPEOBJECT( GeneralEditor )

PY_BEGIN_METHODS( GeneralEditor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( GeneralEditor )
PY_END_ATTRIBUTES()

PY_MODULE_STATIC_METHOD( GeneralEditor, getCurrentEditors, WorldEditor )
PY_MODULE_STATIC_METHOD( GeneralEditor, setCurrentEditors, WorldEditor )

/**
 *	Constructor.
 */
GeneralEditor::GeneralEditor( PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	constructorOver_( false )
{
}


/**
 *	Destructor.
 */
GeneralEditor::~GeneralEditor()
{
	BW_GUARD;

	for (PropList::iterator it = properties_.begin();
		it != properties_.end();
		it++)
	{
		(*it)->deleteSelf();
	}
	properties_.clear();
}


/**
 *	This method adds the given property to this editor. The editor takes
 *	ownership of the pointer passed in. Properties may only be added in
 *	response to an edEdit call on an item ... they are not dynamic.
 *	This is more to enable property views to be written more easily and
 *	not worry about new properties appearing while elected than anything
 *	else. If this ruling proves constrictive then it could be lifted.
 */
void GeneralEditor::addProperty( GeneralProperty * pProp )
{
	BW_GUARD;

	MF_ASSERT( !constructorOver_ );
    MF_ASSERT( pProp );

	// add the item as the parent group property belongs to
	if (lastItemName_.empty())
		pProp->setGroup( pProp->getGroup() );
	else if (pProp->getGroup().size() == 1 && pProp->getGroup()[0] == '/')
		pProp->setGroup( lastItemName_ );
	else
		pProp->setGroup( lastItemName_.str() + '/' + pProp->getGroup().c_str() );

	properties_.push_back( pProp );
}


/**
 *	This method takes whatever action is necessary when a chunk item editor
 *	is elected to the list of currently visible editors
 */
void GeneralEditor::elect()
{
	BW_GUARD;

	s_settingMultipleEditors_ = s_currentEditors_.size() > 1 &&
		std::find( s_currentEditors_.begin(), s_currentEditors_.end(), this )
			!= s_currentEditors_.end();

	for (PropList::iterator it = properties_.begin();
		it != properties_.end(); ++it)
	{
		(*it)->elect();
		(*it)->elected();
	}
}

/**
 *	This method takes whatever action is necessary when a chunk item editor
 *	is expelled from the list of currently visible editors
 */
void GeneralEditor::expel()
{
	BW_GUARD;

	for (PropList::iterator it = properties_.begin();
		it != properties_.end();
		it++)
	{
		(*it)->expel();
	}
}

/**
 *	This static method returns the list of current editors
 */
const GeneralEditor::Editors & GeneralEditor::currentEditors()
{
	BW_GUARD;

	return s_currentEditors_;
}

/**
 *	This static method sets the list of current editors
 */
void GeneralEditor::currentEditors( const Editors & newEds )
{
	BW_GUARD;

	BW::set<int>	notChanged;

	GeneralProperty::View::pLastElected( NULL );

	Editors oldEds = s_currentEditors_;
	s_currentEditors_.clear();

	// get rid of old editors
	for (Editors::iterator it = oldEds.begin(); it != oldEds.end(); it++)
	{
		Editors::const_iterator found =
			std::find( newEds.begin(), newEds.end(), *it );
		if (found != newEds.end())
		{
			notChanged.insert( (int)( found - newEds.begin() ) );
		}
		else
		{
			(*it)->expel();
		}
	}

	oldEds.clear();

	s_currentEditors_ = newEds;
	s_currentEditorIndex_ = (int)notChanged.size();
	s_lastEditorIndex_ = (int)newEds.size() - 1;

	// instate new editors
	for (uint i = 0; i < newEds.size(); i++)
	{
		if (notChanged.find( i ) == notChanged.end())
		{
			newEds[i]->elect();
			s_currentEditorIndex_++;
		}
	}

	
	if (GeneralProperty::View::pLastElected())
	{
		GeneralProperty::View::pLastElected()->lastElected();
	}
}


/**
 *	Get an attribute for python
 */
ScriptObject GeneralEditor::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();
	
	// see if the name matches any of our properties
	for (PropList::iterator it = properties_.begin();
		it != properties_.end();
		it++)
	{
		if ((*it)->name() == attr)
		{
			return ScriptObject(
				(*it)->pyGet(),
				ScriptObject::FROM_NEW_REFERENCE );
		}
	}

	return PyObjectPlus::pyGetAttribute( attrObj );
}

/**
 *	Set an attribute for python
 */
bool GeneralEditor::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	BW_GUARD;

	const char * attr = attrObj.c_str();

	// make sure a tool isn't applying itself
	if (ToolManager::instance().tool()->applying())
	{
		PyErr_Format( PyExc_EnvironmentError, "GeneralEditor.%s "
			"cannot set editor attributes while a tool is applying itself",
			attr );
		return false;
	}

	// see if the name matches any of our properties
	for (PropList::iterator it = properties_.begin();
		it != properties_.end();
		it++)
	{
		if ((*it)->name() == attr)
		{
			return ((*it)->pySet( value.get() ) == 0);
		}
	}

	return PyObjectPlus::pySetAttribute( attrObj, value );
}


/**
 *	This method adds the names of our properties as additional members
 *	of this object.
 */
void GeneralEditor::pyAdditionalMembers( const ScriptList & pList ) const
{
	BW_GUARD;

	for (uint i = 0; i < properties_.size(); i++)
	{
		pList.append( ScriptString::create( properties_[i]->name().c_str() ) );
	}
}


/*~ function WorldEditor.getCurrentEditors
 *	@components{ worldeditor }
 *
 *	This function gets the sequence of current editors.
 *
 *	@return Returns a sequence of the current editors in use.
 */
PyObject * GeneralEditor::py_getCurrentEditors( PyObject * args )
{
	BW_GUARD;

	if (PyTuple_Size(args))
	{
		PyErr_SetString( PyExc_TypeError, "WorldEditor.getCurrentEditors "
			"expects no arguments" );
		return NULL;
	}

	PyObject * pTuple = PyTuple_New( s_currentEditors_.size() );
	for (uint i = 0; i < s_currentEditors_.size(); i++)
	{
		PyObject * editor = s_currentEditors_[i].getObject();
		Py_INCREF( editor );
		PyTuple_SetItem( pTuple, i, editor );
	}
	return pTuple;
}

/*~ function WorldEditor.setCurrentEditors
 *	@components{ worldeditor }
 *
 *	This function sets the sequence of current editors.
 *
 *	@param editors The sequence of editors to be set as the current editors.
 */
PyObject * GeneralEditor::py_setCurrentEditors( PyObject * args )
{
	BW_GUARD;

	// use the first argument as args if it's a sequence
	if (PyTuple_Size( args ) == 1 &&
		PySequence_Check( PyTuple_GetItem( args, 0 ) ))
	{
		args = PyTuple_GetItem( args, 0 );	// borrowed
	}

	Editors	newEds;

	int na = (int)PySequence_Size( args );
	for (int i = 0; i < na; i++)
	{
		PyObject * pItem = PySequence_GetItem( args, i );	// new
		if (pItem == NULL || !GeneralEditor::Check( pItem ))
		{
			Py_XDECREF( pItem );

			PyErr_SetString( PyExc_TypeError, "WorldEditor.setCurrentEditors "
				"expects some (or one sequence of some) GeneralEditors" );
			return NULL;
		}

		newEds.push_back( GeneralEditorPtr(
			static_cast<GeneralEditor*>(pItem), true ) );
	}

	GeneralEditor::currentEditors( newEds );

	Py_RETURN_NONE;
}


/// static initialisers
GeneralEditor::Editors GeneralEditor::s_currentEditors_;
int GeneralEditor::s_currentEditorIndex_ = 0;
int GeneralEditor::s_lastEditorIndex_ = 0;
bool GeneralEditor::s_settingMultipleEditors_ = false;
bool GeneralEditor::s_createViews_ = true;

GeneralProperty::View* GeneralProperty::View::pLastElected_ = NULL;

// -----------------------------------------------------------------------------
// Section: PropManager
// -----------------------------------------------------------------------------


namespace
{
	BW::vector<PropManager::PropFini> *s_finiCallback_ = NULL;
}


void PropManager::registerFini( PropFini fn )
{
	BW_GUARD;

	if (s_finiCallback_ == NULL)
		s_finiCallback_ = new BW::vector<PropManager::PropFini>();
	s_finiCallback_->push_back(fn);
}


void PropManager::fini()
{
	BW_GUARD;

	if (s_finiCallback_ != NULL)
	{
		BW::vector<PropFini>::iterator it = s_finiCallback_->begin();
		for (;it != s_finiCallback_->end(); it++)
		{
			PropFini fin = (*it);
			if (fin != NULL)
				(*fin)( );
		}
		bw_safe_delete(s_finiCallback_);
	}
}


// -----------------------------------------------------------------------------
// Section: Name
// -----------------------------------------------------------------------------
Name::NameHolder* Name::holder_;
int Name::hashIndex_[ Name::NAME_BUCKET ];
Name* Name::names_[ Name::NAME_BUCKET ];


// -----------------------------------------------------------------------------
// Section: GeneralProperty
// -----------------------------------------------------------------------------

GENPROPERTY_VIEW_FACTORY( GeneralProperty )


/**
 *	Constructor.
 */
GeneralProperty::GeneralProperty( const Name& name ) :
	name_( name ),
	propManager_( NULL ),
	WBEditable_( false ),
	canExposeToScript_( false )
{
	GENPROPERTY_MAKE_VIEWS()
}

/**
 *	Destructor
 */
GeneralProperty::~GeneralProperty()
{
}

void GeneralProperty::WBEditable( bool editable )
{
        WBEditable_ = editable;
}

bool GeneralProperty::WBEditable() const
{
        return WBEditable_;
}

void GeneralProperty::descName( const Name& descName )
{
	descName_ = descName;
}

const Name& GeneralProperty::descName()
{
	return descName_;
}

void GeneralProperty::UIName( const Name& name )
{
        UIName_ = name;
}

const Name& GeneralProperty::UIName()
{
        if (UIName_.empty())
        {
			UIName( name_ );
        }

        return UIName_;
}

void GeneralProperty::UIDesc( const Name& name )
{
        UIDesc_ = name;
}

const Name& GeneralProperty::UIDesc()
{
        return UIDesc_;
}

void GeneralProperty::exposedToScriptName( const Name& exposedToScriptName )
{
	exposedToScriptName_ = exposedToScriptName;
}

const Name& GeneralProperty::exposedToScriptName()
{
	return exposedToScriptName_;
}

void GeneralProperty::canExposeToScript( bool canExposeToScript )
{
	canExposeToScript_ = canExposeToScript;
}

bool GeneralProperty::canExposeToScript() const
{
	return canExposeToScript_;
}

/**
 *	This method notifies all the views that this property has been elected.
 *
 *	Most derived property classes will not need to override this method
 */
void GeneralProperty::elect()
{
	BW_GUARD;

	for (int i = 0; i < nextViewKindID_; i++)
	{
		if (views_[i] != NULL)
		{
			views_[i]->elect();
			if (GeneralEditor::settingMultipleEditors() &&
				views_[i]->isEditorView())
			{
				GeneralProperty::View::pLastElected( views_[i] );
			}
		}
	}
}

/**
 *	This method notifies all the views that this property has finished electing.
 *
 *	Most derived property classes will not need to override this method
 */
void GeneralProperty::elected()
{
	BW_GUARD;

	for (int i = 0; i < nextViewKindID_; i++)
	{
		if (views_[i] != NULL)
		{
			views_[i]->elected();
		}
	}
}

/**
 *	This method notifies all the views that this property has been selected as
 *  the current active property.
 *
 *	Most derived property classes will not need to override this method
 */
void GeneralProperty::select()
{
	BW_GUARD;

	for (int i = 0; i < nextViewKindID_; i++)
	{
		if (views_[i] != NULL)
		{
            //DEBUG_MSG( "GeneralProperty [%lx] : electing view %lx\n", (int)this, (int)views_[i] );
			views_[i]->select();
            //DEBUG_MSG( "...done %d\n", i );
		}
	}
}


/**
 *	This method notifies all the views that this property has been expelled.
 *
 *	Most derived property classes will not need to override this method
 */
void GeneralProperty::expel()
{
	BW_GUARD;

	for (int i = 0; i < nextViewKindID_; i++)
	{
		if (views_[i] != NULL)
		{
			views_[i]->expel();
		}
	}
}


/**
 *	Get the python object equivalent to the current value of this property
 *
 *	All classes derived from GeneralProperty should override this method
 */
PyObject * GeneralProperty::pyGet()
{
	BW_GUARD;

	PyErr_Format( PyExc_SystemError,
		"GeneralEditor.%s does not have a get method", name_ );
	return NULL;
}

/**
 *	Set the current value of this property from an equivalent python object
 *
 *	All classes derived from GeneralProperty should override this method
 */
int GeneralProperty::pySet( PyObject * value, bool transient,
													bool addBarrier )
{
	BW_GUARD;

	PyErr_Format( PyExc_SystemError,
		"GeneralEditor.%s does not have a set method", name_ );
	return -1;
}


/**
 *	Allocates a new view kind id and returns it. Each view kind should
 *	only call this function once (at static initialisation time) and
 *	store their own id somewhere.
 */
int GeneralProperty::nextViewKindID()
{
	return nextViewKindID_++;
}


/**
 *	Finds out if the property is visible, that is, has views. This is useful in
 *	setting up editing of common properties.
 */
bool GeneralProperty::hasViews() const
{
	BW_GUARD;

	for (int i = 0; i < nextViewKindID_; i++)
	{
		if (views_[i] != NULL)
		{
			return true;
		}
	}
	return false;
}


/**
 *	This function is a C-style version of GeneralProperty::nextViewKindID. It is
 *	exported by the DLL.
 */
int GeneralProperty_nextViewKindID()
{
	return GeneralProperty::nextViewKindID();
}

/// static initialisers
int GeneralProperty::nextViewKindID_ = 0;



// -----------------------------------------------------------------------------
// Section: GeneralProperty Views
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
GeneralProperty::Views::Views() :
	e_( new View*[ nextViewKindID_ ] )
{
	BW_GUARD;

	for (int i = 0; i < nextViewKindID_; i++)
	{
		e_[i] = NULL;
	}
}

/**
 *	Destructor.
 */
GeneralProperty::Views::~Views()
{
	BW_GUARD;

	for (int i = 0; i < nextViewKindID_; i++)
	{
		if (e_[i] != NULL)
		{
			e_[i]->deleteSelf();
		}
	}
	bw_safe_delete_array(e_);
}


/**
 *	Set a view into the array
 */
void GeneralProperty::Views::set( int i, View * v )
{
	BW_GUARD;

	if (e_[i] != NULL)
	{
		e_[i]->deleteSelf();
	}

	e_[i] = v;
}

/**
 *	Retrieve a view from the array
 */
GeneralProperty::View * GeneralProperty::Views::operator[]( int i )
{
	BW_GUARD;

	return e_[i];
}


/**
 *	Retrieve a view from the array (const version)
 */
const GeneralProperty::View * GeneralProperty::Views::operator[]( int i ) const
{
	BW_GUARD;

	return e_[i];
}


// -----------------------------------------------------------------------------
// Section: GeneralROProperty
// -----------------------------------------------------------------------------


/**
 *	Constructor
 */
GeneralROProperty::GeneralROProperty( const Name& name ) :
	GeneralProperty( name )
{
	GENPROPERTY_MAKE_VIEWS()
}


/**
 *	Python set method
 */
int GeneralROProperty::pySet( PyObject * value, bool transient /*=false*/,
													bool addBarrier /*=true*/ )
{
	BW_GUARD;

	PyErr_Format( PyExc_TypeError, "Sorry, the attribute %s in "
		"GeneralEditor is read-only", name_ );
	return NULL;
}

/// static initialiser
GENPROPERTY_VIEW_FACTORY( GeneralROProperty )

BW_END_NAMESPACE
// general_editor.cpp
