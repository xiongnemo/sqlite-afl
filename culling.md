# How to cull test cases

AFL generates a large number of test cases.  But many of the test cases are
redundant in the sense that they do not cause new behavior in the test program.
"Culling" is the process of selecting a subset of the test cases which cover
all behavior but which are also much smaller than the total set.

This document overviews the steps for culling the test cases.

  1.  Create two new directories, one in which to store the complete set
      of test cases and another in which to store the culled output.

         mkdir tmp1 cull1

  2.  Copy test cases into tmp1.  The example below assumes that afl-fuzz
      has been run (or is running) with "-o out".

         cp out/*/id:* tmp1

  3.  Run afl-cmin on to generate the culled output.  Two examples are
      below.  The first example is for database fuzzing and the second
      is for SQL fuzzing.

         ../afl-cmin -i tmp1 -o cull1 -f testdb -- ./fuzzershell --database testdb db-fuzz-ck.txt

         ../afl-cmin -i tmp1 -o cull1 -- ./fuzzershell
