// primitive_file.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

#if 0	// ifdef'd out since functionality moved to BinSection
INLINE void PrimitiveFile::deleteBinary( const BW::string & name )
{
	this->updateBinary( name, NULL );
}
#endif

// primitive_file.ipp
