#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

INLINE FontMetrics& Font::metrics()
{
	return metrics_;
}


INLINE const FontMetrics& Font::metrics() const
{
	return metrics_;
}


INLINE void Font::fitToScreen( bool state, const Vector2& numCharsXY )
{
	fitToScreen_ = state;
	numCharsXY_ = numCharsXY;
}


INLINE bool Font::fitToScreen() const
{
	return fitToScreen_;
}
