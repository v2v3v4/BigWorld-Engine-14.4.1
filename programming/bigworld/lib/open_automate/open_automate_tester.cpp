#include "pyscript/script.hpp"
#include "open_automate_tester.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "OpenAutomate_internal.h"
#include <python.h>
#include "open_automate_wrapper.hpp"
#include <fstream>


BW_BEGIN_NAMESPACE

//for traversing the commands in the testing file
static DataSectionPtr pTop_;
static DataSectionPtr pOptions_;
static DataSectionPtr currentCommand_;
static DataSectionPtr currentCommandLocation_;
//value to set to the command pointer at the end of the test
static int END_COMMAND_LOCATION = 10000;
//for setting options
static int setOptionsCounter_;
//stream to write the log file
std::ofstream fileStream;
//store the prev command
oaCommandType prevCommandType_ = (oaCommandType)-100;	
//the current benchmark being run
static BW::string currentBenchmark;
//should we restart the app (usually after setting options we restart the app)
bool shouldRestart = false;
//extern to override open automate function table.
extern "C" oaiFunctionTable FuncTable;
extern "C" oaBool InitFlag;
//constant locations and files.
static char TESTING_FILE[] = "scripts/testing/openautomate/oatest3.xml";
static char ALL_OPTINOS_FILE[] = "scripts/testing/openautomate/alloptions.xml";
static char TEST_LOG[] = "test_log.log";
//XML Tags
static const char CURRENT_LOCATION_TAG[] = "currentLocation";
static const char COMMAND_TAG[] = "command";
static const char OPTION_MAIN_TAG[] = "option";
static const char OPTION_CHILD_TAG_VALUE[] = "optionValue";
BW::set<BW::string> benchmarkSet;

//forward declerations
static void dumpAllOptions();
typedef BW::vector<BW::string> StringVector;
/**
* This class stores information about an option and its current/optional values
*/
class OptionInfo
{
public:
	OptionInfo() 
	{
		currentValue_ = INVALID_VALUE;
		expectedValue_ = INVALID_VALUE;
	}
	BW::string name;
	StringVector values;
	int currentValue_;
	int expectedValue_;
	static const int INVALID_VALUE = -1;
};
//option map
typedef BW::map<BW::string, OptionInfo> OptionsMap;
OptionsMap optionsMap;


static BW::string getCurrentTimeString()
{
	time_t now = time( &now );
	return ctime(&now);
}


//used to return specific commands at the beginning of a test.
static int commandNumber = 0;
//use to store in memory the test string being returned
static BW::string retBenchmark;
/**
* Override function to get the next command
*/
static oaCommandType testOAGetNextCommand(oaCommand *command)
{
	//always get all options and current options
	if (commandNumber == 0)
	{
		commandNumber++;
		return OA_CMD_GET_ALL_OPTIONS;
	}
	else if (commandNumber == 1)
	{
		commandNumber++;
		return OA_CMD_GET_CURRENT_OPTIONS;
	}
	if (prevCommandType_ == OA_CMD_GET_ALL_OPTIONS)
	{
		//print all options to a file	
		dumpAllOptions();
	}
	if (commandNumber == 2)
	{
		commandNumber++;
		return OA_CMD_GET_BENCHMARKS;
	}
	if (prevCommandType_ == OA_CMD_SET_OPTIONS)
	{
		//we set options and allow app to restart	
		//now need to check all params
		command->Type = OA_CMD_GET_CURRENT_OPTIONS;
		prevCommandType_ = OA_CMD_GET_CURRENT_OPTIONS;
		shouldRestart = true;
		return command->Type;
	}
	if (shouldRestart) 
	{
		//only if we have more commands we should restart
		if (currentCommandLocation_->asInt() < pTop_->countChildren() )
		{
			command->Type = (oaCommandType)BWOpenAutomate::BWOA_CMD_RESTART;
			return command->Type;
		}
	}

	//loop to find the next command
	while (true)
	{
		if (currentCommandLocation_->asInt() >= pTop_->countChildren() )
		{
			INFO_MSG("Exiting as there are no more OpenAutomate commands.\n");
			command->Type = OA_CMD_EXIT;
			//just to make sure make command really big so even if we add commands we don't have any problems
			currentCommand_ = NULL;
			currentCommandLocation_->setInt(END_COMMAND_LOCATION);
			currentCommandLocation_->save();
			pTop_->save();

			return command->Type;
		}
		currentCommand_ = pTop_->openChild(currentCommandLocation_->asInt());
		currentCommandLocation_->setInt(currentCommandLocation_->asInt() + 1);
		currentCommandLocation_->save();
		pTop_->save();

		if (currentCommand_->sectionName() != COMMAND_TAG)
		{
			continue;
		}

		BW::string currentCommandString = currentCommand_->asString();
		PyObject* tempObj = Script::getData(currentCommandString);
		BWOpenAutomate::eOACommandType eOAcommand;
		Script::setData(tempObj, eOAcommand, "CMD_RUN");
		Py_DecRef(tempObj);
		command->Type = (oaCommandType)eOAcommand;
		bool shouldRunCommand = true;
		if (command->Type == OA_CMD_RUN_BENCHMARK)
		{
			DataSectionPtr benchmarkSect = currentCommand_->openChild(0);
			retBenchmark = benchmarkSect->asString();
			MF_ASSERT("benchmark" == benchmarkSect->sectionName());
			command->BenchmarkName = retBenchmark.c_str();
			currentBenchmark = retBenchmark;
			if (benchmarkSet.find(retBenchmark) == benchmarkSet.end())
			{
				//benchmark not available in this system			
				fileStream << "skipped benchmark " << retBenchmark << " time " <<getCurrentTimeString() << std::endl; 
				shouldRunCommand = false;
			}
			else
			{
				fileStream << "started benchmark " << retBenchmark << " time " <<getCurrentTimeString() << std::endl; 
			}
		}
		else if (command->Type == OA_CMD_GET_ALL_OPTIONS)
		{
			optionsMap.clear();
		}
		else if (command->Type == OA_CMD_SET_OPTIONS)
		{
			setOptionsCounter_ = 0;
		}
		prevCommandType_ = command->Type;
		if (shouldRunCommand)
		{
			return command->Type;
		}
		else 
		{
			//do nothing
		}
	}
}


static BW::string optionName;
static BW::string optionValue;
/**
* Override function to get the next command option
*/
static oaNamedOption *testOAGetNextOption(void)
{
 	if (!currentCommand_ || setOptionsCounter_ >= currentCommand_->countChildren())
	{
		return NULL;
	}
	DataSectionPtr currentOption = currentCommand_->openChild(setOptionsCounter_++);
	//init the named option being returned
	static oaNamedOption ret;
	ret.DataType = OA_TYPE_ENUM;
	MF_ASSERT(OPTION_MAIN_TAG == currentOption->sectionName());
	optionName = currentOption->asString();
	ret.Name = optionName.c_str();
	DataSectionPtr currentOptionValue = currentOption->openChild(0);
	MF_ASSERT(currentOptionValue != NULL);
	MF_ASSERT(OPTION_CHILD_TAG_VALUE == currentOptionValue->sectionName());
	optionValue = currentOptionValue->asString();
	ret.Value.Enum = (oaString)optionValue.c_str();
	//set the expected value
	OptionsMap::iterator mapIter;
	mapIter = optionsMap.find(optionName);
	if (mapIter == optionsMap.end())
	{
		ERROR_MSG("Could not find option %s\n", optionName.c_str());
	}
	MF_ASSERT(mapIter != optionsMap.end());
	StringVector::iterator iter = std::find( mapIter->second.values.begin(), mapIter->second.values.end(), optionValue ); 
	if (iter == mapIter->second.values.end())
	{
		ERROR_MSG("Could not find option value %s\n", optionValue.c_str());
	}
	MF_ASSERT(iter != mapIter->second.values.end());
	mapIter->second.expectedValue_ = (int)(iter - mapIter->second.values.begin());
	return &ret;
}


/**
* Override function to add the next option
*/
static void testOAAddOption(const oaNamedOption *option)
{
	//add the option to the optionMap class
	OptionsMap::iterator mapIter;
	if ((mapIter = optionsMap.find(option->Name)) == optionsMap.end())
	{
		//add a new optionsInfo
		OptionInfo optionInfo;
		optionInfo.name = option->Name;
		optionInfo.currentValue_ = OptionInfo::INVALID_VALUE;
		optionsMap[option->Name] = optionInfo;
		mapIter = optionsMap.find(option->Name);
	}
	mapIter->second.values.push_back(option->Value.Enum);
}


static void dumpAllOptions()
{
	// ok, open the file then
	pOptions_ = BWResource::instance().rootSection()->openSection( ALL_OPTINOS_FILE, true );
	if (!pOptions_)
	{
		ERROR_MSG("failed creating the options file\n");
		return;
	}
	else 
	{
		// clear it out
		pOptions_->delChildren();
	}
	OptionsMap::iterator mapIter;
	for (mapIter = optionsMap.begin(); mapIter != optionsMap.end(); mapIter++)
	{
		DataSectionPtr pNew = pOptions_->newSection(OPTION_MAIN_TAG);
		pNew->setParent(pOptions_);
		pNew->setString(mapIter->second.name);
		StringVector::iterator iter;
		for (iter = mapIter->second.values.begin(); iter != mapIter->second.values.end(); iter++)
		{
			DataSectionPtr child = pNew->newSection(OPTION_CHILD_TAG_VALUE);
			child->setParent(pNew);
			child->setString(*iter);
			child->save();
		}
		pNew->save();
		pOptions_->save();
	}
}


/**
* Override function to add the next option value
*/
static void testOAAddOptionValue(const oaChar *name, 
                      oaOptionDataType value_type,
                      const oaValue *value)
{
	OptionsMap::iterator mapIter;
	if ((mapIter = optionsMap.find(name)) == optionsMap.end())
	{
		ERROR_MSG("Couldn't find option info for the specified option\n");
		return;
	}
	StringVector::iterator iter = std::find( mapIter->second.values.begin(), mapIter->second.values.end(), value->Enum ); 
	if (iter == mapIter->second.values.end()) 
	{
		ERROR_MSG("Couldn't find option info for the specified option\n");
		return;
	}
	mapIter->second.currentValue_ = (int)(iter - mapIter->second.values.begin());
	INFO_MSG("option %s value %s\n", mapIter->second.name.c_str(), mapIter->second.values[mapIter->second.currentValue_].c_str());
	MF_ASSERT(mapIter->second.expectedValue_ == OptionInfo::INVALID_VALUE || mapIter->second.currentValue_ == mapIter->second.expectedValue_);
}


/**
* Override function to add the next benchmark
*/
static void testOAAddBenchmark(const oaChar *benchmark_name)
{
	benchmarkSet.insert(benchmark_name);
}


/**
* Override function to send a signal
*/
static oaBool testOASendSignal(oaSignalType signal, void *param)
{

	return OA_OFF;
}


/**
* Override function to send a message
*/
static void testOAInitMessage(oaMessage *message)
{

}

/******************************************************************************* 
 * Callback functions for benchmark mode
 ******************************************************************************/


/**
* Override function to inform OpenAutomate on starting a benchmark
*/
static void testOAStartBenchmark(void)
{

}


/**
* Override function to inform OpenAutomate on displaying a frame
*/
static void testOADisplayFrame(oaFloat t)
{

}


/**
* Override function to inform OpenAutomate on a test result
*/
static void testOAAddResultValue(const oaChar *name, 
                      oaOptionDataType value_type,
                      const oaValue *value)
{
	if (value_type == OA_TYPE_FLOAT) 
	{
		fileStream << "benchmark:" << currentBenchmark << ":result name:" << name << ":value:" << value->Float << ":" << std::endl; 
	}
}


/**
* Override function to inform OpenAutomate on a frame value
*/
static void testOAAddFrameValue(const oaChar *name, 
                     oaOptionDataType value_type,
                     const oaValue *value)
{

}


/**
* Override function to inform OpenAutomate on ending a benchmark
*/
static void testOAEndBenchmark(void)
{
		fileStream << "Ended benchmark time " << getCurrentTimeString() << std::endl; 
}


/**
* Init the internal testing
*/
oaBool testOAInit(const oaChar *init_str, oaVersion *version)
{
	fileStream.open(TEST_LOG,std::fstream::out | std::fstream::app);
	fileStream << "Testing started " << getCurrentTimeString() << std::endl; 
	version->Major = 1;
	version->Minor = 0;
	version->Custom = 0;
	version->Build = 0;
	//init the functable
	FuncTable.GetNextCommand = testOAGetNextCommand;
	FuncTable.GetNextOption = testOAGetNextOption;
	FuncTable.AddOption = testOAAddOption;
	FuncTable.AddOptionValue = testOAAddOptionValue;
	FuncTable.AddBenchmark = testOAAddBenchmark;
	FuncTable.AddResultValue = testOAAddResultValue;
	FuncTable.StartBenchmark = testOAStartBenchmark;
	FuncTable.DisplayFrame = testOADisplayFrame;
	FuncTable.EndBenchmark = testOAEndBenchmark;
	FuncTable.AddFrameValue = testOAAddFrameValue;
	FuncTable.SendSignal= testOASendSignal;
	InitFlag = OA_ON;
	BW::string file = TESTING_FILE;
	if (BWOpenAutomate::OPEN_AUTOMATE_TEST_ARGUMENT != init_str)
	{
		file = init_str;
	}
	pTop_ = BWResource::instance().rootSection()->openSection(file); 
	MF_ASSERT(pTop_);
	currentCommandLocation_ = pTop_->openChild(0);
	MF_ASSERT(CURRENT_LOCATION_TAG == currentCommandLocation_->sectionName());
	//restart if we reached the end
	if (currentCommandLocation_->asInt() >= pTop_->countChildren() || /*skip the currentCommandLocation tag */currentCommandLocation_->asInt() == 0)
	{
		currentCommandLocation_->setInt(1);
		currentCommandLocation_->save();
		pTop_->save();
	}
#ifndef BWCLIENT_AS_PYTHON_MODULE
	extern bool exitWithErrorCodeOnException;
	exitWithErrorCodeOnException = false;
#endif
	return (oaBool)(pTop_ != NULL);
}

BW_END_NAMESPACE

// open_automate_tester.cpp
