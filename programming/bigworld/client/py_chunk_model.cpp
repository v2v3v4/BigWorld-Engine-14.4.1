#include "pch.hpp"
#include "py_chunk_model.hpp"

#include "chunk/chunk.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "resmgr/xml_section.hpp"
#include "pyscript/script_math.hpp"
#include "entity.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyChunkModel
// -----------------------------------------------------------------------------


// pyscript instantiations
PY_TYPEOBJECT( PyChunkModel )
PY_BEGIN_METHODS( PyChunkModel )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PyChunkModel )
PY_END_ATTRIBUTES()
PY_FACTORY( PyChunkModel, BigWorld )


/**
 *	Constructor
 */
PyChunkModel::PyChunkModel( ChunkModel * pModel, Chunk * pChunk,
		PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	pCM_( pModel )
{
	BW_GUARD;
	pChunk->addStaticItem( pCM_ );
}


/**
 *	Destructor
 */
PyChunkModel::~PyChunkModel()
{
	BW_GUARD;
	if (pCM_->chunk() != NULL)
		pCM_->chunk()->delStaticItem( pCM_ );
}

/*~ function BigWorld PyChunkModel
 *
 *	NOTE:  This function has been deprecated - Use a static PyModelObstacle instead
 *
 *  Creates an instance of PyChunkModel using a given model, with a given
 *  transform. A PyChunkModel is a PyObjectPlus wrapper around a simple model 
 *  which is treated by BigWorld as a part of the level geometry.
 *
 *  Code Example:
 *  @{
 *  # Import Math library
 *  import Math
 *
 *  # Place a PyChunkModel in the world at ( 0, 10, 0 )
 *  modelName = "objects\models\items\Trees & Plants\oak4.model"
 *  matrix = Math.Matrix()
 *  matrix.setTranslate( ( 0, 10, 0 ) )
 *  newPyChunkModel = BigWorld.PyChunkModel( modelName, $p.spaceID, matrix )
 *  @}
 *
 *  @param modelName A string containing the name of the model to use.
 *	@param spaceID The ID of the space in which to place the PyChunkModel.
 *  @param matrix The MatrixProvider which gives the PyChunkModel's transform.
 *  Note that the PyChunkModel's transform is static. It does does not 
 *  dynamically change with this MatrixProvider.
 *  @return The new PyChunkModel.
 */
/**
 *	Our factory method
 */
PyObject * PyChunkModel::pyNew( PyObject * pArgs )
{
	BW_GUARD;
	// These are hereby deprecated 1/9/2003
	WARNING_MSG( "PyChunkModel(): This class is deprecated. "
		"Use static PyModelObstacles instead.\n" );

	if (PyTuple_Size( pArgs ) == 3)
	{
		PyObject * pModelName = PyTuple_GetItem( pArgs, 0 );
		PyObject * pSpaceID = PyTuple_GetItem( pArgs, 1 );
		PyObject * pMatrix = PyTuple_GetItem( pArgs, 2 );

		if (PyString_Check( pModelName ) && PyInt_Check( pSpaceID ) &&
			MatrixProvider::Check( pMatrix ))
		{
			// parse the arguments (should use auto parsing ... oh well)
			SpaceID spaceID = PyInt_AsLong( pSpaceID );
			// find the space
			ChunkSpacePtr pSpace =
				ChunkManager::instance().space( spaceID, false );
			if (!pSpace)
			{
				PyErr_Format( PyExc_ValueError, "PyChunkModel(): "
					"No space ID %d", spaceID );
				return NULL;
			}

			// get the coordinates in the space
			MatrixProvider * pMP = (MatrixProvider*)pMatrix;
			Matrix mat = Matrix::identity;
			pMP->matrix( mat );
			Vector3 pos = mat.applyToOrigin();

			// find the chunk
			Chunk * pChunk = pSpace->findChunkFromPoint( pos );
			if (pChunk == NULL)
			{
				PyErr_SetString( PyExc_EnvironmentError, "PyChunkModel(): "
					"Could not find chunk at given point" );
				return NULL;
			}

			// build a datasection representing the chunk item
			DataSectionPtr dsp = new XMLSection( "model" );
			dsp->writeString( "resource", PyString_AsString( pModelName ) );
			mat.postMultiply( pChunk->transformInverse() );
			dsp->writeMatrix34( "transform", mat );

			// use the chunk loading functions to load it
			ChunkModel * pModel = new ChunkModel();
			if (!pModel->load( dsp ))
			{
				bw_safe_delete(pModel);

				PyErr_SetString( PyExc_ValueError, "PyChunkModel(): "
					"Error loading specified ChunkModel" );
				return NULL;
			}

			// make a wrapper for it
			return new PyChunkModel( pModel, pChunk );
		}
	}

	PyErr_SetString( PyExc_TypeError, "PyChunkModel(): "
		"Expected a model name, a space ID and a matrix provider" );
	return NULL;
}

BW_END_NAMESPACE

// py_chunk_model.cpp

