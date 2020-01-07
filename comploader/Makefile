
instdir = $(DESTDIR)/usr/lib/sysload/cl/
CFLAGS  = -I../../libssh-0.11/include
TFLAGS  = -L../../libssh-0.11/libssh -lssh

.PHONY:	all clean install uninstall

all:	cl_scp

cl_scp:	cl_scp.o
	$(CC) $(CFLAGS) $(TFLAGS) -o $@ $^

clean:
	rm -rf cl_scp cl_scp.o

install: all
	mkdir -p $(instdir)
	install -m 0755	cl_block	$(instdir)
	install -m 0755	cl_dasd		$(instdir)
	install -m 0755	cl_file		$(instdir)
	install -m 0755	cl_ftp		$(instdir)
	install -m 0755	cl_http		$(instdir)
	install -m 0755	cl_scp		$(instdir)
	install -m 0755	cl_zfcp		$(instdir)

uninstall:
	rm -rf $(instdir)
