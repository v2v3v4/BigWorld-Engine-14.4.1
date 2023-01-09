CMAKE_MINIMUM_REQUIRED( VERSION 2.8 )
PROJECT( re2 )

ADD_DEFINITIONS( -DCOMPILER_MSVC )
ADD_DEFINITIONS( -DNOMINMAX )
ADD_DEFINITIONS("-wd4018")
ADD_DEFINITIONS("-wd4244")
ADD_DEFINITIONS("-wd4267")
ADD_DEFINITIONS("-wd4722")
ADD_DEFINITIONS("-wd4800")
ADD_DEFINITIONS("-wd4805")
ADD_DEFINITIONS("-wd4996")

SET( INSTALL_HFILES
	re2/filtered_re2.h
	re2/re2.h
	re2/set.h
	re2/stringpiece.h
	re2/variadic_function.h
)
SOURCE_GROUP( "" FILES ${INSTALL_HFILES} )

SET( HFILES
	util/arena.h
	util/atomicops.h
	util/benchmark.h
	util/flags.h
	util/logging.h
	util/mutex.h
	util/pcre.h
	util/random.h
	util/sparse_array.h
	util/sparse_set.h
	util/test.h
	util/utf.h
	util/util.h
	util/valgrind.h
	re2/filtered_re2.h
	re2/prefilter.h
	re2/prefilter_tree.h
	re2/prog.h
	re2/re2.h
	re2/regexp.h
	re2/set.h
	re2/stringpiece.h
	re2/testing/exhaustive_tester.h
	re2/testing/regexp_generator.h
	re2/testing/string_generator.h
	re2/testing/tester.h
	re2/unicode_casefold.h
	re2/unicode_groups.h
	re2/variadic_function.h
	re2/walker-inl.h
)
SOURCE_GROUP( "" FILES ${HFILES} )

SET( CFILES
	util/arena.cc
	util/hash.cc
	util/rune.cc
	util/stringpiece.cc
	util/stringprintf.cc
	util/strutil.cc
	util/valgrind.cc
	re2/bitstate.cc
	re2/compile.cc
	re2/dfa.cc
	re2/filtered_re2.cc
	re2/mimics_pcre.cc
	re2/nfa.cc
	re2/onepass.cc
	re2/parse.cc
	re2/perl_groups.cc
	re2/prefilter.cc
	re2/prefilter_tree.cc
	re2/prog.cc
	re2/re2.cc
	re2/regexp.cc
	re2/set.cc
	re2/simplify.cc
	re2/tostring.cc
	re2/unicode_casefold.cc
	re2/unicode_groups.cc
)
SOURCE_GROUP( "" FILES ${CFILES} )

ADD_LIBRARY( re2
	${INSTALL_HFILES}
	${HFILES}
	${CFILES}
)

TARGET_INCLUDE_DIRECTORIES( re2 PUBLIC . )

BW_PROJECT_CATEGORY( re2 "ThirdParty" )