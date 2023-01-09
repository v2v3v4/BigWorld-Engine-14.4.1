/*device_callback.ipp*/

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

INLINE
bool DeviceCallback::recreateForD3DExDevice() const
{
	return false;
}

} // namespace Moo

/*device_callback.ipp*/
