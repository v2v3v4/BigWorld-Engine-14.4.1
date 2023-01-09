#include "pch.hpp"

#include "global_embodiments.hpp"

#include "space/space_manager.hpp"
#include "space/entity_embodiment.hpp"
#include "space/deprecated_space_helpers.hpp"
#include "space/client_space.hpp"

BW_BEGIN_NAMESPACE


// ----------------------------------------------------------------------------
//	Section - Global Models.  Display any PyAttachment in the world without
//	them being attahced attached to an entity or another model.
// ----------------------------------------------------------------------------
static EntityEmbodiments globalModels_;


void GlobalEmbodiments::tick( float dTime )
{
	for (EntityEmbodiments::iterator iter = globalModels_.begin();
		iter != globalModels_.end();
		iter++)
	{
		(*iter)->move( dTime );
	}
}


void GlobalEmbodiments::fini()
{
	for (EntityEmbodiments::iterator iter = globalModels_.begin();
		iter != globalModels_.end();
		iter++)
	{
		IEntityEmbodimentPtr pCA = *iter;
		pCA->leaveSpace();
	}

	globalModels_.clear();	
}


/*~ function BigWorld.addModel
 *
 *	This function adds a model to the global model list.  It allows models
 *	to be drawn and ticked without being attached to either an entity or
 *	another model.
 *	This function actually allows any PyAttachment to be added, however
 *	it is called addModel to be consistent with the Entity method addModel.
 *	That method also allows any PyAttachment to be added.
 *
 *	@param	pModel	A PyAttachment to be added to the global models list.
 *	@param	spaceID [optional] space ID.  If not set, the current camera space
 *	is used.
 */
static void addModel( PyObjectPtr scriptObject ,
					 SpaceID spaceID = NULL_SPACE )
{
	BW_GUARD;
	ClientSpacePtr pSpace = NULL;

	if (!SpaceManager::instance().isReady())
	{
		// If space manager has shut down, then don't do anything.
		return;
	}

	if (spaceID == NULL_SPACE)
	{
		pSpace = DeprecatedSpaceHelpers::cameraSpace();
	}
	else
	{
		pSpace = SpaceManager::instance().space( spaceID );
	}

	// convert script object to embodiment
	IEntityEmbodimentPtr embodiment = 
		SpaceManager::instance().createEntityEmbodiment( 
			ScriptObject(scriptObject) );

	if (pSpace)
	{
		embodiment->enterSpace( pSpace );
	}

	globalModels_.push_back( embodiment );	
}

PY_AUTO_MODULE_FUNCTION(
	RETVOID, addModel, NZARG( PyObjectPtr, OPTARG( SpaceID, 0, END )), BigWorld )


/*~ function BigWorld.delModel
 *
 *	This function deletes a model from the global model list.
 *
 *	This function actually works with any PyAttachment, however
 *	it is called delModel to be consistent with the Entity method delModel.
 *	That method also allows any PyAttachment to be deleted.
 *
 *	@param	pModel	A PyAttachment to be added to the global models list.
 */
static void delModel( PyObjectPtr pEmbodimentPyObject )
{
	BW_GUARD;
	EntityEmbodiments::iterator iter;
	for (iter = globalModels_.begin();
		iter != globalModels_.end();
		iter++)
	{
		if ((*iter)->scriptObject() == pEmbodimentPyObject) break;
	}

	if (iter == globalModels_.end())
	{
		PyErr_SetString( PyExc_ValueError, "BigWorld.delModel: "
			"Not added as a global model." );
		return;
	}

	(*iter)->leaveSpace();	

	globalModels_.erase( iter );
}


PY_AUTO_MODULE_FUNCTION(
	RETVOID, delModel, NZARG( PyObjectPtr, END ), BigWorld )


/*~ function BigWorld.models
 *
 *	This function sets or gets the global model list.
 *
 *	@param	Any sequence of PyAttachments, or None.
 *	@return	The global models list, if no arguments were passed in, or None.
 */
static PyObject * models( PyObjectPtr models )
{
	if ( models == NULL )
	{
		PyObject * pList = PyList_New( globalModels_.size() );
		for ( size_t i=0; i<globalModels_.size(); i++)
		{
			PyList_SET_ITEM( pList, i, Script::getData(globalModels_[i]) );
		}
		return pList;
	}

	if ( !PySequence_Check(models.get()) )
	{
		PyErr_Format( PyExc_TypeError, "BigWorld.setModels - Expected a "
			"sequence type." );
		return NULL;
	}

	int st = (int)(globalModels_.size()) - 1;
	for ( int i = st; i >= 0; i-- )
	{
		IEntityEmbodimentPtr pEmb = globalModels_[i];
		delModel( pEmb->scriptObject() );
	}
	globalModels_.clear();

	size_t slen = PySequence_Size(models.get());
	for (size_t i = 0; i < slen; i++)
	{
		PyObject * pModel = PySequence_GetItem( models.get(), i );
		if (pModel != NULL)
		{
			addModel( pModel );
		}
	}

	Py_RETURN_NONE;
}

PY_AUTO_MODULE_FUNCTION(
	RETOWN, models, OPTARG( PyObjectPtr, NULL, END ), BigWorld )

BW_END_NAMESPACE