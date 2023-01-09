#ifndef SCRIPT_OUTPUT_WRITER_HPP
#define SCRIPT_OUTPUT_WRITER_HPP

#if !defined( SCRIPT_PYTHON ) && !defined( SCRIPT_OBJECTIVE_C )
// Neither one of the SCRIPT_OBJECTIVE_C nor SCRIPT_PYTHON preprocessor defines
// were defined, defaulting to SCRIPT_PYTHON.
#define SCRIPT_PYTHON

#endif // !defined( SCRIPT_PYTHON ) && !defined( SCRIPT_OBJECTIVE_C )

#if defined( SCRIPT_OBJECTIVE_C )
#error ScriptOutputWriter not yet implemented for SCRIPT_OBJECTIVE_C
#include "oc_script_output_writer.hpp"
#endif // SCRIPT_OBJECTIVE_C

#if defined( SCRIPT_PYTHON )
#include "py_script_output_writer.hpp"
#endif // SCRIPT_PYTHON

#endif // SCRIPT_OUTPUT_WRITER_HPP
