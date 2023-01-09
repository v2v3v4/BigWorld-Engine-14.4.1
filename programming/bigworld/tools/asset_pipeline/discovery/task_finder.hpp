#ifndef ASSET_PIPELINE_TASK_FINDER
#define ASSET_PIPELINE_TASK_FINDER

#include "asset_pipeline/discovery/conversion_rules.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/stringmap.hpp"

BW_BEGIN_NAMESPACE

class Compiler;

class TaskFinder
{
public:
	TaskFinder( Compiler& compiler,
				ConversionRules& rules );
	~TaskFinder();

	/* iterates recursively over directories to find root tasks. */
	void findTasks( const BW::StringRef& directory );

	/* matches file conversion rules to task. */
	ConversionTask & getTask( const BW::StringRef & filename );

	/* matches file conversion rules to task. */
	ConversionTask * getTask( const BW::StringRef & filename, const bool bRoot );

private:
	/* iterate a file for a root task */
	void iterateFile( const BW::StringRef& file );
	/* iterate a directory for root tasks */
	void iterateDirectory( const BW::StringRef& directory );

	Compiler &		 compiler_;
	ConversionRules& rules_;

	StringHashMap<ConversionTask *> tasks_;
	ReadWriteLock					tasksLock_;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_TASK_FINDER
