#include "pch.hpp"
#include "pydye.hpp"

#include "pymodel.hpp"

#include "model/super_model.hpp"
#include "model/super_model_dye.hpp"

DECLARE_DEBUG_COMPONENT2( "Model", 0 )


BW_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------
// Section: PyDye
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PyDye )

PY_BEGIN_METHODS( PyDye )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyDye )
	/*~ attribute PyDye.matter
	 *
	 *	The name of the attribute within the model file that this dye will
	 *	apply to.
	 *	
	 *	@type	Read-Only String
	 */
	PY_ATTRIBUTE( matter )
	/*~ attribute PyDye.tint
	 *
	 *	The name of the tint that specifies which material this dye will apply
	 *	to the geometry.
	 *
	 *	@type	Read-Only String
	 */
	PY_ATTRIBUTE( tint )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
PyDye::PyDye( SuperModelDyePtr pDye,
		const BW::string & matter, const BW::string & tint,
		PyTypeObject * pType ) :
	PyMaterialFashion( pType ),
	pDye_( pDye ),
	matter_( matter ),
	tint_( tint )
{
}


/**
 *	Destructor.
 */
PyDye::~PyDye()
{
}


/**
 *	This method returns a smart pointer to the underlying fashion.
 *	@return the wrapped MaterialFashion.
 */
const MaterialFashionPtr PyDye::materialFashion() const
{
	BW_GUARD;
	return const_cast<PyDye *>( this )->materialFashion();
}


/**
 *	This method returns a smart pointer to the underlying fashion.
 *	@return the wrapped MaterialFashion.
 */
MaterialFashionPtr PyDye::materialFashion()
{
	BW_GUARD;
	// we calculate and apply our dynamic property values here,
	// instead of in tick (since we might not be drawn)
	Vector4 val;
	for (DynamicPropValues::iterator it = dynamicPropValues_.begin();
		it != dynamicPropValues_.end();
		it++)
	{
		if (*it)
		{
			(*it)->output( val );
			pDye_->properties_[ it - dynamicPropValues_.begin() ].value_ = val;
		}
	}

	// now return the fashion
	return pDye_;
}


/**
 *	This method makes a copy of the dye to apply to the given supermodel
 *
 *	Note that the supermodel we were originally connected to may by now
 *	have disowned us, or even been completely unloaded, since the
 *	SuperModelDyePtr does not store a reference to anything.
 */
PyDye * PyDye::makeCopy( PyModel * pModel, const char * attrName )
{
	BW_GUARD;
	return this->makeCopy( *pModel->pSuperModel(), attrName );
}

/**
 *	Internal copy method that needs only a SuperModel
 */
PyDye * PyDye::makeCopy( SuperModel & superModel, const char * attrName )
{
	BW_GUARD;
	SuperModelDyePtr pDye = superModel.getDye( attrName, this->tint() );
	if (pDye)
	{
		PyDye * newPyDye = new PyDye( pDye, attrName, this->tint() );
		newPyDye->copyProperties( *this );
		return newPyDye;
	}
	else
	{
		PyErr_Format( PyExc_AttributeError,
			"Cannot set given Dye on Model because it has no matter %s",
			attrName );
		return NULL;
	}
}


/**
 *	Python get attribute method
 */
ScriptObject PyDye::pyGetAttribute( const ScriptString & attrObj )
{
	BW_GUARD;

	// see if it's one of our properties
	int i = this->findProperty( attrObj.c_str() );
	if (i >= 0)
	{
		if (uint(i) < pDye_->properties_.size())
		{
			if (uint(i) < dynamicPropValues_.size() && dynamicPropValues_[i])
				return ScriptObject::createFrom( dynamicPropValues_[i] );
			else
				return ScriptObject::createFrom( pDye_->properties_[i].value_ );
		}

		return ScriptObject();	// error already set
	}

	return this->PyFashion::pyGetAttribute( attrObj );
}


/**
 *	Python set attribute method
 */
bool PyDye::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	BW_GUARD;

	// see if it's one of our properties
	int i = this->findProperty( attrObj.c_str() );
	if (i >= 0)
	{
		if (uint(i) < pDye_->properties_.size())
		{
			static char propDesc[256] = "PyDye.";
			static char * propDescName = propDesc + strlen( propDesc );

			strcpy( propDescName, Model::lookupPropName(
				pDye_->properties_[i].index_ ) );
			if (Vector4Provider::Check( value ))
			{
				if (!(uint(i) < dynamicPropValues_.size()))
				{
					dynamicPropValues_.resize( i+1 );
				}
				return value.convertTo( dynamicPropValues_[i], propDesc, 
						ScriptErrorRetain() );
			}
			else
			{
				//make sure we get rid of any dynamic property values stored.
				if ((uint(i) < dynamicPropValues_.size()))
				{
					dynamicPropValues_[i] = NULL;
				}

				return value.convertTo(
						pDye_->properties_[i].value_, propDesc,
						ScriptErrorRetain() );
			}
		}

		return false;	// error already set
	}

	return this->PyFashion::pySetAttribute( attrObj, value );
}


/**
 *	This method find the index of the given property in our vector
 *	of DyePropSettings. If it's not a known property, -1 is returned.
 *	If it's known but we don't have it properties.size() is returned
 *	(and in this case it is the callers responsibility to make sure that
 *	NULL is returned to Python)
 */
int PyDye::findProperty( const char * name )
{
	BW_GUARD;
	BW::stringstream ss;
	ss << matter_ << "." << tint_ << "." << name;
	int findex = Model::getPropInCatalogue( ss .str().c_str() );
	if (findex == -1) return -1;

	uint i;
	for (i = 0; i < pDye_->properties_.size(); i++)
	{
		if (pDye_->properties_[i].index_ == findex)
			break;
	}

	// if it is a property name but not one of ours then we
	//  generate an error instead of leaving it to PyObjectPlus
	if (i >= pDye_->properties_.size())
	{	
		PyErr_Format( PyExc_AttributeError,
			"PyDye.%s no such property in this %s=%s dye "
			"(in the supermodel that created it)",
			name, matter_.c_str(), tint_.c_str() );
	}

	return i;
}


/**
 *	This method copies any matching properties from other to ourselves.
 */
void PyDye::copyProperties( const PyDye & other )
{
	BW_GUARD;
	for (uint i = 0; i < other.pDye_->properties_.size(); i++)
	{
		const DyePropSetting & now = other.pDye_->properties_[i];
		for (uint j = 0; j < pDye_->properties_.size(); j++)
		{
			if (pDye_->properties_[j].index_ == now.index_)
			{
				// ok, we have a match, copy the value
				pDye_->properties_[j].value_ = now.value_;

				// and the dynamic value if it exists
				if (i < other.dynamicPropValues_.size())
				{
					if (!(j < dynamicPropValues_.size()))
						dynamicPropValues_.resize( j+1 );
					dynamicPropValues_[j] = other.dynamicPropValues_[i];
				}
			}
		}
	}
}

BW_END_NAMESPACE

// pydye.cpp
