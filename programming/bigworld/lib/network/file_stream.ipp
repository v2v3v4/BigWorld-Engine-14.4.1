#if defined( CODE_INLINE )
#define INLINE    inline
#else
#define INLINE
#endif // defined( CODE_INLINE )

/**
 *	This method returns whether the FileStream is in a good state (ie: has
 *	encountered an error or not).
 *
 *	@returns true if the FileStream is in a good state (no error encountered),
 *		false otherwise.
 */
INLINE
bool FileStream::good() const
{
	return !error_;
}


/**
 *	This method returns whether the FileStream has encountered an error.
 *
 *	@returns true if an error has been encountered, false otherwise.
 */
INLINE
bool FileStream::error() const
{
	return error_;
}

// file_stream.ipp
