# ![icon](icon.png) Beam
_BEware, Another Mailer_

### Main features

- POP3 and IMAP support for multiple accounts,
- multiple signatures that can be dynamic by using scripts,
- customized filter chains to help with sorting emails,
- a spam filter to separate spam from tufu,
- customizable shortcuts,
- customizable icon sets

![screenshot](screenshot.png)

Beam was originally developed by Oliver Tappe and is distributed under the GPL (see GnuGeneralPublicLicense.txt).   
If you have questions, suggestions, complaints or a nice set of icons, file a ticket at the [issue tracker](https://github.com/HaikuArchives/Beam/issues) or submit a pull-request.

### Software required by / used within Beam:

- The Layout Library (liblayout.so), by Marco Nelissen. Beam requires this to run!
- PCRE, Perl-Compatible-Regular-Expression library, by Philip Hazel. Beam requires this to run!
- Parts from SantasGiftBag, by Brian Tietz, hacked up a bit for use in Beam.
- Regexx, a C++-Regular-Expression library, by Gustavo Niemeyer, ported to BString and hacked up a bit for use in Beam.
- Libiconv, a library that does conversion between different characterset-encodings, by Bruno Haible.
- OSBF from crm114, a bayes-like filter that's good for SPAM-filtering, by Fidelis Assis and Bill Yerazunis.
- openssl (if available), by the OpenSSL-team, is used to provide transport layer security for all network traffic.

### Build instructions

There's already a [HaikuPorts](https://github.com/haikuports/haikuports/wiki) recipe for Beam and the built package is available with the HaikuDepot application. To build Beam manually, make sure you have all needed dependencies. You install them with pkgman:

```
pkgman install devel:libiconv devel:liblayout devel:libpcre devel:libssl
```

After that it's a simple ```jam -qj1``` from the top directory of the cloned github repo (```j1``` will force building with just 1 thread, because there may be issues with more...).
The resulting binary and add-ons etc. are found in the subfolder ```generated/distro-haiku```.