Summary: Platform independent second stage bootloader
Name: sysload
Version: 1.0.0
Release: 0
License: GPL
Group: Applications/System
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
System Loader (sysload) is a platform independent second stage boot loader 
that offers an interactive boot menu to select between several boot kernels. 
It supports booting from generic block devices on all platforms and several 
System z specific boot configurations in addition. For multiple configuration 
examples please refer to the documentation.

%prep
%setup

%build
make

%install
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%attr(0755, root, root) %dir 	/usr/lib/sysload
%attr(0755, root, root) %dir 	/usr/lib/sysload/cl
%attr(0755, root, root)      	/usr/lib/sysload/cl/cl_file
%attr(0755, root, root)      	/usr/lib/sysload/cl/cl_dasd
%attr(0755, root, root)      	/usr/lib/sysload/cl/cl_zfcp
%attr(0755, root, root)      	/usr/lib/sysload/cl/cl_block
%attr(0755, root, root)      	/usr/lib/sysload/cl/cl_ftp
%attr(0755, root, root)      	/usr/lib/sysload/cl/cl_http
%attr(0755, root, root)      	/usr/lib/sysload/cl/cl_scp
%attr(0644, root, root)      	/usr/lib/sysload/config/hosts
%attr(0644, root, root)      	/usr/lib/sysload/config/passwd
%attr(0755, root, root) %dir 	/usr/lib/sysload/ui
%attr(0755, root, root)      	/usr/lib/sysload/ui/ui_linemode
%attr(0755, root, root)      	/usr/lib/sysload/ui/ui_ssh
%attr(0755, root, root) %dir 	/usr/lib/sysload/setup
%attr(0755, root, root)      	/usr/lib/sysload/setup/setup_zfcp
%attr(0755, root, root)      	/usr/lib/sysload/setup/setup_qeth
%attr(0755, root, root)      	/usr/lib/sysload/setup/setup_dasd
%attr(0755, root, root) %dir 	/usr/lib/sysload/scripts
%attr(0755, root, root)      	/usr/lib/sysload/scripts/init.sysload
%attr(0755, root, root)      	/usr/lib/sysload/scripts/sysload-ssh
%attr(0755, root, root)      	/usr/sbin/sysload_admin
%attr(0644, root, root)      	/usr/share/man/man8/sysload.8.gz
%attr(0644, root, root)      	/usr/share/man/man8/sysload_admin.8.gz
%attr(0644, root, root)      	/usr/share/man/man5/sysload.conf.5.gz
%attr(0644, root, root)      	/usr/share/man/man5/sysload_admin.conf.5.gz
%attr(0644, root, root) %config /etc/sysload_admin.conf
%attr(0755, root, root)      	/usr/lib/sysload/sbin/kexec-wrapper
%attr(0755, root, root)      	/usr/lib/sysload/sbin/halt
%attr(0755, root, root)      	/usr/lib/sysload/sysload
%attr(0755, root, root) %dir %doc /usr/share/doc/sysload
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/bootsequence.fig
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/controlflow.fig
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/design.tex
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/design.pdf
%attr(0755, root, root) %dir %doc /usr/share/doc/sysload/examples/
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/sysload.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/sysload_i386.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/sysload_s390.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/Makefile
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/menu.lst
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/menu_sles.lst
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/parmfile_linux41.sysload
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/standard_entries.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/zipl.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/zipl1.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/zipl2.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/sysload_admin_s390.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/sysload_admin_i386_sles.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/examples/sysload_admin_i386_rhel.conf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/sysload_conf_syntax.txt
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/howto.tex
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/howto.pdf
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/ssh_concept.fig
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/README
%attr(0644, root, root) %doc 	/usr/share/doc/sysload/LICENSE

%changelog
* Wed May 14 2008 Christof Schmitt <christof.schmitt@de.ibm.com>
- sysload 1.0.0, spec file release 0

* Fri Nov 26 2007 Stephan Mann <stemann@de.ibm.com>
- sysload 0.5, spec file release 0

* Fri Nov 16 2007 Stephan Mann <stemann@de.ibm.com>
- sysload 0.4, spec file release 0

* Fri Nov 06 2007 Stephan Mann <stemann@de.ibm.com>
- sysload 0.3, spec file release 0

* Fri Oct 30 2007 Stephan Mann <stemann@de.ibm.com>
- sysload 0.2, spec file release 0

* Fri Oct 26 2007 Stephan Mann <stemann@de.ibm.com>
- sysload 0.1, spec file release 0
