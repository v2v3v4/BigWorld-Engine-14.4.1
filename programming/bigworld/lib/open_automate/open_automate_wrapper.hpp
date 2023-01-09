#ifndef OPEN_AUTOMATE_WRAPPER
#define OPEN_AUTOMATE_WRAPPER

#include "pyscript/script.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "cstdmf/bw_string.hpp"
#include "pyscript/pickler.hpp"
#include "OpenAutomate.h"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

/*~ class OpenAutomateWrapper.BWOpenAutomate
 *
 *	This class is used to wrap different OpenAutomate enum values and global values.
 */
/**
 *	This class is used to wrap different OpenAutomate enum values and global values.
 */
class BWOpenAutomate
	{
public:
	static const BW::string OPEN_AUTOMATE_ARGUMENT;
	static const BW::string OPEN_AUTOMATE_TEST_ARGUMENT;

	typedef enum
	{
	 BWOA_CMD_EXIT                = 0,
	 BWOA_CMD_RUN                 = 1,
	 BWOA_CMD_GET_ALL_OPTIONS     = 2,
	 BWOA_CMD_GET_CURRENT_OPTIONS = 3,
	 BWOA_CMD_SET_OPTIONS         = 4,
	 BWOA_CMD_GET_BENCHMARKS      = 5,
	 BWOA_CMD_RUN_BENCHMARK       = 6,
	 BWOA_CMD_RESTART			  = 10000,
	 BWOA_CMD_AUTO_DETECT		  = 10001,
	} eOACommandType;
	PY_BEGIN_ENUM_MAP( eOACommandType, BWOA_ )
		PY_ENUM_VALUE(BWOA_CMD_EXIT )
		PY_ENUM_VALUE(BWOA_CMD_RUN )
		PY_ENUM_VALUE(BWOA_CMD_GET_ALL_OPTIONS )
		PY_ENUM_VALUE(BWOA_CMD_GET_CURRENT_OPTIONS )
		PY_ENUM_VALUE(BWOA_CMD_SET_OPTIONS )
		PY_ENUM_VALUE(BWOA_CMD_GET_BENCHMARKS )
		PY_ENUM_VALUE(BWOA_CMD_RUN_BENCHMARK )
		PY_ENUM_VALUE(BWOA_CMD_RESTART)
		PY_ENUM_VALUE(BWOA_CMD_AUTO_DETECT)
	PY_END_ENUM_MAP()


	typedef enum
	{
	  BWOA_TYPE_INVALID  = 0,
	  BWOA_TYPE_STRING  = 1,
	  BWOA_TYPE_INT     = 2,
	  BWOA_TYPE_FLOAT   = 3,
	  BWOA_TYPE_ENUM    = 4,
	  BWOA_TYPE_BOOL    = 5
	} eOAOptionDataType;
	PY_BEGIN_ENUM_MAP( eOAOptionDataType, BWOA_ )
	  PY_ENUM_VALUE(BWOA_TYPE_INVALID)
	  PY_ENUM_VALUE(BWOA_TYPE_STRING)
	  PY_ENUM_VALUE(BWOA_TYPE_INT)
	  PY_ENUM_VALUE(BWOA_TYPE_FLOAT)
	  PY_ENUM_VALUE(BWOA_TYPE_ENUM)
	  PY_ENUM_VALUE(BWOA_TYPE_BOOL)
	PY_END_ENUM_MAP()
	

	typedef enum
	{
	  BWOA_COMP_OP_INVALID           = 0,
	  BWOA_COMP_OP_EQUAL             = 1,
	  BWOA_COMP_OP_NOT_EQUAL         = 2,
	  BWOA_COMP_OP_GREATER           = 3,
	  BWOA_COMP_OP_LESS              = 4,
	  BWOA_COMP_OP_GREATER_OR_EQUAL  = 5,
	  BWOA_COMP_OP_LESS_OR_EQUAL     = 6,
	} eOAComparisonOpType;
	PY_BEGIN_ENUM_MAP( eOAComparisonOpType, BWOA_ )
		PY_ENUM_VALUE(BWOA_COMP_OP_INVALID)
		PY_ENUM_VALUE(BWOA_COMP_OP_EQUAL)
		PY_ENUM_VALUE(BWOA_COMP_OP_NOT_EQUAL)
		PY_ENUM_VALUE(BWOA_COMP_OP_GREATER)
		PY_ENUM_VALUE(BWOA_COMP_OP_LESS)
		PY_ENUM_VALUE(BWOA_COMP_OP_GREATER_OR_EQUAL)
		PY_ENUM_VALUE(BWOA_COMP_OP_LESS_OR_EQUAL)
	PY_END_ENUM_MAP()


	//Flag to show if we are currently running a test or not
	static bool s_runningBenchmark;
	static bool s_doingExit; 

};
PY_ENUM_CONVERTERS_DECLARE( BWOpenAutomate::eOACommandType )
PY_ENUM_CONVERTERS_DECLARE( BWOpenAutomate::eOAOptionDataType )
PY_ENUM_CONVERTERS_DECLARE( BWOpenAutomate::eOAComparisonOpType )


/*~ class OpenAutomateWrapper.OAVersionStructWrapper
 *
 *	This class wraps a oaVersionStruct which gives version information about OpenAutomate
 */
/**
 *	This class wraps a oaVersionStruct which gives version information about OpenAutomate
 */
class OAVersionStructWrapper : public PyObjectPlus
{
	Py_Header( OAVersionStructWrapper, PyObjectPlus )

public:
	OAVersionStructWrapper(PyTypeObject * pType = &s_type_);
	void fromOAVersion(oaVersionStruct& tempVersion);

	PY_FACTORY_DECLARE()
	PY_RW_ATTRIBUTE_DECLARE( major_, major )
	PY_RW_ATTRIBUTE_DECLARE( minor_, minor )
	PY_RW_ATTRIBUTE_DECLARE( custom_, custom )
	PY_RW_ATTRIBUTE_DECLARE( build_, build )

public:
	uint32 major_;  
	uint32 minor_; 
	uint32 custom_;
	uint32 build_;
};
PY_SCRIPT_CONVERTERS_DECLARE( OAVersionStructWrapper )


/*~ class OpenAutomateWrapper.OACommandWrapper
 *
 *	This class wraps an oaCommand which gives information about a command
 */
/**
 *	This class wraps an oaCommand which gives information about a command
 */
class OACommandWrapper : public PyObjectPlus
{
	Py_Header( OACommandWrapper, PyObjectPlus )

public:
	OACommandWrapper(PyTypeObject * pType = &s_type_);

	PY_FACTORY_DECLARE()
	PyObject * pyGet_benchmark();
	PY_RO_ATTRIBUTE_SET( benchmark )

	oaCommand internalCommand_;
};
PY_SCRIPT_CONVERTERS_DECLARE( OACommandWrapper )


/*~ class OpenAutomateWrapper.OANamedOptionWrapper
 *
 *	This class wraps an oaNamedOptionStruct which send information about a named option to the OpenAutomate program
 */
/**
 *	This class wraps an oaNamedOptionStruct which send information about a named option to the OpenAutomate program
 */
class OANamedOptionWrapper : public PyObjectPlus
{
	Py_Header( OANamedOptionWrapper, PyObjectPlus )

public:
	OANamedOptionWrapper(PyTypeObject * pType = &s_type_);
	OANamedOptionWrapper(oaNamedOptionStruct* option, PyTypeObject * pType = &s_type_);
	void toNamedOption(oaNamedOptionStruct& option) const;

	PY_FACTORY_DECLARE()

	PY_DEFERRED_ATTRIBUTE_DECLARE(dataType)
	PY_DEFERRED_ATTRIBUTE_DECLARE(name)
	PY_DEFERRED_ATTRIBUTE_DECLARE(enumValue)
	PY_DEFERRED_ATTRIBUTE_DECLARE(minValueInt)
	PY_DEFERRED_ATTRIBUTE_DECLARE(minValueFloat)
	PY_DEFERRED_ATTRIBUTE_DECLARE(maxValueInt)
	PY_DEFERRED_ATTRIBUTE_DECLARE(maxValueFloat)
	PY_DEFERRED_ATTRIBUTE_DECLARE(numSteps)
	PY_DEFERRED_ATTRIBUTE_DECLARE(parentName)
	PY_DEFERRED_ATTRIBUTE_DECLARE(comparisonOp)

	void setComparison(BWOpenAutomate::eOAOptionDataType optionType, PyObjectPtr comparisonValArg);

	PY_AUTO_METHOD_DECLARE( RETVOID, setComparison, ARG( BWOpenAutomate::eOAOptionDataType, ARG(PyObjectPtr, END )))

public:
	oaNamedOptionStruct internalOption_;
	BW::string name_;             
	BW::string enumValue_;     
	BW::string parentName_;             
	BW::string comparisonVal_;             
};
PY_SCRIPT_CONVERTERS_DECLARE( OANamedOptionWrapper )

BW_END_NAMESPACE

#endif
/*bw_open_automate.hpp*/
