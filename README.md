# codingstyle checks for libdwarf

See the libdwarf-code/codingstyle
file in the libdwarf distribution.

There is no configure here, just a single simple Makefile.

    # to build:
    make
    # to check it is built properly:
    make check

## dicheck

Usage:   dickeck [-l] file ...

dicheck checks for certain violations of the
libdwarf codingstyle (see the
libdwarf-code/codingstyle
file in the libdwarf distribution)

Add -l to the command and dicheck also reports
all lines longer than 70 characters
(an arbitrary limit).

## trimtrailing

Usage:   trimtrailing  <file.c>

trimtrailing removes all trailing whitespace from
file.c.   Multiple blank lines in a row are
reduced to a single blank line.
It writes the changed file into <file.c>
