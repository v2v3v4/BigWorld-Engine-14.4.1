#ifndef RESOURCE_TABLE_HPP
#define RESOURCE_TABLE_HPP

#include "pyobject_plus.hpp"
#include "script.hpp"

#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class ResourceTable;
typedef SmartPointer<ResourceTable> ResourceTablePtr;
class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

/**
 *	This class is a table for scripts to use as a resource.
 */
class ResourceTable : public PyObjectPlus
{
	Py_Header( ResourceTable, PyObjectPlus )

public:
	ResourceTable( DataSectionPtr pSect, ResourceTablePtr pParent,
		PyTypeObject * pType = &s_type_ );
	~ResourceTable();

	static PyObject * New( const BW::string & resourceID,
		PyObjectPtr updateFn );
	PY_AUTO_FACTORY_DECLARE( ResourceTable,
		ARG( BW::string, OPTARG( PyObjectPtr, NULL, END ) ) )

	ScriptObject pyGetAttribute( const ScriptString & attrObj );

	bool link( PyObjectPtr updateFn, bool callNow = true );
	PY_AUTO_METHOD_DECLARE( RETOK, link, NZARG( PyObjectPtr, END ) )
	void unlink( PyObjectPtr updateFn );
	PY_AUTO_METHOD_DECLARE( RETVOID, unlink, NZARG( PyObjectPtr, END ) )

	PY_RO_ATTRIBUTE_DECLARE( pValue_, value )

	PY_SIZE_INQUIRY_METHOD( pyMap_length )
	PY_BINARY_FUNC_METHOD( pyMap_subscript )

	PyObject * at( int index );
	PY_AUTO_METHOD_DECLARE( RETOWN, at, ARG( int, END ) )
	PyObject * sub( const BW::string & key );
	PY_AUTO_METHOD_DECLARE( RETOWN, sub, ARG( BW::string, END ) )

	BW::string keyOfIndex( int index );
	PY_AUTO_METHOD_DECLARE( RETDATA, keyOfIndex, ARG( int, END ) )
	int indexOfKey( const BW::string & key );
	PY_AUTO_METHOD_DECLARE( RETDATA, indexOfKey, ARG( BW::string, END ) )

	PY_RO_ATTRIBUTE_DECLARE( index_, index );
	BW::string key();
	PY_RO_ATTRIBUTE_DECLARE( this->key(), key );


private:
	ResourceTable( const ResourceTable& );
	ResourceTable& operator=( const ResourceTable& );

	DataSectionPtr		pSect_;
	ResourceTablePtr	pParent_;
	uint				index_;

	PyObjectPtr			pValue_;

	BW::vector<ResourceTablePtr>	keyedEntries_;

	typedef BW::set<PyObjectPtr> LinkSet;
	LinkSet				links_;

	static PyMappingMethods s_map_methods_;

	friend struct RTSCompare;
};

PY_SCRIPT_CONVERTERS_DECLARE( ResourceTable )

BW_END_NAMESPACE

#endif // RESOURCE_TABLE_HPP
