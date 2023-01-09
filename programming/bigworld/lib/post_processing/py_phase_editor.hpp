#ifndef PY_PHASE_EDITOR
#define PY_PHASE_EDITOR


#ifdef EDITOR_ENABLED

#include "../../tools/common/material_properties.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{


class PyPhase;
typedef SmartPointer< PyPhase > PyPhasePtr;


class PyPhaseEditor : public MaterialPropertiesUser, public ReferenceCount
{
public:
	PyPhaseEditor( PyPhase * phase );

	virtual void proxySetCallback();

	virtual BW::string textureFeed( const BW::string& descName ) const;

	virtual BW::string defaultTextureDir() const;

	virtual BW::string exposedToScriptName( const BW::string& descName ) const;

	virtual StringProxyPtr textureProxy( BaseMaterialProxyPtr proxy,
		GetFnTex getFn, SetFnTex setFn, const BW::string& uiName, const BW::string& descName ) const;

	virtual BoolProxyPtr boolProxy( BaseMaterialProxyPtr proxy,
		GetFnBool getFn, SetFnBool setFn, const BW::string& uiName, const BW::string& descName ) const;

	virtual IntProxyPtr enumProxy( BaseMaterialProxyPtr proxy,
		GetFnEnum getFn, SetFnEnum setFn, const BW::string& uiName, const BW::string& descName ) const;

	virtual IntProxyPtr intProxy( BaseMaterialProxyPtr proxy,
		GetFnInt getFn, SetFnInt setFn, RangeFnInt rangeFn, const BW::string& uiName, const BW::string& descName ) const;

	virtual FloatProxyPtr floatProxy( BaseMaterialProxyPtr proxy,
		GetFnFloat getFn, SetFnFloat setFn, RangeFnFloat rangeFn, const BW::string& uiName, const BW::string& descName ) const;

	virtual Vector4ProxyPtr vector4Proxy( BaseMaterialProxyPtr proxy,
		GetFnVec4 getFn, SetFnVec4 setFn, const BW::string& uiName, const BW::string& descName ) const;

	BW::string getOutputRenderTarget() const;

	bool setOutputRenderTarget( const BW::string & rt );

	bool getClearRenderTarget() const;

	bool setClearRenderTarget( const bool & clear );

	BW::string getMaterialFx() const;

	bool setMaterialFx( const BW::string & materialFx );

	int getFilterQuadType() const;

	bool setFilterQuadType( const int & filterQuadType, bool addBarrier );

private:
	PyPhase * pPhase_;
};


} // namespace PostProcessing


BW_END_NAMESPACE

#endif // EDITOR_ENABLED


#endif // PY_PHASE_EDITOR
