#include "pch.hpp"
#include "py_visual_transfer_mesh.hpp"
#include "moo/visual_manager.hpp"

#ifdef EDITOR_ENABLED
#include "gizmo/general_editor.hpp"
#include "gizmo/general_properties.hpp"
#include "resmgr/string_provider.hpp"
#include "../../tools/common/material_properties.hpp"
#endif //EDITOR_ENABLED

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_visual_transfer_mesh.ipp"
#endif

namespace PostProcessing
{

// Python statics
PY_TYPEOBJECT( PyVisualTransferMesh )

PY_BEGIN_METHODS( PyVisualTransferMesh )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyVisualTransferMesh )	
	/*~ attribute PyVisualTransferMesh.resourceID
	 *	@components{ client, tools }
	 *	Resource ID of the visual file (*.visual) with which to
	 *	draw.  Use this as a replacement for PyFilterQuad if you
	 *	want to control the geometry via a Max or Maya exported
	 *	mesh.  For example, you can create a highly-tessellated
	 *	mesh, export it from Max and use it in a post-processing
	 *	effect.
	 *	@type String.
	 */
	PY_ATTRIBUTE( resourceID )
PY_END_ATTRIBUTES()

/*~ class PostProcessing.PyVisualTransferMesh
 *	@components{ client, tools }
 *	This class implements a FilterQuad that just
 *	draws a visual file.
 */
/*~ function PostProcessing.VisualTransferMesh
 *	@components{ client, tools }
 *	Factory function to create and return a PostProcessing
 *	PyVisualTransferMesh object.
 *	@return A new PostProcessing PyVisualTransferMesh object.
 */
PY_FACTORY_NAMED( PyVisualTransferMesh, "VisualTransferMesh", _PostProcessing )
IMPLEMENT_FILTER_QUAD( PyVisualTransferMesh, PyVisualTransferMesh )


PyVisualTransferMesh::PyVisualTransferMesh( PyTypeObject *pType ):
	FilterQuad( pType ),
	resourceID_( "" )
#ifdef EDITOR_ENABLED
	, pCallback_( NULL )
#endif // EDITOR_ENABLED
{
}


PyVisualTransferMesh::~PyVisualTransferMesh()
{
}


/**
 *	This method sets the resourceID of the visual file we are using
 *	as a transfer mesh.
 *	@param	name	r	resourceID of a .visual file.
 */
void PyVisualTransferMesh::resourceID( const BW::string& r )
{
	resourceID_ = r;

	visual_ = Moo::VisualManager::instance()->get( resourceID_ );
	if ( !visual_ )
	{
		ASSET_MSG( "Could not find visual file %s\n", resourceID_.c_str() );
	}

}


/**
 *	This method draws the visual transfer mesh.
 */
void PyVisualTransferMesh::draw()
{
	if (visual_.hasObject())
	{
		visual_->justDrawPrimitives();
	}
}


bool PyVisualTransferMesh::save( DataSectionPtr pDS )
{	
	DataSectionPtr pSect = pDS->newSection( "PyVisualTransferMesh" );
	pSect->writeString( "resourceID", resourceID_ );
	return true;
}


bool PyVisualTransferMesh::load( DataSectionPtr pSect )
{
	this->resourceID( pSect->readString( "resourceID", resourceID_ ) );
	return true;
}


#ifdef EDITOR_ENABLED
void PyVisualTransferMesh::edEdit( GeneralEditor * editor, FilterChangeCallback pCallback )
{
	pCallback_ = pCallback;

	STATIC_LOCALISE_NAME( s_resource,  "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_VISUALXFERMESH/RESOURCE" );

	TextProperty * resourceProp = new TextProperty( s_resource,
			new GetterSetterProxy< PyVisualTransferMesh, StringProxy >(
			this, "resourceProp",
			&PyVisualTransferMesh::edGetResourceID, 
			&PyVisualTransferMesh::edSetResourceID ) );

	resourceProp->fileFilter( Name( "Visual files(*.visual)|*.visual|Visual files(*.visual)|*.visual||" ) );

	STATIC_LOCALISE_NAME( s_resourceDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_VISUALXFERMESH/RESOURCE_DESC" );
	resourceProp->UIDesc( s_resourceDesc );

	editor->addProperty( resourceProp );
}


BW::string PyVisualTransferMesh::edGetResourceID() const
{
	return resourceID();
}


bool PyVisualTransferMesh::edSetResourceID( const BW::string & resID )
{
	resourceID( resID );
	if (pCallback_)
	{
		(*pCallback_)( false );
	}
	return true;
}

#endif // EDITOR_ENABLED

PyObject* PyVisualTransferMesh::pyNew(PyObject* args)
{
	char * id = NULL;
	if (!PyArg_ParseTuple( args, "|s", &id ))
	{
		PyErr_SetString( PyExc_TypeError, "PostProcessing.VisualTransferMesh: "
			"Argument parsing error: Expected a resource ID" );
		return NULL;
	}

	PyVisualTransferMesh * m = new PyVisualTransferMesh;
	if (id != NULL)
		m->resourceID( id );
	return m;
}

} // namespace PostProcessing

BW_END_NAMESPACE

// py_visual_transfer_mesh.cpp
