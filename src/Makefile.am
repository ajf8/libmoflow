AM_CFLAGS = --pedantic -Wall -std=c99 -O2 $(jsonglib_CFLAGS) $(libcurl_CFLAGS)
AM_LDFLAGS = -lcurl $(jsonglib_LIBS) $(libcurl_LIBS) -pthread

libmoflow_la_SOURCES = moflow.c http.c json.c
lib_LTLIBRARIES = libmoflow.la
include_HEADERS = moflow.h
