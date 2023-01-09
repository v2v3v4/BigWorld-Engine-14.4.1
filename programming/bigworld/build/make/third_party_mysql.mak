# This Makefile describes how to use the system MySQL libraries

# useMySQL

# This should potentially be added into a pre-requisite rule check of the
# libraries that need it... eg:
# meta_libdbmgr_mysql: $(MYSQL_CONFIG_PATH)

MYSQL_CONFIG_PATH = /usr/lib64/mysql/mysql_config

# TODO: SYSTEM_HAS_MYSQL 0 = true, 1 = false. should invert this
SYSTEM_HAS_MYSQL := $(shell $(BW_BLDDIR)/test_mysql.sh && echo $$?)

SYSTEM_MYSQL_INCLUDES := `$(MYSQL_CONFIG_PATH) --include`
SYSTEM_MYSQL_CFLAGS   := `$(MYSQL_CONFIG_PATH) --cflags`
SYSTEM_MYSQL_LDLIBS   := `$(MYSQL_CONFIG_PATH) --libs_r`

ifeq ($(BW_CONFIG),$(BW_CONFIG_DEBUG))
#`mysql-config --cflags` unconditionally adds -Wp,-D_FORTIFY_SOURCE=2
#which breaks the build in the debug mode
SYSTEM_MYSQL_CFLAGS += -Wp,-U_FORTIFY_SOURCE
endif

# third_party_mysql.mak
