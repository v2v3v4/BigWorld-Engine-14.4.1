#include "pch.hpp"
#include "resource_table.hpp"

#include "resmgr/bwresource.hpp"
#include "cstdmf/debug.hpp"

#if defined( __GNUC__ )
#include <float.h>
#endif

DECLARE_DEBUG_COMPONENT2( "Script", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: ResTblStruct
// -----------------------------------------------------------------------------

class ResTblStruct;
typedef SmartPointer<class ResTblStruct> ResTblStructPtr;

/**
 *	This class is a struct in a resource table.
 *	It handles the inheritance of values through the ResourceTable tree.
 */
class ResTblStruct : public PyObjectPlus
{
	Py_Header( ResTblStruct, PyObjectPlus )

public:
	ResTblStruct( ResTblStructPtr pParent, DataSectionPtr pValues,
		PyTypeObject * pType = &s_type_ );
	~ResTblStruct();

	ScriptObject pyGetAttribute( const ScriptString & attrObj );

	PyObject * get( const BW::string & key, PyObjectPtr defVal );
	PY_AUTO_METHOD_DECLARE( RETOWN, get, ARG( BW::string,
		OPTARG( PyObjectPtr, NULL, END ) ) )

private:
	ResTblStructPtr		pParent_;
	DataSectionPtr		pValues_;
};

// NOTE: ResMgr.ResTblStruct has been deprecated.
/*	class ResMgr.ResTblStruct
 *	@components{ all }
 *	This class represents the data stored in a ResourceTable.  
 *	It can be accessed through the ResourceTable.value attribute.
 */
PY_TYPEOBJECT( ResTblStruct )

PY_BEGIN_METHODS( ResTblStruct )

	// NOTE: ResMgr.ResTblStruct is deprecated.
	/*	function ResTblStruct.get
	 *	@components{ all }
	 *	This method checks the table and returns the value represented by the key, if it exists.  
	 *	If it doesn't exist, it will recursively check the parent tables, whereupon, if the key 
	 *	is still not found, the given defaultValue will be returned.
	 *	@param	key				Name of resource (including resource path) as a string.
	 *	@param	defaultValue	Value to return if given key is not found (None or otherwise).
	 *	@return	If key is valid, returns stored value.  If key is not found, returns defaultValue.
	 */
	PY_METHOD( get )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ResTblStruct )
PY_END_ATTRIBUTES()

/**
 *	Constructor.
 */
ResTblStruct::ResTblStruct( ResTblStructPtr pParent, DataSectionPtr pValues,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pParent_( pParent ),
	pValues_( pValues )
{
}

/**
 *	Destructor.
 */
ResTblStruct::~ResTblStruct()
{
}

/**
 *	Python get attribute method
 */
ScriptObject ResTblStruct::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();

	// try it as a member of our struct
	PyObject * pVal = this->get( attr, (PyObject*)-1 );
	// (if we passed in NULL we'd get None back)
	if (pVal != (PyObject*)-1)
	{
		return ScriptObject( pVal, ScriptObject::FROM_NEW_REFERENCE );
	}

	return this->PyObjectPlus::pyGetAttribute( attrObj );
}


/**
 *	Temporary helper method to get a value out of a section
 */
static PyObject * guessPythonFromSection( DataSectionPtr pSect )
{
	BW::string str = pSect->asString();
	size_t len = str.length();

	// empty
	if (len == 0) { Py_RETURN_NONE; }

	// double and ints
	int hasDot = 0;
	size_t i;
	for (i = 0; i < len; i++)
	{
		if (str[i] == '.') {hasDot++; continue;}
		if (uint8(str[i]-'0') > uint8('9'-'0')) break;
	}
	if (i >= len && hasDot < 2)
	{
		if (hasDot)
			return Script::getData( pSect->asDouble() );
		else
			return Script::getData( pSect->asInt() );
	}

	// bool
	bool b = pSect->asBool( false );
	if (b == pSect->asBool( true ))
		return Script::getData( b );

	// Vector3
	if (str[0] == '(' && str[len-1] == ')')
	{
		Vector3 dv3(FLT_MAX,-FLT_MAX,FLT_MAX);
		Vector3 v3 = pSect->asVector3( dv3 );
		if (v3 != dv3) return Script::getData( v3 );
	}

	// fall back to string
	return Script::getData( str );
}

/**
 *	Get method. Be careful calling this from C++, as defVal is expected
 *	to have its own reference for us to return (if not NULL).
 */
PyObject * ResTblStruct::get( const BW::string & key, PyObjectPtr defVal )
{
	// see if we have the attribute
	if (pValues_)
	{
		DataSectionPtr pSect = pValues_->openSection( key );
		if (pSect)
		{
			// try to figure out what kind of value it is
			// (in the absence of a schema...)
			return guessPythonFromSection( pSect );
		}
	}

	// otherwise ask our parent
	if (pParent_)
	{
		return pParent_->get( key, defVal );
	}

	// if we got to here then return the default
	if (defVal == NULL)
	{
		// convert NULL back to None
		Py_RETURN_NONE;
	}

	Py_INCREF( defVal.get() );
	return defVal.get();
}

// -----------------------------------------------------------------------------
// Section: ResourceTable
// -----------------------------------------------------------------------------

// NOTE: ResMgr.ResourceTable has been deprecated.
/*	class ResMgr.ResourceTable
 *	@components{ all }
 *	This class loads ResourceTable objects.  A resource table file has the following xml format.
 *	@{
 *		&lt;root&gt;
 *			&lt;!-- The schema can be used for definition and verification of data elements. --&gt;
 *			&lt;schema&gt; STRUCT
 *				&lt;modelName&gt;	STRING
 *					&lt;resource&gt; model &lt;/resource&gt;
 *				&lt;/modelName&gt;
 *			&lt;/schema&gt;
 *
 *			&lt;!-- Each key will form a sub/child ResourceTable, accessible via key or index. --&gt;
 *			&lt;key&gt; DeadTree
 *
 *				&lt;!-- Each value will be stored in a ResTblStruct. --&gt;
 *				&lt;value&gt;
 *					&lt;modelName&gt;	objects/models/trees/dead_tree.model	&lt;/modelName&gt;
 *				&lt;/value&gt;
 *
 *			&lt;/key&gt;
 *
 *			&lt;key&gt; EvergreenTree
 *				&lt;value&gt;
 *					&lt;modelName&gt;	objects/models/trees/evergreen_tree.model	&lt;/modelName&gt;
 *				&lt;/value&gt;
 *			&lt;/key&gt;
 *		&lt;/root&gt;
 *	@}
 */

namespace // anonymous
{
PyMappingMethods s_map_methods_ =
{
	ResourceTable::_pyMap_length,
	ResourceTable::_pyMap_subscript,
	NULL // mp_ass_subscript
};
} // end namespace (anonymous)

PY_TYPEOBJECT_WITH_MAPPING( ResourceTable, &s_map_methods_ )

PY_BEGIN_METHODS( ResourceTable )

	/* function ResourceTable.link
	 *	@components{ all }
	 *	Links a callable python object to this ResourceTable.
	 *	@param	updateFn	An optional python object that implements "def __call__( self, resourceTable ):",  
	 *						which is triggered when the ResourceTable is updated.
	 */
	PY_METHOD( link )

	/* function ResourceTable.unlink
	 *	@components{ all }
	 *	Unlinks the callable python object from this ResourceTable.
	 */
	PY_METHOD( unlink )

	/* function ResourceTable.at
	 *	@components{ all }
	 *	Returns the sub/child table structure corresponding to the given index.
	 *	Square brackets '[]' perform the same operation.
	 *	@param	index	The uint index of the table struct.
	 *	@return	ResourceTablePtr
	 */
	PY_METHOD( at )

	/* function ResourceTable.sub
	 *	@components{ all }
	 *	Returns the sub/child table structure corresponding to the given key.
	 *	@param	key	The key of the table struct as a string.
	 *	@return	ResourceTable
	 */
	PY_METHOD( sub )

	/* function ResourceTable.keyOfIndex
	 *	@components{ all }
	 *	Returns the key of the table that corresponds to the given index.
	 *	@param	index	The uint index to convert to a key.
	 *	@return	string (empty if index not valid)
	 */
	PY_METHOD( keyOfIndex )

	/* function ResourceTable.indexOfKey
	 *	@components{ all }
	 *	Returns the index of the table that corresponds to the given key
	 *	@param	key	The key to convert, as a string
	 *	@return	uint index or -1 if key not found
	 */
	PY_METHOD( indexOfKey )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ResourceTable )

	/* attribute ResourceTable.value
	 *	@components{ all }
	 *	This attribute is the pointer to the data stored by this ResourceTable.
	 *	@type	Read-only ResTblStruct
	 */
	PY_ATTRIBUTE( value )

	/* attribute ResourceTable.key
	 *	@components{ all }
	 *	This attribute is only available on sub/child ResourceTables.  
	 *	It contains the associated key for this sub/child ResourceTable.
	 *	@type	Read-only string
	 */
	PY_ATTRIBUTE( key )

	/* attribute ResourceTable.index
	 *	@components{ all }
	 *	This attribute is only available on sub/child ResourceTables.  
	 *	It contains the associated index for this sub/child ResourceTable.
	 *	@type	Read-only uint
	 */
	PY_ATTRIBUTE( index )

PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( ResourceTable )


/**
 *	Constructor.
 */
ResourceTable::ResourceTable( DataSectionPtr pSect, ResourceTablePtr pParent,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pSect_( pSect ),
	pParent_( pParent ),
	index_( ~0u )
{
	if (!pSect_) return;

	// if we don't have a parent read the schema
	if (!pParent_)
	{
		// read the schema
		DataSectionPtr pSchemaSect = pSect_->openSection( "schema" );
		if (pSchemaSect)
		{
			//pSchema_ = new Schema( pSchemaSect );
		}
		else
		{
			// default schema is just one 'STRUCT'
			//pSchema_ = new Schema( "STRUCT" );
		}
	}
	// otherwise use our parent's
	else
	{
		//pSchema_ = pParent_->pSchema_;
	}

	// find the parent value (only until we get schemas)
	ResTblStructPtr pParentStruct = pParent_ ?
		(ResTblStruct*)pParent_->pValue_.get() : NULL;

	// read the value
	DataSectionPtr pValueSect = pSect_->openSection( "value" );
	if (pValueSect)
	{
		//pValue_ = schema_->readValue( this, pValueSect );
		pValue_ = PyObjectPtr(
			new ResTblStruct( pParentStruct, pValueSect ), true );
	}
	else
	{
		//pValue_ = schema_->defaultValue( this );
		pValue_ = PyObjectPtr(
			new ResTblStruct( pParentStruct, NULL ), true );
	}

	// create all our subtables
	uint childCount = pSect_->countChildren();
	for (uint i = 0; i < childCount; i++)
	{
		DataSectionPtr pKeySect = pSect_->openChild( i );
		if (pKeySect->sectionName() != "key") continue;
		ResourceTablePtr pRT( new ResourceTable( pKeySect, this ), true );
		pRT->index_ = static_cast<uint>( keyedEntries_.size() );
		keyedEntries_.push_back( pRT );
	}
}


/**
 *	Destructor.
 */
ResourceTable::~ResourceTable()
{
}


/**
 *	Python get attribute method
 */
ScriptObject ResourceTable::pyGetAttribute( const ScriptString & attrObj )
{
	// try the value itself
	PyObject * tryRet = PyObject_GetAttr( pValue_.get(), attrObj.get() );
	if (tryRet != NULL) 
	{
		return ScriptObject( tryRet, ScriptObject::FROM_NEW_REFERENCE );
	}
	PyErr_Clear();	// (so error msg comes from us - unsure if wise 'tho)

	// if it had a problem then ignore it and call our default
	return this->PyObjectPlus::pyGetAttribute( attrObj );
}


/**
 *	Link the given update function with this resource.
 */
bool ResourceTable::link( PyObjectPtr updateFn, bool callNow )
{
	if (!PyCallable_Check( updateFn.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
			"ResourceTable.link() arg must be callable" );
		return false;
	}

	links_.insert( updateFn );
	if (callNow)
	{
		PyObject * fn = updateFn.get();
		Py_INCREF( fn );
		Script::call( fn, Py_BuildValue("(O)", (PyObject*)this),
			"ResourceTable::link: " );
	}

	return true;
}

/**
 *	Unlink the given update function from this resource.
 */
void ResourceTable::unlink( PyObjectPtr updateFn )
{
	links_.erase( updateFn );
}


/**
 *	Find the length of the sequence
 */
Py_ssize_t ResourceTable::pyMap_length()
{
	return keyedEntries_.size();
}

/**
 *	Look up a value as a mapping
 */
PyObject * ResourceTable::pyMap_subscript( PyObject * key )
{
	int keyI;
	if (Script::setData( key, keyI ) == 0)
	{
		return this->at( keyI );
	}
	PyErr_Clear();
	BW::string keyS;
	if (Script::setData( key, keyS, "ResourceTable[x]: x" ) == 0)
	{
		return this->sub( keyS );
	}
	return NULL;
}

/**
 *	Get the sub table at the given index
 */
PyObject * ResourceTable::at( int index )
{
	size_t widx = (index >= 0) ? index : (index + keyedEntries_.size());
	if (widx >= keyedEntries_.size())
	{
		PyErr_Format( PyExc_ValueError, "ResourceTable.at: "
			"index %d out of range", index );
		return NULL;
	}

	PyObject * pRet = keyedEntries_[ widx ].get();
	Py_INCREF( pRet );
	return pRet;
}

/**
 *	Get the sub table with the given key
 */
PyObject * ResourceTable::sub( const BW::string & key )
{
	for (uint i = 0; i < keyedEntries_.size(); i++)
	{
		if (keyedEntries_[i]->pSect_->asString() == key)
		{
			PyObject * pRet = keyedEntries_[ i ].get();
			Py_INCREF( pRet );
			return pRet;
		}
	}

	PyErr_Format( PyExc_ValueError, "ResourceTable.find: "
		"no entry for key %s", key.c_str() );
	return NULL;
}


/**
 *	Find the key of the given index.
 *	Returns the empty string if there is no such index.
 */
BW::string ResourceTable::keyOfIndex( int index )
{
	PyObject * pPyRT = this->at( index );
	if (pPyRT != NULL)
	{
		ResourceTablePtr pRT( (ResourceTable*)pPyRT, true );
		return pRT->key();
	}
	else
	{
		PyErr_Clear();
		return BW::string();
	}
}

/**
 *	Find the index of the given key.
 *	Returns -1 if there is no such key.
 */
int ResourceTable::indexOfKey( const BW::string & key )
{
	PyObject * pPyRT = this->sub( key );
	if (pPyRT != NULL)
	{
		ResourceTablePtr pRT( (ResourceTable*)pPyRT, true );
		return pRT->index_;
	}
	else
	{
		PyErr_Clear();
		return -1;
	}
}


/**
 *	Return the key of this table.
 */
BW::string ResourceTable::key()
{
	return pSect_->asString();
}

/**
 * A set of resource tables, keyed on their DataSectionPtr value.
 * (Which is in a census of its own, so will remain constant since it
 * is retained by the ResourceTable ... for now, anyway)
 */
struct RTSCompare 
{ 
	bool operator()( const ResourceTable * a, const ResourceTable * b ) const 
	{
		return a->pSect_.get() < b->pSect_.get(); 
	} 
};

typedef BW::set<ResourceTable*,RTSCompare> ResourceTableSet;
static ResourceTableSet s_census;

/**
 *	Python factory method.
 *	Looks in census first.
 */
PyObject * ResourceTable::New( const BW::string & resourceID,
	PyObjectPtr updateFn )
{
	WARNING_MSG( "ResourceTable has been deprecated and is planned for removal "
			"in the next BigWorld release.\n"
			"Use DataSection support instead.\n" );
	if (updateFn && !PyCallable_Check( updateFn.get() ))
	{
		PyErr_SetString( PyExc_TypeError,
			"ResourceTable() second arg must be callable" );
		return NULL;
	}

	// get the section
	DataSectionPtr pSect = BWResource::openSection( resourceID );
	if (!pSect)
	{
		PyErr_Format( PyExc_ValueError,
			"ResourceTable() no such resource '%s'", resourceID.c_str() );
		return NULL;
	}

	ResourceTable * rt = NULL;

	// check the census
	uint32 fakeRTBuf[8];	// 8 is plenty
	ResourceTable * fakeRT = (ResourceTable*)fakeRTBuf;

	// HACK: this is a hack that prevents the next line from crashing. 
	//			fakeRT->pSect_ was not initialised so smartpointer tried
	//			to decrement the reference count on a random bit of memory.
	*(uint32*)(&fakeRT->pSect_) = 0;

	fakeRT->pSect_ = pSect;
	ResourceTableSet::iterator found = s_census.find( fakeRT );
	if (found != s_census.end())
	{
		rt = *found;	// Note: single-threaded only for now
		Py_INCREF( rt );
	}

	// make a new one
	if (rt == NULL)
	{
		rt = new ResourceTable( pSect, NULL );
		s_census.insert( rt );
	}

	// Decrement the reference count again.
	fakeRT->pSect_ = NULL;

	// link in the update function
	if (updateFn)
	{
		MF_VERIFY( rt->link( updateFn, false ) );
		MF_ASSERT( !PyErr_Occurred() );
	}

	// and return it
	return rt;
}

/*	function ResMgr.ResourceTable
 *	@components{ all }
 *	Loads a ResourceTable, with an optional callback function for updates.  See the class description for 
 *	more information.
 *	@param	resourceID	Resource name (including path from root resource directory) of data section to load.
 *	@param	updateFn	An optional python object that implements "def __call__( self, resourceTable ):",  
 *						which is triggered when the ResourceTable is updated.
 *	@return	ResourceTable
 */
PY_FACTORY( ResourceTable, ResMgr )
#if 0	// unused function
static void removeFromCensus( ResourceTable * pRT )
{
	s_census.erase( pRT );
}
#endif

BW_END_NAMESPACE

// resource_table.cpp
