#ifndef PYDYE_HPP
#define PYDYE_HPP

#include "cstdmf/bw_string.hpp"

#include "duplo/py_material_fashion.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer<class SuperModelDye> SuperModelDyePtr;

class SuperModel;

/*~ class BigWorld.PyDye
 *
 *	A dye is used to apply a particular material to a piece of geometry in a
 *  model.  Dyes are specified in the .model file(s) which the PyModel was 
 *	created from.
 *	
 *	In the model file, a &lt;dye> section is given a &lt;matter> name, which
 *  creates an extra attribute for the PyModel in python of that name.  It also
 *  contains a &lt;replaces> which should be the name of a particular 
 *	primitiveGroup in the .visual file. The &lt;dye> section also contains a
 *  list of &lt;tints>, which have a &lt;name> and a &lt;material> section.  By
 *  assigning the name (as a string) to the models named attribute
 *	that material is applied to the &lt;replaces> geometry of the model.
 *
 *	Dye attributes are special, in that when written to, they accept a string,
 *  which is the name of a tint.  On reading them, they return a PyDye object,
 *  which has a tint attribute with the name that was written.
 *
 *	This class inherits from PyFashion.
 *
 *	For example, the if the following is an extract from the model file:
 *
 *	@{
 *	&lt;dye>
 *	    &lt;matter> Chest &lt;/matter>
 *	    &lt;replaces>	Torso &lt;/replaces>  
 *	    &lt;tint>
 *	        &lt;name>	Blue &lt;/name>
 *	        &lt;material>
 *	            &lt;diffuseTexture>	bluetex.dds	&lt;/diffuseTexture>
 *	        &lt;/material>
 *	    &lt;/tint>
 *	    &lt;tint>
 *	        &lt;name>	Red &lt;/name>
 *	        &lt;material>
 *	            &lt;diffuseTexture>	redtex.dds	&lt;/diffuseTexture>
 *	        &lt;/material>
 *	    &lt;/tint>
 *	&lt;/dye>
 *	@}
 *
 *	This assumes that somewhere in the .visual file is a primitivegroup called 
 *	Torso ( the <replaces> field ).
 *	
 *	This creates a named attribute on the PyModel called chest, which can be
 *  assigned either "Red" or "Blue".  This assignment has the effect of 
 *  applying either the redtex.dds diffuse texture, or the bluetex.dds, 
 *	respectively.
 *
 *	Then, in python, if M contains the model object created from this .model
 *  file, then the following would be a possible console example:
 *
 *	@{
 *	>>> d = M.Chest
 *	>>> d
 *	PyDye at 0x0b771f770
 *	>>> dir(d)
 *	[ 'matter', 'tint' ]
 *	>>> d.matter
 *	'Chest'
 *	>>> d.tint
 *	'Default'
 *	>>> M.Chest = "Red" # applies redtex.dds to the model's Torso
 *	>>> d = M.Chest
 *	>>> d
 *	PyDye at 0x0b771f850
 *	>>> d.matter
 *	'Chest'
 *	>>> d.tint
 *	'Red'
 *	@}
 */
/**
 *	This class represents a dye (or rather, the potential of configuring
 *	a dye in one particular matter) to the python environment.
 */
class PyDye : public BW::PyMaterialFashion
{
	Py_Header( PyDye, BW::PyMaterialFashion )

public:
	PyDye( SuperModelDyePtr pDye,
		const BW::string & matter, const BW::string & tint,
		PyTypeObject * pType = &PyDye::s_type_ );
	~PyDye();

	const MaterialFashionPtr materialFashion() const;
	MaterialFashionPtr materialFashion();

	virtual PyDye * makeCopy( PyModel * pModel, const char * attrName );
	PyDye * makeCopy( SuperModel & superModel, const char * attrName );

	const BW::string & matter() const	{ return matter_; }
	const BW::string & tint() const	{ return tint_; }

	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );

	PY_RO_ATTRIBUTE_DECLARE( matter_, matter )
	PY_RO_ATTRIBUTE_DECLARE( tint_, tint )

private:
	void copyProperties( const PyDye & other );
	int findProperty( const char * name );

	SuperModelDyePtr	pDye_;

	const BW::string	matter_;
	const BW::string	tint_;

	typedef BW::vector< Vector4ProviderPtr > DynamicPropValues;
	DynamicPropValues	dynamicPropValues_;

	PyDye( const PyDye& );
	PyDye& operator=( const PyDye& );
};

BW_END_NAMESPACE

#endif // PYDYE_HPP
