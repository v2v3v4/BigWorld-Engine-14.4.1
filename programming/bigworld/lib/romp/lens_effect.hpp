#ifndef LENS_EFFECT_HPP
#define LENS_EFFECT_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_map.hpp"
#include "moo/moo_math.hpp"
#include "moo/vertex_formats.hpp"
#include "resmgr/datasection.hpp"
#include <iostream>


BW_BEGIN_NAMESPACE

///This const determines how quickly lens effects fade in/out
const float YOUNGEST_LENS_EFFECT = 0.f;		//at this age lens flare alpha = 1
const float OLDEST_LENS_EFFECT = 0.15f;		//at this age lens flare alpha = 0
const float KILL_LENS_EFFECT = 1.15f;		//1 second grace period before die

/**
 *	This class is describes the flare which is part of the lens effect.
 *	Flares can have secondary flares, such as coronas.
 */
class FlareData 
{
public:
	typedef BW::vector< FlareData > Flares;

	FlareData();
	~FlareData();	

	void	load( DataSectionPtr pSection );

	uint32	colour() const;
	void	colour( uint32 c );

	const BW::string & material() const;
	void	material( const BW::string &  m );

	float	clipDepth() const;
	void	clipDepth( float d );

	float	size() const;
	void	size( float s );

	float	width() const;
	void	width( float w );

	float	height() const;
	void	height( float h );

	float	zCullBias() const;
	void	zCullBias( float zcb );

	float	age() const;
	void	age( float a );

	const Flares & secondaries() const;

	void	draw( const Vector4 & clipPos, float alphaStrength, float scale, 
					uint32 lensColour, bool useAttenuationMap ) const;

	void createMesh( const Vector4 & clipPos, float alphaStrength,
		float scale, uint32 lensColour, Moo::VertexTLUV2* vertex,
		float vizCoord = 0.5f, float halfScreenWidth = 0.f,
		float halfScreenHeight = 0.f ) const;
private:
	uint32			colour_;
	BW::string		material_;
	float			clipDepth_;
	float			width_;
	float			height_;
	float			zCullBias_;
	float			age_;

	Flares	secondaries_;
};


/**
 *	This class holds the properties of a lens effect, and
 *	performs tick/draw logic.
 */
class LensEffect : public ReferenceCount
{
public:
	static const uint32 DEFAULT_COLOUR = 0xffffffff;
	static const float DEFAULT_MAXDISTANCE;
	static const float DEFAULT_AREA;
	static const float DEFAULT_FADE_SPEED;

	typedef BW::map< float, class FlareData > OcclusionLevels;

	enum VisibilityType
	{
		VT_UNSET = 0,
		VT_TYPE1,
		VT_TYPE2
	};

	LensEffect();
	virtual ~LensEffect();

	virtual bool load( DataSectionPtr pSection );
	virtual bool save( DataSectionPtr pSection );

	float	age() const;
	void	ageBy( float a );

	size_t	id() const;
	void	id( size_t value ); 

	const Vector3 & position() const;
	void	position( const Vector3 & pos );

	float	maxDistance() const;
	void	maxDistance( float md );
	float	distanceToAlpha( float dist ) const;

	bool	clampToFarPlane() const;
	void	clampToFarPlane( bool s ); 

	float	area() const;
	void	area( float a );

	float	fadeSpeed() const;
	void	fadeSpeed( float f );

	float visibility() const { return visibility_; }

	uint32 colour() const;
	void colour( uint32 c );
	void defaultColour();

	void size( float size );

	uint32	added() const;
	void	added( uint32 when );

	bool	visibilityType2() const;

	virtual void tick( float dTime, float visibility );	
	virtual void draw();

	int operator==( const LensEffect & other );

	const OcclusionLevels & occlusionLevels() const;

	static bool isLensEffect( const BW::string & file );	

	LensEffect & operator=( const LensEffect & );

private: 
	size_t		id_;
	Vector3		position_;

	float		maxDistance_;
	float		area_;
	float		fadeSpeed_;
	float		visibility_;	
	bool		clampToFarPlane_;
	mutable VisibilityType	visibilityType_;

	uint32		colour_;

	OcclusionLevels occlusionLevels_;

	uint32		added_;
};


typedef SmartPointer< LensEffect >	LensEffectPtr;
typedef BW::list<LensEffect>		LensEffects;
typedef BW::map<size_t, LensEffect> LensEffectsMap;


#ifdef CODE_INLINE
#include "lens_effect.ipp"
#endif

BW_END_NAMESPACE

#endif // LENS_EFFECT_HPP
