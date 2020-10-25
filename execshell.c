
/* see LICENSE file for license details */

#include <sys/types.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <unistd.h>

#include <skalibs/env.h>
#include <skalibs/stralloc.h>
#include <execline/execline.h>

#define UTF8

#include "linenoise/linenoise.h"
#ifdef UTF8
#include "linenoise/encodings/utf8.h"
#endif

#define EXECSHELL_FLAGVAR "EXECSHELL_RUNNING"
#define EXECSHELL_EXIT 112
#define EXECSHELL_BUILTIN "self"

static void *xmalloc(size_t sz) {
    void *ret;

    ret = malloc(sz);
    if (ret == NULL) {
        err(1, "could not malloc");
    }

    return ret;
}

static void signal_setup() {
    for (int i = 1; i < NSIG; i++) {
        if (i != SIGKILL && i != SIGSTOP && i != SIGSEGV) {
            signal(i, (i == SIGCHLD ? SIG_DFL : SIG_IGN));
        }
    }
}

static void signal_unsetup() {
    for (int i = 1; i < NSIG; i++) {
        if (i != SIGKILL && i != SIGSTOP && i != SIGSEGV) {
            signal(i, SIG_DFL);
        }
    }
}

int main(int argc, char *argv[]) {
    int ret, wstatus, estatus;
    pid_t pid, pret;
    char *buffer, **pargs, **eargs;
    stralloc sa = STRALLOC_ZERO;

    (void) argc;

    if (getenv(EXECSHELL_FLAGVAR) == NULL) {
        /* parent process */
        signal_setup();

        while ((pid = fork()) > 0) {
            if (waitpid(pid, &wstatus, 0) == -1) {
                err(1, "parent could not wait on command runner");
            }

            if (WIFEXITED(wstatus)) {
                /* normal exit */
                estatus = WEXITSTATUS(wstatus);

                if (estatus == 0 || estatus == EXECSHELL_EXIT) {
                    /* clean exit or internal error */
                    exit(estatus);
                }
            } else {
                /* it didn't exit normally, so something must be up */
                errx(1, "command runner process died with signal");
            }
        }

        if (pid == -1) {
            err(1, "parent process could not fork");
        }
    } else {
        if (unsetenv(EXECSHELL_FLAGVAR) == -1) {
            warn("could not unset flag environment variable");
        }
    }

    /* child process */
    (void) linenoiseHistorySetMaxLen(500);

#ifdef UTF8
    /* set up unicode terminal handling bits */
    linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen,
                                  linenoiseUtf8NextCharLen,
                                  linenoiseUtf8ReadCode);
#endif

    while (1) {
        errno = 0;
        buffer = linenoise("> ");

        if (buffer == NULL) {
            if (errno == EAGAIN) {
                continue;
            } else if (errno != 0) {
                err(EXECSHELL_EXIT, "could not read input");
            } else {
                break;
            }
        }

        (void) linenoiseHistoryAdd(buffer);
        ret = el_parse_from_string(&sa, buffer);

        switch (ret) {
            case -4:
                warnx("unmatched }");
                goto mainloop_out;
            case -3:
                warnx("unmatched {");
                goto mainloop_out;
            case -2:
                warnx("syntax error");
                goto mainloop_out;
            case -1:
                warnx("unable to parse input");
                goto mainloop_out;
            case 0:
                goto mainloop_out;
        }

        pargs = (char **) xmalloc(sizeof(char *) * (ret+2));
        if (!env_make((const char **) pargs, ret, sa.s, sa.len)) {
            err(1, "could not env_make");
        }
        pargs[ret] = 0;

        if (strcmp(pargs[0], EXECSHELL_BUILTIN) == 0) {
            pargs[ret] = argv[0];
            pargs[ret+1] = 0;

            eargs = pargs+1;
            pid = 0;

            if (setenv(EXECSHELL_FLAGVAR, "1", 1) == -1) {
                err(1, "could not set flag environment variable");
            }
        } else {
            eargs = pargs;
            signal_setup();
            pid = fork();
        }

        if (pid == -1) {
            err(1, "could not fork");
        } else if (pid == 0) {
            signal_unsetup();

            (void) execvp(eargs[0], eargs);
            err(1, "could not execvp: %s", eargs[0]);
        }

        pret = waitpid(pid, NULL, 0);
        if (pret == -1) {
            warn("could not waitpid");
        }
        signal_unsetup();

        free(pargs);

    mainloop_out:
        stralloc_free(&sa);
        free(buffer);
    }

    return 0;
}

