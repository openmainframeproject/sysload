
instdir = $(DESTDIR)/usr/lib/sysload

.PHONY:	all install uninstall

all:	
	# Nothing to be done

install: 
	mkdir -p $(instdir)/sbin
	install -m 0755	kexec-wrapper	$(instdir)/sbin
	mkdir -p $(instdir)/scripts
	install -m 0755	init.sysload	$(instdir)/scripts
	install -m 0755	sysload-ssh	$(instdir)/scripts

uninstall:
	rm -f	$(instdir)/sbin/kexec-wrapper
	rm -rf	$(instdir)/scripts
