![BirdChat](birdchat.png)

This is a fork from the now archived hexchat [https://github.com/hexchat/hexchat](https://github.com/hexchat/hexchat). It is called birdchat and is an IRC client for people who are birds in real life. Also people who are not birds in real life can use it too for free.

This `birdchat` still has a lot of problems mostly because it was vibe coded and I gotta learn C to fix them up. Use at your own risk.

Thanks to `hgc55` for fixing a lot of the terrible gtk3 migration code and giving me some hints to make it easier to work out wtf is wrong with this thing.

Come chat with us on IRC at libera `#birdchat`.

### New Stuff

- It contains [99 Color Support](https://github.com/hexchat/hexchat/pull/2456) thanks to `chzchzchz` which the original hexchat author did not want to merge in for some reason. Now we can enjoy proper IRC ASCII art in full.
- Migrated from `gtk2` to `gtk3` while it works mostly there are still a few things to unvibe code and manually fix up. Man, Claude was puttin' in that work, movin' GTK2 to GTK3, for four hours straight.
- A `birdbuild` directory which contains some bash scripts that use a fedora docker container to cross compile and build:
  - Linux build
  - Windows build
  - There are FreeBSD and DragonFlyBSD builds but they are just not working at all because of trying to cross compile them. Sorta don't want to try to fix these again as it was rather painful. Maybe I'll remove them.

### Problems

- Some of the menu modals will crash if you cancel or something.
- Scroll is pretty broken. I guess at least it will keep up with the chats. If you scroll up it will go down though on a new chat.
- No tray icon stuff
- I dono wayland prob crashes on there sometimes

### Original Readme

Here is the original `readme.md` contents.

HexChat is an IRC client for Windows and UNIX-like operating systems.  
See [IRCHelp.org](http://irchelp.org) for information about IRC in general.  
For more information on HexChat please read our [documentation](https://hexchat.readthedocs.org/en/latest/index.html):
- [Downloads](http://hexchat.github.io/downloads.html)
- [FAQ](https://hexchat.readthedocs.org/en/latest/faq.html)
- [Changelog](https://hexchat.readthedocs.org/en/latest/changelog.html)
- [Python API](https://hexchat.readthedocs.org/en/latest/script_python.html)
- [Perl API](https://hexchat.readthedocs.org/en/latest/script_perl.html)

---

<sub>
X-Chat ("xchat") Copyright (c) 1998-2010 By Peter Zelezny.  
HexChat ("hexchat") Copyright (c) 2009-2014 By Berke Viktor.
BirdChat ("birdchat") Copyright (c) 2025 By Birdnest.
</sub>

<sub>
This program is released under the GPL v2 with the additional exemption
that compiling, linking, and/or using OpenSSL is allowed. You may
provide binary packages linked to the OpenSSL libraries, provided that
all other requirements of the GPL are met.
See file COPYING for details.
</sub>
