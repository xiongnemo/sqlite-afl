# Fuzzing SQLite Using AFL

This repository contains seed files and instructions for fuzzing the
SQL language parser and code generator of SQLite using the
American Fuzzy Lop (AFL) fuzzer.

## Prerequesites

  1.  The SQLite source tree available from <https://www.sqlite.org/src>
  2.  The latest version of AFL available from <http://lcamtuf.coredump.cx/afl>

## Steps:

  1.  Untar the AFL download into a directory which we will here call ~/afl

  2.  cd into ~/afl and type "make"

  3.  Create a subdirecctory ~/afl/sqlite

  4.  cd into ~/afl/sqlite and open this repository:
      fossil open ~/repos/afl-sqlite.fossil

  5.  Open the SQLite source repository in a directory which we will here
      call ~/sqlite.

  6.  cd to ~/sqlite and build the amalgamation:
      ./configure; make sqlite3.c

  7.  cd back to ~/afl/sqlite and copy source files into the ~/afl/sqlite
      directory:  cp ~/sqlite/sqlite3.[ch] .; cp ~/sqlite/tool/fuzzershell.c .

  8.  Build the instrumented fuzzershell:
      ../afl-gcc -O3 -o sqlitefuzz -DSQLITE_THREADSAFE=0 -DSQLITE_ENABLE_LOAD_EXTENSION=0 -DSQLITE_NO_SYNC -DSQLITE_DEBUG -I. fuzzershell.c sqlite3.c -ldl

  9.  Make an output directory:  rm -rf out; mkdir out

  10.  Disable the AFL warning about CPU frequency:
       export set AFL_SKIP_CPUFREQ=1

  11.  Run the fuzzer:
       ../afl-fuzz -i minimized_culled -o out -x ../testcases/_extra/sql -- ./sqlitefuzz
