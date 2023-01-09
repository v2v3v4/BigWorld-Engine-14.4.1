#ifndef RECORDING_OPTIONS_HPP
#define RECORDING_OPTIONS_HPP

#include "cstdmf/bw_namespace.hpp"

BW_BEGIN_NAMESPACE


enum RecordingOption
{
	RECORDING_OPTION_METHOD_DEFAULT, 	///< Use the method's defaults.
	RECORDING_OPTION_DO_NOT_RECORD, 	///< Do not record.
	RECORDING_OPTION_RECORD, 			///< Record.
	RECORDING_OPTION_RECORD_ONLY		///< Record-only, do not propagate.
};


BW_END_NAMESPACE

#endif // RECORDING_OPTIONS_HPP
