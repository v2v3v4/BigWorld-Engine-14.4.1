#ifndef TASK_INFO_HPP
#define TASK_INFO_HPP

#include "task_fwd.hpp"
#include "signal.hpp"

#include "asset_pipeline/conversion/conversion_task.hpp"

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_string_ref.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_namespace.hpp"

#include <memory>

BW_BEGIN_NAMESPACE

class TaskInfo : public std::enable_shared_from_this<TaskInfo>
{
public:
	typedef void CallbackSignature(TaskInfoPtr);
	typedef std::function<CallbackSignature> CallbackFunction;

	enum LogDetailLevel
	{
		LOG_DETAIL_ALL,
		LOG_DETAIL_ERRORS,
		LOG_DETAIL_WARNINGS
	};

public:
	TaskInfo();
	explicit TaskInfo(const ConversionTask * task);
	TaskInfo(TaskInfo && other);
	~TaskInfo();

	TaskInfo & operator=(TaskInfo && other);

	bool operator==(const ConversionTask * other) const;
	bool operator!=(const ConversionTask * other) const;


	Connection registerChangedCallback(CallbackFunction callback);


	// Setters
	void setState(TaskInfoState state);

	void appendToLog(const BW::WStringRef & message);
	void clearLog();

	void addOutput(const BW::StringRef & output);
	void clearOutputs();

	void addSubTask(TaskInfoPtr subTask);
	void resolveSubTasks(TaskStore & store);
	void clearSubTasks();

	void setErrors();
	void setWarnings();
	void clearErrorFlags();
	

	// Simple Getters
	const ConversionTask * conversionTask() const;

	TaskInfoState state() const;

	const BW::string & source() const;
	uint64 converterId() const;
	const BW::string & converterVersion() const;
	const BW::string & converterParams() const;

	const BW::wstring & log() const;
	const BW::vector< BW::string > & outputs() const;
	const BW::vector< TaskInfoPtr > & subTasks() const;

	bool hasFailed() const;
	bool hasErrors() const;
	bool hasWarnings() const;

	enum Result
	{
		RESULT_SUCCESS,
		RESULT_WARNING,
		RESULT_ERROR,
	};
	Result getResult() const;

	// Pretty Getters
	BW::wstring getFormattedName() const;

	BW::wstring getFormattedLog( LogDetailLevel detailLevel ) const;

	BW::wstring getTextDump() const;

private:
	TaskInfo(const TaskInfo & other);
	TaskInfo & operator=(const TaskInfo & other);

	void emitChangedSignal();


	const ConversionTask * task_;
	BW::vector< TaskInfoPtr > subTasks_;

	TaskInfoState state_;
	BW::wstring log_;
	BW::vector< BW::string > outputs_;
	bool hasError_;
	bool hasWarning_;

	Signal<CallbackSignature> changedSignal_;
};


BW_END_NAMESPACE

#endif // TASK_INFO_HPP
