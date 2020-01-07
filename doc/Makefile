
instdir = $(DESTDIR)/usr/share/doc/sysload

.PHONY:	all clean install uninstall

all: howto.pdf design.pdf

howto.pdf: howto.tex
	pdflatex howto.tex
	pdflatex howto.tex
	pdflatex howto.tex

design.pdf: bootsequence.pdf controlflow.pdf ssh_concept.pdf design.tex
	pdflatex design.tex
	pdflatex design.tex
	pdflatex design.tex

bootsequence.pdf: bootsequence.fig
	fig2dev -L pdf bootsequence.fig bootsequence.pdf

controlflow.pdf: controlflow.fig
	fig2dev -L pdf controlflow.fig controlflow.pdf

ssh_concept.pdf: ssh_concept.fig
	fig2dev -L pdf ssh_concept.fig ssh_concept.pdf

clean:
	rm -f *.aux *.log *.out *.toc *.pdf *.eps

install:
	mkdir -p $(instdir)/examples
	install -m 0644	bootsequence.fig	$(instdir)/
	install -m 0644	controlflow.fig		$(instdir)/
	install -m 0644	design.tex		$(instdir)/
	install -m 0644	design.pdf		$(instdir)/
	install -m 0644	sysload_conf_syntax.txt	$(instdir)/
	install -m 0644	Makefile		$(instdir)/
	install -m 0644	howto.tex		$(instdir)/
	install -m 0644	howto.pdf		$(instdir)/
	install -m 0644	ssh_concept.fig		$(instdir)/
	install -m 0644	../README 		$(instdir)/
	install -m 0644 ../LICENSE		$(instdir)/
	install -m 0644	sysload.conf		$(instdir)/examples/
	install -m 0644	sysload_i386.conf	$(instdir)/examples/
	install -m 0644	sysload_s390.conf	$(instdir)/examples/
	install -m 0644	menu.lst		$(instdir)/examples/
	install -m 0644	menu_sles.lst		$(instdir)/examples/
	install -m 0644	parmfile_linux41.sysload	$(instdir)/examples/
	install -m 0644	standard_entries.conf	$(instdir)/examples/
	install -m 0644	zipl.conf		$(instdir)/examples/
	install -m 0644	zipl1.conf		$(instdir)/examples/
	install -m 0644	zipl2.conf		$(instdir)/examples/

uninstall:
	rm -rf $(instdir)
