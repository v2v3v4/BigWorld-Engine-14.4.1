#ifndef VERTEX_ELEMENT_SPECIAL_CASES_HPP
#define VERTEX_ELEMENT_SPECIAL_CASES_HPP

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/stdmf.hpp"
#include "vertex_element.hpp"
#include "vertex_element_value.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{
	
namespace VertexElement
{
	//--------------------------------------------------------------------------
	// Structures and functions for special cases of vertex elements/conversions
	//--------------------------------------------------------------------------

	/// Use first index of src to fill all 3 dest components
	void copyBlendIndicesUbyte3FromFloat1( void * dst, const void * src, size_t );


	// "Source" of multiplication * 3 conversions
	// this was initially used to support increasing bone limits
	struct ElementDiv3III
	{
		uint8 index1_;
		uint8 index2_;
		uint8 index3_;

		operator Ubyte3() const
		{
			Ubyte3 ub3;
			ub3.x = index1_ * 3;
			ub3.y = index2_ * 3;
			ub3.z = index3_ * 3;
			return ub3;
		}
	};


	struct ElementReverseIII
	{
		uint8 index1_;
		uint8 index2_;
		uint8 index3_;

		ElementReverseIII( const Float1 & f1 )
			: index1_(static_cast< uint8 >(f1.x)),
			index2_(static_cast< uint8 >(f1.x)),
			index3_(static_cast< uint8 >(f1.x))
		{
		}

		ElementReverseIII( const Ubyte3 & ub3 )
			: index1_(ub3.z),
			index2_(ub3.y),
			index3_(ub3.x)
		{
		}

		operator Ubyte3() const 
		{
			Ubyte3 ub3;
			ub3.z = index1_;
			ub3.y = index2_;
			ub3.x = index3_;
			return ub3;
		}
	};


	struct ElementReversePaddedIII_
	{
		uint8 index1_;
		uint8 index2_;
		uint8 index3_;
		uint8 pad1_;

		ElementReversePaddedIII_( const Float1 & f1 )
			: index1_(static_cast< uint8 >(f1.x)),
			index2_(static_cast< uint8 >(f1.x)),
			index3_(static_cast< uint8 >(f1.x))
		{
		}

		ElementReversePaddedIII_( const Ubyte3 & ub3 )
			: index1_(ub3.z),
			index2_(ub3.y),
			index3_(ub3.x),
			pad1_(0)
		{
		}

		operator Ubyte3() const 
		{
			Ubyte3 ub3;
			ub3.z = index1_;
			ub3.y = index2_;
			ub3.x = index3_;
			return ub3;
		}
	};


	// blend weight components per vertex add up to 255.

	struct ElementReversePadded_WW_
	{
		uint8 pad1_;
		uint8 weight1_;
		uint8 weight2_;
		uint8 pad2_;

		ElementReversePadded_WW_() 
			: weight1_(0), weight2_(255)
		{
		}

		ElementReversePadded_WW_( const Ubyte2 & ub2 )
			: pad1_(0),
			weight1_(ub2.y),
			weight2_(ub2.x),
			pad2_(0)
		{
		}
	};


	struct ElementReversePaddedWW_
	{
		uint8 weight1_;
		uint8 weight2_;
		uint8 pad1_;

		ElementReversePaddedWW_() 
			: weight1_(0), weight2_(255)
		{
		}

		ElementReversePaddedWW_( const Ubyte2 & ub2 )
			: weight1_(ub2.y),
			weight2_(ub2.x),
			pad1_(0)
		{
		}
	};


	// Just an initialisation helper for Ubyte2 in blend weights case
	struct ElementInitWW : public Ubyte2
	{
		ElementInitWW()
		{
			x = 255;
			y = 0;
		}
	};


} // namespace VertexElement


} // namespace Moo

BW_END_NAMESPACE

#endif // VERTEX_ELEMENT_SPECIAL_CASES_HPP
