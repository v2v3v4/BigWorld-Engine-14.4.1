#ifndef ASSET_PIPELINE_CONVERSION_TASK
#define ASSET_PIPELINE_CONVERSION_TASK

#include "cstdmf/bw_namespace.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/stdmf.hpp"
#include "cstdmf/cstdmf_windows.hpp"


BW_BEGIN_NAMESPACE

/// struct associated with an asset that is converted by the asset pipeline.
/// stores the information required for knowing how convert the asset
/// and a status representing how far along the conversion process it is.
struct ConversionTask
{
	/// filename of source asset
	BW::string	source_;

	/// id of the converter used to process this asset.
	uint64		converterId_;
	/// the version number of the converter used to process this asset.
	BW::string	converterVersion_;
	/// the parameters to initialise the converter for processing this asset.
	BW::string	converterParams_;

	/// enum representing the stage of the asset in the conversion process.
	/// DO NOT change the order of this enum as it essential to the logic of the asset pipeline
	enum
	{
		NEW,					/// <task is newly created. has not been queued for processing.
		QUEUED,					/// <task has been queued for processing.
		PROCESSING,				/// <task has been removed from queue and is being processed.
		NEEDS_PRIMARY_DEPS,		/// <task is being processed and needs its primary dependencies evaluated.
		NEEDS_SECONDARY_DEPS,	/// <task is being processed and needs its secondary dependencies evaluated.
		NEEDS_CONVERSION,		/// <task is being processed and needs to be converted.
		DONE,					/// <task has been completed.
		FAILED					/// <task has encountered an error whilst in a previous state.
	}			status_;

	/// a list of tasks that this task depends on
	typedef BW::vector< std::pair< const ConversionTask*, bool > > SubTaskList;
	SubTaskList subTasks_;

	/// the id of the thread that this task is waiting for. Initially set to the
	/// id of the thread processing this task, if a sub task is triggered, the
	/// thread id will be set to the id of the thread processing the sub task
	DWORD		threadId_;

	/// handle to the source file of this task. Used to prevent external changes to
	/// the source file during conversion
	HANDLE		fileHandle_;

	/// the converter id for an task that the asset pipeline does not know how to convert.
	static const uint64 s_unknownId = 0;
};

BW_END_NAMESPACE

#endif //ASSET_PIPELINE_CONVERSION_TASK
