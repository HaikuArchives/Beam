# ![icon](icon.png) Beam
_BEware, Another Mailer_

### Main features

- Multiple mail accounts (POP & SMTP), with support for default-accounts and automatic selection of appropriate account when replying, etc.
- Fully MIME compliant (Beam passes the MIME-Torture-Test)
- Performance adequate for large mail folders (>10,000 messages)
- POP-authentication (POP3, APOP, CRAM-MD5, DIGEST-MD5)
- SMTP-authentication (SmtpAfterPop, PLAIN, LOGIN, CRAM-MD5, DIGEST-MD5)
- Preliminary IMAP-support (fetches mails just as POP3 does)
- Full header-control, mail-headers can be edited directly before sending
- Identities, separating the user info (name, mail address, signature) from the network info (server address, login & password)
- Multiple signatures that can be dynamic by using scripts
- Filter capabilities (using SIEVE and a SPAM-filter)
- Customized filter chains to help with sorting emails
- Customizable shortcuts
- Customizable icon set

![screenshot](screenshot.png)

Beam was originally developed by Oliver Tappe and is distributed under the GPL (see GnuGeneralPublicLicense.txt).   
If you have questions, suggestions, complaints or a nice set of icons, file a ticket at the [issue tracker](https://github.com/HaikuArchives/Beam/issues) or submit a pull-request.

### Frequently asked questions

1. **Can't send mail because of a missing 'sending account'?**   
Make sure you have selected an _SMPT account_ in your Identity preferences. 

2. **What are identities?**   
An identity represents the information about a personality that you'd like to use. Identities separate the personal information (name, mail-address, signature) from the network information (server, username, password).   
You can for instance set up one identity representing your private personality and another one representing your work personality while both are actually using the same mail-account.   
Beam will use the appropriate identity (and thus it's mail-address and signature) automatically.

3. **Why identities?**   
Only with identities it is possible to create different user personalities for one mail-account. This is of particular interest if you own a domain and have a catch-all rule in place. With identities you can consistently use any (faked) mail-address without every exposing your other addresses to the respective communication partners.

4. **How do I set up identities?**   
Whenever you create a receiving mail-account, one identity will automatically be created with it (the identity shares its name with the account). After you have set up the account, please select the new identity and (optionally) enter your name, mail-address and select a signature, that's all.   
You can always create additional identities, you just have to select the mail-accounts that shall be used by the new identity (Beam will tell you to do so, if you haven't).

5. **What's the difference between forwarding and redirecting a mail?**   
Forwarding a mail means to hand the content of a mail that you have received to another person. This is useful if you have just received the newest list of hot P2P servers and want to propagate ("forward") it to your friends.   
Redirecting a mail means to bend the communication channel away from you to another person. Let's say you have received a support query, but you are the wrong person to ask. You know the person who can answer the query, so you redirect the mail to him/her. This way, the receiver of the redirect will get all the info he/she needs to answer the original author of the mail (a reply will go to the original author, not to you!).

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