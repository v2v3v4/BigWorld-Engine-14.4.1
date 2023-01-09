#include "pch.hpp"

#include "gui_shader.hpp"

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )


BW_BEGIN_NAMESPACE

PY_TYPEOBJECT( GUIShader )

PY_BEGIN_METHODS( GUIShader )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( GUIShader )
PY_END_ATTRIBUTES()


template <> GUIShaderFactory::ObjectMap * GUIShaderFactory::pMap_;



/**
 *	Constructor
 */
GUIShader::GUIShader( PyTypeObject * pType )
:PyObjectPlus( pType )
{
	BW_GUARD;	
}


/**
 *	Destructor
 */
GUIShader::~GUIShader()
{
	BW_GUARD;	
}


/**
 *	This method processes a GUI component, applying this shader to its vertices
 *
 *	@param component	The component to process
 *	@param dTime		The delta time for the current frame of the application
 *
 *	@return false		To stop processing child components.
 */
bool
GUIShader::processComponent( SimpleGUIComponent& component, float dTime )
{
	BW_GUARD;
	return false;
}

BW_END_NAMESPACE

// gui_shader.cpp
