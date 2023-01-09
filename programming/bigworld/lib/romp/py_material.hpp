#ifndef PY_MATERIAL_HPP
#define PY_MATERIAL_HPP

#include "moo/effect_material.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

class PyMaterial : public PyObjectPlus
{
	Py_Header( PyMaterial, PyObjectPlus )
public:
	PyMaterial( Moo::EffectMaterialPtr pMaterial, PyTypeObject *pType = &s_type_ );
	~PyMaterial();

	void tick( float dTime );
	Moo::EffectMaterialPtr pMaterial() const	{ return pMaterial_; }

	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );
	void pyAdditionalMembers( const ScriptList & pList ) const;

	PY_FACTORY_DECLARE()
private:
	bool findPropertyAndDesc(
		const char * name, Moo::EffectPropertyPtr & prop,
		D3DXHANDLE & handle,
		D3DXPARAMETER_DESC & desc );
	Moo::EffectMaterialPtr pMaterial_;

	typedef BW::map<Moo::EffectPropertyPtr, Vector4ProviderPtr> Vector4Providers;
	typedef BW::map<Moo::EffectPropertyPtr, MatrixProviderPtr> MatrixProviders;

	Vector4Providers	v4Providers_;
	MatrixProviders		matrixProviders_;
};

typedef SmartPointer<PyMaterial> PyMaterialPtr;

PY_SCRIPT_CONVERTERS_DECLARE( PyMaterial )

#ifdef CODE_INLINE
#include "py_material.ipp"
#endif

BW_END_NAMESPACE

#endif //#ifndef PY_MATERIAL_HPP
