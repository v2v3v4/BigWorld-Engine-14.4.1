#ifndef GUI_SHADER_HPP
#define GUI_SHADER_HPP

#include "cstdmf/named_object.hpp"
#include "cstdmf/smartpointer.hpp"
#include "pyscript/pyobject_plus.hpp"


BW_BEGIN_NAMESPACE

typedef SmartPointer<class DataSection> DataSectionPtr;

class SimpleGUIComponent;
class GUIShader;

/**
 *	This class keeps the factory methods for all types of GUIShaders
 */
typedef NamedObject<GUIShader * (*)()> GUIShaderFactory;

#define SHADER_FACTORY_DECLARE( CONSTRUCT )								\
	static GUIShaderFactory s_factory;									\
	virtual GUIShaderFactory & factory() { return s_factory; }			\
	static GUIShader * s_create() { return new CONSTRUCT; }				\

#define SHADER_FACTORY( CLASS )											\
	GUIShaderFactory CLASS::s_factory( #CLASS, CLASS::s_create );		\


/*~ class GUI.GUIShader
 *	@components{ client, tools }
 *
 *	This is the abstract base class for all GUI shaders. A GUI shader is
 *	something that you apply to a GUI component, and it modifies the way
 *	in which the component is drawn. It also modifies the way that components
 *	children are drawn.
 *
 *	GUIShaders are added to components using the SimpleGUIComponent.addShader
 *  method and removed using the SimpleGUIComponent.delShader method.  All
 *  other gui components inherit these from SimpleGUIComponent.
 */
/**
 *	This is the abstract base class for all GUI shaders. A GUI shader is
 *	something that you apply to a GUI component, and it modifies the way
 *	in which the component is drawn. It's a kind of filter.
 */
class GUIShader : public PyObjectPlus
{
	Py_Header( GUIShader, PyObjectPlus )

public:
	GUIShader( PyTypeObject * pType );
	virtual ~GUIShader();

	//modifies verts - so use a temporary buffer
	//return true to continue processing children,
	//else false to return.
	virtual bool	processComponent( SimpleGUIComponent& component, float dTime );

	//All GUI shaders have at least one constant, a value.
	virtual void	value( float v ) = 0;

	virtual bool	load( DataSectionPtr pSect ) { return true; }
	virtual void	save( DataSectionPtr pSect ) { }

private:
	GUIShader(const GUIShader&);
	GUIShader& operator=(const GUIShader&);

	virtual GUIShaderFactory & factory() = 0;
	friend class SimpleGUIComponent;
};

typedef SmartPointer<GUIShader> GUIShaderPtr;


BW_END_NAMESPACE

#endif // GUI_SHADER_HPP
