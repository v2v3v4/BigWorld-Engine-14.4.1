#include "pch.hpp"
#include "py_material.hpp"
#include "pyscript/py_data_section.hpp"
#include "romp/py_texture_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "py_material.ipp"
#endif

// Python statics
PY_TYPEOBJECT( PyMaterial )

PY_BEGIN_METHODS( PyMaterial )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMaterial )
PY_END_ATTRIBUTES()

/*~ function BigWorld.Material
 *	@components{ client,  tools }
 *	Factory function to create and return a PyMaterial object.
 *
 *	Note that the parameter list is complicated, so although
 *	three parameters are listed below, the PyMaterial constructor takes :
 *	Either the name of an .fx file, (and optionally the name of a diffuse map.)
 *	Or a PyDataSection containing a material section.
 *
 *	There are no listed attributes for a PyMaterial, however on creation,
 *	the attribute dictionary will be filled with all the "artist editable"
 *	variables contained in the effect file.
 *
 *	@param	effectName	Name of the effect file (*.fx)
 *	@param	diffuseMap	Name of the diffuse map for the .fx file
 *	@param	DataSection DataSection for a material section.
 *
 *	@return A new PyMaterial object.
 */
PY_FACTORY_NAMED( PyMaterial, "Material", BigWorld )


PyMaterial::PyMaterial( Moo::EffectMaterialPtr pMaterial, PyTypeObject *pType ):
	PyObjectPlus( pType ),
	pMaterial_( pMaterial )
{
	BW_GUARD;
}


PyMaterial::~PyMaterial()
{
	BW_GUARD;
}


/**
 * This method finds the material property, effect handle and decsription
 * given the string name of the property.
 * @param	name	name of the property to find
 * @param	prop	[out] the found property
 * @param	handle	[out] handle of the found property
 * @param	desc	[out] decsription of the found property
 * @return	bool	whether or not the property was found.
 */
bool PyMaterial::findPropertyAndDesc(
	const char * name, Moo::EffectPropertyPtr & prop,
	D3DXHANDLE & handle,
	D3DXPARAMETER_DESC & desc )
{
	BW_GUARD;
	Moo::EffectMaterial::Properties::iterator it = pMaterial_->properties().begin();
	Moo::EffectMaterial::Properties::iterator en = pMaterial_->properties().end();
	for( ; it != en; it++ )
	{
		handle = it->first;		
		pMaterial_->pEffect()->pEffect()->GetParameterDesc( handle, &desc );
		if(!bw_stricmp( name, desc.Name ))
		{
			prop = it->second;
			return true;
		}
	}
	return false;
}


/**
 * This method updates the realtime material properties.
 * @param	dTime	delta frame time, in seconds.
 */
void PyMaterial::tick( float dTime )
{
	BW_GUARD;
	Vector4 v4;
	Vector4Providers::iterator vit = v4Providers_.begin();
	Vector4Providers::iterator ven = v4Providers_.end();
	while (vit != ven)
	{
		vit->second->output(v4);
		vit->first->be(v4);
		++vit;
	}

	Matrix m;
	MatrixProviders::iterator mit = matrixProviders_.begin();
	MatrixProviders::iterator men = matrixProviders_.end();
	while (mit != men)
	{
		mit->second->matrix(m);
		mit->first->be(m);
		++mit;
	}
}


/**
 * This method returns a python attribute for PyMaterial.  It overrides the
 * default pyGetAttribute method and allows the script programmer direct
 * access to D3DEffect variables
 * @param	attr	name of the attribute
 * @return	PyObject returned python attribute object, or None
 */
ScriptObject PyMaterial::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;
	D3DXHANDLE param;
	D3DXPARAMETER_DESC propertyDesc;
	Moo::EffectPropertyPtr prop;
	
	const char * attr = attrObj.c_str();

	if (this->findPropertyAndDesc(attr,prop,param,propertyDesc))
	{
		//First check our providers to see if we have one registered
		//for this property.
		Vector4Providers::iterator vit = v4Providers_.find(prop);
		if (vit != v4Providers_.end())
		{
			return ScriptObject::createFrom(vit->second);
		}

		MatrixProviders::iterator mit = matrixProviders_.find(prop);
		if (mit != matrixProviders_.end())
		{
			return ScriptObject::createFrom(mit->second);
		}

		//Otherwise we will get the data out of the property iteself.
		switch( propertyDesc.Type )
		{
		case D3DXPT_BOOL:
			bool b;
			prop->getBool(b);
			return ScriptObject::createFrom(b);
		case D3DXPT_INT:
			int i;
			prop->getInt(i);
			return ScriptObject::createFrom(i);
		case D3DXPT_FLOAT:
			if (propertyDesc.Class == D3DXPC_VECTOR)
			{
				Vector4 v;
				prop->getVector(v);
				return ScriptObject::createFrom(v);
			}
			else if ((propertyDesc.Class == D3DXPC_MATRIX_ROWS ||
				propertyDesc.Class == D3DXPC_MATRIX_COLUMNS))
			{
				Matrix m;
				prop->getMatrix(m);
				return ScriptObject::createFrom(m);
			}
			else
			{
				float f;
				prop->getFloat(f);
				return ScriptObject::createFrom(f);
			}
		case D3DXPT_TEXTURE:
		case D3DXPT_TEXTURECUBE:
			{
				BW::string r;
				prop->getResourceID(r);
				if (!r.empty())
				{
					Moo::BaseTexturePtr pTex = Moo::TextureManager::instance()->get(r);
					return ScriptObject(
							new PyTextureProvider( NULL, pTex ),
							ScriptObject::FROM_NEW_REFERENCE );
				}
				else
				{
					return ScriptObject::none();
				}
			}
			break;
		default:
			PyErr_SetString( PyExc_TypeError, "BigWorld.PyMaterial: "
				"Could not determine the type of the requested material attribute." );
			return ScriptObject();
		}
	}

	return PyObjectPlus::pyGetAttribute( attrObj );
}


/**
 * This method sets a python attribute for PyMaterial.  It overrides the
 * default pySetAttribute method and allows the script programmer direct
 * control over D3DEffect variables
 * @param	attr	name of the attribute to set
 * @param	value	python object containing the value to set the attribute to.
 * @return	bool	true for success, false for failure.
 */
bool PyMaterial::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	BW_GUARD;

	D3DXHANDLE param;
	D3DXPARAMETER_DESC propertyDesc;
	Moo::EffectPropertyPtr prop;
	
	const char * attr = attrObj.c_str();

	if (this->findPropertyAndDesc( attr, prop, param, propertyDesc ))
	{
		//First check our providers to see if we have one registered
		//for this property, and remove it.
		Vector4Providers::iterator vit = v4Providers_.find( prop );
		if (vit != v4Providers_.end())
		{
			v4Providers_.erase(vit);
		}

		MatrixProviders::iterator mit = matrixProviders_.find(prop);
		if (mit != matrixProviders_.end())
		{
			matrixProviders_.erase(mit);
		}

		ScriptErrorPrint scriptErrorPrint( "PyMaterial::pySetAttribute" );
		switch( propertyDesc.Type )
		{
		case D3DXPT_BOOL:
			bool b;
			value.convertTo( b, scriptErrorPrint );
			prop->be(b);
			return true;
		case D3DXPT_INT:
			int i;
			value.convertTo( i, scriptErrorPrint );
			prop->be(i);
			return true;
		case D3DXPT_FLOAT:
			if (propertyDesc.Class == D3DXPC_VECTOR)
			{
				Vector4 v;
				if (Vector4Provider::Check(value))
				{
					//INFO_MSG( "material property %s is now a Vector4Provider", propertyDesc.Name );
					Vector4Provider* v4Prov = static_cast<Vector4Provider*>( value.get() );
					v4Providers_[prop] = v4Prov;					
					v4Prov->output(v);
				}
				else
				{
					//INFO_MSG( "material property %s is not a Vector4Provider", propertyDesc.Name );
					value.convertTo( v, scriptErrorPrint );
				}
				prop->be(v);
				return true;
			}
			else if ((propertyDesc.Class == D3DXPC_MATRIX_ROWS ||
				propertyDesc.Class == D3DXPC_MATRIX_COLUMNS))
			{
				Matrix m;
				if (MatrixProvider::Check(value))
				{
					//INFO_MSG( "material property %s is now a MatrixProvider", propertyDesc.Name );
					MatrixProvider* mProv = static_cast<MatrixProvider*>( value.get() );
					matrixProviders_[prop] = mProv;
					mProv->matrix(m);
				}
				else
				{
					INFO_MSG( "material property %s is not a MatrixProvider", propertyDesc.Name );
					value.convertTo( m, scriptErrorPrint );
				}				
				prop->be( m );
				return true;
			}
			else
			{
				if (Vector4Provider::Check(value))
				{
					//INFO_MSG( "material property %s is now a Vector4Provider", propertyDesc.Name );
					Vector4 v;
					Vector4Provider* v4Prov = static_cast<Vector4Provider*>(value.get());
					v4Providers_[prop] = v4Prov;
					v4Prov->output(v);
					prop->be(v);
				}
				else if (PyVector<Vector4>::Check(value))
				{
					//INFO_MSG( "material property %s is not a Vector4Provider", propertyDesc.Name );
					Vector4 v;
					value.convertTo( v, scriptErrorPrint );
					prop->be( v.w );
				}
				else
				{
					//INFO_MSG( "material property %s is not a Vector4Provider", propertyDesc.Name );
					float f;
					value.convertTo( f, scriptErrorPrint );
					prop->be( f );
				}
				return true;
			}
		case D3DXPT_TEXTURE:
		case D3DXPT_TEXTURECUBE:
			{
				if (PyTextureProvider::Check(value))
				{
					PyTextureProvider * pTex = static_cast<PyTextureProvider*>(value.get());
					prop->be(pTex->texture());
					return true;
				}
				else
				{
					BW::string s;
					value.convertTo( s, scriptErrorPrint );
					prop->be( s );
					return true;
				}
			}
			break;
		default:
			PyErr_SetString( PyExc_TypeError, "BigWorld.PyMaterial: "
				"Could not determine the type of the requested material attribute." );
			return false;
		}
	}

	return PyObjectPlus::pySetAttribute( attrObj, value );
}

void PyMaterial::pyAdditionalMembers( const ScriptList & pList ) const
{
	Moo::EffectMaterial::Properties::iterator it = pMaterial_->properties().begin();
	Moo::EffectMaterial::Properties::iterator en = pMaterial_->properties().end();
	for( ; it != en; it++ )
	{
		D3DXHANDLE param = it->first;
		D3DXPARAMETER_DESC paramDesc;
		pMaterial_->pEffect()->pEffect()->GetParameterDesc( param, &paramDesc );
		pList.append( ScriptString::create( paramDesc.Name ) );
	}
}

PyObject* PyMaterial::pyNew(PyObject* args)
{
	BW_GUARD;
	PyObject * pObj = NULL;
	char * effectName = NULL;
	char * diffuseMap = NULL;
	Moo::EffectMaterial * m = NULL;

	if (PyArg_ParseTuple( args, "s|s", &effectName, &diffuseMap ))
	{
		if (effectName != NULL)
		{
			m = new Moo::EffectMaterial();
			bool ok = false;
			if (diffuseMap != NULL)
			{
				ok = m->initFromEffect(effectName, diffuseMap);
			}
			else
			{
				ok = m->initFromEffect(effectName);
			}

			if (ok)
			{
				//effect materials by default share their properties, unless
				//created via a material section, in which the listed
				//properties in the material section become instanced.  In
				//this case, we have created a material directly based on an
				//.fx file, so we need to replace all default properties
				//with instanced ones.
				m->replaceDefaults();
			}
			else
			{
				bw_safe_delete(m);
				PyErr_SetString( PyExc_TypeError, "BigWorld.PyMaterial: "
					"Error creating material from effect filename." );
				return NULL;
			}
		}
	}
	else if (PyArg_ParseTuple(args, "O", &pObj ))
	{
		//Failure of ParseTuple above sets the python error string,
		//but its ok to fail that test.
		PyErr_Clear();

		if ( pObj != NULL && PyDataSection::Check(pObj) )
		{
			PyDataSection * pSect = static_cast<PyDataSection*>(pObj);		
			m = new Moo::EffectMaterial();
			if (!m->load( pSect->pSection() ))
			{
				bw_safe_delete(m);
				PyErr_SetString( PyExc_TypeError, "BigWorld.PyMaterial: "
					"Error loading material from data section." );
				return NULL;
			}
		}
	}

	if (!m)
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.PyMaterial: "
			"Argument parsing error: Expected a d3dEffect filename or a material data section. "
			"If using d3dEffect filename, you may also specify a diffuse map name." );
		return NULL;
	}

	return new PyMaterial( m );
}


PY_SCRIPT_CONVERTERS( PyMaterial )

BW_END_NAMESPACE

// py_material.cpp
