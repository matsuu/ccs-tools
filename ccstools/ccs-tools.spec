Summary: TOMOYO Linux tools

Name: ccs-tools
Version: 1.6.4
Release: 1
License: GPL
Group: System Environment/Kernel
ExclusiveOS: Linux
Autoreqprov: no
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Conflicts: ccs-tools < 1.6.4-1

Source0: http://osdn.dl.sourceforge.jp/tomoyo/30298/ccs-tools-1.6.4-20080903.tar.gz

%description
This is TOMOYO Linux tools.

%prep

%setup -q -n ccstools

%build

make -s all

%install

make -s install INSTALLDIR=%{buildroot}

%clean

rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/sbin/ccs-init
/sbin/tomoyo-init
/usr/lib/ccs/
/usr/sbin/
/usr/share/man/
%attr(4755,root,root) /usr/lib/ccs/misc/force-logout
%config(noreplace) /usr/lib/ccs/ccstools.conf

%changelog
* Wed Sep 03 2008 1.6.4-1
- Minor update release.

* Tue Jul 15 2008 1.6.3-1
- Bug fix release.
- Dropped suid-root from /usr/lib/ccs/misc/proxy because /usr/lib/ccs/ is 0755.

* Wed Jun 25 2008 1.6.2-1
- Minor update release.
- Change permission of /usr/lib/ccs/ to 0755

* Sat May 10 2008 1.6.1-1
- Minor update release.

* Tue Apr 01 2008 1.6.0-1
- Feature enhancement release.

* Thu Jan 31 2008 1.5.3-1
- Minor update release.

* Wed Dec 05 2007 1.5.2-1
- Minor update release.
- Added manpage.

* Thu Oct 19 2007 1.5.1-1
- Minor update release.

* Thu Sep 20 2007 1.5.0-1
- First-release.
