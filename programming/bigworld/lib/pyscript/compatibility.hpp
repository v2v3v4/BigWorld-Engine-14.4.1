#ifndef COMPATIBILITY_HPP
#define COMPATIBILITY_HPP

// This magic is needed to enable building the libraries against potentially
// older version of Python that are installed by default in CentOS
#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 5 && !defined( HAVE_PY_SSIZE_T )

 // The following magic was taken from Python 2.5's PC/pyconfig.h
 
 /* Define like size_t, omitting the "unsigned" */
 #ifdef MS_WIN64
  typedef __int64 ssize_t;
  #define HAVE_SSIZE_T 1
 #elif MS_WIN32
  /* _W64 is not defined for VC6 or eVC4 */
  #ifndef _W64
   #define _W64
  #endif

  typedef _W64 int ssize_t;
  #define HAVE_SSIZE_T 1
 #endif

// // This magic was taken from Python 2.5's pyport.h
// #ifdef HAVE_SSIZE_T
 // TODO: can we safely assume that all supported platforms now have ssize_t?
 typedef int			Py_ssize_t;
 //typedef ssize_t		Py_ssize_t;
// #elif SIZEOF_VOID_P == SIZEOF_SIZE_T
//  typedef Py_intptr_t	Py_ssize_t;
// #else
//  #   error "Unable to find a suitable typedef for Py_ssize_t."
// #endif

 // Ensure we don't redefine
 #define HAVE_PY_SSIZE_T 1
#endif

#endif // COMPATIBILITY_HPP

