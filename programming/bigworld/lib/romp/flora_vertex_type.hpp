#ifndef FLORA_VERTEX_TYPE_HPP
#define FLORA_VERTEX_TYPE_HPP


BW_BEGIN_NAMESPACE

#define PACK_UV	16383.f
#define PACK_FLEX 255.f
typedef short POS_TYPE;
typedef short UV_TYPE;
typedef short ANIM_TYPE;

#pragma pack(push, 1 )

/**
 * TODO: to be documented.
 */
struct FloraVertex
{
	Vector3 pos_;
	short   uv_[2];
	short   animation_[2]; //flex, animationIdx	
	Vector4 tint_; //-- tint position as xyz, and tint factor as w

	void set( const Moo::VertexXYZNUV& v )
	{		
		pos_ = v.pos_;
		uv_[0] = (UV_TYPE)(v.uv_[0] * PACK_UV);
		uv_[1] = (UV_TYPE)(v.uv_[1] * PACK_UV);
		tint_.setZero();
	}

	void flex( float f )
	{
		animation_[0] = (ANIM_TYPE)(PACK_FLEX * f);
	}

	void uv( const Vector2& uv )
	{
		uv_[0] = (UV_TYPE)(uv[0] * PACK_UV);
		uv_[1] = (UV_TYPE)(uv[1] * PACK_UV);
	}
};

#pragma pack( pop )

BW_END_NAMESPACE

#endif
