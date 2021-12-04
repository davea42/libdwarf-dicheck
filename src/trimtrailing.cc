/*
Copyright (c) 2011-2020 David Anderson.
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:
    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the
    following disclaimer.

    Redistributions in binary form must reproduce the
    above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or
    other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
NO EVENT SHALL David Anderson BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  */

// compile with something like
// g++ trimtrailing.cc -o trimtrailing

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

// Usage:  trimtrailing  file ...

#define OURBUFSIZE  2000
static void
process_a_line(int line,
    string&path,
    unsigned char* inbuf,
    int inbuflen,
    ofstream *fileout,
    unsigned & blankline_count)
{
    int i = 0;
    unsigned char c = 0;
    unsigned char outbuf[OURBUFSIZE+1];
    int outpos = 0;
    int trailingwhitespace = 0;
    int done = 0;

    outbuf[0] = 0;
    for ( ; !done && i < inbuflen ; ++i) {
        c = inbuf[i];
        switch(c) {
        case '\n': {
            if (!(outpos-trailingwhitespace)) {
                if (blankline_count) {
                    break;
                }
                ++blankline_count;
            } else {
                blankline_count = 0;
            }
            fileout->write((const char *)outbuf,
                outpos-trailingwhitespace);
            fileout->put('\n');
            done = 1;
            break;
        }
        case '\v': // Vertical tab.
            trailingwhitespace += 1;
            outbuf[outpos++] = c;
            break;
        case ' ':
            trailingwhitespace += 1;
            outbuf[outpos++] = c;
            break;
        case '\t':
            trailingwhitespace += 1;
            outbuf[outpos++] = c;
            break;
        default:
            trailingwhitespace = 0;
            outbuf[outpos++] = c;
            break;
        }
    }
}

static void
processfile(string &path, ifstream&mystream,
    ofstream *fileout)
{
    static unsigned char inbuf[OURBUFSIZE+1];
    bool done = false;
    int line = 0;
    unsigned blankline_count = 0;

    while (!done) {
        int           indx = 0;
        bool          havenewline = false;
        char          c = 0;
        unsigned char uc = 0;

        ++line;
        while (mystream.get(c)) {
            if (indx < OURBUFSIZE) {
                inbuf[indx] = (unsigned char)c;
                indx++;
                inbuf[indx] = 0;
                if (c == '\n') {
                    havenewline = true;
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
        process_a_line(line,path,inbuf,indx,fileout, blankline_count);
        indx = 0;
        if (!havenewline) {
            done = true;
        }
    }
    return;
}

void
usage()
{
    cout << "trimtrailing [-s] [-h] file ..."<<endl;
    cout << "  where file is copied to file.out with "
        "trailing spaces removed" <<endl;
    cout << "  and then replaced by file.out" << endl;
    cout << "  where -s means save the file.out, not replace file."
        <<endl;
    cout << "  where -h means print this message and exit(1)." <<endl;
    exit(1);
}

int
main(int argc, char**argv)
{
    if (argc == 1) {
        usage();
    } else {
        bool saveout = false;
        string sopt("-s"); // Meaning save .out
        string hopt("-h"); // Meaning save .out
        unsigned i = 1;
        for (; i < argc; ++i) {
            string f(argv[i]);
            if (f == hopt) {
                usage();
            }
            if (f == sopt) {
                saveout = true;
                continue;
            }
            ifstream ist(f.c_str());
            if (!ist) {
                cout << "Cannot open " << f << endl;
                exit(1);
            }
            string outname = f + ".out";
            ofstream fout(outname.c_str());
            if (!fout) {
                cout << "Cannot open output " << outname << endl;
                exit(1);
            }
            processfile(f,ist,&fout);
            fout.flush();
            if (! saveout) {
                int res = rename(outname.c_str(),f.c_str());
                if (res < 0) {
                    cout << "Rename " <<outname <<" to "<<
                        f<< " failed." <<endl;
                }
            }
        }
    }
    exit(0);
}
