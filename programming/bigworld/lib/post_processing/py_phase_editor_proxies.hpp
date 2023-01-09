#ifndef PY_PHASE_EDITOR_PROXIES
#define PY_PHASE_EDITOR_PROXIES


#ifdef EDITOR_ENABLED

#include "gizmo/general_properties.hpp"
#include "resmgr/string_provider.hpp"
#include "../../tools/common/material_proxies.hpp"
#include "../../tools/common/material_properties.hpp"


BW_BEGIN_NAMESPACE

class BaseMaterialProxy;
typedef SmartPointer< BaseMaterialProxy > BaseMaterialProxyPtr;


namespace PostProcessing
{

class PyPhase;
typedef SmartPointer< PyPhase > PyPhasePtr;


class PyPhaseEditor;
typedef SmartPointer< PyPhaseEditor > PyPhaseEditorPtr;


/**
 *	This class implements a texture filename editor property proxy
 */
class PyPhaseTextureProxy : public UndoableDataProxy< StringProxy >
{
public:
	PyPhaseTextureProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnTex getFn, MaterialPropertiesUser::SetFnTex setFn,
		const BW::string& descName, PyPhasePtr pPhase, Moo::EffectMaterialPtr pEffectMaterial );

	virtual BW::string get() const;

	virtual BW::string opName() { return LocaliseUTF8(L"POST_PROCESSING/EDIT_PHASE", descName_.c_str() ); }

	virtual void setTransient( BW::string v );

	virtual bool setPermanent( BW::string v );

private:
	BaseMaterialProxyPtr proxyHolder_;
	MaterialPropertiesUser::GetFnTex getFn_;
	MaterialPropertiesUser::SetFnTex setFn_;
	BW::string descName_;
	PyPhasePtr pPhase_;
	Moo::EffectMaterialPtr pMaterial_;
};


/**
 *	This class implements a bool editor property proxy
 */
class PyPhaseBoolProxy : public UndoableDataProxy< BoolProxy >
{
public:
	PyPhaseBoolProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnBool getFn, MaterialPropertiesUser::SetFnBool setFn,
		const BW::string& descName, PyPhasePtr pPhase );

	virtual bool get() const;

	virtual BW::string opName() { return LocaliseUTF8(L"POST_PROCESSING/EDIT_PHASE", descName_.c_str() ); }

	virtual void setTransient( bool v );

	virtual bool setPermanent( bool v );

private:
	BaseMaterialProxyPtr proxyHolder_;
	MaterialPropertiesUser::GetFnBool getFn_;
	MaterialPropertiesUser::SetFnBool setFn_;
	BW::string descName_;
	PyPhasePtr pPhase_;
};


/**
 *	This class implements an integer editor property proxy
 */
class PyPhaseIntProxy : public UndoableDataProxy< IntProxy >
{
public:
	PyPhaseIntProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnInt getFn, MaterialPropertiesUser::SetFnInt setFn,
		MaterialPropertiesUser::RangeFnInt rangeFn, const BW::string& descName, PyPhasePtr pPhase );

	virtual int32 get() const;

	virtual BW::string opName() { return LocaliseUTF8(L"POST_PROCESSING/EDIT_PHASE", descName_.c_str() ); }

	virtual void setTransient( int32 v );

	virtual bool setPermanent( int32 v );

	bool getRange( int32& min, int32& max ) const;

private:
	BaseMaterialProxyPtr proxyHolder_;
	MaterialPropertiesUser::GetFnInt getFn_;
	MaterialPropertiesUser::SetFnInt setFn_;
	MaterialPropertiesUser::RangeFnInt rangeFn_;
	BW::string descName_;
	PyPhasePtr pPhase_;
};


/**
 *	This class implements an enum editor property proxy
 */
class PyPhaseEnumProxy : public UndoableDataProxy< IntProxy >
{
public:
	PyPhaseEnumProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnEnum getFn, MaterialPropertiesUser::SetFnEnum setFn,
		const BW::string& descName, PyPhasePtr pPhase );

	virtual int32 get() const;

	virtual BW::string opName() { return LocaliseUTF8(L"POST_PROCESSING/EDIT_PHASE", descName_.c_str() ); }

	virtual void setTransient( int32 v );

	virtual bool setPermanent( int32 v );

private:
	BaseMaterialProxyPtr proxyHolder_;
	MaterialPropertiesUser::GetFnEnum getFn_;
	MaterialPropertiesUser::SetFnEnum setFn_;
	BW::string descName_;
	PyPhasePtr pPhase_;
};


/**
 *	This class implements a floating-point editor property proxy
 */
class PyPhaseFloatProxy : public UndoableDataProxy< FloatProxy >
{
public:
	PyPhaseFloatProxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnFloat getFn, MaterialPropertiesUser::SetFnFloat setFn,
		MaterialPropertiesUser::RangeFnFloat rangeFn, const BW::string& descName, PyPhasePtr pPhase );

	virtual float get() const;

	virtual BW::string opName() { return LocaliseUTF8(L"POST_PROCESSING/EDIT_PHASE", descName_.c_str() ); }

	virtual void setTransient( float v );

	virtual bool setPermanent( float v );

	bool getRange( float& min, float& max, int& digits ) const;

private:
	BaseMaterialProxyPtr proxyHolder_;
	MaterialPropertiesUser::GetFnFloat getFn_;
	MaterialPropertiesUser::SetFnFloat setFn_;
	MaterialPropertiesUser::RangeFnFloat rangeFn_;
	BW::string descName_;
	PyPhasePtr pPhase_;
};


/**
 *	This class implements a Vector4 editor property proxy
 */
class PyPhaseVector4Proxy : public UndoableDataProxy< Vector4Proxy >
{
public:
	PyPhaseVector4Proxy( BaseMaterialProxyPtr proxy,
		MaterialPropertiesUser::GetFnVec4 getFn, MaterialPropertiesUser::SetFnVec4 setFn,
		const BW::string& descName, PyPhasePtr pPhase );

	virtual Vector4 get() const;

	virtual BW::string opName() { return LocaliseUTF8(L"POST_PROCESSING/EDIT_PHASE", descName_.c_str() ); }

	virtual void setTransient( Vector4 v );

	virtual bool setPermanent( Vector4 v );

private:
	BaseMaterialProxyPtr proxyHolder_;
	MaterialPropertiesUser::GetFnVec4 getFn_;
	MaterialPropertiesUser::SetFnVec4 setFn_;
	BW::string descName_;
	PyPhasePtr pPhase_;
};


/**
 *	This helper class is a proxy that uses simple a getter and a setter to
 *	edit a property, but does not insert undo operations.
 */
class PyPhaseFilterQuadTypeProxy : public IntProxy
{
public:
	PyPhaseFilterQuadTypeProxy( PyPhaseEditorPtr pItem );

	virtual IntProxy::Data get() const;

	virtual void set( IntProxy::Data v, bool transient, bool addBarrier = true );

private:
	PyPhaseEditorPtr pItem_;
};


} // namespace PostProcessing

BW_END_NAMESPACE

#endif // EDITOR_ENABLED


#endif // PY_PHASE_EDITOR_PROXIES
