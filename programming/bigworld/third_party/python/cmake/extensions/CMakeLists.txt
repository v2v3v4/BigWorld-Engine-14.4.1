set(MSVC_BUILTIN )
if(MSVC)
    set(MSVC_BUILTIN BUILTIN)
endif(MSVC)

# BW client list of builtin extenstions begin
SET(BUILTIN_CSTRINGIO ON)
SET(BUILTIN_CPICKLE ON)
SET(BUILTIN_SOCKET ON)
SET(BUILTIN_PYEXPAT ON)
SET(BUILTIN_SELECT ON)
SET(BUILTIN_HASHLIB ON)
SET(BUILTIN_ZLIB ON)
SET(BUILTIN_TIME ON)
SET(BUILTIN_BINASCII ON)
SET(BUILTIN_RANDOM ON)
SET(BUILTIN_COLLECTIONS ON)
SET(BUILTIN_ITERTOOLS ON)
SET(BUILTIN_STRUCT ON)
SET(BUILTIN_UNICODEDATA ON)
SET(BUILTIN_MULTIPROCESSING ON)
SET(BUILTIN_TESTCAPI ON)
SET(BUILTIN_ELEMENTTREE ON)
SET(BUILTIN_CTYPES ON)
SET(BUILTIN_CMATH ON)
SET(BUILTIN_CODECS_CN ON)
SET(BUILTIN_CODECS_HK ON)
SET(BUILTIN_CODECS_ISO2022 ON)
SET(BUILTIN_CODECS_JP ON)
SET(BUILTIN_CODECS_KR ON)
SET(BUILTIN_CODECS_TW ON)
SET(BUILTIN_JSON ON)
SET(BUILTIN_CSV ON)
SET(BUILTIN_BISECT ON)
SET(BUILTIN_ARRAY ON)
SET(BUILTIN_AUDIOOP ON)
SET(BUILTIN_FUTURE_BUILTINS ON)
SET(BUILTIN_HEAPQ ON)
SET(BUILTIN_HOTSHOT ON)
SET(BUILTIN_DATATIME ON)
SET(BUILTIN_LOCALE ON)
SET(BUILTIN_LSPROF ON)
SET(BUILTIN_DATETIME ON)

# BW client list of builtin extenstions end

add_python_extension(array SOURCES arraymodule.c)
add_python_extension(audioop SOURCES audioop.c)
add_python_extension(_bisect SOURCES _bisectmodule.c)
add_python_extension(cmath REQUIRES HAVE_LIBM SOURCES _math.c cmathmodule.c LIBRARIES ${M_LIBRARIES})
add_python_extension(_codecs_cn SOURCES cjkcodecs/_codecs_cn.c)
add_python_extension(_codecs_hk SOURCES cjkcodecs/_codecs_hk.c)
add_python_extension(_codecs_iso2022 SOURCES cjkcodecs/_codecs_iso2022.c)
add_python_extension(_codecs_jp SOURCES cjkcodecs/_codecs_jp.c)
add_python_extension(_codecs_kr SOURCES cjkcodecs/_codecs_kr.c)
add_python_extension(_codecs_tw SOURCES cjkcodecs/_codecs_tw.c)
add_python_extension(_collections SOURCES _collectionsmodule.c)
add_python_extension(cPickle SOURCES cPickle.c)
add_python_extension(crypt REQUIRES HAVE_LIBCRYPT SOURCES cryptmodule.c LIBRARIES ${HAVE_LIBCRYPT})
add_python_extension(cStringIO SOURCES cStringIO.c)
add_python_extension(_csv SOURCES _csv.c)
add_python_extension(_ctypes_test SOURCES _ctypes/_ctypes_test.c)
add_python_extension(datetime REQUIRES HAVE_LIBM SOURCES datetimemodule.c timemodule.c LIBRARIES ${M_LIBRARIES})
if(ENABLE_DATETIME AND UNIX)
    set_property(SOURCE ${SRC_DIR}/Modules/datetimemodule.c PROPERTY COMPILE_FLAGS -Wno-unused-value)
endif()
add_python_extension(_functools ${MSVC_BUILTIN} SOURCES _functoolsmodule.c)
add_python_extension(future_builtins SOURCES future_builtins.c)
add_python_extension(_heapq SOURCES _heapqmodule.c)
add_python_extension(_hotshot SOURCES _hotshot.c)
add_python_extension(_io ${MSVC_BUILTIN} SOURCES
    _io/_iomodule.c
    _io/bufferedio.c
    _io/bytesio.c
    _io/fileio.c
    _io/iobase.c
    _io/stringio.c
    _io/textio.c
)
add_python_extension(itertools SOURCES itertoolsmodule.c)
add_python_extension(_json SOURCES _json.c)
add_python_extension(_locale ${MSVC_BUILTIN} SOURCES _localemodule.c)
add_python_extension(_lsprof SOURCES _lsprof.c rotatingtree.c)
add_python_extension(math ${MSVC_BUILTIN} SOURCES _math.c mathmodule.c)
add_python_extension(mmap ${MSVC_BUILTIN} SOURCES mmapmodule.c)
add_python_extension(_multibytecodec SOURCES cjkcodecs/multibytecodec.c)
add_python_extension(operator ${MSVC_BUILTIN} SOURCES operator.c)
add_python_extension(parser ${MSVC_BUILTIN} SOURCES parsermodule.c)
add_python_extension(_random SOURCES _randommodule.c)
add_python_extension(strop SOURCES stropmodule.c)
add_python_extension(_struct SOURCES _struct.c)
#add_python_extension(_testcapi SOURCES _testcapimodule.c)
add_python_extension(time REQUIRES HAVE_LIBM SOURCES timemodule.c LIBRARIES ${M_LIBRARIES})
add_python_extension(unicodedata SOURCES unicodedata.c)

# UNIX-only extensions
add_python_extension(fcntl REQUIRES UNIX SOURCES fcntlmodule.c)
add_python_extension(grp REQUIRES UNIX SOURCES grpmodule.c)
add_python_extension(nis REQUIRES UNIX HAVE_LIBNSL SOURCES nismodule.c LIBRARIES ${HAVE_LIBNSL})
add_python_extension(posix REQUIRES UNIX BUILTIN SOURCES posixmodule.c)
add_python_extension(pwd REQUIRES UNIX BUILTIN SOURCES pwdmodule.c)
add_python_extension(resource REQUIRES UNIX SOURCES resource.c)
add_python_extension(spwd REQUIRES UNIX HAVE_GETSPNAM HAVE_GETSPENT SOURCES spwdmodule.c)
add_python_extension(syslog REQUIRES UNIX SOURCES syslogmodule.c)
add_python_extension(termios REQUIRES UNIX SOURCES termios.c)

# MacOSX-only extensions
add_python_extension(_scproxy
    REQUIRES APPLE HAVE_LIBSYSTEMCONFIGURATION
    SOURCES ${SRC_DIR}/Mac/Modules/_scproxy.c
    LIBRARIES ${HAVE_LIBSYSTEMCONFIGURATION}
)

# Linux-only extensions
add_python_extension(linuxaudiodev REQUIRES LINUX SOURCES linuxaudiodev.c)
add_python_extension(ossaudiodev REQUIRES LINUX SOURCES ossaudiodev.c)

# Windows-only extensions
add_python_extension(_msi
    REQUIRES WIN32 BUILTIN
    SOURCES ${SRC_DIR}/PC/_msi.c
    LIBRARIES cabinet.lib msi.lib rpcrt4.lib
)
add_python_extension(msvcrt REQUIRES MSVC BUILTIN SOURCES ${SRC_DIR}/PC/msvcrtmodule.c)
add_python_extension(nt REQUIRES WIN32 BUILTIN SOURCES posixmodule.c)
add_python_extension(_subprocess REQUIRES WIN32 BUILTIN SOURCES ${SRC_DIR}/PC/_subprocess.c)
add_python_extension(_winreg REQUIRES WIN32 BUILTIN SOURCES ${SRC_DIR}/PC/_winreg.c)

# Multiprocessing is different on unix and windows
if(UNIX)
    add_python_extension(_multiprocessing
        SOURCES _multiprocessing/multiprocessing.c
                _multiprocessing/semaphore.c
                _multiprocessing/socket_connection.c
    )
    if(ENABLE_MULTIPROCESSING)
        set_property(SOURCE ${SRC_DIR}/Modules/semaphore.c PROPERTY COMPILE_FLAGS -Wimplicit-function-declaration)
    endif()
elseif(WIN32)
    add_python_extension(_multiprocessing
        SOURCES _multiprocessing/multiprocessing.c
                _multiprocessing/semaphore.c
                _multiprocessing/pipe_connection.c
                _multiprocessing/socket_connection.c
                _multiprocessing/win32_functions.c
        LIBRARIES ws2_32
    )
endif(UNIX)

# Select needs winsock on Windows
if(WIN32)
    add_python_extension(select SOURCES selectmodule.c LIBRARIES ws2_32)
else(WIN32)
    add_python_extension(select SOURCES selectmodule.c)
endif(WIN32)

# Socket is different on Windows
if(WIN32)
    add_python_extension(_socket
        SOURCES socketmodule.c
        DEFINITIONS EAI_ADDRFAMILY
                    HAVE_SOCKADDR_STORAGE
                    HAVE_ADDRINFO
        LIBRARIES ws2_32
    )
else(WIN32)
    add_python_extension(_socket SOURCES socketmodule.c)
endif(WIN32)

# ctypes is different on Windows
if(WIN32)
    if(MINGW)
    add_python_extension(_ctypes
        SOURCES _ctypes/_ctypes.c
                _ctypes/callbacks.c
                _ctypes/callproc.c
                _ctypes/cfield.c
                _ctypes/libffi_mingw/closures.c
                _ctypes/libffi_mingw/debug.c
                _ctypes/libffi_mingw/prep_cif.c
                _ctypes/libffi_mingw/raw_api.c
                _ctypes/libffi_mingw/x86/ffi.c
                _ctypes/libffi_mingw/x86/win32.S
                _ctypes/stgdict.c
        INCLUDEDIRS ${SRC_DIR}/Modules/_ctypes/libffi/src/x86
                    ${SRC_DIR}/Modules/_ctypes/libffi/include
        DEFINITIONS X86_WIN32
      )
    endif(MINGW)
else(WIN32)
    if(APPLE)
        # TODO - Jc:02/20/2013
        #add_python_extension(_ctypes
        #    SOURCES _ctypes/_ctypes.c
        #            _ctypes/callbacks.c
        #            _ctypes/callproc.c
        #            _ctypes/cfield.c
        #            _ctypes/libffi_osx/src/closures.c
        #            _ctypes/libffi_osx/src/dlmalloc.c
        #            _ctypes/libffi_osx/x86/ffi.c
        #            _ctypes/libffi_osx/x86/ffi64.c
        #            _ctypes/libffi_osx/src/prep_cif.c
        #            _ctypes/stgdict.c
        #            _ctypes/libffi_osx/x86/sysv.S
        #            _ctypes/libffi_osx/x86/darwin64.S
        #    INCLUDEDIRS ${SRC_DIR}/Modules/_ctypes/libffi/src/x86
        #                ${SRC_DIR}/Modules/_ctypes/libffi/include
        #)
    else(APPLE)
        add_python_extension(_ctypes
            SOURCES _ctypes/_ctypes.c
                    _ctypes/callbacks.c
                    _ctypes/callproc.c
                    _ctypes/cfield.c
                    _ctypes/libffi/src/closures.c
                    _ctypes/libffi/src/dlmalloc.c
                    _ctypes/libffi/src/x86/ffi.c
                    _ctypes/libffi/src/x86/ffi64.c
                    _ctypes/libffi/src/prep_cif.c
                    _ctypes/stgdict.c
                    _ctypes/libffi/src/x86/sysv.S
                    _ctypes/libffi/src/x86/unix64.S
            INCLUDEDIRS ${SRC_DIR}/Modules/_ctypes/libffi/src/x86
                        ${SRC_DIR}/Modules/_ctypes/libffi/include
        )
    endif(APPLE)
endif(WIN32)


# Build expat using the system expat if it's installed, otherwise use the
# builtin version.
if(EXPAT_LIBRARIES AND EXPAT_INCLUDE_DIRS)
    add_python_extension(pyexpat
        SOURCES pyexpat.c
        LIBRARIES ${EXPAT_LIBRARIES}
        INCLUDEDIRS ${EXPAT_INCLUDE_DIRS}
    )
    add_python_extension(_elementtree
        SOURCES _elementtree.c
        LIBRARIES ${EXPAT_LIBRARIES}
        INCLUDEDIRS ${EXPAT_INCLUDE_DIRS}
    )
else(EXPAT_LIBRARIES AND EXPAT_INCLUDE_DIRS)
    set(_pyexpat_definitions XML_STATIC)
    if(UNIX)
        list(APPEND _pyexpat_definitions HAVE_EXPAT_CONFIG_H)
    elseif(MSVC)
        list(APPEND _pyexpat_definitions COMPILED_FROM_DSP)
    endif(UNIX)
    add_python_extension(pyexpat
        SOURCES pyexpat.c
                expat/xmlparse.c
                expat/xmlrole.c
                expat/xmltok.c
        DEFINITIONS ${_pyexpat_definitions}
        INCLUDEDIRS ${SRC_DIR}/Modules/expat
    )
    set(_elementtree_definitions USE_PYEXPAT_CAPI)
    if(MSVC)
        list(APPEND _elementtree_definitions COMPILED_FROM_DSP)
    endif()
    add_python_extension(_elementtree
        SOURCES _elementtree.c
        DEFINITIONS ${_elementtree_definitions}
        INCLUDEDIRS ${SRC_DIR}/Modules/expat
    )
endif(EXPAT_LIBRARIES AND EXPAT_INCLUDE_DIRS)

# If openssl is NOT available then build some other hash implementations
if(OPENSSL_LIBRARIES)
    set(OPENSSL_NOT_FOUND OFF)
else(OPENSSL_LIBRARIES)
    set(OPENSSL_NOT_FOUND ON)
endif(OPENSSL_LIBRARIES)

add_python_extension(_md5 REQUIRES OPENSSL_NOT_FOUND SOURCES md5.c md5module.c)
add_python_extension(_sha REQUIRES OPENSSL_NOT_FOUND SOURCES shamodule.c)
add_python_extension(_sha256 REQUIRES OPENSSL_NOT_FOUND SOURCES sha256module.c)
add_python_extension(_sha512 REQUIRES OPENSSL_NOT_FOUND SOURCES sha512module.c)

# Extensions that depend on other libraries
set(binascii_DEFINITIONS "")
if(ZLIB_LIBRARIES)
  list(APPEND binascii_DEFINITIONS USE_ZLIB_CRC32)
endif()
add_python_extension(binascii
    SOURCES binascii.c
    DEFINITIONS ${binascii_DEFINITIONS}
#    LIBRARIES ${ZLIB_LIBRARIES}
)
add_python_extension(_bsddb
    REQUIRES DB_INCLUDE_PATH DB_LIBRARIES
    SOURCES _bsddb.c
    LIBRARIES ${DB_LIBRARIES}
    INCLUDEDIRS ${DB_INCLUDE_PATH}
)
add_python_extension(bz2
    REQUIRES BZIP2_LIBRARIES
    SOURCES bz2module.c
    LIBRARIES ${BZIP2_LIBRARIES}
)
add_python_extension(_curses_panel
    REQUIRES CURSES_LIBRARIES
    SOURCES _curses_panel.c
    LIBRARIES ${CURSES_LIBRARIES}
)
add_python_extension(_curses
    REQUIRES CURSES_LIBRARIES
    SOURCES _cursesmodule.c
    LIBRARIES ${CURSES_LIBRARIES}
)
add_python_extension(dbm
    REQUIRES GDBM_INCLUDE_PATH GDBM_LIBRARY GDBM_COMPAT_LIBRARY
    SOURCES dbmmodule.c
    DEFINITIONS HAVE_GDBM_DASH_NDBM_H
    LIBRARIES ${GDBM_LIBRARY} ${GDBM_COMPAT_LIBRARY}
    INCLUDEDIRS ${GDBM_INCLUDE_PATH}
)
add_python_extension(gdbm
    REQUIRES GDBM_INCLUDE_PATH GDBM_LIBRARY GDBM_COMPAT_LIBRARY
    SOURCES gdbmmodule.c
    DEFINITIONS HAVE_GDBM_DASH_NDBM_H
    LIBRARIES ${GDBM_LIBRARY}
    INCLUDEDIRS ${GDBM_INCLUDE_PATH}
)
add_python_extension(_hashlib
    REQUIRES OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES
    SOURCES _hashopenssl.c
    LIBRARIES ${OPENSSL_LIBRARIES}
    INCLUDEDIRS ${OPENSSL_INCLUDE_DIR}
)
if(ENABLE_HASHLIB AND UNIX)
    set_property(SOURCE ${SRC_DIR}/Modules/_hashopenssl.c PROPERTY COMPILE_FLAGS -Wno-deprecated-declarations)
endif()
add_python_extension(readline
    REQUIRES READLINE_INCLUDE_PATH READLINE_LIBRARY CURSES_LIBRARIES
    SOURCES readline.c
    LIBRARIES ${READLINE_LIBRARY} ${CURSES_LIBRARIES}
    INCLUDEDIRS ${READLINE_INCLUDE_PATH}
)
add_python_extension(_sqlite3
    REQUIRES SQLITE3_INCLUDE_PATH SQLITE3_LIBRARY
    SOURCES _sqlite/cache.c
            _sqlite/connection.c
            _sqlite/cursor.c
            _sqlite/microprotocols.c
            _sqlite/module.c
            _sqlite/prepare_protocol.c
            _sqlite/row.c
            _sqlite/statement.c
            _sqlite/util.c
    DEFINITIONS MODULE_NAME="sqlite3"
    INCLUDEDIRS ${SQLITE3_INCLUDE_PATH}
    LIBRARIES ${SQLITE3_LIBRARY}
)
if(ENABLE_SQLITE3 AND UNIX)
    set_property(SOURCE ${SRC_DIR}/Modules/_sqlite/module.c PROPERTY COMPILE_FLAGS -Wno-deprecated-declarations)
endif()
set(_ssl_LIBRARIES ${OPENSSL_LIBRARIES})
if(WIN32)
    list(APPEND _ssl_LIBRARIES ws2_32)
endif()
add_python_extension(_ssl
    REQUIRES OPENSSL_INCLUDE_DIR OPENSSL_LIBRARIES
    SOURCES _ssl.c
    LIBRARIES ${_ssl_LIBRARIES}
    INCLUDEDIRS ${OPENSSL_INCLUDE_DIR}
)
add_python_extension(_tkinter
    REQUIRES TCL_LIBRARY TK_LIBRARY TCL_INCLUDE_PATH TK_INCLUDE_PATH
    SOURCES _tkinter.c tkappinit.c
    DEFINITIONS WITH_APPINIT=1
    LIBRARIES ${TCL_LIBRARY} ${TK_LIBRARY}
    INCLUDEDIRS ${TCL_INCLUDE_PATH} ${TK_INCLUDE_PATH}
)
add_python_extension(zlib
    REQUIRES ZLIB_LIBRARIES
    SOURCES zlibmodule.c
#    LIBRARIES ${ZLIB_LIBRARIES}
)

if(USE_LIBEDIT)
    set_target_properties(extension_readline PROPERTIES
        COMPILE_DEFINITIONS "USE_LIBEDIT")
endif()

# Uncomment the following to see the list of extensions builtin.
#show_extension_summary()
