// dispatch/sonar_gate_smoke_test.cc
//
// TEMPORARY. Deliberately vulnerable program used only to verify that the
// SonarCloud quality gate fails a PR build correctly. Not part of the BES
// product. Delete this file and the noinst_PROGRAMS stanza in
// dispatch/Makefile.am before merging. Note that this does not, in and of
// itself, trigger a Quality Gate failure. I changed the gate value to
// try this smoke test. jhrg 9/2/26
//
// This is built using a target in the dispatch/Makefile.am. Unless we want
// the smoke test to run, that should be commented out. jhrg 9/2/26
//
// CWE-120 / CWE-787: unbounded strcpy() into a fixed-size stack buffer.
#include <cstring>

int main(int argc, char **argv) {
    char buf[16];
    if (argc > 1) {
        strcpy(buf, argv[1]);
    }
    return 0;
}
