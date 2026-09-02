// dispatch/sonar_gate_smoke_test.cc
//
// TEMPORARY. Deliberately vulnerable program used only to verify that the
// SonarCloud quality gate fails a PR build correctly. Not part of the BES
// product. Delete this file and the noinst_PROGRAMS stanza in
// dispatch/Makefile.am before merging.
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
