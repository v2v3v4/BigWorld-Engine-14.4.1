#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include "resmgr/dataresource.hpp"
#include "pyscript/script.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class handles the options associated with this program.
 */
class Options
{
public:
	static DataSectionPtr pRoot();
	static bool init( int argc, char * argv[], const wchar_t* defaultFilename );
	static bool init( const BW::string& optionsFilename );
	static void initLoggers();
	static bool save( const char * path = NULL );
	static bool optionsFileExisted();
	static void fini();

	static bool optionExists( const BW::string& name );
	static void setOptionString( const BW::string& name, const BW::string& value );
	static BW::string getOptionString( const BW::string& name );
	static BW::string getOptionString( const BW::string& name, const BW::string& defaultVal );
	static void setOptionInt( const BW::string& name, int value );
	static int getOptionInt( const BW::string& name );
	static int getOptionInt( const BW::string& name, int defaultVal );
	static void setOptionBool( const BW::string& name, bool value );
	static bool getOptionBool( const BW::string& name );
	static bool getOptionBool( const BW::string& name, bool defaultVal );
	static void setOptionFloat( const BW::string& name, float value );
	static float getOptionFloat( const BW::string& name );
	static float getOptionFloat( const BW::string& name, float defaultVal );
	static void setOptionVector2( const BW::string& name, const Vector2& value );
	static Vector2 getOptionVector2( const BW::string& name );
	static Vector2 getOptionVector2( const BW::string& name, const Vector2& defaultVal );
	static void setOptionVector3( const BW::string& name, const Vector3& value );
	static Vector3 getOptionVector3( const BW::string& name );
	static Vector3 getOptionVector3( const BW::string& name, const Vector3& defaultVal );
	static void setOptionVector4( const BW::string& name, const Vector4& value );
	static Vector4 getOptionVector4( const BW::string& name );
	static Vector4 getOptionVector4( const BW::string& name, const Vector4& defaultVal );
	static void setOptionMatrix34( const BW::string& name, const Matrix& value );
	static Matrix getOptionMatrix34( const BW::string& name );
	static Matrix getOptionMatrix34( const BW::string& name, const Matrix& defaultVal );
private:
	Options();

	void syncRoot();

	// These are not implemented.
	Options( const Options & );
	Options & operator=( const Options & );

	

	typedef Options This;
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionString )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionString )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionInt )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionInt )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionBool )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionBool )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionFloat )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionFloat )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionVector2 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionVector2 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionVector3 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionVector3 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionVector4 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionVector4 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOptionMatrix34 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_getOptionMatrix34 )
	PY_MODULE_STATIC_METHOD_DECLARE( py_setOption )

	typedef BW::map<BW::string, BW::string> OptionsCache;

	static Options & instance();

	static Options				s_instance_;
	OptionsCache				cache_;
	SmartPointer<DataResource>	options_;
	DataSectionPtr				pRootSection_;
	bool						rootDirty_;
	bool						optionsExisted_;
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "options.ipp"
#endif

#endif
/*options.hpp*/
