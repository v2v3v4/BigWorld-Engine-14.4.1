#include "task_info.hpp"
#include "task_store.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/string_utils.hpp"

BW_BEGIN_NAMESPACE


TaskInfo::TaskInfo() :
	task_(nullptr),
	subTasks_(),
	state_(NONE),
	log_(),
	outputs_(),
	hasError_(false),
	hasWarning_(false)
{
}

TaskInfo::TaskInfo(const ConversionTask * task) :
	task_(task),
	subTasks_(),
	state_(NONE),
	log_(),
	outputs_(),
	hasError_(false),
	hasWarning_(false)
{
}

TaskInfo::TaskInfo(TaskInfo && other)
{
	*this = std::move(other);
}

TaskInfo::~TaskInfo()
{
}

TaskInfo & TaskInfo::operator=(TaskInfo && other)
{
	if (&other != this)
	{
		task_ = other.task_;
		subTasks_ = std::move(other.subTasks_);
		state_ = other.state_;
		log_ = std::move(other.log_);
		outputs_ = std::move(other.outputs_);
		hasError_ = other.hasError_;
		hasWarning_ = other.hasWarning_;
	}

	return *this;
}

bool TaskInfo::operator==(const ConversionTask * other) const
{
	return task_ == other;
}

bool TaskInfo::operator!=(const ConversionTask * other) const
{
	return task_ != other;
}


Connection TaskInfo::registerChangedCallback(CallbackFunction callback)
{
	return changedSignal_.connect(callback);
}

void TaskInfo::emitChangedSignal()
{
	changedSignal_.call(shared_from_this());
}


void TaskInfo::setState(TaskInfoState state)
{
	state_ = state;
	emitChangedSignal();
}

void TaskInfo::appendToLog(const BW::WStringRef & message)
{
	log_.append(message.data(), message.length());
	emitChangedSignal();
}

void TaskInfo::clearLog()
{
	log_.clear();
	emitChangedSignal();
}

void TaskInfo::addOutput(const BW::StringRef & output)
{
	outputs_.push_back(output.to_string());
	emitChangedSignal();
}

void TaskInfo::clearOutputs()
{
	outputs_.clear();
	emitChangedSignal();
}

void TaskInfo::addSubTask(TaskInfoPtr subTask)
{
	subTasks_.push_back(subTask);
	emitChangedSignal();
}

void TaskInfo::resolveSubTasks(TaskStore & store)
{
	subTasks_.clear();
	std::transform(std::begin(task_->subTasks_), std::end(task_->subTasks_), 
		std::back_inserter(subTasks_), 
		[&store](ConversionTask::SubTaskList::const_reference item) -> TaskInfoPtr
	{
		return store.findOrCreateTaskInfo(item.first);
	});
}

void TaskInfo::clearSubTasks()
{
	subTasks_.clear();
	emitChangedSignal();
}

void TaskInfo::setErrors()
{
	hasError_ = true;
}

void TaskInfo::setWarnings()
{
	hasWarning_ = true;
}

void TaskInfo::clearErrorFlags()
{
	hasWarning_ = hasError_ = false;
}


const ConversionTask * TaskInfo::conversionTask() const
{
	return task_;
}

TaskInfoState TaskInfo::state() const
{
	return state_;
}

const BW::string & TaskInfo::source() const
{
	MF_ASSERT(task_ != nullptr);
	return task_->source_;
}

uint64 TaskInfo::converterId() const
{
	MF_ASSERT(task_ != nullptr);
	return task_->converterId_;
}

const BW::string & TaskInfo::converterVersion() const
{
	MF_ASSERT(task_ != nullptr);
	return task_->converterVersion_;
}

const BW::string & TaskInfo::converterParams() const
{
	MF_ASSERT(task_ != nullptr);
	return task_->converterParams_;
}

const BW::wstring & TaskInfo::log() const
{
	return log_;
}

const BW::vector< BW::string > & TaskInfo::outputs() const
{
	return outputs_;
}

const BW::vector< TaskInfoPtr > & TaskInfo::subTasks() const
{
	return subTasks_;
}

bool TaskInfo::hasFailed() const
{
	return task_->status_ == ConversionTask::FAILED;
}

bool TaskInfo::hasErrors() const
{
	return hasError_;
}

bool TaskInfo::hasWarnings() const
{
	return hasWarning_;
}

TaskInfo::Result TaskInfo::getResult() const
{
	if ( hasErrors() || hasFailed() )
	{
		return RESULT_ERROR;
	}
	if ( hasWarnings() )
	{
		return RESULT_WARNING;
	}
	return RESULT_SUCCESS;
}

BW::wstring TaskInfo::getFormattedName() const
{
	BW::wstring result;

	bw_utf8tow(source(), result);

	return result;
}

BW::wstring TaskInfo::getFormattedLog( LogDetailLevel detailLevel ) const
{
	BW::wstring result;
	const BW::wstring & inputLog = log();

	const size_t inputLength = inputLog.length();
	result.reserve(inputLength + 100);

	size_t lineStart = 0;
	size_t lineEnd = lineStart;
	while (lineStart < inputLength)
	{
		lineEnd = inputLog.find(L'\n', lineStart);
		if (lineEnd == std::wstring::npos)
		{
			lineEnd = inputLength;
		}

		int crPadding = (inputLog[lineEnd - 1] == L'\r' ? 1 : 0);

		bool addLine = true;
		if (detailLevel != LOG_DETAIL_ALL)
		{
			const wchar_t * detailComp = L"ERROR";
			
			if (detailLevel == LOG_DETAIL_WARNINGS)
			{
				detailComp = L"WARNING";
			}

			BW::wstring line = inputLog.substr( lineStart, lineEnd - crPadding - lineStart );
			addLine = line.find( detailComp ) != BW::wstring::npos;
		}

		if (addLine)
		{
			result.append(&inputLog[lineStart], &inputLog[lineEnd - crPadding]);
			result.append(L"\r\n");
		}

		lineStart = lineEnd + 1;
	}

	return result;
}

BW::wstring TaskInfo::getTextDump() const
{
	static const BW::wstring::value_type * newline = L"\r\n";

	BW::wstring result;

	// Gather Input

	BW::wstring name = getFormattedName();
	BW::wstring convertParams = bw_utf8tow(converterParams());
	BW::wstring log = getFormattedLog( TaskInfo::LOG_DETAIL_ALL );

	BW::wstring outputs;
	for (auto& output : outputs_)
	{
		outputs.append(bw_utf8tow(output));
		outputs.append(newline);
	}

	BW::wstring depends;
	for (auto& subTask : subTasks_)
	{
		depends.append(subTask->getFormattedName());
		depends.append(newline);
	}

	// Format

	result.reserve(name.size() + convertParams.size() + log.size() + outputs.size() + depends.size() + 100);
	result.append(L"Task: ").append(name).append(newline);
	result.append(L"Converter Params: \"").append(convertParams).append(L"\"").append(newline);
	result.append(L"Outputs:").append(newline).append(outputs).append(newline);
	result.append(L"Dependencies:").append(newline).append(depends).append(newline);
	result.append(L"Log:").append(newline).append(log).append(newline);

	return result;
}


BW_END_NAMESPACE
