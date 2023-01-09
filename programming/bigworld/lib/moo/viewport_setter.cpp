#include "pch.hpp"
#include "viewport_setter.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

/**
 *	This constructor takes no arguments.  It simply rememebers the current
 *	viewport state, and sets it on the device when the object goes out of scope.
 */
ViewportSetter::ViewportSetter()
{
	BW_GUARD;
	rc().getViewport( &oldViewport_ );
}


/**
 *	This constructor takes a width and height and sets a new
 *	temporary viewport on the device using the passed in values
 *	and the existing values of the old viewport.
 *
 *	@param	width		new viewport width.
 *	@param	height		new viewport height.
 */
ViewportSetter::ViewportSetter(  uint32 width, uint32 height )
{
	BW_GUARD;
	rc().getViewport( &oldViewport_ );
	newViewport_ = oldViewport_;
	newViewport_.Width = width;
	newViewport_.Height = height;
	this->apply( newViewport_ );
}


/**
 *	This constructor takes x and y, width, height and sets a new
 *	temporary viewport on the device using the passed in values
 *	and the existing values of the old viewport.
 *
 *	@param	x			x pixel offset of top left corner of new viewport.
 *	@param	y			y pixel offset of top left corner of new viewport. 
 *	@param	width		new viewport width.
 *	@param	height		new viewport height.
 */
ViewportSetter::ViewportSetter( uint32 x, uint32 y, uint32 width, uint32 height )
{
	BW_GUARD;
	rc().getViewport( &oldViewport_ );
	newViewport_.Width = width;
	newViewport_.Height = height;
	newViewport_.X = x;
	newViewport_.Y = y;
	newViewport_.MinZ = oldViewport_.MinZ;
	newViewport_.MaxZ = oldViewport_.MaxZ;
	this->apply( newViewport_ );
}


/**
 *	This constructor takes x and y, width, height and min and max Z
 *	and sets a new temporary viewport on the device using the passed
 *	in values.
 *
 *	@param	x			x pixel offset of top left corner of new viewport.
 *	@param	y			y pixel offset of top left corner of new viewport. 
 *	@param	width		new viewport width.
 *	@param	height		new viewport height.
 *	@param	minZ		new viewport minimum Z value.
 *	@param	maxZ		new viewport maximum Z value.
 */
ViewportSetter::ViewportSetter(
	uint32 width,
	uint32 height,
	uint32 x,
	uint32 y,
	float minZ,
	float maxZ )
{
	BW_GUARD;
	rc().getViewport( &oldViewport_ );	
	newViewport_.Width = width;
	newViewport_.Height = height;
	newViewport_.X = x;
	newViewport_.Y = y;
	newViewport_.MinZ = minZ;
	newViewport_.MaxZ = maxZ;
	this->apply( newViewport_ );
}


/**
 *	This destructor automatically resets the saved viewport when
 *	the class goes out of scope.
 */
ViewportSetter::~ViewportSetter()
{
	BW_GUARD;
	this->apply( oldViewport_ );
}


/**
 *	This method applies the viewport on the device.
 *
 *	@param	v	Viewport to apply to the device.
 */
void ViewportSetter::apply( DX::Viewport& v )
{
	BW_GUARD;
	rc().setViewport( &v );
	rc().screenWidth( v.Width );
	rc().screenHeight( v.Height );
}

}	//namespace Moo

BW_END_NAMESPACE

// viewport_setter.cpp

