#ifndef SCRIPT_OBJECT_HPP
#define SCRIPT_OBJECT_HPP

#if !defined( SCRIPT_PYTHON ) && !defined( SCRIPT_OBJECTIVE_C )
// Neither one of the SCRIPT_OBJECTIVE_C nor SCRIPT_PYTHON preprocessor defines
// were defined, defaulting to SCRIPT_PYTHON.
#define SCRIPT_PYTHON

#endif // !defined( SCRIPT_PYTHON ) && !defined( SCRIPT_OBJECTIVE_C )

#if defined( SCRIPT_OBJECTIVE_C )
#include "oc_script_object.hpp"
#endif // SCRIPT_OBJECTIVE_C

#if defined( SCRIPT_PYTHON )
#include "py_script_object.hpp"
#endif // SCRIPT_PYTHON


#endif // SCRIPT_OBJECT_HPP
