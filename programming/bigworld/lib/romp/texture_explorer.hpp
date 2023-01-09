#ifndef TEXTURE_EXPLORER_HPP
#define TEXTURE_EXPLORER_HPP


BW_BEGIN_NAMESPACE

/**
 *	This class utilitses the MF_WATCH macro system
 *	to implement a texture debugging tool on the Xbox.
 *
 *	This class is not supported on the PC build.
 */
class TextureExplorer
{
public:
	TextureExplorer();
	~TextureExplorer();
	void	tick();
	void	draw();
private:

	bool	enabled_;
	bool	fitToScreen_;
	bool	preserveAspect_;
	int		index_;
	int		currentIndex_;

	bool	doReload_;

	BW::string textureName_;
	BW::string texturePath_;
	Moo::BaseTexturePtr pTexture_;
	BW::string format_;
	uint32	width_;
	uint32	height_;
	uint32	memoryUsage_;
	uint32	maxMipLevel_;

	bool	alpha_;
	Moo::Material*	pAlphaMaterial_;
	Moo::Material*	pColourMaterial_;

	typedef BW::map< uint32, BW::string > Formats;
	Formats surfaceFormats_;

	TextureExplorer( const TextureExplorer& );
	TextureExplorer& operator=( const TextureExplorer& );
};

BW_END_NAMESPACE

#endif // TEXTURE_EXPLORER_HPP
