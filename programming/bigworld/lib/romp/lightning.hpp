#ifndef LIGHTNING_HPP
#define LIGHTNING_HPP

BW_BEGIN_NAMESPACE

class Lightning
{
public:
	void lightningStrike( const Vector3 & top );
	Vector4	decideLightning( float dTime );	
private:
	float conflict_;
};

BW_END_NAMESPACE

#endif
