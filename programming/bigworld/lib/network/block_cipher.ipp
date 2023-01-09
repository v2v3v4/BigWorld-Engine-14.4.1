
namespace Mercury
{

/**
 *	This XORs the two input blocks into a destination block.
 *
 *	@param pIn1 		The first input block.
 *	@param pIn2			The second input block.
 *	@param pOut 		The destination block. This can point to either of pIn1
 *						or pIn2.
 */
inline void BlockCipher::combineBlocks( const uint8 * pIn1, 
		const uint8 * pIn2, uint8 * pOut )
{
	const size_t BLOCK_SIZE = this->blockSize();
	size_t bytesLeft = BLOCK_SIZE;

#if !defined( __APPLE__ ) && !defined( __ANDROID__ ) && !defined( __EMSCRIPTEN__ )
	while (bytesLeft > sizeof(uint64))
	{
		// Special case for Blowfish on architectures that allow assignment
		// across word boundaries (x86_64).
		*(uint64 *)pOut = *(uint64 *)pIn1 ^ *(uint64 *)pIn2;
		pOut += sizeof(uint64);
		pIn1 += sizeof(uint64);
		pIn2 += sizeof(uint64);
		bytesLeft -= sizeof(uint64);
	}
#endif
	while (bytesLeft > 0)
	{
		*(pOut++) = *(pIn1++) ^ *(pIn2++);
		--bytesLeft;
	}
}


/**
 *	This method returns a human-readable representation of the cipher key.
 */
inline const BW::string BlockCipher::readableKey() const
{
	static char buf[ 1024 ];
	const Key & key = this->key();

	char * c = buf;

	for (size_t i = 0; 
			(i < key.size()) && 
				(size_t(c - buf) < sizeof( buf )); 
			++i)
	{
		bw_snprintf( c, buf + sizeof( buf ) - c, 
			(i ? " %02hhX" : "%02hhX"),
			(unsigned char)key[i] );
		c += (i > 1) ? 3 : 2;
	}

	return buf;
}


} // end namespace Mercury

// block_cipher.ipp
