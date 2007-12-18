noinst_PROGRAMS = exceptions interrupt cond_select thread

exceptions_SOURCES = exceptions.cpp
interrupt_SOURCES = interrupt.cpp
cond_select_SOURCES = cond_select.cpp
thread_SOURCES = thread.cpp

LDADD = ../../core/libarchon_core.la ../libarchon_thread.la

AM_CPPFLAGS = -I$(top_srcdir)/src
