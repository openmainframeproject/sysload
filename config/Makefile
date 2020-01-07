
instdir = $(DESTDIR)/usr/lib/sysload/config

.PHONY:	all install uninstall

all:	
	# Nothing to be done

install: 
	mkdir -p $(instdir)
	install -m 0644	hosts	$(instdir)
	install -m 0644	passwd	$(instdir)

uninstall:
	rm -rf $(instdir)
