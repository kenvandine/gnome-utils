INCLUDES = \
	$(GNOME_UTILS_CFLAGS) \
	-I../

noinst_PROGRAMS = test-reader

test_reader_SOURCES = test-reader.c ../logview-log.c ../logview-utils.c
test_reader_LDADD = $(GNOME_UTILS_LIBS) $(Z_LIBS) -lm
