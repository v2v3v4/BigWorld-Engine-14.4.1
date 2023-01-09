#include "pch.hpp"
#include "py_phase.hpp"
#include "py_copy_back_buffer.hpp"
#include "filter_quad_factory.hpp"
#include "manager.hpp"
#include "debug.hpp"
#include "moo/render_context.hpp"
#include "moo/resource_load_context.hpp"


#ifdef EDITOR_ENABLED
#include "py_phase_editor.hpp"
#include "py_phase_editor_proxies.hpp"
#include "gizmo/general_properties.hpp"
#include "resmgr/xml_section.hpp"
#endif //EDITOR_ENABLED

DECLARE_DEBUG_COMPONENT2( "PostProcessing", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_phase.ipp"
#endif

namespace PostProcessing
{

// Python statics
PY_TYPEOBJECT( PyPhase )

PY_BEGIN_METHODS( PyPhase )	
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyPhase )
	/*~ attribute PyPhase.filterQuad
	 *	@components{ client, tools }
	 *	Provides a full-screen quad with n filter-tap locations
	 *	to draw this phase onto the screen.
	 *	@type PyFilterQuad.
	 */
	PY_ATTRIBUTE( filterQuad )
	/*~ attribute PyPhase.renderTarget
	 *	@components{ client, tools }
	 *	Provides the output target for this phase.  If None, the
	 *	phase will draw to the back buffer.
	 *	@type PyRenderTarget.
	 */
	PY_ATTRIBUTE( renderTarget )
	/*~ attribute PyPhase.clearRenderTarget
	 *	@components{ client, tools }
	 *	Set this to true if you  want the phase to clear the render
	 *	target before drawing.  This is useful when the material
	 *	is additive.  An example of this would be multi-pass filter
	 *	quads, using 24 filter taps when the hardware can do only 4
	 *	taps at a time.  The material needs to be additive so the
	 *	passes accumulate, and the render target needs to be cleared
	 *	before the operation.
	 *	If the material blend mode specifies that the entire render
	 *	target will be overwritten, then you should not choose to
	 *	clear the render target first, as it will simply be a waste of
	 *	time.
	 *	@type Boolean.
	 */
	PY_ATTRIBUTE( clearRenderTarget )
	/*~ attribute PyPhase.material
	 *	@components{ client, tools }
	 *	Material with which to draw the filterQuad.
	 *	@type PyMaterial.
	 */
	PY_ATTRIBUTE( material )
	/*~ attribute PyPhase.name
	 *	@components{ client, tools }
	 *	Arbitrary name for the phase.
	 *	@type String.
	 */
	PY_ATTRIBUTE( name )
PY_END_ATTRIBUTES()

/*~ class PostProcessing.PyPhase
 *	@components{ client, tools }
 *	This class derives from PostProcessing::Phase, and
 *	provides a generic phase designed to be configured
 *	entirely via python.
 *	A phase is a single step within a PostProcessing Effect.  It generally
 *	performs a single draw of a filter quad using a material into a render
 *	target.
 */
/*~ function PostProcessing.Phase
 *	@components{ client, tools }
 *	Factory function to create and return a PostProcessing PyPhase object.
 *	@return A new PostProcessing PyPhase object.
 */
PY_FACTORY_NAMED( PyPhase, "Phase", _PostProcessing )

IMPLEMENT_PHASE( PyPhase, PyPhase )

PyPhase::PyPhase( PyTypeObject *pType ) :
	Phase( pType ),
	clearRenderTarget_( false ),
	name_( "phase" )
#ifdef EDITOR_ENABLED
	, calcPreview_()
#endif
{
}


PyPhase::~PyPhase()
{
}


/**
 *	This method ticks the phase.
 *	@param	dTime	delta frame time, in seconds.
 */
void PyPhase::tick( float dTime )
{
	if (pMaterial_.hasObject())
	{
		pMaterial_->tick( dTime );
	}
}


/**
 *	This method draws the phase.
 *	@param	debug		optional debug object to record the output of the phase.
 *	@param	debugRect	optional viewport in which to draw debug output to.
 */
void PyPhase::draw( Debug* debug, RECT* debugRect )
{
	if (!pFilterQuad_.hasObject() || !pMaterial_.hasObject())
	{
		return;
	}

	Moo::EffectMaterialPtr mat = pMaterial_->pMaterial();

	bool usingRenderTarget = false;
	if (pRenderTarget_.hasObject() &&
		pRenderTarget_->pRenderTarget()->push())
	{
		usingRenderTarget = true;
	}
	else
	{
		//we are about to write to the back buffer, so increment its cookie.
		PyCopyBackBuffer::incrementDrawCookie();
	}

	if (clearRenderTarget_ && !debugRect)
	{
		Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET, 0x00000000, 1, 0 );
	}

	DX::Viewport oldViewport;
	DX::Viewport newViewport;
	if ( debugRect )
	{		
		Moo::rc().getViewport( &oldViewport );
		newViewport.X = debugRect->left;
		newViewport.Y = debugRect->top;
		newViewport.Width = debugRect->right - debugRect->left;
		newViewport.Height = debugRect->bottom - debugRect->top;
		newViewport.MinZ = 0.0;
		newViewport.MaxZ = 1.0;
		Moo::rc().setViewport( &newViewport );
	}

	if (mat->begin())
	{
		pFilterQuad_->preDraw(mat);
		for (uint32 i=0; i<mat->numPasses(); i++)
		{
			mat->beginPass(i);
			pFilterQuad_->draw();
			mat->endPass();
		}
		mat->end();
	}

	if ( debugRect )
	{		
		Moo::rc().setViewport( &oldViewport );
	}

	if (usingRenderTarget)
	{
		pRenderTarget_->pRenderTarget()->pop();
	}

	if ( debug )
	{
		if (pRenderTarget_.hasObject() && pRenderTarget_->pRenderTarget()->valid())
		{
			D3DXHANDLE previewTechnique = mat->pEffect()->pEffect()->GetTechniqueByName("Preview");
			if ( previewTechnique )
			{
				D3DXHANDLE currTechnique = mat->hTechnique();
				PyRenderTargetPtr pCurrRT = this->pRenderTarget_;
				RECT r = debug->nextPhase();

				this->pRenderTarget_ = debug->pRT();
				mat->hTechnique( previewTechnique );
				this->draw( NULL, &r );

				mat->hTechnique( currTechnique );
				this->pRenderTarget_ = pCurrRT;

			}
			else
			{
				debug->copyPhaseOutput( pRenderTarget_->pRenderTarget() );
			}
		}
		else
		{
			debug->copyPhaseOutput( NULL );
		}
	}
}


bool PyPhase::load( DataSectionPtr pSect )
{
	name_ = pSect->readString( "name", name_ );

	Moo::ScopedResourceLoadContext resLoadCtx( "post-processing phase " + name_ );

	clearRenderTarget_ = pSect->readBool( "clearRenderTarget", clearRenderTarget_ );
	DataSectionPtr materialSect = pSect->openSection( "material" );
	Moo::EffectMaterialPtr pMat = new Moo::EffectMaterial();
	if (pMat->load( materialSect ))
	{
		pMaterial_ = PyMaterialPtr( new PyMaterial( pMat ), true );
	}
	BW::string rtName = pSect->readString( "renderTarget" );
	if (!rtName.empty())
	{
		renderTargetFromString( rtName );
	}
	DataSectionPtr pFilterQuadSect = pSect->openSection( "filterQuad" );

	pFilterQuad_ = FilterQuadPtr(
					FilterQuadFactory::loadItem( pFilterQuadSect->openChild(0) ), true );
	return true;
}


bool PyPhase::save( DataSectionPtr pDS )
{
	DataSectionPtr pSect = pDS->newSection( "PyPhase" );
	pSect->writeString( "name", name_ );
	if (pRenderTarget_.hasObject())
	{
		pSect->writeString( "renderTarget", pRenderTarget_->pRenderTarget()->resourceID() );
	}
	pSect->writeBool( "clearRenderTarget", clearRenderTarget_ );
	DataSectionPtr fq = pSect->openSection( "filterQuad", true );
	pFilterQuad_->save(fq);
	DataSectionPtr material = pSect->openSection( "material", true );
	pMaterial_->pMaterial()->save(material);
	return true;
}


bool PyPhase::renderTargetFromString( const BW::StringRef & resourceID )
{
	bool ok = false;

	if (!resourceID.empty())
	{
		Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get( resourceID );
		if (pTex.hasObject())
		{
			Moo::RenderTarget* pRT = dynamic_cast<Moo::RenderTarget*>( pTex.getObject() );
			if ( pRT )
			{
				pRenderTarget_ = PyRenderTargetPtr( new PyRenderTarget(pRT), true );
				ok = true;
			}
			else
			{
				pRenderTarget_ = NULL;
				ok = false;
			}
		}
	}
	else
	{
		pRenderTarget_ = NULL;
		ok = true;
	}

	return ok;
}


#ifdef EDITOR_ENABLED


void PyPhase::edChangeCallback( PhaseChangeCallback pCallback )
{
	pCallback_ = pCallback;
}


void PyPhase::edEdit( GeneralEditor * editor )
{
	pPropertyEditor_ = new PyPhaseEditor( this );

	STATIC_LOCALISE_NAME( s_renderTarget, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/RENDER_TARGET" );
	STATIC_LOCALISE_NAME( s_renderTargetDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/RENDER_TARGET_DESC" );
	STATIC_LOCALISE_NAME( s_clearRenderTarget, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/CLEAR_RENDER_TARGET" );
	STATIC_LOCALISE_NAME( s_clearRenderTargetDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/CLEAR_RENDER_TARGET_DESC" );
	STATIC_LOCALISE_NAME( s_materialFX, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/MATERIAL_FX" );
	STATIC_LOCALISE_NAME( s_materialFXDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/MATERIAL_FX_DESC" );
	STATIC_LOCALISE_NAME( s_filterQuadType, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/FILTERQUAD_TYPE" );
	STATIC_LOCALISE_NAME( s_filterQuadTypeDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_PHASE/FILTERQUAD_TYPE_DESC" );


	TextProperty * outputRT = new TextProperty( s_renderTarget,
			new GetterSetterProxy< PyPhaseEditor, StringProxy >(
				pPropertyEditor_, "outputRenderTarget",
				&PyPhaseEditor::getOutputRenderTarget, 
				&PyPhaseEditor::setOutputRenderTarget ) );

	outputRT->fileFilter( Name( "Render Targets (*.rt)|*.rt|Render Targets (*.rt)|*.rt||" ) );

	outputRT->UIDesc( s_renderTargetDesc );

	editor->addProperty( outputRT );

	GenBoolProperty * clearRT = new GenBoolProperty( s_clearRenderTarget,
		new GetterSetterProxy< PyPhaseEditor, BoolProxy >(
			pPropertyEditor_, "clearRenderTarget",
			&PyPhaseEditor::getClearRenderTarget, 
			&PyPhaseEditor::setClearRenderTarget ) );

	clearRT->UIDesc( s_clearRenderTargetDesc );

	editor->addProperty( clearRT );

	TextProperty * materialFx = new TextProperty( s_materialFX,
			new GetterSetterProxy< PyPhaseEditor, StringProxy >(
				pPropertyEditor_, "materialFx",
				&PyPhaseEditor::getMaterialFx, 
				&PyPhaseEditor::setMaterialFx ) );

	materialFx->fileFilter( Name( "Effect Files (*.fx)|*.fx|Effect Files (*.fx)|*.fx||" ) );
	materialFx->UIDesc( s_materialFXDesc );

	editor->addProperty( materialFx );

	DataSectionPtr pFilterQuadTypesDS = new XMLSection( "filterQuadTypes" );

	int idx = 0;
	for (FilterQuadCreators::iterator it = FilterQuadCreators::instance().begin();
		it != FilterQuadCreators::instance().end(); ++it)
	{
		pFilterQuadTypesDS->writeInt( (*it).first, idx );
		++idx;
	}

	ChoiceProperty * filterQuadType = new ChoiceProperty( s_filterQuadType,
			new PyPhaseFilterQuadTypeProxy( pPropertyEditor_ ), pFilterQuadTypesDS );

	filterQuadType->UIDesc( s_filterQuadTypeDesc );

	editor->addProperty( filterQuadType );

	if (pFilterQuad_)
	{
		pFilterQuad_->edEdit( editor, pCallback_ );
	}

	if (pMaterial_)
	{
		MaterialProperties::beginProperties();
		MaterialProperties::addMaterialProperties(
			pMaterial_->pMaterial(), editor, false, false, pPropertyEditor_.get() );
		MaterialProperties::endProperties();
	}
}


PyMaterialPtr PyPhase::material() const
{
	return pMaterial_;
}


void PyPhase::material( const PyMaterialPtr & newVal )
{
	pMaterial_ = newVal;
}


FilterQuadPtr PyPhase::filterQuad() const
{
	return pFilterQuad_;
}


void PyPhase::filterQuad( const FilterQuadPtr & newVal )
{
	pFilterQuad_ = newVal;
}


const bool PyPhase::clearRenderTarget() const
{
	return clearRenderTarget_;
}


void PyPhase::clearRenderTarget( bool newVal )
{
	clearRenderTarget_ = newVal;
}

void PyPhase::callCallback( bool refreshProperties )
{
	if (pCallback_)
	{
		(*pCallback_)( refreshProperties );
	}
}


#endif // EDITOR_ENABLED

PyObject* PyPhase::pyNew(PyObject* args)
{
	return new PyPhase;
}


} //namespace PostProcessing

BW_END_NAMESPACE

// py_phase.cpp
