#include "open_automate_wrapper.hpp"
#include "cstdmf/bw_string.hpp"
#include "pyscript/script_math.hpp"
#include "openautomateutil.h"
#include "resmgr/bwresource.hpp"
#include "resmgr/multi_file_system.hpp"
#include "cstdmf/debug.hpp"
#include "open_automate_tester.hpp"
#include "cstdmf/debug_exception_filter.hpp"


BW_BEGIN_NAMESPACE

int PyOAVersionStructWrapper_token = 1;
int PyOOACommandWrapper_token = 2;


bool bwOARegisterApp();

///The command line argument for open automate
const BW::string BWOpenAutomate::OPEN_AUTOMATE_ARGUMENT = "-openautomate";
const BW::string BWOpenAutomate::OPEN_AUTOMATE_TEST_ARGUMENT = "INTERNAL_TEST";

/*~ module OpenAutomateWrapper
 */

/*~ function OpenAutomateWrapper.getOpenAutomateArgument
 *
 *  This function returns the OPEN_AUTOMATE_ARGUMENT 
 *
 *	@return	the OPEN_AUTOMATE_ARGUMENT 
 */
/**
 *  This function returns the OPEN_AUTOMATE_ARGUMENT
 */
BW::string getOpenAutomateArgument()
{
	return BWOpenAutomate::OPEN_AUTOMATE_ARGUMENT;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, getOpenAutomateArgument, END, OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.getOpenAutomateTestArgument
 *
 *  This function returns the OPEN_AUTOMATE_TEST_ARGUMENT 
 *
 *	@return	the OPEN_AUTOMATE_TEST_ARGUMENT 
 */
/**
 *  This function returns the OPEN_AUTOMATE_TEST_ARGUMENT 
 */
BW::string getOpenAutomateTestArgument()
{
	return BWOpenAutomate::OPEN_AUTOMATE_TEST_ARGUMENT;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, getOpenAutomateTestArgument, END, OpenAutomateWrapper )


bool BWOpenAutomate::s_runningBenchmark = false;
bool BWOpenAutomate::s_doingExit = false; 

PY_ENUM_MAP( BWOpenAutomate::eOACommandType );
PY_ENUM_CONVERTERS_SCATTERED( BWOpenAutomate::eOACommandType );

PY_ENUM_MAP( BWOpenAutomate::eOAOptionDataType );
PY_ENUM_CONVERTERS_CONTIGUOUS( BWOpenAutomate::eOAOptionDataType );

PY_ENUM_MAP( BWOpenAutomate::eOAComparisonOpType );
PY_ENUM_CONVERTERS_CONTIGUOUS( BWOpenAutomate::eOAComparisonOpType );


PY_TYPEOBJECT( OAVersionStructWrapper )

PY_BEGIN_METHODS( OAVersionStructWrapper )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( OAVersionStructWrapper )

	PY_ATTRIBUTE( major )
	PY_ATTRIBUTE( minor )
	PY_ATTRIBUTE( custom )
	PY_ATTRIBUTE( build )

PY_END_ATTRIBUTES()
PY_SCRIPT_CONVERTERS( OAVersionStructWrapper )


/*~ function OpenAutomateWrapper.CreateOAVersionStructWrapper
 *
 *	This function returns a new OAVersionStructWrapper which is used
 *	to get the OpenAutomate version information.
 */
PY_FACTORY_NAMED( OAVersionStructWrapper, "CreateOAVersionStructWrapper", OpenAutomateWrapper )


OAVersionStructWrapper::OAVersionStructWrapper(PyTypeObject * pType) : PyObjectPlus(pType),
major_(0),
minor_(0),
custom_(0),
build_(0)
{

}

/**
 *	Static python factory method
 */
PyObject * OAVersionStructWrapper::pyNew( PyObject * args )
{
	return new OAVersionStructWrapper( );
}

void OAVersionStructWrapper::fromOAVersion(oaVersionStruct& tempVersion)
{
	major_ = tempVersion.Major;
	minor_ = tempVersion.Minor; 
	custom_ = tempVersion.Custom;
	build_ = tempVersion.Build;
}

PY_TYPEOBJECT( OACommandWrapper )
PY_BEGIN_METHODS( OACommandWrapper )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( OACommandWrapper )
	/*~ attribute OACommandWrapper.benchmark
	 *
	 *  Contains the current benchmark name (invalid only for run benchmark commands)
	 *	
	 *  @type String 
	 */
	PY_ATTRIBUTE( benchmark )
PY_END_ATTRIBUTES()
PY_SCRIPT_CONVERTERS( OACommandWrapper )


/*~ function OpenAutomateWrapper.CreateOACommandWrapper
 *
 *	This function returns a new OACommandWrappe which is used
 *	to get the next Open Automate command.
 */
PY_FACTORY_NAMED( OACommandWrapper, "CreateOACommandWrapper", OpenAutomateWrapper )


OACommandWrapper::OACommandWrapper(PyTypeObject * pType) : PyObjectPlus(pType)
{
	oaInitCommand(&internalCommand_);
}


/**
 *	Static python factory method
 */
PyObject * OACommandWrapper::pyNew( PyObject * args )
{
	return new OACommandWrapper( );
}


PyObject * OACommandWrapper::pyGet_benchmark()
{
	return Script::getData( BW::string(internalCommand_.BenchmarkName) );
}


PY_TYPEOBJECT( OANamedOptionWrapper )

PY_BEGIN_METHODS( OANamedOptionWrapper )
	PY_METHOD( setComparison )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( OANamedOptionWrapper )

	/*~ attribute OANamedOptionWrapper.dataType
	 *
	 *  Stores the data type of this option
	 *	
	 *  @type BWOpenAutomate::eOAOptionDataType
	 */
	PY_ATTRIBUTE( dataType )
	/*~ attribute OANamedOptionWrapper.name
	 *
	 *  Stores the name of this option
	 *	
	 *  @type String
	 */
	PY_ATTRIBUTE( name )
	/*~ attribute OANamedOptionWrapper.enumValue
	 *
	 *  Stores the value of this option if its type is enum
	 *	
	 *  @type String
	 */
	PY_ATTRIBUTE( enumValue )
	/*~ attribute OANamedOptionWrapper.minValueInt
	 *
	 *  Stores the minimal value for this option
	 *	
	 *  @type int
	 */
	PY_ATTRIBUTE( minValueInt )
	/*~ attribute OANamedOptionWrapper.minValueFloat
	 *
	 *  Stores the minimal value for this option
	 *	
	 *  @type float
	 */
	PY_ATTRIBUTE( minValueFloat )
	/*~ attribute OANamedOptionWrapper.maxValueInt
	 *
	 *  Stores the maximal value for this option
	 *	
	 *  @type int
	 */
	PY_ATTRIBUTE( maxValueInt )
	/*~ attribute OANamedOptionWrapper.maxValueFloat
	 *
	 *  Stores the maximal value for this option
	 *	
	 *  @type float
	 */
	PY_ATTRIBUTE( maxValueFloat )
	/*~ attribute OANamedOptionWrapper.numSteps
	 *
	 *  Stores the number of steps between the minimal and maximal value
	 *	
	 *  @type int
	 */
	PY_ATTRIBUTE( numSteps )
	/*~ attribute OANamedOptionWrapper.parentName
	 *
	 *  Stores the name of the parent option for options dependencies
	 *	
	 *  @type String
	 */
	PY_ATTRIBUTE( parentName )
	/*~ attribute OANamedOptionWrapper.comparisonOp
	 *
	 *  Stores the operation used to compare the parent value with ComparisonVal
	 *	
	 *  @type String
	 */
	PY_ATTRIBUTE( comparisonOp )
	
PY_END_ATTRIBUTES()
PY_SCRIPT_CONVERTERS( OANamedOptionWrapper )

/*~ function OpenAutomateWrapper.CreateOANamedOptionWrapper
 *
 *	This function returns a new OANamedOptionWrapper which is used
 *	to update OpenAutomate about the game options
 */
PY_FACTORY_NAMED( OANamedOptionWrapper, "CreateOANamedOptionWrapper", OpenAutomateWrapper )


OANamedOptionWrapper::OANamedOptionWrapper(PyTypeObject * pType) : PyObjectPlus(pType)
{
	oaInitOption(&internalOption_);
}


OANamedOptionWrapper::OANamedOptionWrapper(oaNamedOptionStruct* option, PyTypeObject * pType) : 
	PyObjectPlus(pType),
	internalOption_(*option)
{
}


/**
 *	Static python factory method
 */
PyObject * OANamedOptionWrapper::pyNew( PyObject * args )
{
	return new OANamedOptionWrapper( );
}


int OANamedOptionWrapper::pySet_dataType( PyObject * value )
{
	BWOpenAutomate::eOAOptionDataType temp;
	int ret = Script::setData( value, temp, "0" );
	internalOption_.DataType = (oaOptionDataType)temp;
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_dataType()
{
	BWOpenAutomate::eOAOptionDataType temp = (BWOpenAutomate::eOAOptionDataType)internalOption_.DataType;
	return Script::getData(temp) ;
}


int OANamedOptionWrapper::pySet_name( PyObject * value )
{
	int ret = Script::setData( value, name_, "" );
	internalOption_.Name = name_.c_str();
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_name()
{
	return Script::getData( BW::string(internalOption_.Name) );
}


int OANamedOptionWrapper::pySet_enumValue( PyObject * value )
{

	int ret = Script::setData( value, enumValue_, "" );
	internalOption_.Value.String = (char*)enumValue_.c_str();
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_enumValue()
{
	switch (internalOption_.DataType)
	{
	case OA_TYPE_STRING:
		{
			return Script::getData( BW::string(internalOption_.Value.String) );
		}
	case OA_TYPE_INT   :
		{
			return Script::getData( (uint32)(internalOption_.Value.Int) );
		}
	case OA_TYPE_FLOAT :
		{
			return Script::getData( (float)(internalOption_.Value.Float) );
		}
	case OA_TYPE_ENUM  :
		{
			return Script::getData( BW::string(internalOption_.Value.Enum) );
		}
	case OA_TYPE_BOOL  :
		{
			return Script::getData( (bool)((internalOption_.Value.Bool) != 0) );
		}
	case OA_TYPE_INVALID:
		{
			break;
		}
	}
	PyErr_SetString( PyExc_TypeError,
	"Failed geting the type of an option" );
	return NULL;
}


int OANamedOptionWrapper::pySet_minValueInt( PyObject * value )
{
	uint32 temp = 0;
	int ret = Script::setData( value, temp, "0" );
	internalOption_.MinValue.Int = temp;
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_minValueInt()
{
	return Script::getData( uint32(internalOption_.MinValue.Int) );
}


int OANamedOptionWrapper::pySet_minValueFloat( PyObject * value )
{
	float temp = 0;
	int ret = Script::setData( value, temp, "0" );
	internalOption_.MinValue.Float = temp;
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_minValueFloat()
{
	return Script::getData( float(internalOption_.MinValue.Float) );
}


int OANamedOptionWrapper::pySet_maxValueInt( PyObject * value )
{
	uint32 temp = 0;
	int ret = Script::setData( value, temp, "0" );
	internalOption_.MaxValue.Int = temp;
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_maxValueInt()
{
	return Script::getData( uint32(internalOption_.MaxValue.Int) );
}


int OANamedOptionWrapper::pySet_maxValueFloat( PyObject * value )
{
	float temp = 0;
	int ret = Script::setData( value, temp, "0" );
	internalOption_.MaxValue.Float = temp;
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_maxValueFloat()
{
	return Script::getData( float(internalOption_.MaxValue.Float) );
}


int OANamedOptionWrapper::pySet_numSteps( PyObject * value )
{
	int temp = 0;
	int ret = Script::setData( value, temp, "0" );
	internalOption_.NumSteps = temp;
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_numSteps()
{
	return Script::getData( uint32(internalOption_.NumSteps) );
}


int OANamedOptionWrapper::pySet_parentName( PyObject * value )
{
	int ret = Script::setData( value, parentName_, "" );
	internalOption_.Dependency.ParentName = parentName_.c_str();
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_parentName()
{
	return Script::getData( BW::string(internalOption_.Name) );
}


int OANamedOptionWrapper::pySet_comparisonOp( PyObject * value )
{
	BWOpenAutomate::eOAComparisonOpType temp;
	int ret = Script::setData( value, temp, "0" );
	internalOption_.Dependency.ComparisonOp = (oaComparisonOpType)temp;
	return ret;
}


PyObject * OANamedOptionWrapper::pyGet_comparisonOp()
{
	BWOpenAutomate::eOAComparisonOpType temp = (BWOpenAutomate::eOAComparisonOpType)internalOption_.Dependency.ComparisonOp;
	return Script::getData(temp);
}


/*~ function OANamedOptionWrapper.setComparison
 *
 *	Set the value compared to the dependent parent option in order to see if this option is actionve
 *
 *	@param	optionType a String specifying the option type
 *	@param	comparisonValArg a value being compared to the parent option
 */
/**
 *	Set the value compared to the dependent parent option in order to see if this option is actionve
 */
void OANamedOptionWrapper::setComparison(BWOpenAutomate::eOAOptionDataType optionType, PyObjectPtr comparisonValArg)
{
	internalOption_.Dependency.ComparisonValType = (oaOptionDataType)optionType;
	switch (optionType)
	{

	case BWOpenAutomate::BWOA_TYPE_STRING:
		{
			Script::setData( comparisonValArg.get(), comparisonVal_, "" );
			internalOption_.Dependency.ComparisonVal.String = (oaString)comparisonVal_.c_str();
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_INT   :
		{
			uint32 temp;
			Script::setData( comparisonValArg.get(), temp, "" );
			internalOption_.Dependency.ComparisonVal.Int = (oaInt)temp;
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_FLOAT :
		{
			float temp;
			Script::setData( comparisonValArg.get(), temp, "" );
			internalOption_.Dependency.ComparisonVal.Float = temp;
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_ENUM  :
		{
			Script::setData( comparisonValArg.get(), comparisonVal_, "" );
			internalOption_.Dependency.ComparisonVal.Enum = (oaString)comparisonVal_.c_str();
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_BOOL  :
		{	
			bool temp;
			Script::setData( comparisonValArg.get(), temp, "" );
			internalOption_.Dependency.ComparisonVal.Bool = (oaBool)temp;
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_INVALID:
		{
			break;
		}
	}
}


//---------------------------------------------------------


/*~ function OpenAutomateWrapper.bwOAInit
 *
 *  This function initialises the open automate library.  
 *
 *	@param		initStr the string used to init the open automate environment 
 *				can be found as the second argument of the program 
 *	@param		version an OAVersionStructWrapper which will receive the OpenAutomate
 *              version information.
 *	@param		bwTestMode bool marking whether we should use the open automate testing
 *
 *	@return		true if successful
 */
/**
 *  This function initialises the open automate library.  
 */
bool bwOAInit(const BW::string& initStr, SmartPointer<OAVersionStructWrapper> version, bool bwTestMode)
{
	//first make sure we are registered
	bwOARegisterApp();
	//mark that exceptions will cause exit with an error code and not error message
#ifdef CONSUMER_CLIENT
	SetUnhandledExceptionFilter(ErrorCodeExceptionFilter);
#else
	extern bool exitWithErrorCodeOnException;
	exitWithErrorCodeOnException = true;
#endif
	oaVersion tempVersion;
	oaBool temp = OA_OFF;
	if (! bwTestMode) 
	{
		temp = oaInit(initStr.c_str(), &tempVersion);
		version->fromOAVersion(tempVersion);
	}
	else
	{
		temp = testOAInit(initStr.c_str(), &tempVersion);
		version->fromOAVersion(tempVersion);
	}
	return temp != 0;
}	
PY_AUTO_MODULE_FUNCTION( RETDATA, bwOAInit,
		ARG(BW::string,
		ARG(SmartPointer<OAVersionStructWrapper>,
		ARG(bool, END ))), OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.bwOAGetNextCommand
 *
 *  This function gets the next OpenAutomate command for the application to run.  
 *
 *	@param		command the OACommandWrapper object which will get the command additional info.
 *
 *	@return	eOACommandType the command specific type
 */
/**
 *  This function gets the next OpenAutomate command for the application to run.  
 */
BWOpenAutomate::eOACommandType bwOAGetNextCommand(SmartPointer<OACommandWrapper> command)
{
	return (BWOpenAutomate::eOACommandType)oaGetNextCommand(&command->internalCommand_);
}
PY_AUTO_MODULE_FUNCTION( RETDATA, bwOAGetNextCommand, ARG( SmartPointer< OACommandWrapper >, END ), OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.bwOAGetNextOption
 *
 *  This function gets the Option sent by OpenAutomate. The application should set this option.
 *  based on the details available in the OANamedOptionWrapper
 *
 *	@return	an OANamedOptionWrapper containing the option to be set
 */
/**
 *  This function gets the Option sent by OpenAutomate. The application should set this option.
 *  based on the details available in the OANamedOptionWrapper
 */
PyObject* bwOAGetNextOption()
{
	oaNamedOption * temp = oaGetNextOption();
	if (temp == NULL) 
	{
		Py_RETURN_NONE;
	}
	return Script::getData( new OANamedOptionWrapper(temp) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, bwOAGetNextOption, END, OpenAutomateWrapper )



/*~ function OpenAutomateWrapper.bwOAAddOption
 *
 *  This function is used to inform OpenAutomate on a client option
 *  based on the details available in the OANamedOptionWrapper
 *
 *	@param optionWrapper an OANamedOptionWrapper containing the option to add
 */
/**
 *  This function is used to inform OpenAutomate on a client option
 *  based on the details available in the OANamedOptionWrapper
 */
void bwOAAddOption(const SmartPointer< OANamedOptionWrapper > optionWrapper)
{
	oaAddOption(&optionWrapper->internalOption_);
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOAAddOption, ARG( SmartPointer< OANamedOptionWrapper >, END ), OpenAutomateWrapper )



/*~ function OpenAutomateWrapper.bwOAAddOptionvValue
 *
 *  This function is used to inform OpenAutomate on the current active value for a specific option
 *
 *	@param	name a String specifying the option name
 *	@param	valueType a String specifying the option value type 
 *	@param	value the option value (string, int, float, enum value and bool are supported)
 */
/**
 *  This function is used to inform OpenAutomate on the current active value for a specific option
 */
void bwOAAddOptionValue(const BW::string& name, 
					  BWOpenAutomate::eOAOptionDataType valueType,
                      PyObjectPtr value)
{
	switch (valueType)
	{
	case BWOpenAutomate::BWOA_TYPE_STRING:
		{
			BW::string temp;
			int ret = Script::setData( value.get(), temp, "0" );
			if (ret != 0)
			{
				char error[255];
				bw_snprintf (error, sizeof(error), "Failed translating value %s %d", __FILE__, __LINE__);
				PyErr_SetString( PyExc_TypeError, error);
			}
			else 
			{
				oaValue valueTemp;
				valueTemp.String = (char*)temp.c_str();
				oaAddOptionValue(name.c_str(), (oaOptionDataType)valueType, &valueTemp);
			}
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_INT   :
		{
			int temp;
			int ret = Script::setData( value.get(), temp, "0" );
			if (ret != 0)
			{
				char error[255];
				bw_snprintf (error, sizeof(error), "Failed translating value %s %d", __FILE__, __LINE__);
				PyErr_SetString( PyExc_TypeError, error);
			}
			else 
			{
				oaValue valueTemp;
				valueTemp.Int = temp;
				oaAddOptionValue(name.c_str(), (oaOptionDataType)valueType, &valueTemp);
			}
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_FLOAT :
		{
			float temp;
			int ret = Script::setData( value.get(), temp, "0" );
			if (ret != 0)
			{
				char error[255];
				bw_snprintf (error, sizeof(error), "Failed translating value %s %d", __FILE__, __LINE__);
				PyErr_SetString( PyExc_TypeError, error);
			}
			else 
			{
				oaValue valueTemp;
				valueTemp.Float = temp;
				oaAddOptionValue(name.c_str(), (oaOptionDataType)valueType, &valueTemp);
			}
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_ENUM  :
		{
			BW::string temp;
			int ret = Script::setData( value.get(), temp, "0" );
			if (ret != 0)
			{
				char error[255];
				bw_snprintf (error, sizeof(error), "Failed translating value %s %d", __FILE__, __LINE__);
				PyErr_SetString( PyExc_TypeError, error);
			}
			else 
			{
				oaValue valueTemp;
				valueTemp.Enum = (char*)temp.c_str();
				oaAddOptionValue(name.c_str(), (oaOptionDataType)valueType, &valueTemp);
			}
			break;
		}
	case BWOpenAutomate::BWOA_TYPE_BOOL  :
		{
			bool temp;
			int ret = Script::setData( value.get(), temp, "0" );
			if (ret != 0)
			{
				char error[255];
				bw_snprintf (error, sizeof(error), "Failed translating value %s %d", __FILE__, __LINE__);
				PyErr_SetString( PyExc_TypeError, error);
			}
			else 
			{
				oaValue valueTemp;
				valueTemp.Bool = temp ? OA_TRUE: OA_FALSE;
				oaAddOptionValue(name.c_str(), (oaOptionDataType)valueType, &valueTemp);
			}
			break;
		}
	default:
		{
			char error[255];
			bw_snprintf (error, sizeof(error), "invalid option %s %d %d", __FILE__, __LINE__, valueType);
			PyErr_SetString( PyExc_TypeError, error);
		}
	}
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOAAddOptionValue,
						ARG( BW::string,
						ARG( BWOpenAutomate::eOAOptionDataType,
						ARG( PyObjectPtr, END ))),
						OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.bwOAAddBenchmark
 *
 *  This function is used to inform OpenAutomate on a benchmark (based on the benchmark name)
 *
 *	@param	benchmarkName a String containing the name the benchmark
 */
/**
 *  This function is used to inform OpenAutomate on a benchmark (based on the benchmark name)
 */
void bwOAAddBenchmark(const BW::string& benchmarkName)
{
	oaAddBenchmark(benchmarkName.c_str());
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOAAddBenchmark, ARG(BW::string, END ), OpenAutomateWrapper )



/*~ function OpenAutomateWrapper.bwOAStartBenchmark
 *
 *  This function is used to inform OpenAutomate that a benchmark is starting
 */
/**
 *  This function is used to inform OpenAutomate that a benchmark is starting
 */
void bwOAStartBenchmark(void)
{
	BWOpenAutomate::s_runningBenchmark = true;
	oaStartBenchmark();
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOAStartBenchmark, END, OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.bwOAEndBenchmark
 *
 *  This function is used to inform OpenAutomate that a benchmark is ending
 */
/**
 *  This function is used to inform OpenAutomate that a benchmark is ending
 */
void bwOAEndBenchmark(void)
{
	oaEndBenchmark();
	BWOpenAutomate::s_runningBenchmark = false;
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOAEndBenchmark, END, OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.OARegisterApp
 *
 *  This function is used to register an application as an open automate supporting app
 */
/**
 *  This function is used to register an application as an open automate supporting app
 */
 bool bwOARegisterApp()
{
	oauAppBuildId buildId;
	buildId.DevName = "BigWorld";
	buildId.AppName = "Fantasydemo";
	//get the version of this file
	BW::string productVersion = BWResource::instance().appProductVersion();
	buildId.AppBuildName = (oaChar*)productVersion.c_str();

	oauAppBuildInfo appInfo;
	BW::string installRootPath = BWResource::appDirectory();
	appInfo.InstallRootPath = (oaChar*)installRootPath.c_str();
	BW::string appPath = BWResource::appPath();
	appInfo.EntryExe = (oaChar*)appPath.c_str();
	//get the InstallDateTime
	IFileSystem::FileInfo fInfo;
	IFileSystem::FileType fType =
		NativeFileSystem::getAbsoluteFileType( appPath, &fInfo );
	if ( fType != IFileSystem::FT_NOT_FOUND )
	{
		SYSTEMTIME systemTime;
		::FileTimeToSystemTime((FILETIME*)&fInfo.created, &systemTime);
		char buf[500];
		bw_snprintf (buf, sizeof(buf), "%i-%02i-%02iT%02i:%02i:%02i.%03iZ", systemTime.wYear, systemTime.wMonth, 
			systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
		appInfo.InstallDateTime = buf;

	}
	else 
	{
		ERROR_MSG("Couldn't get file creation time for %s\n", appPath.c_str());
	}
	appInfo.Region = "en";

	//check for existing app
	bool alreadyRegistered = false;
	oauAppBuildList * allApps = oauGetAllRegisteredAppBuilds();
	for (int i = 0; i < allApps->NumAppBuilds; ++i)
	{
		if (memcmp(allApps->AppBuilds[i]->Id, &buildId, sizeof(oauAppBuildId)) == 0 && 
			memcmp(allApps->AppBuilds[i]->Info, &appInfo, sizeof(oauAppBuildInfo)) == 0)
		{
			alreadyRegistered = true;
		}
	}
	if (! alreadyRegistered) 
	{
		oauRegisterAppBuild(OAU_REGISTER_REGISTRY_SYSTEM,
							   &buildId,
							   &appInfo);
	}
	return true;
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOARegisterApp, END, OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.bwOAAddResultValue
 *
 *  This function is called to add a test result
 *	@param		name a string containing the result name (should be unique foreach test)
 *	@param		value a float containing the result value.
 *
 */
/**
 *  This function is called to add a test result
 */
void bwOAAddResultValue(const BW::string& name, float value)
{
	oaValue usedValue;
	usedValue.Float = value;
	oaAddResultValue((oaChar *)name.c_str(), 
                      OA_TYPE_FLOAT,
                      &usedValue);
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOAAddResultValue, ARG(BW::string, ARG(float, END)), OpenAutomateWrapper )


/*~ function OpenAutomateWrapper.bwOAPrepareForQuit
 *
 *  This function is called before calling quit to make sure quit doesn't fail due to singeltons issues
 */
/**
 *  This function is called before calling quit to make sure quit doesn't fail due to singeltons issues
 */
void bwOAPrepareForQuit()
{
#ifdef CONSUMER_CLIENT
	SetUnhandledExceptionFilter(SuccessExceptionFilter);
	BWOpenAutomate::s_doingExit = true;
#else
	extern int errorCodeForExitOnException;
	errorCodeForExitOnException = 0;
	BWOpenAutomate::s_doingExit = true;
#endif
}
PY_AUTO_MODULE_FUNCTION( RETVOID, bwOAPrepareForQuit, END, OpenAutomateWrapper )

BW_END_NAMESPACE

// open_automate_wrapper.cpp
