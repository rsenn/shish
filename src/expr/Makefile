# ===========================================================================
#
# Makefile for src/debug subdirectory
#
# relative directories
# ---------------------------------------------------------------------------
srcdir       = .
top_srcdir   = ../..
top_builddir = ../..
thisdir      = src/expr/



# include global make configuration
# ---------------------------------------------------------------------------
include $(top_builddir)/config.mk

# configuration for this directory
# ---------------------------------------------------------------------------
TL_ARCHIVE = src/libshell.a
ARCHIVE = expr.a
MODULES = expr_tokens

SOURCES = $(MODULES:%=%.c)
OBJECTS = $(MODULES:%=%.o)
DEPS    = $(MODULES:%=%.d)

# targets for this directory
# ---------------------------------------------------------------------------
all: Makefile $(ARCHIVE)
install:

# dependencies
# ---------------------------------------------------------------------------
-include $(DEPS)

# local build configuration
# ---------------------------------------------------------------------------
INCLUDES += -I$(top_srcdir)/lib -I$(top_builddir)
INCLUDES += -iquote $(top_srcdir)/src -iquote $(top_builddir)/src
INCLUDES += -I$(top_builddir)

# include build.mk (makefile helper)
# ---------------------------------------------------------------------------
include $(top_builddir)/build.mk

