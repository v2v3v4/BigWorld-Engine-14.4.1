// app.ipp

#ifdef CODE_INLINE
    #define INLINE    inline
#else
    #define INLINE
#endif

BW_BEGIN_NAMESPACE

/**
 *	This method returns the HWND associated with this application.
 */
INLINE
HWND App::hwndApp()
{
	return hWndApp_;
}

BW_END_NAMESPACE

// app.ipp
