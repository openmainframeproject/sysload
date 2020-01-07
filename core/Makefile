YACC=bison
CC=gcc
LEX=flex

progs = sysload halt ui_linemode ui_ssh man
instdir = $(DESTDIR)/usr/lib/sysload

.PHONY: all clean install uninstall

all: $(progs)

config_parser.c: sysload.conf.y
	$(YACC) -d -o $@ $<

config_scanner.c: sysload.conf.l config_parser.h
	$(LEX) $(LFLAGS) -o$@ $<

sysload: sysload.o debug.o config.o parser.o comp_load.o parser_sysload.o \
	ui_control.o loader.o netbase.o modbase.o config_parser.o \
	config_scanner.o bootmap_dasd.o bootmap_fcp.o bootmap_common.o \
	insfile.o dhcp_request.o

halt:	halt.o

ui_linemode: ui_linemode.o config.o debug.o

ui_ssh: ui_ssh.o config.o comp_load.o debug.o

man: 	sysload.8 sysload.conf.5
	gzip -c sysload.8 > sysload.8.gz
	gzip -c sysload.conf.5 > sysload.conf.5.gz

clean:
	rm -f *.o config_* *.gz $(progs)

install: all
	mkdir -p $(instdir)/sbin
	install -m 0755	halt		$(instdir)/sbin/
	install -m 0755 sysload		$(instdir)/
	mkdir -p $(instdir)/ui
	install -m 0755	ui_linemode	$(instdir)/ui/
	install -m 0755	ui_ssh		$(instdir)/ui/
	mkdir -p $(DESTDIR)/usr/share/man/man8
	install -m 0644 sysload.8.gz	$(DESTDIR)/usr/share/man/man8
	mkdir -p $(DESTDIR)/usr/share/man/man5
	install -m 0644 sysload.conf.5.gz $(DESTDIR)/usr/share/man/man5

uninstall:
	rm -f 	$(instdir)/sbin/halt
	rm -f 	$(instdir)/sysload
	rm -rf	$(instdir)/ui/
	rm -f	$(DESTDIR)/usr/share/man/man8/sysload.8.gz
	rm -f	$(DESTDIR)/usr/share/man/man5/sysload.conf.5.gz
