#ifndef PY_CELL_SPATIAL_DATA_HPP
#define	PY_CELL_SPATIAL_DATA_HPP

#include "pyscript/pyobject_plus.hpp"

#include "network/basictypes.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to wrap spatial cell data such as position and direction
 *	as well as templateID and spaceID. The class is used if base entity is 
 *	created from a template.
 */
class PyCellSpatialData : public PyObjectPlus
{
	Py_Header( PyCellSpatialData, PyObjectPlus )
public:
	PyCellSpatialData( const Position3D & position,
					   const Direction3D & direction,
					   SpaceID spaceID,
					   bool isOnGround,
					   const BW::string & templateID );
	
	const Position3D &	position()		const { return position_; }
	const Direction3D &	direction()		const { return direction_; }
	SpaceID				spaceID()		const { return spaceID_; }
	bool				isOnGround()	const { return isOnGround_; }
	BW::string			templateID()	const { return templateID_; }

	// dictionary emulation
	bool has_key( const BW::string & propName ) const;
	PY_AUTO_METHOD_DECLARE( RETOK, has_key, ARG( BW::string, END ) )
	
	PyObject * keys() const;
	PY_AUTO_METHOD_DECLARE( RETOWN, keys, END )
	
	PyObject * values() const;
	PY_AUTO_METHOD_DECLARE( RETOWN, values, END )

	PyObject * items() const;
	PY_AUTO_METHOD_DECLARE( RETOWN, items, END )

	PyObject * get( const BW::string & propName, 
					const ScriptObject & defaultValue ) const;
	PY_AUTO_METHOD_DECLARE( RETOWN, get, 
		ARG( BW::string, 
			OPTARG( ScriptObject, ScriptObject::none(), END ) ) )
	
	size_t size() const;
	
	static PyObject * 	s_subscript( PyObject * self, PyObject * key );
	static Py_ssize_t	s_length( PyObject * self );
	
private:
	ScriptObject get( const BW::string & propName ) const;

	Position3D	position_;
	Direction3D	direction_;
	SpaceID		spaceID_;
	bool		isOnGround_;
	BW::string	templateID_;
};

BW_END_NAMESPACE

#endif	/* PY_CELL_SPATIAL_DATA_HPP */

