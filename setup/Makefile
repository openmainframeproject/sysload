
instdir = $(DESTDIR)/usr/lib/sysload/setup

.PHONY:	all install uninstall

all:	
	# Nothing to be done

install: 
	mkdir -p $(instdir)
	install -m 0755	setup_dasd 	$(instdir)
	install -m 0755	setup_qeth	$(instdir)
	install -m 0755	setup_zfcp	$(instdir)

uninstall:
	rm -rf $(instdir)
