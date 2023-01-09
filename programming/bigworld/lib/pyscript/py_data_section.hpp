#ifndef PY_DATA_SECTION_HPP
#define PY_DATA_SECTION_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to expose a data section.
 */
class PyDataSection : public PyObjectPlus
{
	Py_Header( PyDataSection, PyObjectPlus )

public:
	PyDataSection( DataSectionPtr pSection,
		PyTypeObject * pType = &PyDataSection::s_type_ );

	DataSectionPtr	pSection() const;

	ScriptObject	pyGetAttribute( const ScriptString & attrObj );
	bool 			pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );
	PyObject * 		subscript( PyObject * entityID );
	Py_ssize_t				length();

	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )

	PY_METHOD_DECLARE( py_readString )
	PY_METHOD_DECLARE( py_readWideString )
	PY_METHOD_DECLARE( py_readFloat )
	PY_METHOD_DECLARE( py_readInt )
	PY_METHOD_DECLARE( py_readInt64 )
	PY_METHOD_DECLARE( py_readVector2 )
	PY_METHOD_DECLARE( py_readVector3 )
	PY_METHOD_DECLARE( py_readVector4 )
	PY_METHOD_DECLARE( py_readMatrix )
	PY_METHOD_DECLARE( py_readBool )
	PY_METHOD_DECLARE( py_readBlob )

	PY_METHOD_DECLARE( py_writeString )
	PY_METHOD_DECLARE( py_writeWideString )
	PY_METHOD_DECLARE( py_writeFloat )
	PY_METHOD_DECLARE( py_writeInt )
	PY_METHOD_DECLARE( py_writeInt64 )
	PY_METHOD_DECLARE( py_writeVector2 )
	PY_METHOD_DECLARE( py_writeVector3 )
	PY_METHOD_DECLARE( py_writeVector4 )
	PY_METHOD_DECLARE( py_writeMatrix )
	PY_METHOD_DECLARE( py_writeBool )
	PY_METHOD_DECLARE( py_writeBlob )

	PY_METHOD_DECLARE( py_readStrings )
	PY_METHOD_DECLARE( py_readWideStrings )
	PY_METHOD_DECLARE( py_readFloats )
	PY_METHOD_DECLARE( py_readInts )
	PY_METHOD_DECLARE( py_readVector2s )
	PY_METHOD_DECLARE( py_readVector3s )
	PY_METHOD_DECLARE( py_readVector4s )

	PY_METHOD_DECLARE( py_writeStrings )
	PY_METHOD_DECLARE( py_writeWideStrings )
	PY_METHOD_DECLARE( py_writeFloats )
	PY_METHOD_DECLARE( py_writeInts )
	PY_METHOD_DECLARE( py_writeVector2s )
	PY_METHOD_DECLARE( py_writeVector3s )
	PY_METHOD_DECLARE( py_writeVector4s )

	PY_METHOD_DECLARE( py_write )

	PY_METHOD_DECLARE( py_cleanName )
	PY_METHOD_DECLARE( py_isNameClean )

	PY_METHOD_DECLARE( py_copyToZip )

	PY_METHOD_DECLARE( py_createSection )
	PY_METHOD_DECLARE( py_createSectionFromString )
	PY_METHOD_DECLARE( py_deleteSection )
	PY_METHOD_DECLARE( py_save )

	PY_METHOD_DECLARE( py_copy )

	PyObject * child( int index );
	PY_AUTO_METHOD_DECLARE( RETOWN, child, ARG( int, END ) )
	PyObject * childName( int index );
	PY_AUTO_METHOD_DECLARE( RETOWN, childName, ARG( int, END ) )

	PyObject * pyGet_isAttribute();
	int pySet_isAttribute( PyObject * value );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, asString, asString )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::wstring, asWideString, asWideString )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, asFloat, asFloat )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int, asInt, asInt )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( int64, asInt64, asInt64 )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector2, asVector2, asVector2 )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, asVector3, asVector3 )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector4, asVector4, asVector4 )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Matrix, asMatrix34, asMatrix )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, asBinary, asBinary )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, asBlob, asBlob )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, asBool, asBool )

	PY_RO_ATTRIBUTE_DECLARE( pSection_->sectionName(), name )

	static PyObject * 	s_subscript( PyObject * self, PyObject * entityID );
	static Py_ssize_t			s_length( PyObject * self );

	PY_FACTORY_DECLARE()

#define ORDINARY_ACCESSOR(NAME, TYPE)										\
	TYPE as##NAME() const			{ return pSection_->as<TYPE>(); }		\
	void as##NAME( const TYPE & v ) { pSection_->be<TYPE>( v ); }			\

	ORDINARY_ACCESSOR( String, BW::string )
	ORDINARY_ACCESSOR( WideString, BW::wstring )
	ORDINARY_ACCESSOR( Float, float )
	ORDINARY_ACCESSOR( Int, int )
	ORDINARY_ACCESSOR( Int64, int64 )
	ORDINARY_ACCESSOR( Vector2, Vector2 )
	ORDINARY_ACCESSOR( Vector3, Vector3 )
	ORDINARY_ACCESSOR( Vector4, Vector4 )
	ORDINARY_ACCESSOR( Matrix34, Matrix )
	ORDINARY_ACCESSOR( Bool, bool )

	BW::string asBlob() const	{ return pSection_->asBlob(); }
	void asBlob( const BW::string & v ) const { pSection_->setBlob( v ); }

	BW::string asBinary() const;
	void asBinary( const BW::string & v ) const;

private:
	DataSectionPtr pSection_;
};

typedef SmartPointer<PyDataSection> PyDataSectionPtr;

PY_SCRIPT_CONVERTERS_DECLARE( PyDataSection )

#ifdef CODE_INLINE
#include "py_data_section.ipp"
#endif

BW_END_NAMESPACE

#endif // PY_DATA_SECTION_HPP
