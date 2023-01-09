#ifndef DX_ENUM_HPP__
#define DX_ENUM_HPP__

BW_BEGIN_NAMESPACE

/******************************************************************************
	The Enum Type Support for Material ( fx files )
	The relation between name and value is 1:1, i.e. no 2 names will have same values
	This is because we will save the result as value and look them up in the name table
	when loaded again
******************************************************************************/
class DXEnum : public Singleton<DXEnum>
{
public:
	typedef int EnumType;
private:
	class EnumEntry
	{
		BW::map<BW::string, EnumType> nameToValueMap_;
		BW::map<EnumType, BW::string> valueToNameMap_;
	public:
		EnumEntry(){}
		EnumEntry( DataSectionPtr dataSection );
		typedef BW::map<BW::string, EnumType>::size_type size_type;
		size_type size() const;
		const BW::string& entry( size_type index ) const;
		EnumType value( const BW::string& name ) const;
		const BW::string& name( EnumType value ) const;
	};

	BW::map<BW::string, EnumEntry> enums_;
	BW::map<BW::string, BW::string> alias_;
	const BW::string& getRealType( const BW::string& typeName ) const;
public:
	typedef EnumEntry::size_type size_type;
	DXEnum( const BW::string& resFileName );
	bool isEnum( const BW::string& typeName ) const;
	size_type size( const BW::string& typeName ) const;
	const BW::string& entry( const BW::string& typeName, size_type index ) const;
	EnumType value( const BW::string& typeName, const BW::string& name ) const;
	const BW::string& name( const BW::string& typeName, EnumType value ) const;
};

BW_END_NAMESPACE
#endif//DX_ENUM_HPP__
