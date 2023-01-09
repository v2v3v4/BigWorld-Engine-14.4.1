#ifndef VERTEX_FORMATS_HPP
#define VERTEX_FORMATS_HPP

#include "moo_math.hpp"

#ifdef MF_SERVER
#define FVF( FORMAT )
#else
#define FVF( FORMAT ) static int fvf() { return FORMAT; }
#endif

BW_BEGIN_NAMESPACE

namespace Moo
{

//-------------------------------------------------------------------------
// GPU Vertex Packing selection and typedefs
//-------------------------------------------------------------------------
//
//	use half precision or even ubyte4 vectors for normals and float16_2
//	for the texture coordinates instead full precision to save video 
//	memory and bandwidth:
//		  format			   bytes		  freed memory %
//	- VertexXYZNUV			( 32 / 20 )			- 37.5%
//	- VertexXYZNUV2			( 40 / 24 )			- 40.0%
//	- VertexXYZNUVTB		( 56 / 28 )			- 50.0%
//	- VertexXYZNUV2TB		( 64 / 32 )			- 50.0%
//	- VertexXYZNUVIIIWWTB	( 64 / 36 )			- 43.7%
  
// The choice of the difference packing rules are done via vertex packing sets.
// Only one packing choice can be made from each semantic category. E.g.
// Only one normals packing method and only one texcoords packing method.
  
// Notation/Naming Scheme of vertex types:
//	 No Suffix -	Format used for loading and/or manipulating vertices on 
//					the CPU side prior to possible conversion to a GPU/"PC"
//					vertex format for sending to the GPU.
//	 "PC" Suffix -	Used to represent the data layout on the GPU side as the 
//					vertex shader sees them. This may eventually be replaced 
//					with a "GPU" prefix.
//--------------------------------------------------------------------------------------------------

//
// Packing choices need to be reflected in the shader definitions 
// (vertex_declarations.fxh, stdinclude.fxh), in relevant 
// format XML definition files for relevant formats, with XML names
// matched up using corresponding vertex format naming traits types.

// Set all vertex packing choices via packing sets so that we can
// correlate these choices correctly to entire type definitions.
#define BW_GPU_VERTEX_PACKING_SET 1

//-------------------------------------------------------------------------
// preset vertex packing sets.
#if BW_GPU_VERTEX_PACKING_SET == 1
	#define BW_GPU_VERTEX_PACKING_USE_VEC3_NORMAL_UBYTE4_8_8_8		1
	#define BW_GPU_VERTEX_PACKING_USE_VEC2_TEXCOORD_INT16_X2		1

#elif BW_GPU_VERTEX_PACKING_SET == 2
	// these types currently rely on D3D and cant necessarily be used 
	// on the server or all offline tools yet.
	#define BW_GPU_VERTEX_PACKING_USE_VEC3_NORMAL_FLOAT16_X4		1
	#define BW_GPU_VERTEX_PACKING_USE_VEC2_TEXCOORD_FLOAT16_X2		1
	BW_STATIC_ASSERT( false, Selected_vertex_packing_set_unsupported );

#else
	BW_STATIC_ASSERT( false, Selected_vertex_packing_set_unsupported );

#endif

//-------------------------------------------------------------------------
// normals packing
#if BW_GPU_VERTEX_PACKING_USE_VEC3_NORMAL_UBYTE4_8_8_8
	typedef PackedVector3NormalUByte4_8_8_8		GPUNormalVector3;
#elif BW_GPU_VERTEX_PACKING_USE_VEC3_NORMAL_FLOAT16_X4
	typedef PackedVector3NormalFloat16_X4		GPUNormalVector3;
#else
	typedef Vector3								GPUNormalVector3
#endif

//-------------------------------------------------------------------------
// texture coordinates packing
#if BW_GPU_VERTEX_PACKING_USE_VEC2_TEXCOORD_INT16_X2
	typedef PackedVector2TexcoordInt16_X2		GPUTexcoordVector2;
#elif BW_GPU_VERTEX_PACKING_USE_VEC2_TEXCOORD_FLOAT16_X2
	typedef PackedVector2TexcoordFloat16_X2		GPUTexcoordVector2;
#else
	typedef Vector2								GPUTexcoordVector2
#endif


//-----------------------------------------------------------------------------
#pragma pack(push, 1 )
	

	/**
	 * Position, Normal, UV
	 */
	struct VertexXYZNUV
	{
		Vector3		pos_;
		Vector3		normal_;
		Vector2		uv_;

		FVF( D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1 )

		VertexXYZNUV( const Vector3 & pos, const Vector2 & uv ) 
		: pos_(pos)
		, uv_(uv)
		{
		}

		VertexXYZNUV()
		{
		}
	};
	


	//-- Position, Normal, UV
	//----------------------------------------------------------------------------------------------
	struct VertexXYZNUVPC
	{
		Vector3				pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;

		VertexXYZNUVPC & operator=( const VertexXYZNUV & in )
		{
			pos_ = in.pos_; 
			normal_ = in.normal_; 
			uv_ = in.uv_;
			return *this;
		}

		VertexXYZNUVPC(const VertexXYZNUV & in)
		{
			*this = in;
		}
		
		VertexXYZNUVPC(const Vector3 & pos, const Vector2 & uv) 
		: pos_(pos), 
		uv_(uv)
		{
		}
		
		VertexXYZNUVPC()
		{
		}
	};
	


	/**
	* Position, Normal, UV, UV2
	*/
	struct VertexXYZNUV2
	{
		Vector3		pos_;
		Vector3		normal_;
		Vector2		uv_;
		Vector2		uv2_;

		FVF( D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX2 )
	};


	/**
	* Position, Normal (packed), UV (packed), UV2 (packed)
	*/

	struct VertexXYZNUV2PC
	{
		Vector3				pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		GPUTexcoordVector2	uv2_;


		VertexXYZNUV2PC & operator=( const VertexXYZNUV2 & in )
		{
			pos_ = in.pos_; 
			normal_ = in.normal_; 
			uv_ = in.uv_; 
			uv2_ = in.uv2_;
			return *this;
		}

		VertexXYZNUV2PC( const VertexXYZNUV2 & in )
		{
			*this = in;
		}

		VertexXYZNUV2PC()
		{
		}
	};


	/**
	* Position, Normal, Colour, UV
	*/
	struct VertexXYZNDUV
	{
		Vector3		pos_;
		Vector3		normal_;
		DWORD		colour_;
		Vector2		uv_;

		FVF( D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1 )
	};
	

	/**
	* Position, Normal
	*/
	struct VertexXYZN
	{
		Vector3		pos_;
		Vector3		normal_;

		FVF( D3DFVF_XYZ|D3DFVF_NORMAL )
	};
	

	/**
	* Position, Normal, Colour
	*/
	struct VertexXYZND
	{
		Vector3		pos_;
		Vector3		normal_;
		DWORD		colour_;

		FVF( D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE )
	};


	/**
	* Position, Normal, Colour, Specular
	*/
	struct VertexXYZNDS
	{
		Vector3		pos_;
		Vector3		normal_;
		DWORD		colour_;
		DWORD		specular_;

		FVF( D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_SPECULAR )
	};
	

	/**
	* Position, Colour
	*/
	struct VertexXYZL
	{
		Vector3		pos_;
		DWORD		colour_;

		FVF( D3DFVF_XYZ|D3DFVF_DIFFUSE )
	};

	/**
	* Position only
	*/
	struct VertexXYZ
	{
		Vector3		pos_;

		FVF( D3DFVF_XYZ )
	};

	/**
	* Position, UV
	*/
	struct VertexXYZUV
	{
		Vector3		pos_;
		Vector2		uv_;

		FVF( D3DFVF_XYZ|D3DFVF_TEX1 )
	};
	

	/**
	* Position, UV, UV2
	*/
    struct VertexXYZUV2
    {
	    Vector3		pos_;
	    Vector2		uv_;
	    Vector2		uv2_;

	    FVF( D3DFVF_XYZ|D3DFVF_TEX2 )
    };

	/**
	* Position, UV, UV2, UV3, UV4
	*/
	struct VertexUV4
	{
		Vector4		pos_;
		Vector2		uv_[4];

		FVF( D3DFVF_XYZRHW|D3DFVF_TEX4 )
	};

	/**
	* Position, Colour, UV
	*/
	struct VertexXYZDUV
	{
		Vector3		pos_;
		DWORD		colour_;
		Vector2		uv_;

		FVF( D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1 )
	};


	/**
	* Position, Colour, UV, UV2
	*/
	struct VertexXYZDUV2
	{
		Vector3		pos_;
		DWORD		colour_;
		Vector2		uv_;
		Vector2		uv2_;

		FVF( D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2 )
	};
	

	/**
	* Normal, UV
	*/
	struct VertexNUV
	{
		Vector3		normal_;
		Vector2		uv_;

		FVF( D3DFVF_NORMAL|D3DFVF_TEX1 )
	};

	/**
	 * Normal only
	 */
	struct VertexN
	{
		Vector3		normal_;

		FVF( D3DFVF_NORMAL )
	};

	/**
	* UV only
	*/
	struct VertexUV
	{
		Vector2		uv_;

		FVF( D3DFVF_TEX1 )
	};

	/**
	* Position, Normal, UV, Index
	*/
	struct VertexXYZNUVI
	{
		Vector3		pos_;
		Vector3		normal_;
		Vector2		uv_;
		float		index_;
	};
	

	/**
	 * Position, Normal, UV, Index
	 * 
	 * 
	 */

	struct VertexXYZNUVIPC
	{
		Vector3				pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		float				index_;

		VertexXYZNUVIPC & operator=( const VertexXYZNUVI & in )
		{
			pos_ = in.pos_; 
			normal_ = in.normal_; 
			uv_ = in.uv_; 
			index_ = in.index_;
			return *this;
		}

		VertexXYZNUVIPC( const VertexXYZNUVI & in )
		{
			*this = in;
		}

		VertexXYZNUVIPC()
		{
		}
	};


	/**
	 * Y, Normal
	 */
	struct VertexYN
	{
		float		y_;
		Vector3		normal_;
	};

	/**
	 * Four-component position, Colour
	 */
	struct VertexTL
	{
		Vector4		pos_;
		DWORD		colour_;

		FVF( D3DFVF_XYZRHW|D3DFVF_DIFFUSE )
	};

	/**
	* Four-component position, UV
	*/
	struct VertexTUV
	{
		Vector4		pos_;
		Vector2		uv_;
		FVF( D3DFVF_XYZRHW|D3DFVF_TEX1 )
	};

	/**
	* Four-component position, Colour, UV
	*/
	struct VertexTLUV
	{
		Vector4		pos_;
		DWORD		colour_;
		Vector2		uv_;
		FVF( D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1 )
	};

	/**
	* Four-component position, Colour, 2 UVs
	*/
	struct VertexTLUV2
	{
		Vector4		pos_;
		DWORD		colour_;
		Vector2		uv_;
		Vector2		uv2_;
		FVF( D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX2 )
	};

	/**
	* Four-component position, Colour, Specular, UV, UV2
	*/
	struct VertexTDSUV2
	{
		Vector4		pos_;
		DWORD		colour_;
		DWORD		specular_;
		Vector2		uv_;
		Vector2		uv2_;

		FVF( D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX2 )
	};

	/**
	* Position, Colour, Specular, UV
	*/
	struct VertexXYZDSUV
	{
		float	x;
		float	y;
		float	z;
		uint32	colour;
		uint32	spec;
		float	tu;
		float	tv;

		FVF( D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1 )
	};

	/**
	* Position, Colour, Specular, UVUUVV, UVUUVV
	*/
	struct VertexTDSUVUUVV2
	{
		Vector4		pos_;
		DWORD		colour_;
		DWORD		specular_;
		Vector4		uvuuvv_;
		Vector4		uvuuvv2_;
	};

	

	/**
	* Position, Normal, UV, Packed Tangent, Packed Binormal
	*/
	struct VertexXYZNUVTB
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		uint32		tangent_;
		uint32		binormal_;
	};

	/**
	* Position, Packedv2 Normal, packed UV, Packedv2 Tangent, Packedv2 Binormal
	*/
	struct VertexXYZNUVTBPC
	{
		Vector3				pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		GPUNormalVector3	tangent_;
		GPUNormalVector3	binormal_;

		VertexXYZNUVTBPC& operator =(const VertexXYZNUVTB& in)
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			tangent_ = unpackNormal( in.tangent_ );
			binormal_ = unpackNormal( in.binormal_ );
			return *this;
		}
		VertexXYZNUVTBPC(const VertexXYZNUVTB& in)
		{
			*this = in;
		}
		VertexXYZNUVTBPC& operator =(const VertexXYZNUV& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			tangent_.setZero();
			binormal_.setZero();
			return *this;
		}
		VertexXYZNUVTBPC(const VertexXYZNUV& in)
		{
			*this = in;
		}
		VertexXYZNUVTBPC()
		{
		}
	};


	/**
	* Position, Normal, UVx2, Packed Tangent, Packed Binormal
	*/
	struct VertexXYZNUV2TB
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		Vector2		uv2_;
		uint32		tangent_;
		uint32		binormal_;
	};
	


	/**
	* Position, Normal, UVx2, Tangent, Binormal
	*/
	struct VertexXYZNUV2TBPC
	{
		Vector3				pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		GPUTexcoordVector2	uv2_;
		GPUNormalVector3	tangent_;
		GPUNormalVector3	binormal_;

		VertexXYZNUV2TBPC& operator =(const VertexXYZNUV2TB& in)
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			uv2_ = in.uv2_;
			tangent_ = unpackNormal( in.tangent_ );
			binormal_ = unpackNormal( in.binormal_ );
			return *this;
		}
		VertexXYZNUV2TBPC(const VertexXYZNUV2TB& in)
		{
			*this = in;
		}
		VertexXYZNUV2TBPC& operator =(const VertexXYZNUV2& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			uv2_ = in.uv2_;
			tangent_.setZero();
			binormal_.setZero();
			return *this;
		}
		VertexXYZNUV2TBPC(const VertexXYZNUV2& in)
		{
			*this = in;
		}
		VertexXYZNUV2TBPC()
		{
		}
	};
	

	/**
	* Position, Normal, UV, Index, Packed Tangent, Packed Binormal
	*/
	struct VertexXYZNUVITB
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		float		index_;
		uint32		tangent_;
		uint32		binormal_;
	};


	/**
	* Position, packed Normal, packed UV, Index, packed Tangent, packed Binormal
	*/
	struct VertexXYZNUVITBPC
	{
		Vector3				pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		float				index_;
		GPUNormalVector3	tangent_;
		GPUNormalVector3	binormal_;

		VertexXYZNUVITBPC& operator =(const VertexXYZNUVITB& in)
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			index_ = in.index_;
			tangent_ = unpackNormal( in.tangent_ );
			binormal_ = unpackNormal( in.binormal_ );
			return *this;
		}
		VertexXYZNUVITBPC(const VertexXYZNUVITB& in)
		{
			*this = in;
		}
		VertexXYZNUVITBPC()
		{
		}
	};
	

	/**
	* Position, Normal, UV, index, index2, index3, weight, weight2
	*/
	struct VertexXYZNUVIIIWW_v2
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		uint8		index_;
		uint8		index2_;
		uint8		index3_;
		uint8		weight_;
		uint8		weight2_;
	};


	/**
	* Position, Normal, UV, index, index2, index3, weight, weight2
	*/
	struct VertexXYZNUVIIIWW
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		uint8		index_;
		uint8		index2_;
		uint8		index3_;
		uint8		weight_;
		uint8		weight2_;
	
		VertexXYZNUVIIIWW& operator =(const VertexXYZNUVI& in)
		{
			pos_ = in.pos_;
			normal_ = packNormal(in.normal_);
			uv_ = in.uv_;
			weight_ = 255;
			weight2_ = 0;
			index_ = (uint8)in.index_;
			index2_ = index_;
			index3_ = index_;
			return *this;
		}
		VertexXYZNUVIIIWW& operator =(const VertexXYZNUVIIIWW_v2& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			index_ = in.index_ * 3;
			index2_ = in.index2_ * 3;
			index3_ = in.index3_ * 3;
			weight_ = in.weight_;
			weight2_ = in.weight2_;
			return *this;
		}
	};
	


	/**
	* Position, Normal, UV1, UV2, index, index2, index3, weight, weight2
	*/
	struct VertexXYZNUV2IIIWW
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		Vector2		uv2_;
		uint8		index_;
		uint8		index2_;
		uint8		index3_;
		uint8		weight_;
		uint8		weight2_;
		
		VertexXYZNUV2IIIWW& operator =(const VertexXYZNUVI& in)
		{
			pos_ = in.pos_;
			normal_ = packNormal(in.normal_);
			uv_ = in.uv_;
			weight_ = 255;
			weight2_ = 0;
			index_ = (uint8)in.index_;
			index2_ = index_;
			index3_ = index_;
			return *this;
		}
		//VertexXYZNUV2IIIWW& operator =(const VertexXYZNUV2I& in)
		//{
		//	pos_ = in.pos_;
		//	normal_ = packNormal(in.normal_);
		//	uv_ = in.uv_;
		//	uv2_ = in.uv2_;
		//	weight_ = 255;
		//	weight2_ = 0;
		//	index_ = (uint8)in.index_;
		//	index2_ = index_;
		//	index3_ = index_;
		//	return *this;
		//}
	};


	/**
	* Position, Normal, UV, index, index2, index3, weight, weight2, 
	*	packed tangent, packed binormal.
	*/
	struct VertexXYZNUVIIIWWTB_v2
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		uint8		index_;
		uint8		index2_;
		uint8		index3_;
		uint8		weight_;
		uint8		weight2_;
		uint32		tangent_;
		uint32		binormal_;
	};
	

	/**
	* Position, Normal, UV, index, index2, index3, weight, weight2, 
	*	packed tangent, packed binormal.
	*/
	struct VertexXYZNUVIIIWWTB
	{
		Vector3		pos_;
		uint32		normal_;
		Vector2		uv_;
		uint8		index_;
		uint8		index2_;
		uint8		index3_;
		uint8		weight_;
		uint8		weight2_;
		uint32		tangent_;
		uint32		binormal_;
		
		VertexXYZNUVIIIWWTB& operator =(const VertexXYZNUVITB& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			weight_ = 255;
			weight2_ = 0;
			index_ = (uint8)in.index_;
			index2_ = index_;
			index3_ = index_;
			tangent_ = in.tangent_;
			binormal_ = in.binormal_;
			return *this;
		}
		VertexXYZNUVIIIWWTB& operator =(const VertexXYZNUVIIIWWTB_v2& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			index_ = in.index_ * 3;
			index2_ = in.index2_ * 3;
			index3_ = in.index3_ * 3;
			weight_ = in.weight_;
			weight2_ = in.weight2_;
			tangent_ = in.tangent_;
			binormal_ = in.binormal_;
			return *this;
		}
	};
	

	/**
	* Position, Normal, UV, index3, index2, index, padding, weight2, weight1, 
	*	padding.
	*/
	struct VertexXYZNUVIIIWWPC
	{
		Vector3				pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		uint8		index3_;
		uint8		index2_;
		uint8		index_;
		uint8		pad_;
		uint8		pad2_;
		uint8		weight2_;
		uint8		weight_;
		uint8		pad3_;

		VertexXYZNUVIIIWWPC& operator =(const VertexXYZNUVIIIWW& in)
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			weight_ = in.weight_;
			weight2_ = in.weight2_;
			index_ = in.index_;
			index2_ = in.index2_;
			index3_ = in.index3_;
			return *this;
		}
	
		VertexXYZNUVIIIWWPC& operator =(const VertexXYZNUVI& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			weight_ = 255;
			weight2_ = 0;
			index_ = (uint8)in.index_;
			index2_ = index_;
			index3_ = index_;
			return *this;
		}
		VertexXYZNUVIIIWWPC(const VertexXYZNUVIIIWW& in)
		{
			*this = in;
		}
		VertexXYZNUVIIIWWPC()
		{
		}
	};


	/**
	* Position, Normal, UV1, UV2, index3, index2, index, padding, weight2, weight1, 
	*	padding.
	*/
	struct VertexXYZNUV2IIIWWPC
	{
		Vector3		pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		GPUTexcoordVector2	uv2_;
		uint8		index3_;
		uint8		index2_;
		uint8		index_;
		uint8		weight2_;
		uint8		weight_;
		uint8		pad3_;
		VertexXYZNUV2IIIWWPC& operator =(const VertexXYZNUVIIIWW& in)
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			weight_ = in.weight_;
			weight2_ = in.weight2_;
			index_ = in.index_;
			index2_ = in.index2_;
			index3_ = in.index3_;
			return *this;
		}
		VertexXYZNUV2IIIWWPC& operator =(const VertexXYZNUV2IIIWW& in)
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			uv2_ = in.uv2_;
			weight_ = in.weight_;
			weight2_ = in.weight2_;
			index_ = in.index_;
			index2_ = in.index2_;
			index3_ = in.index3_;
			return *this;
		}
	
		VertexXYZNUV2IIIWWPC& operator =(const VertexXYZNUVI& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			weight_ = 255;
			weight2_ = 0;
			index_ = (uint8)in.index_;
			index2_ = index_;
			index3_ = index_;
			return *this;
		}
		/*VertexXYZNUV2IIIWWPC& operator =(const VertexXYZNUV2I& in)
		{
			pos_ = in.pos_;
			normal_ = in.normal_;
			uv_ = in.uv_;
			uv_ = in.uv2_;
			weight_ = 255;
			weight2_ = 0;
			index_ = (uint8)in.index_;
			index2_ = index_;
			index3_ = index_;
			return *this;
		}*/
		VertexXYZNUV2IIIWWPC(const VertexXYZNUV2IIIWW& in)
		{
			*this = in;
		}
		VertexXYZNUV2IIIWWPC()
		{
		}
	};





	/**
	* Position, Normal, UV, index3, index2, index, padding, weight2, weight, 
	*	padding, packed tangent, packed binormal.
	*/
	struct VertexXYZNUVIIIWWTBPC
	{
		Vector3		pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_; 
		uint8		index3_;
		uint8		index2_;
		uint8		index_;
		uint8		pad_;
		uint8		pad2_;
		uint8		weight2_;
		uint8		weight_;
		uint8		pad3_;
		GPUNormalVector3	tangent_;
		GPUNormalVector3	binormal_;

		VertexXYZNUVIIIWWTBPC& operator =(const VertexXYZNUVIIIWWTB& in)
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			weight_ = in.weight_;
			weight2_ = in.weight2_;
			index_ = in.index_;
			index2_ = in.index2_;
			index3_ = in.index3_;
			tangent_ = unpackNormal( in.tangent_ );
			binormal_ = unpackNormal( in.binormal_ );
			return *this;
		}
		VertexXYZNUVIIIWWTBPC& operator =(const VertexXYZNUVITB& in)		
		{
			pos_ = in.pos_;
			normal_ = unpackNormal( in.normal_ );
			uv_ = in.uv_;
			weight_ = 255;
			weight2_ = 0;
			index_ = (uint8)in.index_;
			index2_ = index_;
			index3_ = index_;
			tangent_ = unpackNormal( in.tangent_ );
			binormal_ = unpackNormal( in.binormal_ );			
			return *this;
		}
		VertexXYZNUVIIIWWTBPC(const VertexXYZNUVIIIWWTB& in)
		{
			*this = in;
		}
		VertexXYZNUVIIIWWTBPC()
		{
		}
	};


	struct VertexTex7
	{
		Vector4		tex1;
		Vector4		tex2;
		Vector4		tex3;
		Vector4		tex4;

		Vector4		tex5;
		Vector4		tex6;
		Vector4		tex7;

		static DWORD fvf() { return 0;  }

		static BW::string decl()
		{
			return "xyznuv8tb";
		}
	};

	/**
	 * Y, Normal, Diffuse, Shadow
	 */
	struct VertexYNDS
	{
		float		y_;
		uint32		normal_;
		uint32		diffuse_;
		uint16		shadow_;
	};

	/**
	 * 16 bit U, 16 bit V
	 */
	struct VertexUVXB
	{
		int16		u_;
		int16		v_;
	};

	/**
	 * Position, Colour, float size
	 */
	struct VertexXYZDP
	{
		Vector3		pos_;
		uint32		colour_;
		float		size_;

		FVF( D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_PSIZE )
	};

	template <int N>
	class FilterVertex
	{
	public:
		Vector3		pos_;
		Vector3		uv_[N];
		Vector3		worldNormal_;
		Vector3		viewNormal_;
		static DWORD fvf() { return 0;  }
	};

	typedef FilterVertex<4>	FourTapVertex;
	typedef FilterVertex<8>	EightTapVertex;

	struct VertexXYZNUVC
	{
		Vector3	pos_;
		GPUNormalVector3	normal_;
		GPUTexcoordVector2	uv_;
		static DWORD fvf() { return 0;  }
	};

#pragma pack( pop )


//-----------------------------------------------------------------------------
/**
 * This class defines a mapping between a Vertex runtime type and its format 
 * name. Format names are used as a key by the VertexFormatCache to retrieve 
 * vertex formats from vertex format XML definitions.
 */
template <typename VertexType>
class VertexFormatDefinitionTypeTraits{};

#define DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexType, definitionName )\
template <>											\
class VertexFormatDefinitionTypeTraits< VertexType >\
{													\
public:												\
	static const char * name()						\
	{												\
		return #definitionName;						\
	}												\
};													\


//-----------------------------------------------------------------------------
/**
 * Connect our Vertex format types to their XML definition names 
 * ...based on the selected packing set.
 * 
 * This indirection lets us add new packing choices and types with corresponding
 * XML definitions without having to change existing XML definitions and
 * not tying down to a particular naming scheme.
 *
 * The result of this is that all client code that requests a VertexFormat
 * can (and should) use the definition accessor methods in VertexFormatCache
 * to make requests that use vertex format definitions. Accesses to vertex format
 * definitions via hard-coded format name strings (e.g. "xyznuv") should be
 * avoided in the future.
 */

#if BW_GPU_VERTEX_PACKING_SET == 1
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUV, xyznuv );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVPC, xyznuvpc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVC, xyznuvpc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUV2, xyznuv2 );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUV2PC, xyznuv2pc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNDUV, xyznduv );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZN, xyzn );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZND, xyznd );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNDS, xyznds );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZUV, xyzuv );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZDUV, xyzduv );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZDUV2, xyzduv2 );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVI, xyznuvi );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVIPC, xyznuvipc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVTB, xyznuvtb );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVTBPC, xyznuvtbpc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUV2TB, xyznuv2tb );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUV2TBPC, xyznuv2tbpc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVITB, xyznuvitb );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVITBPC, xyznuvitbpc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVIIIWW_v2, xyznuviiiww_v2 );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVIIIWW, xyznuviiiww );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUV2IIIWW, xyznuv2iiiww );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVIIIWWTB_v2, xyznuviiiwwtb_v2 );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVIIIWWTB, xyznuviiiwwtb );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVIIIWWPC, xyznuviiiwwpc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUV2IIIWWPC, xyznuv2iiiwwpc );
	DECLARE_VERTEX_TYPE_DEFINITION_NAME( VertexXYZNUVIIIWWTBPC, xyznuviiiwwtbpc );

#else
	BW_STATIC_ASSERT( false, Vertex_format_definitions_not_defined_for_selected_vertex_packing_set );

#endif

//-----------------------------------------------------------------------------

// Purposely not undefined since macro is used in other files e.g. vertex streams
//#undef DECLARE_VERTEX_TYPE_DEFINITION_NAME

} // namespace Moo

BW_END_NAMESPACE

#endif
