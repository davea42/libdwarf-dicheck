/*
Copyright (c) 2011-2021 David Anderson.
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

// David's Indent Checker.
// It's unsophisticated and does no real parsing.
// It does not change the target source. It just
// reads the list of files named and reports on all lines
// that do not have a certain format to the beginnings of lines.
//
// It reports any lines not following:
//   Lines start with space characters, no tabs.
//   Non-space characters begin on column 0,4,8,12, etc.
//     (or think of these as 1,5,9,13, etc.)
//   Indentation increases by 4 if it increases.
//   Indentation decreases by any amount.
//   Any line with trailing whitespace.
//
//   As of May 2018 also reports any leading lines or
//   trailing lines that are all whitespace.
//
// To build try:
//      g++ dicheck.cc -o dicheck
// or some equivalent C++ compile command.
// It will not work on Macintosh OSX files
// unless you modify this source as it does not
// treat the '\r' (CarriageReturn) character as end-of-line.
// It should work on  CRLF or POSIX LF line endings
// without change.

#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>  // for exit()
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <libgen.h> /* for basename */

using std::ofstream;
using std::ifstream;
using std::string;
using std::cout;
using std::endl;
using std::cin;

// Usage:  dicheck  file ...

static unsigned indentamount = 4;
static bool showtrailingspaces (true);
static unsigned errcount = 0;
static bool checklinelength = true; //cannot be turned off now.
static bool pythonsource = false;
static long  maxlinelength = 70;

#define OURBUFSIZ  2000
static unsigned char inbuf[OURBUFSIZ+1];
static unsigned inpos = 0;
static unsigned incharcount = 0;

// bsb stands for brace space brace
// as in
//     }
// blankline
//     }
//suggesting the blankline is pointless.
static int bsb[2] = {-1,-1};

static void
resetbsb()
{
    bsb[0] = -1;
    bsb[1] = -1;
}
static void
newbsbblank(int line)
{
    if (bsb[0] != -1) {
        bsb[1] = line;
    }
}
static void
newbsbbrace(int line, string &path)
{
    if (bsb[0] == -1) {
        bsb[0] = line;
        return;
    }
    if (bsb[1] != -1) {
        if (line == (bsb[1]+1)) {
            if (line == (bsb[0]+2)) {
                cout << line << " of " << path  <<
                    " is blank surrounded by }" << endl;
            }
        }
    }
    resetbsb();
    bsb[0] = line;
}

static bool
lastnmatch(const char *test_string,
    int curpos)
{
    int testlen = strlen(test_string);

    if (testlen > curpos) {
        return false;
    }
    int startindex = curpos-testlen;
    if (strncmp(test_string,
        (const char *)(inbuf+startindex),testlen)) {
        return false;
    }
    return true;
}

static void
process_a_line(int line,
    string   &path,
    bool&     incomment,
    bool&     lastlinemacro,
    bool&     found_nonblank_ever,
    unsigned& lastlineindent,
    unsigned& current_blankline_count,
    unsigned& sequential_blankline_count)
{
    unsigned char c = 0;
    bool leadingchar = false;
    char leadingcharv = ' ';
    bool curlinemacro  = false;
    bool blankline     = true;
    bool inquotes = false;
    bool insquote = false;
    bool trailingwhitespace = false;
    bool onelinecomment = false; // Meaning C++ comment.
    int  curlineindent = 0;
    for (inpos = 0 ; inpos < incharcount ; ++inpos) {
        c = inbuf[inpos];
        bool onquoteterminator = false;

        if (c == '\n') {
            if (!leadingchar) {
                newbsbblank(line);
            } else if (leadingcharv == '}') {
                newbsbbrace(line,path);
            }
        } else if (!leadingchar && c != ' ') {
            leadingchar = true;
            leadingcharv =  c;
        }
        switch(c) {
        case ' ':
            if (inquotes || insquote) {
                break;
            }
            trailingwhitespace += 1;
            break;
        case '\n': {
            bool saidleadingblank = false;

            if (inquotes || insquote) {
                cout << line << " of " << path  <<
                    " has a non-terminated quote"<< endl;
            }
            if (blankline) {
                newbsbblank(line);
                if (!found_nonblank_ever) {
                    cout << line << " of " << path  <<
                        " is a leading blank line "<< endl;
                    saidleadingblank = true;
                }
            } else {
                current_blankline_count = 0;
            }
            if (curlineindent%indentamount) {
                if (incomment && (curlineindent <= 3)) {
                    // For our copyright and other comment blocks.
                    if (!saidleadingblank && showtrailingspaces &&
                        trailingwhitespace) {
                        cout << line << ":" << inpos <<
                            " of " << path <<
                            " has " << trailingwhitespace <<
                            " whitespace chars on the end. " << endl;
                        errcount++;
                    }
                    break;
                }
                cout << line << ":" << curlineindent << " of "
                    << path <<
                    " has a bad indent. " << endl;
                errcount++;
            } else {
                if (curlineindent == lastlineindent) {
                    // OK.
                } else if ( curlineindent < lastlineindent) {
                    if (!blankline && !curlinemacro) {
                        // OK. we allow skipping back a
                        // fair amount due to function
                        // calls adding nesting.
                        lastlineindent = curlineindent;
                    }
                } else if ( (curlineindent >= indentamount) &&
                    (((curlineindent - indentamount) ==
                        lastlineindent) ||
                    (lastlinemacro)) ) {
                    // lastline macro means ignore the indent
                    // of the macro in the last line.
                    // This is not a great fit with python :-)
                    lastlineindent = curlineindent;
                    // OK.
                } else {
                    cout << line << ":" << inpos << " of "
                        << path <<
                        " has a bad indent change, last indent "
                        <<
                        lastlineindent << "  cur indent "
                        << curlineindent << endl;
                    errcount++;
                }
            }
            lastlinemacro = curlinemacro;

            if (!saidleadingblank && showtrailingspaces &&
                trailingwhitespace) {
                cout << line << ":" << inpos << " of " << path <<
                    " has " << trailingwhitespace <<
                    " whitespace chars on the end. " << endl;
                errcount++;
            }
            if (checklinelength) {
                /*  Let the initial few lines run over,
                    they are copyright notices. */
                if (inpos > maxlinelength &&
                    line > 6) {
                    cout << line << ": " << inpos <<
                        " of " << path <<
                        "  is " << inpos << " characters long"
                        <<endl;
                }
            }
            trailingwhitespace = 0;
            if (!blankline && !curlinemacro) {
                lastlineindent = curlineindent;
            }
            break;
        }
        case '\'':
            if (incomment) {
                if (blankline) {
                    curlineindent = inpos;
                    blankline = false;
                }
                trailingwhitespace = 0;
                break;
            }
            if (onelinecomment) {
                break;
            }
            if (inquotes) {
                break;
            }
            if (insquote) {
                if (lastnmatch("\\\\",inpos) ) {
                    insquote = false;
                    onquoteterminator = true;
                } else if (!lastnmatch("\\",inpos) ) {
                    insquote = false;
                    onquoteterminator = true;
                } else {
                    /* nothing needed. */
                }
            } else {
                insquote = true;
                trailingwhitespace = 0;
            }
            if (blankline) {
                curlineindent = inpos;
                blankline = false;
            }
            break;
        case '"':
            if (incomment) {
                if (blankline) {
                    curlineindent = inpos;
                    blankline = false;
                }
                trailingwhitespace = 0;
                break;
            }
            if (onelinecomment) {
                break;
            }
            if (insquote) {
                break;
            }
            if (inquotes) {
                if (lastnmatch("\\\\",inpos) ) {
                    inquotes = false;
                    onquoteterminator = true;
                } else if (!lastnmatch("\\",inpos) ) {
                    inquotes = false;
                    onquoteterminator = true;
                } else {
                    /* nothing needed. */
                }
            } else {
                inquotes = true;
                trailingwhitespace = 0;
            }
            if (blankline) {
                curlineindent = inpos;
                blankline = false;
            }
            break;
        case '/':
            trailingwhitespace = 0;
            if (blankline) {
                curlineindent = inpos;
                blankline = false;
            }
            if (onelinecomment) {
                break;
            }
            if (inquotes || insquote) {
                break;
            }
            if (incomment) {
                if (lastnmatch("*",inpos)) {
                    incomment = false;
                } else {
                    break;
                }
            }
            if (lastnmatch("/",inpos)) {
                // C++ comment
                onelinecomment = true;
            }
            break;
        case '*':
            if (blankline) {
                curlineindent = inpos;
                blankline = false;
            }
            if (onelinecomment) {
                break;
            }
            if (inquotes || insquote) {
                break;
            }
            if (!incomment) {
                if (lastnmatch("/",inpos)) {
                    incomment = true;
                }
            } else {
                /* nothing to do */
            }
            trailingwhitespace = 0;
            break;
        case '\v': // Vertical tab.
            if (inquotes) {
                break;
            }
            trailingwhitespace += 1;
            break;
        case '\t':
            cout << line << ":" << inpos << " of " << path <<
                " is a tab. " << endl;
            if (inquotes || insquote) {
                break;
            }
            trailingwhitespace += 1;
            break;
        case '#':
            trailingwhitespace = 0;
            if (onelinecomment) {
                break;
            }
            if (inquotes || insquote) {
                if (blankline) {
                    curlineindent = inpos;
                    blankline = false;
                }
                break;
            }
            if (pythonsource) {
                onelinecomment = true;
                if (blankline) {
                    curlineindent = inpos;
                    blankline = false;
                }
                break;
            } else if (blankline) {
                // Macro of some kind. Indent 0 ok.
                // Do not count this indent, really.
                curlinemacro = true;
                blankline = false;
            }
            break;
        default:
            trailingwhitespace = 0;
            if (blankline) {
                curlineindent = inpos;
                blankline = false;
            }
            if (onelinecomment || incomment) {
                break;
            }
            if (inquotes || insquote) {
                break;
            }
            if (curlinemacro) {
                break;
            }
            break;
        } // End switch on character
        if (!incomment && !onelinecomment &&
            !onquoteterminator && !inquotes) {
            if (lastnmatch("if  ",inpos)) {
                cout << line << ":" << inpos << " of "
                    << path <<
                    " has an if  , 2+ spaces after if"
                    << endl;
            } else if (lastnmatch("if(",inpos)) {
                cout << line << ":" << inpos << " of "
                    << path <<
                    " has an if(, no space after if"
                    << endl;
            } else if (lastnmatch("for(",inpos)) {
                cout << line << ":" << inpos << " of "
                    << path <<
                    " has a for(, no space after for"
                    << endl;
            } else if (lastnmatch("for  ",inpos)) {
                cout << line << ":" << inpos << " of "
                    << path <<
                    " has a for  , two spaces after for"
                    << endl;
            }
        }
    }
    if (blankline) {
        ++current_blankline_count;
        ++ sequential_blankline_count;
    } else {
        current_blankline_count = 0;
        found_nonblank_ever = true;
        sequential_blankline_count = 0;
    }
}

static void
processfile(string &path, ifstream * mystream)
{
    unsigned char uc = 0;
    char c = 0;
    bool found_nonblank = false;
    unsigned trailingwhitespace = 0;
    unsigned line = 1;
    unsigned lastlineindent = 0;
    unsigned current_blankline_count = 0;
    unsigned sequential_blankline_count = 0;
    bool lastlinemacro = false;
    bool done = false;
    bool incomment = false;

    while (!done) {
        int indx = 0;
        bool havenewline = false;
        while ( mystream->get(c)) {
            uc = c;
            if (indx < OURBUFSIZ) {
                inbuf[indx] = uc;
                indx++;
                inbuf[indx] = 0;
                if (uc == '\n') {
                    havenewline = true;
                    incharcount = indx;
                    break;
                }
                continue;
            }
            cout << line << " of " << path  <<
                "Is too long. Likely not a text file at all. "
                "Giving up"
                <<endl;
                exit(1);
        }
        if (!havenewline) {
            if (indx > 0) {
                // Non-terminated last line
                cout << line << " of " << path  <<
                    " does not have a newline!" <<endl;
            } else {
                done = true;
                break;
            }
        }
        process_a_line(line,path,
            incomment,lastlinemacro,
            found_nonblank,
            lastlineindent, current_blankline_count,
            sequential_blankline_count);
        if (sequential_blankline_count > 1) {
            cout << line << " of " << path  <<
                " is " << sequential_blankline_count <<
                " blank lines in a row" <<endl;
        }
        ++line;
    }
    if (current_blankline_count > 0) {
        if (current_blankline_count == 1 ){
            string singular= " last line is empty";
            cout << "In "<< path  << singular<< endl;
        } else {
            string plural= " lines are empty";
            cout << "In " << path << " last " <<
                current_blankline_count <<
                plural << endl;
        }
    }
    return;
}

void
usage()
{
    cout << "dicheck [-t] [-h] [-l] [-p] file ..." << endl;
    cout << "  where -t means ignore trailing whitespace" << endl;
    cout << "  where -h means print this message "
        "and nothing else." <<endl;
    cout << "  where -l means nothing now,"
        " lines longer than 70 characters get a warning"<<endl;
    cout << "  where -p means python and # is comment not macro" <<
        endl;
    cout << "  where --linelength=<n> means report lines "
        "greater" <<endl;
    cout << "    than n characters long"<<endl;
    cout << "Named files required as arguments" << endl;
    cout << "Use trimtrailing to remove trailing whitespace" << endl;
    exit(1);
}

int
main(int argc, char**argv)
{
    if (argc == 1) {
        usage();
    } else {
        unsigned i = 1;
        for (; i < argc; ++i) {
            string f(argv[i]);
            if (f == "-l") {
                checklinelength = true;
                continue;
            } else if (f == "-h") {
                usage();
            }
            const char *fp = f.c_str();
            if (!strncmp(fp,"--linelength=",13)) {
                char *endptr = 0;
                char*numval = (char *)fp+13;

                errno = 0;
                maxlinelength = strtol(numval,&endptr,0);
                if (endptr == numval) {
                    cout << " Option --linelength= "
                        "Has no digits for length" <<endl;
                    exit(1);
                }
                if (*endptr) {
                    cout << " Option --linelength= "
                        " value is not all digits for length"
                        <<endl;
                    exit(1);
                }
                checklinelength = true;
                continue;
            }
            if (f == "-t") {
                showtrailingspaces = false;
                continue;
            }
            if (f == "-p") {
                pythonsource = true;
                continue;
            }
            break;
        }
        for (; i < argc; ++i) {
            string f(argv[i]);
            ifstream ist(f.c_str());
            if (!ist) {
                cout << "Cannot open " << f << endl;
                exit(1);
            }
            processfile(f,&ist);
        }
    }
    if (errcount) {
        exit(1);
    }
    exit(0);
}
