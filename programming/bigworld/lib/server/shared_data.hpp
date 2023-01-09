#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include "pyscript/pyobject_plus.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"


BW_BEGIN_NAMESPACE

typedef uint8 SharedDataType;
class BinaryOStream;
class Pickler;

/*~ class NoModule.SharedData
 *  @components{ base, cell }
 *  An instance of this class emulates a dictionary of data shared between
 *	server components.
 *
 *  Code Example:
 *  @{
 *  sharedData = BigWorld.globalData
 *  print "The main mission entity is", sharedData[ "MainMission" ]
 *  print "There are", len( sharedData ), "global data entries."
 *  @}
 */
/**
 *	This class is used to expose the collection of CellApp data.
 */
class SharedData : public PyObjectPlus
{
Py_Header( SharedData, PyObjectPlus )

public:
	typedef void (*SetFn)( const BW::string & key, const BW::string & value,
			SharedDataType dataType );
	typedef void (*DelFn)( const BW::string & key, SharedDataType dataType );

	typedef void (*OnSetFn)( PyObject * pKey, PyObject * pValue,
			SharedDataType dataType );
	typedef void (*OnDelFn)( PyObject * pKey, SharedDataType dataType );

	SharedData( SharedDataType dataType,
			SharedData::SetFn setFn,
			SharedData::DelFn delFn,
			SharedData::OnSetFn onSetFn,
			SharedData::OnDelFn onDelFn,
			Pickler * pPickler,
			PyTypeObject * pType = &SharedData::s_type_ );
	~SharedData();

	PyObject *			subscript( PyObject * key );
	int					ass_subscript( PyObject * key, PyObject * value );
	int					length();

	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )
	PY_METHOD_DECLARE( py_get )

	static PyObject *	s_subscript( PyObject * self, PyObject * key );
	static int			s_ass_subscript( PyObject * self,
							PyObject * key, PyObject * value );
	static Py_ssize_t	s_length( PyObject * self );

	bool setValue( const BW::string & key, const BW::string & value,
			bool isAck );
	bool delValue( const BW::string & key, bool isAck );

	bool addToStream( BinaryOStream & stream ) const;


private:
	BW::string pickle( ScriptObject pObj ) const;
	ScriptObject unpickle( const BW::string & str ) const;

	void changeOutstandingAcks( const BW::string & key, int delta );
	bool hasOutstandingAcks( const BW::string & key ) const;

	const char * sharedDataTypeAsString() const;

	PyObject * pMap_;
	SharedDataType dataType_;

	BW::map< BW::string, int > outstandingAcks_;

	SetFn	setFn_;
	DelFn	delFn_;
	OnSetFn	onSetFn_;
	OnDelFn	onDelFn_;

	Pickler * pPickler_;
};

BW_END_NAMESPACE

#endif // SHARED_DATA_HPP
