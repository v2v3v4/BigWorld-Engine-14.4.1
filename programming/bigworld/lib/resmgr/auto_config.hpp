#ifndef AUTO_CONFIG_HPP
#define AUTO_CONFIG_HPP

#include "cstdmf/bw_vector.hpp"
#include "datasection.hpp"
#include "bwresource.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is an object that automatically configures itself from
 *	a global configuration file.
 */
class AutoConfig
{
public:
	static const BW::string s_resourcesXML;

	AutoConfig();
	virtual ~AutoConfig();

	virtual void configureSelf( DataSectionPtr pConfigSection ) = 0;

	static void configureAllFrom( DataSectionPtr pConfigSection );
	static bool configureAllFrom( BW::vector<DataSectionPtr>& pConfigSections );
	static bool configureAllFrom( const BW::string& xmlResourceName );

private:
	AutoConfig( const AutoConfig& );
	AutoConfig& operator=( const AutoConfig& );

	typedef BW::vector<AutoConfig*> AutoConfigs;
	static AutoConfigs * s_all;
};


/**
 *	This class is a templatised auto configurated value.
 */
template< typename T >
class BasicAutoConfig : public AutoConfig
{
public:
	BasicAutoConfig( const char * path, const T & deft = Datatype::DefaultValue<T>::val() ) :
		AutoConfig(),
		path_( path ),
		value_( deft )
	{}

	virtual void configureSelf( DataSectionPtr pConfigSection )
	{
		DataSectionPtr pValue = pConfigSection->openSection( path_ );
		if (pValue)
			value_ = pValue->as< T >();
	}

	operator const T &() const	{ return value_; }
	const T & value() const		{ return value_; }

private:
	const char *	path_;
	T				value_;
};


/**
 *	Specialisation of BasicAutoConfig for BW::string.
 */
template<>
class BasicAutoConfig< BW::string > : public AutoConfig
{
public:
	BasicAutoConfig( const char * path, const BW::string & deft = "" );
	virtual ~BasicAutoConfig();

	virtual void configureSelf( DataSectionPtr pConfigSection );

	operator const BW::string &() const	{ return value_; }
	operator BW::StringRef() const { return value_; }
	const BW::string & value() const		{ return value_; }

private:
	const char *	path_;
	BW::string		value_;
};


typedef BasicAutoConfig< BW::string > AutoConfigString;


/**
 *	This class is a vector of strings that automatically configures themselves.
 */
class AutoConfigStrings : public AutoConfig
{
public:
	AutoConfigStrings( const char * path, const char * tag );	

	virtual void configureSelf( DataSectionPtr pConfigSection );
	
	typedef BW::vector<BW::string> Vector;

	operator const Vector &() const	{ return value_; }
	const Vector & value() const	{ return value_; }

private:
	const char *	path_;
	const char *	tag_;
	Vector			value_;
};

BW_END_NAMESPACE

#endif // AUTO_CONFIG_HPP
