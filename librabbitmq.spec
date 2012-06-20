Name            : librabbitmq
Version         : 0.2.0
Release         : 1.20120620git83c5c6a2
Summary         : RabbitMQ C AMQP client library
Group           : Development/Libraries

Source0         : %{name}-%{version}.tar.gz
URL             : https://github.com/alanxz/rabbitmq-c
License         : MIT
Packager        : Matt Dainty <matt@bodgit-n-scarper.com>

BuildRoot       : %{_tmppath}/%{name}-%{version}-root
%if %{?el5:1}0
BuildRequires   : popt
BuildRequires   : python-simplejson
%else
BuildRequires   : popt-devel
%endif
BuildRequires   : xmlto

%description
RabbitMQ C AMQP client library

%package devel
Summary: Development files for the librabbitmq package
Group: Development/Libraries
Requires: %{name} = %{version}

%description devel
RabbitMQ C AMQP client library. This package contains files needed to
develop applications using librabbitmq.

%package tools
Summary: Example tools built using the librabbitmq package
Group: Development/Libraries
Requires: %{name} = %{version}

%description tools
RabbitMQ C AMQP client library. This package contains example tools
built using librabbitmq.

%prep
%setup -q

%build
%configure --enable-static
%{__make} %{?_smp_mflags}

%install
%{__rm} -rf %{buildroot}
%makeinstall

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-,root,root)
%doc AUTHORS LICENSE-MIT THANKS TODO
%{_libdir}/librabbitmq.so.*

%files devel
%defattr(-,root,root)
%{_libdir}/librabbitmq.a
%{_libdir}/librabbitmq.la
%{_libdir}/librabbitmq.so
%{_libdir}/pkgconfig/librabbitmq.pc
%{_includedir}/amqp.h
%{_includedir}/amqp_framing.h

%files tools
%defattr(-,root,root)
%{_bindir}/amqp-*
%doc %{_mandir}/man1/amqp-*.1*
%doc %{_mandir}/man7/librabbitmq-tools.7.gz

%changelog
* Wed Jun 20 2012 Matt Dainty <matt@bodgit-n-scarper.com> 0.2.0-1.20120620git83c5c6a2
- Bump to 0.2.0-1.20120620git83c5c6a2.

* Sun Feb 19 2012 Matt Dainty <matt@bodgit-n-scarper.com> 0.0.1-2.20120121hg281
- Fix build dependency for el5.

* Sat Jan 21 2012 Matt Dainty <matt@bodgit-n-scarper.com> 0.0.1-1.20120121hg281
- Initial version 0.0.1-1.20120121hg281.
