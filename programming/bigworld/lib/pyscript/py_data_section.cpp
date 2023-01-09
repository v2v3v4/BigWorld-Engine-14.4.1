#include "pch.hpp"
#include "py_data_section.hpp"
#include "script_math.hpp"

#include "resmgr/xml_section.hpp"
#include "resmgr/bwresource.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_data_section.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: Static methods
// -----------------------------------------------------------------------------

/**
 *	This method creates a new data section.
 */
PyObject * PyDataSection::pyNew( PyObject * args )
{
	const char * name = NULL;

	if (!PyArg_ParseTuple( args, "|s", &name ))
	{
		PyErr_SetString( PyExc_TypeError, "PyDataSection() "
			"expected an optional string argument" );
		return NULL;
	}

	return new PyDataSection( new XMLSection( name == NULL ? "" : name ) );
}

/**
 *	This function is used to implement operator[] for the scripting object.
 */
PyObject * PyDataSection::s_subscript( PyObject * self, PyObject * index )
{
	return ((PyDataSection *) self)->subscript( index );
}


/**
 *	This function returns the number of entities in the system.
 */
Py_ssize_t PyDataSection::s_length( PyObject * self )
{
	return ((PyDataSection *) self)->length();
}


// -----------------------------------------------------------------------------
// Section: Script definition
// -----------------------------------------------------------------------------

/**
 *	This structure contains the function pointers necessary to provide a Python
 *	Mapping interface.
 */
static PyMappingMethods g_dataSectionMapping =
{
	PyDataSection::s_length,	// mp_length
	PyDataSection::s_subscript,	// mp_subscript
	NULL						// mp_ass_subscript
};


/*~ class ResMgr.DataSection
 *	@components{ all }
 *	A DataSection controls the reading and writing of xml data sections.
 *	New DataSections can be created via ResMgr.DataSection().  In this
 *	case section cannot be saved because it has no file system association.
 *	To open an existing DataSection, use ResMgr.openSection().
 *
 *	A DataSection can be either an attribute or an tag - both work exactly
 *	the same internally.
 */
PY_TYPEOBJECT_WITH_MAPPING( PyDataSection, &g_dataSectionMapping )

PY_BEGIN_METHODS( PyDataSection )
	PY_METHOD( has_key )
	PY_METHOD( keys )
	PY_METHOD( items )
	PY_METHOD( values )

	PY_METHOD( readString )
	PY_METHOD( readWideString )
	PY_METHOD( readInt )
	PY_METHOD( readInt64 )
	PY_METHOD( readFloat )
	PY_METHOD( readVector2 )
	PY_METHOD( readVector3 )
	PY_METHOD( readVector4 )
	PY_METHOD( readMatrix )
	PY_METHOD( readBool )
	PY_METHOD( readBlob )

	PY_METHOD( writeString )
	PY_METHOD( writeWideString )
	PY_METHOD( writeInt )
	PY_METHOD( writeInt64 )
	PY_METHOD( writeFloat )
	PY_METHOD( writeVector2 )
	PY_METHOD( writeVector3 )
	PY_METHOD( writeVector4 )
	PY_METHOD( writeMatrix )
	PY_METHOD( writeBool )
	PY_METHOD( writeBlob )

	PY_METHOD( write )

	PY_METHOD( cleanName )
	PY_METHOD( isNameClean )

	PY_METHOD( readStrings )
	PY_METHOD( readWideStrings )
	PY_METHOD( readInts )
	PY_METHOD( readFloats )
	PY_METHOD( readVector2s )
	PY_METHOD( readVector3s )
	PY_METHOD( readVector4s )

	PY_METHOD( writeStrings )
	PY_METHOD( writeWideStrings )
	PY_METHOD( writeInts )
	PY_METHOD( writeFloats )
	PY_METHOD( writeVector2s )
	PY_METHOD( writeVector3s )
	PY_METHOD( writeVector4s )

	PY_METHOD( copyToZip )

	PY_METHOD( createSection )
	PY_METHOD( createSectionFromString )
	PY_METHOD( deleteSection )
	PY_METHOD( save )

	PY_METHOD( copy )

	PY_METHOD( child )
	PY_METHOD( childName )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyDataSection )
	/*~ attribute DataSection.isAttribute
	 *	@components{ all }
	 *
	 * This attribute controls if a DataSection is an attribute of its parent
	 * DataSection
	 *
	 * This attribute is only avaliable on XML DataSections which have no
	 * children, any other DataSections will throw an AttributeError exception.
	 *
	 * NOTE: Changing a data section to an attribute, will cause it to change
	 * order within the parent data section.
	 */
	 PY_ATTRIBUTE( isAttribute )
	/*~ attribute DataSection.asString
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	string.  When reading it, it doesn't include any of the child sections.
	 *	Also on reading, the leading and trailing white space is trimmed off
	 *	the string.  On writing, it doesn't effect any of the child nodes.
	 *
	 *	@type String
	 */
	PY_ATTRIBUTE( asString )
	/*~ attribute DataSection.asWideString
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	wide string.  When reading it, it doesn't include any of the child
	 *	sections.  Also on reading, the leading and trailing white space is
	 *	trimmed off the string.  On writing, it doesn't effect any of the
	 *	child nodes.
	 *
	 *	@type Wide String
	 */
	PY_ATTRIBUTE( asWideString )
	/*~ attribute DataSection.asInt
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as an
	 *	integer.  On reading, it reads the string from the top level, and tries
	 *	to parse an integer off the front of it.  If this is successful, the
	 *	integer is returned, otherwise zero.
	 *
	 *	On writing, it doesn't effect any of the child nodes, but replaces the
	 *	top level string with the string version of the integer.
	 *
	 *	@type Integer
	 */
	PY_ATTRIBUTE( asInt )
	/*~ attribute DataSection.asInt64
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a 64
	 *	bit integer. On reading, it reads the string from the top level and
	 *	tries to parse an integer off the front of it. If this is successful,
	 *	the integer is returned, otherwise zero.
	 *
	 *	On writing, it doesn't effect any of the child nodes, but replaces the
	 *	top level string with the string version of the integer.
	 *
	 *	@type Integer
	 */
	PY_ATTRIBUTE( asInt64 )
	/*~ attribute DataSection.asFloat
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as an
	 *	float.  On reading, it reads the string from the top level, and tries
	 *	to parse a float off the front of it.  If this is successful, the float
	 *	is returned, otherwise zero.
	 *
	 *	On writing, it doesn't effect any of the child nodes, but replaces the
	 *	top level string with the string version of the float.
	 *
	 *	@type Float
	 */
	PY_ATTRIBUTE( asFloat )
	/*~ attribute DataSection.asVector2
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	Vector2.  On reading, it reads the string from the top level, and tries
	 *	to parse it as two floats, separated by white space.  If this is
	 *	successful, the Vector2 of these floats is returned, otherwise the
	 *	Vector2 (0,0).
	 *
	 *	On writing, it doesn't effect any of the child nodes, but replaces the
	 *	top level string with the string version of the two components of the
	 *	Vector2 as floats, separated by white space.
	 *
	 *	@type Vector2
	 */
	PY_ATTRIBUTE( asVector2 )
	/*~ attribute DataSection.asVector3
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	Vector3.  On reading, it reads the string from the top level, and tries
	 *	to parse it as three floats, separated by white space.  If this is
	 *	successful, the Vector3 of these floats is returned, otherwise the
	 *	Vector3 (0,0,0).
	 *
	 *	On writing, it doesn't effect any of the child nodes, but replaces the
	 *	top level string with the string version of the three components of the
	 *	Vector3 as floats, separated by white space.
	 *
	 *	@type Vector3
	 */
	PY_ATTRIBUTE( asVector3 )
	/*~ attribute DataSection.asVector4
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	Vector4.  On reading, it reads the string from the top level, and tries
	 *	to parse it as four floats, separated by white space.  If this is
	 *	successful, the Vector4 of these floats is returned, otherwise the
	 *	Vector4 (0,0,0,0).
	 *
	 *	On writing, it doesn't effect any of the child nodes, but replaces the
	 *	top level string with the string version of the four components of the
	 *	Vector4 as floats, separated by white space.
	 *
	 *	@type Vector4
	 */
	PY_ATTRIBUTE( asVector4 )
	/*~ attribute DataSection.asMatrix
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	Matrix.
	 *
	 *	On reading, it checks for four attributes, called row0, row1,
	 *	row2 and row3.  If each of these exist and contain three floats
	 *	separated by whitespace, then they are interpreted as the four rows
	 *	of a Matrix, which is created and returned.  Otherwise, the Matrix
	 *	of all zeros is returned.
	 *
	 *	On Writing, it accepts a Matrix, and writes it out to the four
	 *	attributes row0, row1, row2 and row3, each of which will
	 *	contain one row of the Matrix - three floats separated by white
	 *	space.  This will not effect any top level text stored in this
	 *	DataSection.
	 *
	 *	These attributes may also be sub-sections within the matrix.
	 *
	 *	@type Matrix
	 */
	PY_ATTRIBUTE( asMatrix )
	/*~ attribute DataSection.asBool
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	boolean.
	 *
	 *	On reading, if the text contains "true" in any capitalisation, it
	 *	returns 1, otherwise it returns 0.
	 *
	 *	On writing, if it is given a 0, it writes "false" otherwise it writes
	 *	"true".
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( asBool )
	/*~ attribute DataSection.asBinary
	 *	@components{ all }
	 *
	 *	This attribute is the underlying binary data of this DataSection,
	 *	encapsulated in a string.  On reading, it includes all the child
	 *	sections.  On writing, it may invalidate previously accessed child
	 *	sections.  Use with care.  It is better to use the 'copy' method to
	 *	copy DataSections around than to use this attribute.
	 *
	 *	@type String
	 */
	PY_ATTRIBUTE( asBinary )
	/*~ attribute DataSection.asBlob
	 *	@components{ all }
	 *
	 *	This attribute is the value of this DataSection, interpreted as a
	 *	BASE64 string.  When reading it, it doesn't include any of the child
	 *	sections. Also on reading, the leading and trailing white space is
	 *	trimmed off the string.  On writing, it doesn't effect any of the
	 *	child nodes.
	 *
	 *	@type String
	 */
	PY_ATTRIBUTE( asBlob )

	/*~ attribute DataSection.name
	 *	@components{ all }
	 *
	 *	This attribute is the name of the data section.  It is read-only,
	 *	and consists of the tag surrounding this particular section
	 *
	 *	@type	Read-Only String
	 */
	PY_ATTRIBUTE( name )
PY_END_ATTRIBUTES()

/*~ function ResMgr.DataSection
 *	@components{ all }
 *
 *	This module function constructs a blank data section.  It can be optionally
 *	supplied with a new name.  This DataSection will not be savable using the
 *	save function, as it has no file system association.
 *
 *	@param	sectionName	Optional name for the new section
 *
 *	@return				the section that was created
 */
PY_FACTORY_NAMED( PyDataSection, "DataSection", ResMgr )

PY_SCRIPT_CONVERTERS( PyDataSection )

// -----------------------------------------------------------------------------
// Section: General methods
// -----------------------------------------------------------------------------

/**
 *	The constructor for PyDataSection.
 */
PyDataSection::PyDataSection( DataSectionPtr pSection, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pSection_( pSection )
{
	IF_NOT_MF_ASSERT_DEV( pSection_ )
	{
		MF_EXIT( "PyDataSection: NULL data section passed" );
	}
}


/**
 *	This method overrides the PyObjectPlus method.
 */
ScriptObject PyDataSection::pyGetAttribute( const ScriptString & attrObj )
{
	const char * attr = attrObj.c_str();
	
	// try it as a subsection
	if (attr[0] == '_' && (attr[1] != '_' && attr[1] != 0))
	{
		DataSectionPtr pChildSection = pSection_->openSection( attr+1 );
		if (pChildSection)
		{
			return ScriptObject(
				new PyDataSection( pChildSection ),
				ScriptObject::FROM_NEW_REFERENCE );
		}
	}

	return PyObjectPlus::pyGetAttribute( attrObj );
}

/**
 *	A helper function to see if an pyobject can be converted to some type.
 *	It does not set an exception, unlike setData ordinarily does.
 */
template <class C> bool harmlessSetData( PyObject * obj, C & res )
{
	int ret = Script::setData( obj, res );
	if (ret == 0) return true;
	PyErr_Clear();
	return false;
}

/**
 *	This method overrides the PyObjectPlus method.
 */
bool PyDataSection::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	const char * attr = attrObj.c_str();

	// try it as a subsection
	if (attr[0] == '_')
	{
		DataSectionPtr pChildSection = pSection_->openSection( attr+1 );
		if (pChildSection)
		{
			// hmmm... or we could just call PyObject_Str on it and
			//  set that into the section... :)
			Vector2 v2;
			Vector3 v3;
			Vector4 v4;
			if (ScriptString strValue = ScriptString::create( value ))
			{
				pChildSection->setString( strValue.c_str() );
			}
			else if (PyUnicode_Check( value.get() ))
			{
				// TODO: Create ScriptUnicode
				wchar_t buf[256];
				Py_ssize_t nChars = PyUnicode_AsWideChar( 
					(PyUnicodeObject*)value.get(), buf, 255 );
				if ( nChars != -1 )
				{
					buf[nChars] = L'\0';
					pChildSection->setWideString( buf );
				}
			}
			else if (ScriptFloat floatVal = ScriptFloat::create( value ))
			{
				pChildSection->setFloat( float( floatVal.asDouble() ) );
			}
			else if (ScriptInt intVal = ScriptInt::create( value ))
			{
				pChildSection->setInt( intVal.asLong() );
			}
			else if (harmlessSetData( value.get(), v2 ))
			{
				pChildSection->setVector2( v2 );
			}
			else if (harmlessSetData( value.get(), v3 ))
			{
				pChildSection->setVector3( v3 );
			}
			else if (harmlessSetData( value.get(), v4 ))
			{
				pChildSection->setVector4( v4 );
			}
			else
			{
				PyErr_Format( PyExc_TypeError, "PyDataSection.%s "
					"must be set to a string, float, int, "
					"vector2 or vector3 or vector4", attr );
				return false;
			}

			return true;
		}
	}

	return PyObjectPlus::pySetAttribute( attrObj, value );
}


// -----------------------------------------------------------------------------
// Section: Mapping methods
// -----------------------------------------------------------------------------

/**
 *	This method finds the data section at the input path.
 *
 *	@param	pathArg	The path to the desired section.
 *
 *	@return	The data section associated with input path.
 */
PyObject * PyDataSection::subscript( PyObject* pathArg )
{
	char * path = PyString_AsString( pathArg );

	if(PyErr_Occurred())
	{
		return NULL;
	}

	DataSectionPtr pChildSection = pSection_->openSection( path );

	if (!pChildSection)
	{
		Py_RETURN_NONE;
	}

	return new PyDataSection( pChildSection );
}


/**
 *	This method returns the number of child data sections.
 */
Py_ssize_t PyDataSection::length()
{
	return pSection_->countChildren();
}


/*~ function DataSection.has_key
 *	@components{ all }
 *
 *	This method checks if one of the attributes or tags within this data
 *	section has the specified name, and returns 1 if it does, 0 otherwise.
 *
 *	@param	key		A string containing the name of the section to check for.
 *
 *	@return			An integer: 1 if the section exists, 0 otherwise.
 */
/**
 *	This method returns true if the given section exists.
 *
 *	@param args		A python tuple containing the arguments.
 */
PyObject* PyDataSection::py_has_key( PyObject* args )
{
	char * sectionName;

	if (!PyArg_ParseTuple( args, "s", &sectionName ))
	{
		PyErr_SetString( PyExc_TypeError, "Expected a string argument." );
		return NULL;
	}

	if (pSection_->openSection( sectionName ))
	{
		return PyInt_FromLong(1);
	}
	else
	{
		return PyInt_FromLong(0);
	}
}


/*~ function DataSection.keys
 *	@components{ all }
 *
 *	This method returns a list of all the attributes and tags names, in
 *	the order in which they appear in the xml code.
 *
 *	@return		The list of the names (strings)
 */
/**
 *	This method returns a list of all the child section names.
 */
PyObject* PyDataSection::py_keys( PyObject* /*args*/ )
{
	int size = pSection_->countChildren();

	PyObject * pList = PyList_New( size );

	for (int i = 0; i < size; i++)
	{
		PyList_SetItem( pList, i,
			PyString_FromString( pSection_->childSectionName( i ).c_str() ) );
	}

	return pList;
}


/*~ function DataSection.values
 *	@components{ all }
 *
 *	This method returns a list of all child sections, in the order in which they
 *	appear in the XML.
 *
 *	@return		a list of DataSections
 */
/**
 *	This method returns a list of all child sections.
 */
PyObject* PyDataSection::py_values(PyObject* /*args*/)
{
	int size = pSection_->countChildren();
	PyObject* pList = PyList_New( size );

	DataSection::iterator iter = pSection_->begin();
	int i = 0;

	while (iter != pSection_->end())
	{
		PyList_SetItem( pList, i, new PyDataSection( *iter ) );

		i++;
		++iter;
	}

	return pList;
}


/*~ function DataSection.items
 *	@components{ all }
 *	This method returns a list of name and section tuples of this section's
 *	children, in the order in which they appear in the XML file
 *
 *	@return		The list of (name, DataSection) tuples of this section's
 *				children.
 */
/**
 *	This method returns a list of name and section tuples of this section's
 *	children.
 */
PyObject* PyDataSection::py_items( PyObject* /*args*/ )
{
	int size = pSection_->countChildren();
	PyObject* pList = PyList_New( size );

	DataSection::iterator iter = pSection_->begin();
	int i = 0;

	while (iter != pSection_->end())
	{
		DataSectionPtr pChild = *iter;

		PyObject * pTuple = PyTuple_New( 2 );

		PyTuple_SetItem( pTuple, 0,
			PyString_FromString( pChild->sectionName().c_str() ) );
		PyTuple_SetItem( pTuple, 1, new PyDataSection( pChild ) );

		PyList_SetItem( pList, i, pTuple );

		i++;
		++iter;
	}

	return pList;
}


// -----------------------------------------------------------------------------
// Section: Script methods
// -----------------------------------------------------------------------------

/**
 *	Binary data accessor for Python
 */
BW::string PyDataSection::asBinary() const
{
	BinaryPtr pBinary = pSection_->asBinary();
	if (!pBinary) return BW::string();
	return BW::string( (const char*)pBinary->data(), pBinary->len() );
}

/**
 *	Binary data accessor for Python
 */
void PyDataSection::asBinary( const BW::string & v ) const
{
	BinaryPtr pBinary = new BinaryBlock( v.data(), v.length(), "BinaryBlock/PyDataSection" );
	pSection_->setBinary( pBinary );
}


/*
 *	This is a simple helper function used by the read functions.
 */
inline char * getStringArg( PyObject * args )
{
	if (PyTuple_Size( args ) == 1 &&
		PyString_Check( PyTuple_GetItem( args, 0 ) ))
	{
		return PyString_AsString( PyTuple_GetItem( args, 0 ) );
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "Expected one string argument." );
		return NULL;
	}
}

#define IMPLEMENT_READ_VALUE_WITH_METHOD( METHOD, TYPE, M2 )						\
PyObject * PyDataSection::py_##METHOD( PyObject * args )						\
{																				\
	char * path = NULL;															\
	PyObject * pDef = NULL;														\
	TYPE defVal = TYPE();														\
	if (!PyArg_ParseTuple( args, "s|O", &path, &pDef ) ||						\
		(pDef != NULL && Script::setData( pDef, defVal ) != 0))					\
	{																			\
		PyErr_SetString( PyExc_TypeError,										\
			"Expected a string and an optional " #TYPE );						\
		return NULL;															\
	}																			\
																				\
	if (pDef != NULL)															\
		return Script::getData( pSection_->M2( path, defVal ) );		\
	else																		\
		return Script::getData( pSection_->M2( path ) );				\
}

#define IMPLEMENT_READ_VALUE( METHOD, TYPE )									\
	IMPLEMENT_READ_VALUE_WITH_METHOD( METHOD, TYPE, read<TYPE> )

#define IMPLEMENT_READ_VALUES( METHOD, TYPE )									\
PyObject * PyDataSection::py_##METHOD( PyObject * args )						\
{																				\
	char * path = getStringArg( args );											\
																				\
	if (path == NULL)															\
	{																			\
		return NULL;															\
	}																			\
																				\
	BW::vector< TYPE > values;													\
																				\
	pSection_->METHOD( path, values );											\
																				\
	PyObject * pTuple = PyTuple_New( values.size() );							\
																				\
	for (uint i = 0; i < values.size(); i++)									\
	{																			\
		PyTuple_SetItem( pTuple, i, Script::getData( values[i] ) );				\
	}																			\
																				\
	return pTuple;																\
}

#define IMPLEMENT_WRITE_VALUE_WITH_METHOD( METHOD, TYPE, M2 )					\
PyObject * PyDataSection::py_##METHOD( PyObject * args )						\
{																				\
	char * path;																\
	PyObject * pValueObject;													\
	TYPE value;																	\
																				\
	if (!PyArg_ParseTuple( args, "sO", &path, &pValueObject ) ||				\
		Script::setData( pValueObject, value ) != 0)							\
	{																			\
		PyErr_SetString( PyExc_TypeError, "Expected string and " #TYPE "." );	\
		return NULL;															\
	}																			\
																				\
	if (!pSection_->M2( path, value ))											\
	{																			\
		PyErr_SetString( PyExc_TypeError, "Write failed." );					\
		return NULL;															\
	}																			\
																				\
	return this->subscript( PyTuple_GetItem( args, 0 ) );						\
}

#define IMPLEMENT_WRITE_VALUE( METHOD, TYPE )									\
IMPLEMENT_WRITE_VALUE_WITH_METHOD( METHOD, TYPE, write<TYPE> )

#define IMPLEMENT_WRITE_VALUES( METHOD, TYPE )									\
PyObject * PyDataSection::py_##METHOD( PyObject * args )						\
{																				\
	char * path;																\
	PyObject * pSeq;															\
																				\
	if (!PyArg_ParseTuple( args, "sO", &path, &pSeq ) ||						\
		!PySequence_Check( pSeq ))												\
	{																			\
		PyErr_SetString( PyExc_TypeError, "Expected string and a sequence." );	\
		return NULL;															\
	}																			\
																				\
	Py_ssize_t size = PySequence_Size( pSeq );									\
	BW::vector< TYPE > values( size );											\
																				\
	for (Py_ssize_t i = 0; i < size; i++)										\
	{																			\
		PyObject * pySeqItem = PySequence_GetItem( pSeq, i );					\
		int res = Script::setData( pySeqItem, values[i] );						\
		Py_DECREF( pySeqItem );													\
		if (res != 0)															\
		{																		\
			PyErr_Format( args, "Section %ld is not of of type " #TYPE ".", i );\
			return NULL;														\
		}																		\
	}																			\
																				\
	pSection_->METHOD( path, values );											\
																				\
	Py_RETURN_NONE;																\
}																				\


/*~ function DataSection.readString
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified,
 *	interpreted as a string. For an tag, leading and trailing white space is
 *	trimmed.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the string from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the string contained in the specified section,
 *							or the default value (if specified),
 *							an empty string otherwise.
 */
IMPLEMENT_READ_VALUE( readString, BW::string )
/*~ function DataSection.readWideString
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified,
 *	assuming that the contents are a wide string. For an tag, leading and
 *	trailing white space is trimmed.
 *
 *	Most wide strings in text xml files are encoded as base64.  However, you
 *	can create readable wide strings in xml by hand, simply by prefixing a
 *	standard ascii string with an exclamation mark.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the wide string from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the string contained in the specified section,
 *							or the default value (if specified),
 *							an empty string otherwise.
 */
IMPLEMENT_READ_VALUE( readWideString, BW::wstring )
/*~ function DataSection.readInt
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified
 *	parsed as an integer.
 *
 *	For an tag, leading and trailing white space is trimmed. It tries to parse
 *	an integer at the front of the resulting string, returning the integer if
 *	the parse succeeds, 0 otherwise.
 *
 *	If the sectionName specified doesn't match any sections, then 0 is returned.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the int from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the int contained in the specified section,
 *							or the default value (if specified),
 *							0 otherwise.
 */
IMPLEMENT_READ_VALUE( readInt, int )
/*~ function DataSection.readInt64
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified
 *	parsed as an integer.
 *
 *	For an tag, leading and trailing white space is trimmed. It tries to parse
 *	an integer at the front of the resulting string, returning the integer if
 *	the parse succeeds, 0 otherwise.
 *
 *	If the sectionName specified doesn't match any sections, then 0 is returned.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the int from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the int contained in the specified section,
 *							or the default value (if specified),
 *							0 otherwise.
 */
IMPLEMENT_READ_VALUE( readInt64, int64 )
/*~ function DataSection.readFloat
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified
 *	parsed as a floating point number.
 *
 *	For an tag, leading and trailing white space is trimmed. It tries to parse
 *	an float at the front of the resulting string, returning the integer if the
 *	parse succeeds, 0 otherwise.
 *
 *	If the sectionName specified doesn't match any sections, then 0 is returned.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the float from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the float contained in the specified section,
 *							or the default value (if specified),
 *							0 otherwise.
 */
IMPLEMENT_READ_VALUE( readFloat, float )
/*~ function DataSection.readVector2
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified
 *	parsed as a Vector2.
 *
 *	It expects two floating point numbers, separated by white space. If it gets
 *	this, it returns the two floats as a Vector2, otherwise it returns (0, 0).
 *
 *	For an tag, leading and trailing white space is trimmed.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the Vector2 from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the Vector2 contained in the specified section,
 *							or the default value (if specified),
 *							(0, 0) otherwise.
 *
 */
IMPLEMENT_READ_VALUE( readVector2, Vector2 )
/*~ function DataSection.readVector3
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified
 *	parsed as a Vector3.
 *
 *	It expects three floating point numbers, separated by white space. If it
 *	gets this, it returns the three floats as a Vector3, otherwise it returns
 *	(0, 0, 0).
 *
 *	For an tag, leading and trailing white space is trimmed.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the Vector3 from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the Vector3 contained in the specified section,
 *							or the default value (if specified),
 *							(0, 0, 0) otherwise.
 *
 */
IMPLEMENT_READ_VALUE( readVector3, Vector3 )
/*~ function DataSection.readVector4
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified
 *	parsed as a Vector4.
 *
 *	It expects four floating point numbers, separated by white space. If it gets
 *	this, it returns the four floats as a Vector4, otherwise it returns
 *	(0, 0, 0, 0).
 *
 *	For an tag, leading and trailing white space is trimmed.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the Vector4 from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the Vector4 contained in the specified section,
 *							or the default value (if specified),
 *							(0, 0, 0, 0) otherwise.
 *
 */
IMPLEMENT_READ_VALUE( readVector4, Vector4 )
/*~ function DataSection.readMatrix
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified
 *	parsed as a Matrix.
 *
 *	It expects there there to be four attributes or tags, called
 *	row0, row1, row2 and row3. Each of these should have three floats separated
 *	by white space. If these conditions are met, a Matrix will be read in
 *	from the four rows otherwise, a Matrix with all zeros will be created.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the Matrix from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the Matrix contained in the specified section,
 *							or the default value (if specified), a
 *							Null Matrix otherwise.
 */
IMPLEMENT_READ_VALUE( readMatrix, Matrix )
/*~ function DataSection.readBool
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified,
 *	parsed as a boolean.
 *
 *	If it contains the string "true", with any capitalisation, then it returns
 *	1, otherwise, it returns 0.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName	the name of the section to get the boolean from.
 *	@param	defaultVal	the default value should an error occur.
 *
 *	@return				the boolean (integer) contained in the specified
 *						section, or the default value (if specified),
 *						false otherwise.
 */
IMPLEMENT_READ_VALUE( readBool, bool )

/*~ function DataSection.readStrings
 *	@components{ all }
 *
 *	This function reads the contents of any subsections of the current section 
 *	with the specified name, and returns the results as tuple of strings.
 *
 *	@param	sectionName	the name of the subsections from which strings will be
 *			extracted.
 *
 *	@return				a tuple of the strings from the named subsections.
 */
IMPLEMENT_READ_VALUES( readStrings, BW::string )
/*~ function DataSection.readWideStrings
 *	@components{ all }
 *
 *	This function reads the contents of any subsections of the current section
 *	with the specified name, and returns the results as tuple of wide strings.
 *
 *	Most wide strings in text xml files are encoded as base64.  However, you
 *	can create readable wide strings in xml by hand, simply by prefixing a
 *	standard ascii string with an exclamation mark.
 *
 *	@param	sectionName	the name of the subsections from which wide strings
 *			will be extracted.
 *
 *	@return				a tuple of the wide strings from the named
 *						subsections.
 */
IMPLEMENT_READ_VALUES( readWideStrings, BW::wstring )
/*~ function DataSection.readInts
 *	@components{ all }
 *
 *	This function reads the contents of any subsections of the current section
 *	with the specified name, and returns the results as tuple of integers.  It 
 *	tries to parse the contents of each as an integer, if unsuccessful the value
 *	of 0 will be used.
 *
 *	@param	sectionName	the name of the subsections from which integers will be
 *						parsed.
 *
 *	@return				a tuple of the integers from the named subsections.
 */
IMPLEMENT_READ_VALUES( readInts, int )
/*~ function DataSection.readFloats
 *	@components{ all }
 *
 *	This function reads the contents of any subsections of the current section
 *	with the specified name, and returns the results as tuple of floats.  It 
 *	tries to parse the contents of each as an float, if unsuccessful the value
 *	of 0 will be used.
 *
 *	@param	sectionName	the name of the subsections from which floats will be
 *			parsed.
 *
 *	@return				a tuple of the floats from the named subsections.
 */
IMPLEMENT_READ_VALUES( readFloats, float )
/*~ function DataSection.readVector2s
 *	@components{ all }
 *
 *	This function reads the contents of any subsections of the current section
 *	with the specified name, and returns the results as tuple of Vector2s.  It
 *	tries to parse the contents of each as two floats, if unsuccessful the value
 *	of (0, 0) will be used.
 *
 *	@param	sectionName	the name of the subsections from which Vector2s will be
 *						parsed.
 *
 *	@return				a tuple of the Vector2s from the named subsections.
 */
IMPLEMENT_READ_VALUES( readVector2s, Vector2 )
/*~ function DataSection.readVector3s
 *	@components{ all }
 *
 *	This function reads the contents of any subsections of the current section
 *	with the specified name, and returns the results as tuple of Vector3s.  It
 *	tries to parse the contents of each as three floats, if unsuccessful the
 *	value of (0, 0, 0) will be used.
 *
 *	@param	sectionName	the name of the subsections from which Vector3s will be
 *						parsed.
 *
 *	@return				a tuple of the Vector3s from the named subsections.
 */
IMPLEMENT_READ_VALUES( readVector3s, Vector3 )
/*~ function DataSection.readVector4s
 *	@components{ all }
 *
 *	This function reads the contents of any subsections of the current section
 *	with the specified name, and returns the results as tuple of Vector4s.  It
 *	tries to parse the contents of each as four floats, if unsuccessful the
 *	value of (0, 0, 0, 0) will be used.
 *
 *	@param	sectionName	the name of the subsections from which Vector4s will be
 *						parsed.
 *
 *	@return				a tuple of the Vector4s from the named subsections.
 */
IMPLEMENT_READ_VALUES( readVector4s, Vector4 )
/*~ function DataSection.readBlob
 *	@components{ all }
 *
 *	This function returns the contents of the section with the name specified,
 *	interpreted as a Blob string.  If this section is an tag it performs a
 *	trim (drop off leading and trailing white space) on the text immediately
 *	following the opening tag, and preceding the first open tag for a tag.
 *
 *	If there is more than one section with the specified name, then only the
 *	first one is used.
 *
 *	@param	sectionName		the name of the section to get the string from.
 *	@param	defaultVal		the default value should an error occur.
 *
 *	@return					the string contained in the specified section,
 *							or the default value (if specified),
 *							an empty string otherwise.
 */
IMPLEMENT_READ_VALUE_WITH_METHOD( readBlob, BW::string, readBlob )

/*~ function DataSection.writeString
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified string.  If one or more subsections with that name already exist,
 *	then the value of the first one is replaced with the specified string.
 *	This will not effect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	text		the text to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeString, BW::string )
/*~ function DataSection.writeWideString
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified wide string.  If one or more subsections with that name already
 *	exist, then the value of the first one is replaced with the specified wide
 *	string.  This will not effect any subsections of the named subsection, only
 *	its actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	text		the text to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeWideString, BW::wstring )
/*~ function DataSection.writeInt
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified integer as a string.  If one or more subsections with that name
 *	already exist, then the value of the first one is replaced with the
 *	specified integer.  This will not effect any subsections of the named
 *	subsection, only its actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the integer to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeInt, int )
/*~ function DataSection.writeInt64
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified 64-bit integer. If one or more subsections with that name already
 *	exist, then the value of the first one is replaced with the specified value.
 *	This will not affect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the integer to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeInt64, int64 )

/*~ function DataSection.writeFloat
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified float as a string.  If one or more subsections with that name
 *	already exist, then the value of the first one is replaced with the
 *	specified float.  This will not effect any subsections of the named
 *	subsection, only its actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the float to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeFloat, float )
/*~ function DataSection.writeVector2
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified Vector2 as a string, consisting of two floats, separated by white
 *	space.  If one or more subsections with that name already exist, then the
 *	value of the first one is replaced with the specified Vector2.
 *	This will not effect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the Vector2 to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeVector2, Vector2 )
/*~ function DataSection.writeVector3
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified Vector3 as a string, consisting of three floats, separated by
 *	white space.  If one or more subsections with that name already exist,
 *	then the value of the first one is replaced with the specified Vector3.
 *	This will not effect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the Vector3 to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeVector3, Vector3 )
/*~ function DataSection.writeVector4
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified Vector4 as a string, consisting of four floats, separated by
 *	white space.  If one or more subsections with that name already exist,
 *	then the value of the first one is replaced with the specified Vector4.
 *	This will not effect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the Vector4 to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeVector4, Vector4 )
/*~ function DataSection.writeMatrix
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified Matrix. If one or more subsections with that name already exist,
 *	then the value of the first one is replaced with the specified value.
 *	This will not effect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the Matrix to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeMatrix, Matrix )
/*~ function DataSection.writeBool
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified integer as a string.  If the integer is zero, 'false' is written,
 *	otherwise 'true'. If one or more subsections with that name already exist,
 *	then the value of the first one is replaced with the specified boolean.
 *	This will not effect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	val			the integer to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE( writeBool, bool )

/*~ function DataSection.writeStrings
 *	@components{ all }
 *
 *	This function creates a subsection of the specified name for each element
 *	of the supplied list, writing the corresponding string from the list as the
 *	contents of that subsection.
 *
 *	For example, the following statement:
 *
 *	@{
 *	ds.writeStrings( "sect", ( "str1", "str2" ) )
 *	@}
 *
 *	would result in the generation of the following xml:
 *
 *	@{
 *		&lt;sect> str1 &lt;/sect>
 *		&lt;sect> str2 &lt;/sect>
 *	@}

 *	This function has no effect on existing subsections of the same name.  A
 *	new subsection is always created for each list element.
 *
 *	@param	sectionName	the name of the subsection to create.
 *	@param	values		the list of strings to use as values for the new
 *						subsections.
 */
IMPLEMENT_WRITE_VALUES( writeStrings, BW::string )
/*~ function DataSection.writeWideStrings
 *	@components{ all }
 *
 *	This function creates a subsection of the specified name for each element
 *	of the supplied list, writing the corresponding wide string from the list
 *	as the contents of that subsection.
 *
 *	This function has no effect on existing subsections of the same name.  A
 *	new subsection is always created for each list element.
 *
 *	@param	sectionName	the name of the subsection to create.
 *	@param	values		the list of wide strings to use as values for the new
 *						subsections.
 */
IMPLEMENT_WRITE_VALUES( writeWideStrings, BW::wstring )
/*~ function DataSection.writeInts
 *	@components{ all }
 *
 *	This function creates a subsection of the specified name for each element
 *	of the supplied sequence, writing the corresponding integer from the
 *	sequence as the contents of that subsection.
 *
 *	For example, the following statement:
 *
 *	@{
 *	ds.writeInts( "sect", ( 123, 456 ) )
 *	@}
 *
 *	would result in the generation of the following xml
 *
 *	@{
 *		&lt;sect> 123 &lt;/sect>
 *		&lt;sect> 456 &lt;/sect>
 *	@}
 *
 *	This function has no effect on existing subsections of the same name.  A
 *	new subsection is always created for each element in the sequence.
 *
 *	@param	sectionName	the name of the subsection to create.
 *	@param	values		the sequence of integers to use as values for the new
 *						subsections.
 */
IMPLEMENT_WRITE_VALUES( writeInts, int )
/*~ function DataSection.writeFloats
 *	@components{ all }
 *
 *	This function creates a subsection of the specified name for each element
 *	of the supplied sequence, writing the corresponding float from the sequence
 *	as the contents of that subsection.
 *
 *	For example, the following statement:
 *
 *	@{
 *	ds.writeInts( "sect", ( 123.4, 456.7 ) )
 *	@}
 *
 *	would result in the generation of the following xml
 *
 *	@{
 *		&lt;sect> 123.4 &lt;/sect>
 *		&lt;sect> 456.7 &lt;/sect>
 *	@}
 *
 *	This function has no effect on existing subsections of the same name.  A
 *	new subsection is always created for each element in the sequence.
 *
 *	@param	sectionName	the name of the subsection to create.
 *	@param	values		the sequence of floats to use as values for the new
 *						subsections.
 */
IMPLEMENT_WRITE_VALUES( writeFloats, float )
/*~ function DataSection.writeVector2s
 *	@components{ all }
 *
 *	This function creates a subsection of the specified name for each element
 *	of the supplied sequence, writing the corresponding Vector2 from the
 *	sequence as the contents of that subsection.
 *
 *	For example, the following statement:
 *
 *	@{
 *	ds.writeVector2s( "sect", ( (1.1, 2.2) , (3.3, 4.4) ) )
 *	@}
 *
 *	would result in the generation of the following xml
 *
 *	@{
 *		&lt;sect> 1.100000 2.200000 &lt;/sect>
 *		&lt;sect> 3.300000 4.400000 &lt;/sect>
 *	@}
 *
 *	This function has no effect on existing subsections of the same name.  A
 *	new subsection is always created for each element in the sequence.
 *
 *	@param	sectionName	the name of the subsection to create.
 *	@param	values		the sequence of Vector2s to use as values for the new
 *						subsections.
 */
IMPLEMENT_WRITE_VALUES( writeVector2s, Vector2 )
/*~ function DataSection.writeVector3s
 *	@components{ all }
 *
 *	This function creates a subsection of the specified name for each element
 *	of the supplied sequence, writing the corresponding Vector3 from the
 *	sequence as the contents of that subsection.
 *
 *	For example, the following statement:
 *
 *	@{
 *	ds.writeVector3s( "sect", ( (1.1, 2.2, 3.3) , (3.3, 4.4, 5.5) ) )
 *	@}
 *
 *	would result in the generation of the following xml
 *
 *	@{
 *		&lt;sect> 1.100000 2.200000 3.300000 &lt;/sect>
 *		&lt;sect> 3.300000 4.400000 5.500000 &lt;/sect>
 *	@}
 *
 *	This function has no effect on existing subsections of the same name.  A
 *	new subsection is always created for each element in the sequence.
 *
 *	@param	sectionName	The name of the subsection to create.
 *	@param	values		The sequence of Vector3s to use as values for the new
 *						subsections.
 */
IMPLEMENT_WRITE_VALUES( writeVector3s, Vector3 )
/*~ function DataSection.writeVector4s
 *	@components{ all }
 *
 *	This function creates a subsection of the specified name for each element
 *	of the supplied sequence, writing the corresponding Vector4 from the
 *	sequence as the contents of that subsection.
 *
 *	For example, the following statement:
 *
 *	@{
 *	ds.writeVector3s( "sect", ( (1.1, 2.2, 3.3, 4.4) , (3.3, 4.4, 5.5, 6.6) ) )
 *	@}
 *
 *	would result in the generation of the following xml
 *
 *	@{
 *		&lt;sect> 1.100000 2.200000 3.300000 4.400000 &lt;/sect>
 *		&lt;sect> 3.300000 4.400000 5.500000 6.600000 &lt;/sect>
 *	@}
 *
 *	This function has no effect on existing subsections of the same name.  A
 *	new subsection is always created for each element in the sequence.
 *
 *	@param	sectionName	The name of the subsection to create.
 *	@param	values		The sequence of Vector4s to use as values for the new
 *						subsections.
 */
IMPLEMENT_WRITE_VALUES( writeVector4s, Vector4 )

/*~ function DataSection.writeBlob
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name, writing the
 *	specified BLOB string.  If one or more subsections with that name already exist,
 *	then the value of the first one is replaced with the specified string.
 *	This will not effect any subsections of the named subsection, only its
 *	actual contents.
 *
 *	@param	sectionName	the name of the subsection to create or overwrite.
 *	@param	text		the text to replace the subsection contents with.
 *
 *	@return				the DataSection that was written to.
 */
IMPLEMENT_WRITE_VALUE_WITH_METHOD( writeBlob, BW::string, writeBlob )

/*~ function DataSection.write
 *	@components{ all }
 *
 *	This function writes a subsection with the specified name.  It creates the
 *	section if it does not already exist.  If one or more sections of that name
 *	already exist, then it modifies the first one.  It writes whatever value
 *	it was given as its second argument as the contents for the named section.
 *
 *	@param	sectionName	The name of the subsection to write to.
 *	@param	value		The value to write to the named section.
 *
 *	@return				The DataSection that was written to.
 */
 /**
 *	This method writes any Python type to this section.
 */
PyObject * PyDataSection::py_write( PyObject * args )
{
	char * path;
	PyObject * pValueObject;

	if (!PyArg_ParseTuple( args, "sO", &path, &pValueObject ))
	{
		PyErr_SetString( PyExc_TypeError, "Expected string and an object." );

		return NULL;
	}

	if (PyInt_Check( pValueObject ))
	{
		return this->py_writeInt( args );
	}
	else if (PyFloat_Check( pValueObject ))
	{
		return this->py_writeFloat( args );
	}
	else if (PyString_Check( pValueObject ))
	{
		return this->py_writeString( args );
	}
	else if (PyUnicode_Check( pValueObject ))
	{
		return this->py_writeWideString( args );
	}
	else if (PyLong_Check( pValueObject ))
	{
		return this->py_writeInt64( args );
	}
	else if (PySequence_Check( pValueObject ))
	{
		const Py_ssize_t size = PySequence_Size( pValueObject );

		if (size == 3)
		{
			return this->py_writeVector3( args );
		}
		else if (size == 2)
		{
			return this->py_writeVector2( args );
		}
		else if (size == 4)
		{
			return this->py_writeVector4( args );
		}
	}
	else if (PyVector<Vector2>::Check( pValueObject ))
	{
		return this->py_writeVector2( args );
	}
	else if (PyVector<Vector3>::Check( pValueObject ))
	{
		return this->py_writeVector3( args );
	}
	else if (PyVector<Vector4>::Check( pValueObject ))
	{
		return this->py_writeVector4( args );
	}
	else if (MatrixProvider::Check( pValueObject ))
	{
		return this->py_writeMatrix( args );
	}

	PyErr_Format( PyExc_TypeError, "PyDataSection.write: "
		"Unrecognised object type %s", pValueObject->ob_type->tp_name );

	return NULL;
}


/*~ function DataSection.cleanName
 *	@components{ all }
 *
 *	This method cleans xml section names by replacing spaces with double
 *	periods and puts "id." in front of xml section names that begin with
 *	numbers.
 *
 *	@return			A boolean indicating that a change took place in the name.
 */
PyObject * PyDataSection::py_cleanName( PyObject * args )
{
	bool changed = false;
	if (pSection_)
		changed = pSection_->sanitiseSectionName();
	return PyBool_FromLong(changed ? 1 : 0);
}


/*~ function DataSection.isNameClean
 *	@components{ all }
 *
 *	This method returns whether the section name is valid.  XML can be a little
 *	more strict about allowed names.
 *
 *	@return			A boolean indicating whether the name is clean.
 */
PyObject * PyDataSection::py_isNameClean( PyObject * args )
{
	bool ok = true;
	if (pSection_)
		ok = pSection_->isValidSectionName();
	return PyBool_FromLong(ok ? 1 : 0);
}


/*~ function DataSection.createSection
 *	@components{ all }
 *
 *	This method creates the section specified by the input path, as a
 *	subsection of the parent. It will always create a new subsection,
 *	even if a section of the same name already exists.  The path can contain
 *	slashes, which will allow for the creation of subsection within subsection.
 *	The path will be evaluated through the first section that has that name and
 *	if the whole path already exists, only the innermost section will be
 *	duplicated.
 *
 *	For example, if ds represents the following piece of xml:
 *
 *	@{
 *		&lt;red>
 *			&lt;green blue="" />
 *		&lt;/red>
 *		&lt;red>
 *			&lt;green blue="" />
 *		&lt;/red>
 *	@}
 *
 *	and then the following code is executed:
 *
 *	@{
 *	ds.createSection( "red/green/blue" )
 *	@}
 *
 *	then the following will be the resultant xml represented by ds:
 *
 *	@{
 *		&lt;red>
 *			&lt;green>
 *				&lt;blue />
 *				&lt;blue />
 *			&lt;/green>
 *		&lt;/red>
 *		&lt;red>
 *			&lt;green blue="" />
 *		&lt;/red>
 *	@}
 *
 *	@param	sectionPath		The path to the section to create.
 *
 *	@return					The DataSection that was created.
 */
/**
 *	This method creates the section at the input path.
 */
PyObject * PyDataSection::py_createSection( PyObject * args )
{
	const char * path = getStringArg( args );

	if (path == NULL)
	{
		return NULL;
	}

	BW::string sectionBase = BWResource::getFilePath( path );
	// Strip the trailing / off the directory name
	// (this gets added even if the path has no dir blank)
	sectionBase = sectionBase.substr( 0, sectionBase.length() - 1 );
	BW::StringRef sectionName = BWResource::getFilename( path );

	// We do this, so that names such as foldername/filename.xml can be passed in
	DataSectionPtr pBaseSection = pSection_;
	if ( !sectionBase.empty() )
	{
		pBaseSection = pSection_->openSection( sectionBase, true );
		if (!pBaseSection)
			Py_RETURN_NONE;
	}

	DataSectionPtr pSection = pBaseSection->newSection( sectionName );
	if (pSection)
	{
		return new PyDataSection( pSection );
	}
	else
	{
		Py_RETURN_NONE;
	}
}


/*~ function DataSection.createSectionFromString
 *	@components{ all }
 *
 *	This method creates the section represented by the input string, as a
 *	subsection of the parent.
 *
 *	For example, if ds represents the following piece of xml:
 *
 *	@{
 *		&lt;red>
 *			&lt;green blue="" />
 *		&lt;/red>
 *	@}
 *
 *	and then the following code is executed:
 *
 *	@{
 *	string = "&lt;red>&lt;green blue="" />&lt;/red>"
 *	ds.createSectionFromString( string )
 *	@}
 *
 *	then the following will be the resultant xml represented by ds:
 *
 *	@{
 *		&lt;red>
 *			&lt;green blue="" />
 *		&lt;/red>
 *		&lt;red>
 *			&lt;green blue="" />
 *		&lt;/red>
 *	@}
 *
 *	@param	string			the xml string to create the section from.
 *
 *	@return					the DataSection that was created.
 */
/**
 *	This method creates the section from a xml formatted string.
 */
PyObject * PyDataSection::py_createSectionFromString( PyObject * args )
{
	char * string = getStringArg( args );

	if (string == NULL)
	{
		return NULL;
	}

	BW::stringstream sstream;
	sstream << string;

	DataSectionPtr newSection =
		XMLSection::createFromStream( "", sstream ).get();

	if (newSection)
	{
		DataSectionPtr copiedSection = pSection_->newSection(
											newSection->sectionName() );
		copiedSection->copy( newSection );
		return new PyDataSection( newSection );
	}
	else
	{
		Py_RETURN_NONE;
	}
}


/*~ function DataSection.deleteSection
 *	@components{ all }
 *
 *	This method deletes a section referenced either by a PyDataSection or an
 *	input string path. In the case of input string path, if more than one
 *	section is addressed by the path, then only the first one is deleted.
 *	It returns 1 if the section was successfully deleted, 0 otherwise.
 *
 *	@param	section		Can be either a PyDataSection or a string path of the
 *						section to delete
 *
 *	@return					1 if it was deleted, 0 otherwise.
 */
/**
 *	This method deletes a section referenced either by a PyDataSection or an
 *	input string path.
 */
PyObject * PyDataSection::py_deleteSection( PyObject * args )
{
	char * path( NULL );
	PyDataSection * pDeleteSection( NULL );

	if (PyTuple_Size( args ) != 1)
	{
		PyErr_SetString( PyExc_TypeError, "py_deleteSection: Expected a DataSection or String argument." );
		return NULL;
	}

	PyObject * pItem = PyTuple_GetItem( args, 0 );
	if (PyString_Check( pItem ))
	{
		path = PyString_AsString( pItem );
	}
	else if (PyDataSection::Check( pItem ))
	{
		pDeleteSection =  (PyDataSection*)pItem;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "py_deleteSection: Expected a DataSection	or String argument." );
			return NULL;
	}


	bool result( false );

	if (path)
	{
		result = pSection_->deleteSection( path );
	}
	else if (pDeleteSection)
	{
		pSection_->delChild( pDeleteSection->pSection() );
		result = true;
	}

	return Script::getData( result );
}


/*~ function DataSection.save
 *	@components{ all }
 *
 *	This method saves the data section back to its underlying file.  If called
 *	on a DataSection which does not correspond to a file, then it causes an
 *	IO error.
 */
/**
 *	This method implements a script function. It saves this section at the input
 *	path.
 */
PyObject * PyDataSection::py_save( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "Expected no arguments." );
		return NULL;
	}

	if (pSection_->save( ) )
	{
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString( PyExc_IOError, "Save failed" );
		return NULL;
	}
}

/*~ function DataSection.copy
 *	@components{ all }
 *
 *	This function makes this DataSection into a copy of the specified
 *	DataSection.
 *
 *	@param	source	The section to copy this one from.
 */
PyObject * PyDataSection::py_copy( PyObject * args )
{
	PyObject * pPyDS;
	if (!PyArg_ParseTuple( args, "O", &pPyDS) ||
		!PyDataSection::Check(pPyDS))
	{
		PyErr_SetString( PyExc_TypeError, "Expected a PyDataSection.");
		return NULL;
	}

	pSection_->copy( static_cast<PyDataSection*>( pPyDS )->pSection_ );

	Py_RETURN_NONE;
}


/*~ function DataSection.copyToZip
 *	@components{ all }
 *
 *	This function makes takes this DataSection and copies it
 *	into a new DataSection as well as converting it to a zip
 * 	if possible.
 *
 *
 *	@return		A 2-tuple with the new data section and a boolean
 *				to indicate if the conversion was successful.
 */
PyObject * PyDataSection::py_copyToZip( PyObject * args )
{
	DataSectionPtr pSection = pSection_->convertToZip();
	PyObject * pTuple = PyTuple_New( 2 );
	PyTuple_SET_ITEM( pTuple, 0, new PyDataSection( pSection ) );
	if (pSection != pSection_)
		PyTuple_SET_ITEM( pTuple, 1, PyBool_FromLong( 1 ) );
	else
		PyTuple_SET_ITEM( pTuple, 1, PyBool_FromLong( 0 ) );
	return pTuple;
}


/*~ function DataSection.child
 *	@components{ all }
 *
 *	This method returns the child at the given index.
 *
 *	@param	index	The index to look up the child subsection at.
 *
 *	@return			The child at the specified index.
 */
/**
 *	This method implements a script function to return the child at the given
 *	index.
 */
PyObject * PyDataSection::child( int index )
{
	if (0 <= index && index < pSection_->countChildren())
	{
		DataSectionPtr pChild = pSection_->openChild( index );
		if (pChild) return new PyDataSection( pChild );

		const BW::string childName = pSection_->childSectionName( index );
		PyErr_Format( PyExc_EnvironmentError,
			"child index %d '%s' has disappeared",
			index, childName.c_str() );
		return NULL;
	}
	else
	{
		PyErr_SetString( PyExc_IndexError, "child index out of range" );
		return NULL;
	}
}


/*~ function DataSection.childName
 *	@components{ all }
 *
 *	This method returns the name of the child at the given index.
 *
 *	@param	index	The index to look up the child subsection at.
 *
 *	@return			The child at the specified index.
 */
/**
 *	This method implements a script function to return the name of the
 *	child at the given index.
 */
PyObject * PyDataSection::childName( int index )
{
	if (0 <= index && index < pSection_->countChildren())
	{
		return Script::getData( pSection_->childSectionName( index ) );
	}
	else
	{
		PyErr_SetString( PyExc_IndexError, "child index out of range" );
		return NULL;
	}
}


/**
 *	This method is the getter for the Python DataSection.isAttribute field
 */
PyObject * PyDataSection::pyGet_isAttribute()
{
	return Script::getData( pSection_->isAttribute() );
}


/**
 *	This method is the setter for the Python DataSection.isAttribute field
 */
int PyDataSection::pySet_isAttribute( PyObject * value )
{
	bool newValue;
	int retVal = Script::setData( value, newValue, "isAttribute" );
	if ( retVal != 0 )
	{
		return retVal;
	}

	if (pSection_->countChildren() != 0)
	{
		PyErr_Format( PyExc_AttributeError, "isAttribute can only be used on "
			"XML DataSections which have no children" );
		return -1;
	}

	if (!pSection_->isAttribute( newValue ))
	{
		PyErr_Format( PyExc_AttributeError, "Failed to set isAttribute" );
		return -1;
	}
	return 0;
}


BW_END_NAMESPACE

// py_data_section.cpp
