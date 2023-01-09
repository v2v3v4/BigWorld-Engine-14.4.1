#ifndef SCAN_VISION_CONTROLLER_HPP
#define SCAN_VISION_CONTROLLER_HPP

#include "vision_controller.hpp"


BW_BEGIN_NAMESPACE

/**
 *  This class is the controller for an entity's vision.
 */
class ScanVisionController : public VisionController
{
	DECLARE_CONTROLLER_TYPE( ScanVisionController )

public:
	 // period in ticks. defaults to 10 ticks (1 second)
	ScanVisionController( float visionAngle = 1.f,
						  float visionRange = 20.f,
						  float seeingHeight = 2.f,
						  float amplitude = 0.f,
						  float scanPeriod = 0.f,
						  float timeOffset = 0.f,
						  int updatePeriod = 10 );

	virtual float getYawOffset();

	void	writeRealToStream( BinaryOStream & stream );
	bool 	readRealFromStream( BinaryIStream & stream );

private:
	float 	amplitude_;
	float	scanPeriod_;
	float   timeOffset_;
};

BW_END_NAMESPACE

#endif // VISION_CONTROLLER_HPP
