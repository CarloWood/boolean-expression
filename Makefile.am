AM_CPPFLAGS = -iquote $(top_builddir) -iquote $(top_srcdir) -iquote $(top_srcdir)/cwds

noinst_LTLIBRARIES =
noinst_LTLIBRARIES += libboolean_expression.la
bin_PROGRAMS = boolean_permutations_test boolean_simplification_test

SOURCES = \
	BooleanExpression.cxx \
	BooleanExpression.h

boolean_permutations_test_SOURCES = boolean_permutations_test.cxx

boolean_simplification_test_SOURCES = boolean_simplification_test.cxx

libboolean_expression_la_SOURCES = ${SOURCES}
libboolean_expression_la_CXXFLAGS = -std=c++11 @LIBCWD_FLAGS@
libboolean_expression_la_LIBADD = @LIBCWD_LIBS@

boolean_permutations_test_CXXFLAGS = -fmax-errors=1 -ferror-limit=1 -Wno-unused-command-line-argument @LIBCWD_FLAGS@
boolean_permutations_test_LDADD = $(top_builddir)/cwds/libcwds.la

boolean_simplification_test_CXXFLAGS = -fmax-errors=1 -ferror-limit=1 -Wno-unused-command-line-argument @LIBCWD_FLAGS@
boolean_simplification_test_LDADD = $(top_builddir)/cwds/libcwds.la

# --------------- Maintainer's Section

if MAINTAINER_MODE
distclean-local:
	-rm -f *.s *.ii

clean-local:
endif

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
