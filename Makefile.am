AM_CPPFLAGS = -iquote $(top_builddir) -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

noinst_LTLIBRARIES =
noinst_LTLIBRARIES += libboolean_expression.la

SOURCES = \
	BooleanExpression.cxx \
	BooleanExpression.h

libboolean_expression_la_SOURCES = ${SOURCES}
libboolean_expression_la_CXXFLAGS = -std=c++11 @LIBCWD_FLAGS@
libboolean_expression_la_LIBADD = @LIBCWD_LIBS@

# --------------- Maintainer's Section

if MAINTAINER_MODE
distclean-local:
	-rm -f *.s *.ii

clean-local:
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
