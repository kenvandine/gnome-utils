

all : test1 edit-menus

CC=gcc

# won't compile with -ansi, many errors, need to fix that.
# define DEMO_MODE to use the sample icewm file in this directory
CFLAGS=-g -Wall `gtk-config --cflags` -I/usr/local/include/libgnomesupport #-DDEMO_MODE 

LDFLAGS=-Wall `gtk-config --libs` -lgnome -lgnomeui -lgnomesupport

OBJECTS=iconbrowse.o is_image.o menuentrywiz.o menu.o icewm-parser.o icewm-saver.o run_command.o

.c.o:
	$(CC) $(CFLAGS) -c $<

debian-parser.o: debian-parser.c parser.h menu.h
edit-menus.o: edit-menus.c menuentrywiz.h menu.h parser.h saver.h \
 run_command.h
icewm-parser.o: icewm-parser.c parser.h menu.h icewm.h
icewm-saver.o: icewm-saver.c saver.h menu.h icewm.h
iconbrowse.o: iconbrowse.c iconbrowse.h is_image.h
is_image.o: is_image.c is_image.h
menu.o: menu.c menu.h
menuentrywiz.o: menuentrywiz.c menuentrywiz.h iconbrowse.h is_image.h \
 run_command.h
run_command.o: run_command.c run_command.h
test1.o: test1.c iconbrowse.h menuentrywiz.h

test1 : $(OBJECTS) test1.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o test1 $(OBJECTS) test1.o

edit-menus : $(OBJECTS) edit-menus.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o edit-menus $(OBJECTS) edit-menus.o

clean :
	/bin/rm -f *.o *~ test1 edit-menus

deps:
	$(CC) $(CFLAGS) -MM *.c


