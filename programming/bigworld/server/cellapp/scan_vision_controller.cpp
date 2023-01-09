#include "script/first_include.hpp"

#include "scan_vision_controller.hpp"

#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "entity.hpp"

DECLARE_DEBUG_COMPONENT(0)


BW_BEGIN_NAMESPACE

IMPLEMENT_CONTROLLER_TYPE( ScanVisionController, DOMAIN_REAL )


/**
 * 	This constructor makes a vision controller that scans in a sine wave
 *	movement.
 *
 *	@param visionAngle	This is angle (in radians) of the vision cone.
 * 						The field of view is double this value.
 *	@param visionRange	The range of the vision.
 *	@param seeingHeight	The height above the entity position to see from.
 *	@param amplitude	The amplitude (in radians)
 *	@param scanPeriod	The period (in seconds) of the sine wave.
 *	@param timeOffset	The offset of the sine wave.
 *	@param updatePeriod This is how much in between checking vision (in ticks).
 */
ScanVisionController::ScanVisionController(
						float visionAngle,
						float visionRange,
						float seeingHeight,
						float amplitude,
						float scanPeriod,
						float timeOffset,
						int updatePeriod
	):
	VisionController( visionAngle, visionRange, seeingHeight, updatePeriod ),
	amplitude_( amplitude ),
	scanPeriod_( scanPeriod ),
	timeOffset_( timeOffset )
{
}


/**
 *	Yaw offset for vision
 *
 */
float ScanVisionController::getYawOffset()
{
	float yawOffset   = 0.f;

	double timeNow = (double)CellApp::instance().time() /
		CellAppConfig::updateHertz();
	float posInCycle = float(
		fmod( timeNow / scanPeriod_ + timeOffset_, 1.0 ) );
	yawOffset = 2.f * MATH_PI * posInCycle;
	yawOffset = amplitude_ * sinf( yawOffset );
	return yawOffset;
}


/**
 *	Write our state to a stream
 *
 *	@param stream		Stream to which we should write
 */
void ScanVisionController::writeRealToStream(BinaryOStream & stream)
{
	this->VisionController::writeRealToStream( stream );
	stream << amplitude_ << scanPeriod_ << timeOffset_ ;
}


/**
 *	Read our state from a stream
 *
 *	@param stream		Stream from which to read
 *	@return				true if successful, false otherwise
 */
bool ScanVisionController::readRealFromStream(BinaryIStream & stream)
{
	this->VisionController::readRealFromStream( stream );
	stream >> amplitude_ >> scanPeriod_ >> timeOffset_ ;
	return true;
}

BW_END_NAMESPACE

// scan_vision_controller.cpp
