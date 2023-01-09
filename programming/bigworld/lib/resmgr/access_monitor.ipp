#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE AccessMonitor::AccessMonitor() :
	active_( false )
{
}

INLINE void AccessMonitor::active( bool flag )
{
	active_ = flag;
}

/* access_monitor.ipp */

