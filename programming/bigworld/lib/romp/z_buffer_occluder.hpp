/**
 *	This class checks line-of-sight through the z-buffer using occlusion
 *	queries.
 */
#ifndef Z_BUFFER_OCCLUDER_HPP
#define Z_BUFFER_OCCLUDER_HPP

#include "occlusion_query_helper.hpp"
#include "photon_occluder.hpp"
#include "moo/device_callback.hpp"


BW_BEGIN_NAMESPACE

class LensEffect;

/**
 * TODO: to be documented.
 */
class ZBufferOccluder : public PhotonOccluder, public Moo::DeviceCallback
{
public:
	ZBufferOccluder();
	~ZBufferOccluder();

	static bool isAvailable();
	static void init();
	static void fini();

	virtual float collides(
			const Vector3 & photonSourcePosition,
			const Vector3 & cameraPosition,
			const LensEffect& le );
	virtual void beginOcclusionTests();
	virtual void endOcclusionTests();

protected:
	//our own interface
	virtual void writePixel( const Vector3& source );
	virtual void writeArea( const Vector3& source, float size );

private:
	ZBufferOccluder(const ZBufferOccluder&);
	ZBufferOccluder& operator=(const ZBufferOccluder&);	

	OcclusionQueryHelper	helper_;
	OcclusionQueryHelper	helperZBuffer_;

	Moo::EffectMaterialPtr	mat_;
	void setDeviceState();

	friend std::ostream& operator<<(std::ostream&, const ZBufferOccluder&);
};

#ifdef CODE_INLINE
#include "z_buffer_occluder.ipp"
#endif

BW_END_NAMESPACE

#endif //Z_BUFFER_OCCLUDER_HPP
