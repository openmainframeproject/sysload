
subdirs = admin comploader config doc scripts setup core 
     
.PHONY: all $(subdirs) clean install uninstall
	       
all: $(subdirs)

$(subdirs):
	$(MAKE) -C $@

clean:	
	for dir in $(subdirs); do \
		${MAKE} -C $$dir clean; \
	done

install:
	for dir in $(subdirs); do \
		${MAKE} -C $$dir DESTDIR=$(DESTDIR) install; \
	done

uninstall:
	for dir in $(subdirs); do \
		${MAKE} -C $$dir DESTDIR=$(DESTDIR) uninstall; \
	done
	rm -rf $(DESTDIR)/usr/lib/sysload
