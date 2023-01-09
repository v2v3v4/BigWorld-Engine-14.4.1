#ifndef TIME_OF_DAY_HPP
#define TIME_OF_DAY_HPP

#include "math/linear_animation.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class DirectionalLight;


/**
 *	This structure holds an outside lighting setting.
 *
 *	It is set by the TimeOfDay class, but implemented by other classes.
 */
class OutsideLighting
{
public:
	OutsideLighting():
		useMoonColour_(false)
	{
		sunColour.setZero();	
		ambientColour.setZero();
		fogColour.setZero();
		sunTransform.setIdentity();
		moonTransform.setIdentity();
	};

	Matrix	sunTransform;
	Matrix	moonTransform;

	Vector4	sunColour;	
	Vector4	ambientColour;
	Vector4	fogColour;

	const Matrix& mainLightTransform() const
	{
		return useMoonColour_ ? moonTransform : sunTransform;
	}

	const Vector3& mainLightDir() const
	{
		return mainLightTransform().applyToUnitAxisVector(2);
	}

	void useMoonColour( bool u )
	{
		useMoonColour_ = u;
	}

private:
	bool	useMoonColour_;
};


/**
 *	This class manages the game time of day, and calculates the
 *	outside lighting conditions based on this.
 */
class TimeOfDay
{
public:
	TimeOfDay( float currentTime = 12.0f );
	~TimeOfDay();

	void tick( float dTime );
	void load( DataSectionPtr root, bool ignoreTimeChanged = false );

	float	gameTime() const;
	void	gameTime( float t );

	float	startTime() const;
    void	startTime( float time );

	float	secondsPerGameHour() const;
	void	secondsPerGameHour( float t );

	float	sunAngle() const;
	void	sunAngle( float angle );

	Vector3 sunColor() const;
	void	sunColor( Vector3 color );

	float	moonAngle() const;
	void	moonAngle( float angle );

	Vector3 moonColor() const;
	void	moonColor( Vector3 color );

	bool	sunAndMoonNeedUpdate() const { return 	sunAndMoonNeedUpdate_; }
	void	resetSunMoonUpdate() { sunAndMoonNeedUpdate_ = false; }

	const OutsideLighting & lighting() const;
	OutsideLighting & lighting();

	const BW::string& getTimeOfDayAsString() const;
	void  setTimeOfDayAsString( const BW::string &newTime );

    static const BW::string& gameTimeToString( float gameTime );
    static float gameTimeFromString( const BW::string &newTime, float defaultTime );

	/**
	 *	A class that is notified when time of day settings in this
	 *	class are changed externally (i.e. not by tick)
	 */
	class UpdateNotifier : public ReferenceCount
	{
	public:
		virtual void updated( const TimeOfDay & tod ) = 0;
	};
	void addUpdateNotifier( SmartPointer<UpdateNotifier> pUN );
	void delUpdateNotifier( SmartPointer<UpdateNotifier> pUN );


	void updateNotifiersOn( bool on );

#ifdef EDITOR_ENABLED
	void	load( const BW::string filename );
	void	save( void );
	void	save( const BW::string filename );
	void	save( DataSectionPtr root );
	BW::string file( void ) { return file_; };
	void	file( BW::string filename ) { file_ = filename; };

	const LinearAnimation<Vector3>&	sunAnimation() const { return sunAnimation_; }
	const LinearAnimation<Vector3>& ambientAnimation() const { return ambientAnimation_; }
    const LinearAnimation<Vector3>& fogAnimation() const { return fogAnimation_; }
	int		sunAnimations( void );
	void	getSunAnimation( int index, float& time, Vector3& colour );
	void	setSunAnimation( int index, float time, const Vector3 colour );
	void	addSunAnimation( float time, const Vector3 colour );
    void    clearSunAnimations();
	int		ambientAnimations( void );
	void	getAmbientAnimation( int index, float& time, Vector3& colour );
	void	setAmbientAnimation( int index, float time, const Vector3 colour );
	void	addAmbientAnimation( float time, const Vector3 colour );
    void    clearAmbientAnimations();
	int		fogAnimations( void );
	void	getFogAnimation( int index, float& time, Vector3& colour );
	void	setFogAnimation( int index, float time, const Vector3 colour );
	void	addFogAnimation( float time, const Vector3 colour );
    void    clearFogAnimations();

	void	clear( void );
#endif

private:
	void	timeConfigChanged();

	OutsideLighting	now_;

	float						time_;			///< game time in hours
    float						startTime_;
	float						secondsPerGameHour_;

	float						sunAngle_;
	Vector3						sunColor_;
	float						moonAngle_;	
	Vector3						moonColor_;

	bool						sunAndMoonNeedUpdate_;

	LinearAnimation< Vector3 >	sunAnimation_;	
	LinearAnimation< Vector3 >	ambientAnimation_;
	LinearAnimation< Vector3 >	fogAnimation_;

	BW::string					file_;

	BW::vector< SmartPointer<UpdateNotifier> >		updateNotifiers_;
	bool						updateNotifiersOn_;
	bool						timeConfigChanged_;
};

#ifdef CODE_INLINE
#include "time_of_day.ipp"
#endif

BW_END_NAMESPACE

#endif // TIME_OF_DAY_HPP
