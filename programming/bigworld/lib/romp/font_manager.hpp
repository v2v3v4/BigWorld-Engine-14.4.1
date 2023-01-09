#ifndef FONT_MANAGER_HPP
#define FONT_MANAGER_HPP

#include "font.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/stringmap.hpp"
#include "moo/material.hpp"
#include "moo/effect_constant_value.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/init_singleton.hpp"


BW_BEGIN_NAMESPACE

class FontMetrics;
typedef SmartPointer<FontMetrics>	FontMetricsPtr;


/**
 *	This class manages font resources.
 *	There are 2 main interfaces, get() and getCachedFont().
 *
 *	Standard Fonts can be used to draw immediately to the screen, and
 *	do not hold onto their glyphs for more than a single frame.
 *
 *	Cached Fonts hold onto every character drawn with them, until
 *	explicitly released.
 */
class FontManager : public Singleton<FontManager>
{
public:
	FontManager();
	virtual ~FontManager();

	FontPtr			get( const BW::string& resourceName, bool cached = false );
	CachedFontPtr	getCachedFont( const BW::string& resourceName );
	const BW::string& findFontName( const Font& font );
	void			setUVDivisor( const Font& font );

	Moo::EffectMaterialPtr material() const	{ return material_; }
	void			prepareMaterial( Font& font, uint32 colour, bool pixelSnap, D3DXHANDLE technique );
	bool			begin( Font& font, D3DXHANDLE technique = NULL, bool pixelSnap = true );
	void			end();
	void			recreateAll();

private:

	bool doInit();
	bool doFini();

	bool			createFont( DataSectionPtr pSection );
	BW::string		checkFontGenerated( DataSectionPtr fontDataSection );

	class FontMapSetter : public Moo::EffectConstantValue
	{
	public:
		FontMapSetter();
		bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle);
		void pTexture( Moo::BaseTexturePtr p )	{ pTexture_ = p; }
	private:
		Moo::BaseTexturePtr pTexture_;
	};

	class UVDivisorSetter : public Moo::EffectConstantValue
	{
	public:
		UVDivisorSetter();
		bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle);
		void value( float f )	{ value_ = f; }
	private:
		float	value_;
	};

	class FontColourSetter : public Moo::EffectConstantValue
	{
	public:
		FontColourSetter();
		bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle);
		void value( uint32 v );

	private:
		Vector4	value_;
	};

	class FontPixelSnapSetter : public Moo::EffectConstantValue
	{
	public:
		FontPixelSnapSetter();
		bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle);
		void value( bool v ) { value_ = v; }

	private:
		bool value_;
	};

	typedef StringHashMap<FontMetricsPtr> StringFontMetricsMap;
	StringFontMetricsMap	fonts_;
	SimpleMutex				mutex_;
	Moo::EffectMaterialPtr	material_;
	D3DXHANDLE				blendedTechnique_;
	SmartPointer< FontMapSetter >		fontMapSetter_;
	SmartPointer< UVDivisorSetter >		uvDivisorSetter_;
	SmartPointer< FontColourSetter >	fontColourSetter_;
	SmartPointer< FontPixelSnapSetter> 	pixelSnapSetter_;
	Matrix					origView_, origProj_;
	ULONG_PTR				gdiPlusToken_;
};

BW_END_NAMESPACE

#endif	//FONT_MANAGER_HPP
