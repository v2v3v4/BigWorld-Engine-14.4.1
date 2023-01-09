#include "pch.hpp"
#include "vertex_element_special_cases.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
void VertexElement::copyBlendIndicesUbyte3FromFloat1( void * dst, const void * src, size_t )
{
	Ubyte3 * ub3 = 
		reinterpret_cast< Ubyte3 * >( dst );

	const Float1 * f1 = 
		reinterpret_cast< const Float1 * >( src );

	ub3->x = static_cast< uint8 >( f1->x );
	ub3->y = ub3->x;
	ub3->z = ub3->x;
}

} // namespace Moo

BW_END_NAMESPACE

