# Hacker News Symbiotic Application for Mac OS and AIX

[Another Old VCR Atom Bomb of Genius!](https://oldvcr.blogspot.com/2023/11/the-apple-network-servers-all-too.html)

Copyright 1992-97 Apple Inc. (as Apple Computer, Inc.)  
Copyright 2023 Cameron Kaiser.  
All rights reserved.  
See the License section for an important notice.

## What it is

This project is a demonstration of offloading computation to an Apple server from a Mac client using the Program-to-Program Communication Toolbox introduced with Macintosh System 7. It specifically uses the support for this facility in AIX for the Apple Network Server systems.

When set up, the client is pushed the top 10 stories in Hacker News every minute via its Firebase API, a task that involves modern encryption and proceesing JSON, and thus would take most old classic Macs a long time and tie up the CPU doing so. Here, the process is asynchronous and allows the Mac to be used normally.

Because it does the actual work in a Perl script, you can replace the Perl script with code of your own to do other tasks, as long the result is text separated by newlines, and the total size of your text is less than 1024 bytes. The C portion will call your script through a pipe and take its standard output to be sent onward.

For comparison purposes, the source code for the original Trident daemon on which the Hacker News daemon is based is included, with minor tweaks for building with `gcc`.

[Read more about how this works.](https://oldvcr.blogspot.com/2023/11/the-apple-network-servers-all-too.html)

## This code has very specific technical requirements!

This source code has very specific requirements: an Apple Network Server 500 or 700 running ANS AIX 4.1.5 (4.1.4.1 should also work) with Perl 5.004 or later, [Crypto Ancienne](https://github.com/classilla/cryanc) `carl` 2.2 or later, and either IBM `xlc` or `gcc` 2.7.2.2 or later for the server portion, and any Mac running version 7.0 through 8.6 with CodeWarrior Pro 2 or better for the client portion. The server component is not compatible with Linux or NetBSD, even on actual Network Server hardware, and the client is not compatible with Mac OS 9 or Classic.

## How to build

For the original Trident, the client can built by unstuffing `NewTridentDemo.sit` and opening the project file in CodeWarrior Pro 2 or later. The server can be built by entering the `nutridentd` directory and either `make -f Makefile.xlc` if you have `xlc` (which will make both the flexible and simple versions of the daemon), or `make -f Makefile.simple` if you have `gcc` (which only supports the simple version). `Makefile.flexible` currently does not generate a functional executable. Entries like this should be in `/etc/ppcd.conf`, adjusted to where you have the binaries stored (note that the flexible daemon is irrelevant if you only have `gcc`):

```
Status Demo:100:100:/usr/sbin/simpled:JVLN:guest:*
:::/usr/sbin/flexibled::guest:
```

Restart `ppcd` after changing this file. These or similar entries may already exist in `/etc/ppcd.conf`, but multiple daemons are allowed to respond to the same signature. See the `man` page for `ppcd.conf` or directions in that file to explain the various fields.

For the Hacker News daemon, the client can be built by unstuffing `HackerANewSDemo.sit` and opening the project file in CodeWarrior Pro 2 or later. The server can be built by entering the `hnd` directory and typing `make`. The current version of the `Makefile` assumes you are building with `gcc`. Make sure to modify the line

```
f = popen("/usr/bin/perl /home/spectre/src/hnd/hn.pl", "r");
```

to point to the desired Perl script. `hn.pl` uses the Crypto Ancienne `carl` utility to download JSON from the API and parse it, but `simple_hn.pl` can be used in its stead for test purposes. An entry like this should be in `/etc/ppcd.conf`, adjusted to where you have the binary stored:

```
Hacker News:100:100:/usr/src/hnd/hnd:HCKN:guest:NONE
```

Restart `ppcd` after changing this file. See the `man` page for `ppcd.conf` or directions in that file to explain the various fields.

## Don't post bugs or pull requests

This is a toy which will not run on most people's computers. It will probably never run on yours. Bug reports without a pull request will be ignored or deleted, and pull requests without a good reason will get the same. Seriously, this is a toy. Have fun with it, fork it, make it your own.

## License

`hn.pl` is Copyright 2023 Cameron Kaiser and is released under the BSD 2-clause license.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

All other source code is Copyright 1992-1997 Apple Computer, Inc., now Apple Inc. THIS IS PUBLISHED SAMPLE SOURCE CODE OF APPLE COMPUTER, INC. It is not clear what, if any, license this code was issued under, only that it is provided as sample code for developer usage.
