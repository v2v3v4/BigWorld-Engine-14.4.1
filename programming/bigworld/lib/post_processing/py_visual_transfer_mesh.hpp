#ifndef PY_VISUAL_TRANSFER_MESH_HPP
#define PY_VISUAL_TRANSFER_MESH_HPP

#include "filter_quad.hpp"
#include "filter_quad_factory.hpp"
#include "moo/visual.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{
	/**
	 *	This class implements a FilterQuad that just
	 *	draws a visual file.
	 */
	class PyVisualTransferMesh : public FilterQuad
	{
		Py_Header( PyVisualTransferMesh, FilterQuad )
		DECLARE_FILTER_QUAD( PyVisualTransferMesh )
	public:
		PyVisualTransferMesh( PyTypeObject *pType = &s_type_ );
		~PyVisualTransferMesh();

		void preDraw( Moo::EffectMaterialPtr pMat )	{};
		void draw();
		bool save( DataSectionPtr );
		bool load( DataSectionPtr );

#ifdef EDITOR_ENABLED
		void edEdit( GeneralEditor * editor, FilterChangeCallback pCallback );
		BW::string edGetResourceID() const;
		bool edSetResourceID( const BW::string & resID );
#endif // EDITOR_ENABLED

		const BW::string& resourceID() const	{ return resourceID_; }
		void resourceID( const BW::string& );

		PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, resourceID, resourceID )

		PY_FACTORY_DECLARE()
	private:
		Moo::VisualPtr		visual_;
		BW::string			resourceID_;

#ifdef EDITOR_ENABLED
		FilterChangeCallback pCallback_;
#endif // EDITOR_ENABLED
	};
};

#ifdef CODE_INLINE
#include "py_visual_transfer_mesh.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PY_VISUAL_TRANSFER_MESH_HPP
