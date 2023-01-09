
#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{

INLINE
BaseTexture::Status	BaseTexture::status() const
{ 
	return status_;
}

INLINE
void			BaseTexture::status( BaseTexture::Status status)
{
	status_ = status;
	this->onStatusChange( status_ );
}

} // namespace Moo

/*base_texture.ipp*/
