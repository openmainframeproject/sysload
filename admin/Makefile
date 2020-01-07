
.PHONY:	all man clean install uninstall

all:	man

man:    sysload_admin.8 sysload_admin.conf.5
	gzip -c sysload_admin.8 > sysload_admin.8.gz
	gzip -c sysload_admin.conf.5 > sysload_admin.conf.5.gz

clean:
	rm -rf *.gz

install: all
	mkdir -p $(DESTDIR)/usr/sbin
	mkdir -p $(DESTDIR)/etc
	mkdir -p $(DESTDIR)/usr/share/doc/sysload/examples
	mkdir -p $(DESTDIR)/usr/share/man/man8
	mkdir -p $(DESTDIR)/usr/share/man/man5
	install -m 0755	sysload_admin		$(DESTDIR)/usr/sbin/
	install -m 0644	sysload_admin.conf	$(DESTDIR)/etc/
	install -m 0644	sysload_admin_s390.conf	$(DESTDIR)/usr/share/doc/sysload/examples/
	install -m 0644	sysload_admin_i386_sles.conf	$(DESTDIR)/usr/share/doc/sysload/examples/
	install -m 0644	sysload_admin_i386_rhel.conf	$(DESTDIR)/usr/share/doc/sysload/examples/
	install -m 0644	sysload_admin.8.gz	$(DESTDIR)/usr/share/man/man8/
	install -m 0644	sysload_admin.conf.5.gz	$(DESTDIR)/usr/share/man/man5/

uninstall:
	rm -f $(DESTDIR)/usr/sbin/sysload_admin
	rm -f $(DESTDIR)/usr/share/doc/sysload/examples/sysload_admin_*
	rm -f $(DESTDIR)/usr/share/man/man8/sysload_admin.8.gz
	rm -f $(DESTDIR)/usr/share/man/man5/sysload_admin.conf.5.gz
