#ifndef VIEWPORT_SETTER_HPP
#define VIEWPORT_SETTER_HPP

#include "moo/moo_dx.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
/**
 *	This class is a scoped viewport setter, allowing you to
 *	easily set a new viewport on the device.
 *
 *	The previous settings are restored automatically when the
 *	class goes out of scope.
 */
	class ViewportSetter
	{
	public:
		ViewportSetter();

		ViewportSetter( uint32 width, uint32 height );

		ViewportSetter( uint32 x, uint32 y, uint32 width, uint32 height );

		ViewportSetter(
			uint32 width,
			uint32 height,
			uint32 x,
			uint32 y,
			float minZ,
			float maxZ );

		~ViewportSetter();
	private:
		void apply( DX::Viewport& v );

		DX::Viewport	oldViewport_;
		DX::Viewport	newViewport_;
	};
} //namespace Moo

BW_END_NAMESPACE

#endif	//VIEWPORT_SETTER_HPP
