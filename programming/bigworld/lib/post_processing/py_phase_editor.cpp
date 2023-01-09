
#include "pch.hpp"
#include "py_phase_editor.hpp"
#include "py_phase_editor_proxies.hpp"
#include "py_phase.hpp"
#include "filter_quad_factory.hpp"
#include "resmgr/xml_section.hpp"


#ifdef EDITOR_ENABLED


BW_BEGIN_NAMESPACE

namespace
{

const char * BACK_BUFFER_STRING = "backBuffer";


/**
 *	This helper class implements an undo/redo operation for when changing a
 *	phase's filter quad type.
 */
class PyPhaseFilterQuadTypeUndo : public UndoRedo::Operation
{
public:
	PyPhaseFilterQuadTypeUndo( PostProcessing::PyPhasePtr pPhase ) :
		UndoRedo::Operation( size_t(typeid(PyPhaseFilterQuadTypeUndo).name()) ),
		pPhase_( pPhase )
	{
		pOldFilterQuad_ = pPhase_->filterQuad();
	}

private:

	virtual void undo()
	{
		// first add the current state of this proxy to the undo/redo list
		UndoRedo::instance().add( new PyPhaseFilterQuadTypeUndo( pPhase_ ) );

		pPhase_->filterQuad( pOldFilterQuad_ );
		pPhase_->callCallback( true /* refresh properties */ );
	}

	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		// these operations never replace each other
		return false;
	}

	PostProcessing::PyPhasePtr pPhase_;
	PostProcessing::FilterQuadPtr pOldFilterQuad_;
};


} // anonymous namespace


namespace PostProcessing
{


PyPhaseEditor::PyPhaseEditor( PyPhase * phase ) :
	pPhase_( phase )
{
}


void PyPhaseEditor::proxySetCallback()
{
}


BW::string PyPhaseEditor::textureFeed( const BW::string& descName ) const
{
	return "";
}


BW::string PyPhaseEditor::defaultTextureDir() const
{
	return "";
}


BW::string PyPhaseEditor::exposedToScriptName( const BW::string& descName ) const
{
	return "";
}


StringProxyPtr PyPhaseEditor::textureProxy( BaseMaterialProxyPtr proxy,
	GetFnTex getFn, SetFnTex setFn, const BW::string& uiName, const BW::string& descName ) const
{
	return new PyPhaseTextureProxy( proxy, getFn, setFn, descName, pPhase_, pPhase_->material()->pMaterial() );
}


BoolProxyPtr PyPhaseEditor::boolProxy( BaseMaterialProxyPtr proxy,
	GetFnBool getFn, SetFnBool setFn, const BW::string& uiName, const BW::string& descName ) const
{
	return new PyPhaseBoolProxy( proxy, getFn, setFn, descName, pPhase_ );
}


IntProxyPtr PyPhaseEditor::enumProxy( BaseMaterialProxyPtr proxy,
	GetFnEnum getFn, SetFnEnum setFn, const BW::string& uiName, const BW::string& descName ) const
{
	return new PyPhaseEnumProxy( proxy, getFn, setFn, descName, pPhase_ );
}


IntProxyPtr PyPhaseEditor::intProxy( BaseMaterialProxyPtr proxy,
	GetFnInt getFn, SetFnInt setFn, RangeFnInt rangeFn, const BW::string& uiName, const BW::string& descName ) const
{
	return new PyPhaseIntProxy( proxy, getFn, setFn, rangeFn, descName, pPhase_ );
}


FloatProxyPtr PyPhaseEditor::floatProxy( BaseMaterialProxyPtr proxy,
	GetFnFloat getFn, SetFnFloat setFn, RangeFnFloat rangeFn, const BW::string& uiName, const BW::string& descName ) const
{
	return new PyPhaseFloatProxy( proxy, getFn, setFn, rangeFn, descName, pPhase_ );
}


Vector4ProxyPtr PyPhaseEditor::vector4Proxy( BaseMaterialProxyPtr proxy,
	GetFnVec4 getFn, SetFnVec4 setFn, const BW::string& uiName, const BW::string& descName ) const
{
	return new PyPhaseVector4Proxy( proxy, getFn, setFn, descName, pPhase_ );
}


BW::string PyPhaseEditor::getOutputRenderTarget() const
{
	BW::string ret( BACK_BUFFER_STRING );

	PyObjectPtr pyRT( PyObject_GetAttrString( pPhase_, "renderTarget" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyRT && pyRT.get() != Py_None)
	{
		PyObjectPtr pyRTName( PyObject_GetAttrString( pyRT.get(), "name" ),	PyObjectPtr::STEAL_REFERENCE );
		if (pyRTName)
		{
			ret = PyString_AS_STRING( pyRTName.get() );
		}
	}

	return ret;
}


bool PyPhaseEditor::setOutputRenderTarget( const BW::string & rt )
{
	BW::StringRef rtName;
	if (rt != BACK_BUFFER_STRING)
	{
		rtName = rt;
	}

	// We need this postfix so the properties list understands this is a texture.
	BW::StringRef renderTargetPostfix = ".rt";
	BW::StringRef::size_type postfixPos = rtName.rfind( renderTargetPostfix );

	if (postfixPos != BW::string::npos)
	{
		// It's a render target, remove the postfix so we get the name right.
		rtName = rtName.substr( 0, postfixPos );
	}

	pPhase_->callCallback( false );

	return pPhase_->renderTargetFromString( rtName );
}


bool PyPhaseEditor::getClearRenderTarget() const
{
	return pPhase_->clearRenderTarget();
}


bool PyPhaseEditor::setClearRenderTarget( const bool & clear )
{
	pPhase_->clearRenderTarget( clear );
	pPhase_->callCallback( false );
	return true;
}


BW::string PyPhaseEditor::getMaterialFx() const
{
	BW::string ret;

	PyMaterialPtr material = pPhase_->material();

	if (material &&
		material->pMaterial() &&
		material->pMaterial()->pEffect())
	{
		ret = material->pMaterial()->pEffect()->resourceID();
	}

	return ret;
}


bool PyPhaseEditor::setMaterialFx( const BW::string & materialFx )
{
	bool ok = false;

	Moo::EffectMaterialPtr effect = new Moo::EffectMaterial();

	if (effect->initFromEffect( materialFx ))
	{
		pPhase_->material(
			PyMaterialPtr( new PyMaterial( effect ), PyObjectPtr::STEAL_REFERENCE ) );
		ok = true;

		pPhase_->callCallback( true /* needs reload */ );
	}

	return ok;
}


int PyPhaseEditor::getFilterQuadType() const
{
	bool found = false;
	int idx = 0;

	BW::string curFilterQuadName = pPhase_->filterQuad()->creatorName();

	for (FilterQuadCreators::iterator it = FilterQuadCreators::instance().begin();
		it != FilterQuadCreators::instance().end(); ++it)
	{
		if (curFilterQuadName == (*it).first)
		{
			found = true;
			break;
		}
		idx++;
	}

	return found ? idx : -1;
}


bool PyPhaseEditor::setFilterQuadType( const int & filterQuadType, bool addBarrier )
{
	int idx = 0;

	FilterQuadCreators::iterator it = FilterQuadCreators::instance().begin();
	for (; it != FilterQuadCreators::instance().end(); ++it)
	{
		if (idx == filterQuadType)
		{
			break;
		}
		idx++;
	}

	bool ok = false;

	if (it != FilterQuadCreators::instance().end())
	{
		DataSectionPtr pTempDS = new XMLSection( "root" );
		pTempDS->openSection( (*it).first, true );
		FilterQuad * pNewFilterQuad = (*(*it).second)( pTempDS );

		if (pNewFilterQuad)
		{
			UndoRedo::instance().add( new PyPhaseFilterQuadTypeUndo( pPhase_ ) );

			if (addBarrier)
			{
				UndoRedo::instance().barrier(
					LocaliseUTF8( L"WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PY_FILTERQUAD/MODIFY_FILTERQUAD" ),
					false );
			}

			pPhase_->filterQuad( pNewFilterQuad );

			pPhase_->callCallback( true /* needs reload */ );

			ok = true;
		}
	}

	if (!ok)
	{
		ERROR_MSG( "Could not change the post-processing phase's filter quad type.\n" );
	}

	return ok;
}


} // namespace PostProcessing

BW_END_NAMESPACE

#endif // EDITOR_ENABLED
