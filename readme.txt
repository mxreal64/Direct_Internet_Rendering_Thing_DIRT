dirt - direct internet rendering thing
======================================

what is this?
-------------
it is a browser engine. no frameworks. no bloat. no chrome copycat garbage. 
just a raw pipe that grabs bytes off a network card and mashes them onto your screen.
it has a strict 40mb ram ceiling because modern web engines are bloated monsters
and i got tired of watching standard browsers turn laptops into space heaters.

why dirt?
---------
- 40mb ram wall: if it takes more than 40mb to parse a webpage, the webpage is wrong (or i'm wrong idk).

the tech stack
--------------
- language: pure c++23 standard modules (import std;) (in most of them, i think)
- lexer: zero-copy pointer scanner using std::string_view windows (maybe)
- allocator: custom static arena buffers with explicit destructor flushing (prolly)
- renderer: bare-metal immediate-mode layout engine mapped directly to framebuffers (possibly)

how to run it
-------------
don't overcomplicate it. compile the module blocks, feed it a clean html file like index.html,
and watch the pixels snap onto the display grid in under 50ms.
if it doesn't work, well that's why i made it open source, i'm absolutely not a maxlevel code guy.

licensed under gplv3 because open-source infrastructure belongs to the builders.
real64 labs // 2026
