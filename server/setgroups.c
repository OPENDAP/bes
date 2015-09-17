
/* This code was copied on 13 May 2015 from:
 * https://www.securecoding.cert.org/confluence/display/c/POS36-C.+Observe+correct+revocation+order+while+relinquishing+privileges
 * and then minimally modified to make it fit with the besdaemon code.
 * jhrg 5/13/15
 */

#include <unistd.h>
#include <stdlib.h>

/* Returns nonzero if the two group lists are equivalent (taking into
 account that the lists may differ wrt the egid */
static int eql_sups(const int cursups_size, const gid_t* const cursups_list, const int targetsups_size,
        const gid_t* const targetsups_list)
{
    int i;
    int j;
    const int n = targetsups_size;
    const int diff = cursups_size - targetsups_size;
    const gid_t egid = getegid();
    if (diff > 1 || diff < 0) {
        return 0;
    }
    for (i = 0, j = 0; i < n; i++, j++) {
        if (cursups_list[j] != targetsups_list[i]) {
            if (cursups_list[j] == egid) {
                i--; /* skipping j */
            }
            else {
                return 0;
            }
        }
    }
    /* If reached here, we're sure i==targetsups_size. Now, either
     j==cursups_size (skipped the egid or it wasn't there), or we didn't
     get to the egid yet because it's the last entry in cursups */
    return j == cursups_size || (j + 1 == cursups_size && cursups_list[j] == egid);
}

/* Sets the supplementary group list, returns 0 if successful  */
int set_sups(const int target_sups_size, const gid_t* const target_sups_list)
{
#ifdef __FreeBSD__
    const int targetsups_size = target_sups_size + 1;
    gid_t* const targetsups_list = (gid_t* const) malloc(sizeof(gid_t) * targetsups_size);
    if (targetsups_list == NULL) {
        /* handle error */
    }
    memcpy(targetsups_list+1, target_sups_list, target_sups_size * sizeof(gid_t) );
    targetsups_list[0] = getegid();
#else
    const int targetsups_size = target_sups_size;
    const gid_t* const targetsups_list = target_sups_list;
#endif
    if (geteuid() == 0) { /* allowed to setgroups, let's not take any chances */
        if (-1 == setgroups(targetsups_size, targetsups_list)) {
            /* handle error */
            return -1;
        }
    }
    else {
        int cursups_size = getgroups(0, NULL);
        if (cursups_size == -1)
            return -1;

        gid_t* cursups_list = (gid_t*) malloc(sizeof(gid_t) * cursups_size);
        if (cursups_list == NULL) {
            /* handle error */
            return -1;
        }
        if (-1 == getgroups(cursups_size, cursups_list)) {
            /* handle error */
            free(cursups_list);
            return -1;
        }
        if (!eql_sups(cursups_size, cursups_list, targetsups_size, targetsups_list)) {
            if (-1 == setgroups(targetsups_size, targetsups_list)) { /* will probably fail... :( */
                /* handle error */
                free(cursups_list);
                return -1;
            }
        }
        free(cursups_list);
    }

#ifdef __FreeBSD__
    free( targetsups_list);
#endif
    return 0;
}

