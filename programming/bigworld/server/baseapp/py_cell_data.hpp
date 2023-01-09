#ifndef PY_CELL_DATA_HPP
#define PY_CELL_DATA_HPP

#include "pyscript/pyobject_plus.hpp"
#include "entity_type.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to wrap cell data. It keeps the information as a blob and
 *	only creates a dictionary if necessary.
 */
class PyCellData : public PyObjectPlus
{
	Py_Header( PyCellData, PyObjectPlus )

public:
	PyCellData( EntityTypePtr pEntityType, BinaryIStream & data,
		bool persistentDataOnly, PyTypeObject * pType = &PyCellData::s_type_ );

	bool		addToStream( BinaryOStream & stream, bool addPosAndDir,
							bool addPersistentOnly );

	void		migrate( EntityTypePtr pType );

	PyObjectPtr	getDict();
	PY_AUTO_METHOD_DECLARE( RETDATA, getDict, END )

	PyObjectPtr createPyDictOnDemand();

	static bool addToStream( BinaryOStream & stream, EntityTypePtr pType,
			PyObject * pCellData, bool addPosAndDir, bool addPersistentOnly );
	
	static Vector3 getPos( PyObjectPtr pCellData );
	static Direction3D getDir( PyObjectPtr pCellData );
	static SpaceID getSpaceID( PyObjectPtr pCellData );
	static bool getOnGround( PyObjectPtr pCellData );
	static BW::string getTemplateID( PyObjectPtr pCellData );
	static void addSpatialData(  PyObjectPtr pCellData, const Vector3 & pos,
								 const Direction3D & dir, SpaceID spaceID );

private:
	static bool addMapToStream( BinaryOStream & stream,
								EntityTypePtr pEntityType, 
								const ScriptMapping & dict, 
								bool addPosAndDir,
								bool addPersistentOnly );
	
	static Vector3 getPosOrDir( PyObjectPtr pCellData,
									const char * pName, int offset );

	EntityTypePtr	pEntityType_;
	PyObjectPtr		pDict_;
	BW::string		data_;
	bool			dataHasPersistentOnly_;
};

BW_END_NAMESPACE

#endif // PY_CELL_DATA_HPP
