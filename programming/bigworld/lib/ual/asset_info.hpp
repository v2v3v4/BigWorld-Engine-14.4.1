/**
 *	AssetInfo: generic asset info class
 */

#ifndef ASSET_INFO_HPP
#define ASSET_INFO_HPP

#include "cstdmf/smartpointer.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE


//XmlItem
class AssetInfo : public ReferenceCount
{
public:
	AssetInfo()
	{};
	AssetInfo(
		const BW::wstring& type,
		const BW::wstring& text,
		const BW::wstring& longText,
		const BW::wstring& thumbnail = L"",
		const BW::wstring& description = L"" ) :
		type_( type ),
		text_( text ),
		longText_( longText ),
		thumbnail_( thumbnail ),
		description_( description )
	{};
	AssetInfo( DataSectionPtr sec )
	{
		if ( sec )
		{
			type_ = sec->readWideString( "type" );
			text_ = sec->asWideString();
			longText_ = sec->readWideString( "longText" );
			thumbnail_ = sec->readWideString( "thumbnail" );
			description_ = sec->readWideString( "description" );
		}
	}

	AssetInfo operator=( const AssetInfo& other )
	{
		type_ = other.type_;
		text_ = other.text_;
		longText_ = other.longText_;
		thumbnail_ = other.thumbnail_;
		description_ = other.description_;
		return *this;
	};

	bool empty() const { return text_.empty(); };
	bool equalTo( const AssetInfo& other ) const
	{
		return
			type_ == other.type_ &&
			text_ == other.text_ &&
			longText_ == other.longText_;
	};
	const BW::wstring& type() const { return type_; };
	const BW::wstring& text() const { return text_; };
	const BW::wstring& longText() const { return longText_; };
	const BW::wstring& thumbnail() const { return thumbnail_; };
	const BW::wstring& description() const { return description_; };

	void type( const BW::wstring& val ) { type_ = val; };
	void text( const BW::wstring& val ) { text_ = val; };
	void longText( const BW::wstring& val ) { longText_ = val; };
	void thumbnail( const BW::wstring& val ) { thumbnail_ = val; };
	void description( const BW::wstring& val ) { description_ = val; };
private:
	BW::wstring type_;
	BW::wstring text_;
	BW::wstring longText_;
	BW::wstring thumbnail_;
	BW::wstring description_;
};
typedef SmartPointer<AssetInfo> AssetInfoPtr;

BW_END_NAMESPACE

#endif // ASSET_INFO
