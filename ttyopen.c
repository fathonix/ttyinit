/*
 * ttyopen - TTY opener, based on Busybox implementation of init
 *
 * usage: ttyopen TTYPATH CMD [ARG1] [ARG2] ...
 *
 * Copyright (C) 1995-2023 The Busybox Project
 * Copyright (C) 2023 Aldo Adirajasa Fathoni
 *
 * Licensed under GPLv2 or later, see file LICENSE in this source tree.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <paths.h>
#include <termios.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

void printerr(int err, const char *fmt, ...)
{
	va_list argptr;
    va_start(argptr, fmt);
	vfprintf(stderr, fmt, argptr);
	va_end(argptr);
	exit(err);
}

void sanitize_stdio(void)
{
	int fd;

	fd = open(_PATH_DEVNULL, O_RDWR);
	if (fd < 0) {
		/* NB: we can be called as bb_sanitize_stdio() from init
		 * or mdev, and there /dev/null may legitimately not (yet) exist!
		 * Do not use xopen above, but obtain _ANY_ open descriptor,
		 * even bogus one as below. */
		if ((fd = open("/", O_RDONLY, 0666)) < 0) /* don't believe this can fail */
			printerr(errno, "cannot open '/'");
	}
	while ((unsigned)fd < 2)
		fd = dup(fd); /* have 0,1,2 open at least to /dev/null */
	/* if daemonizing, detach from stdio & ctty */
	setsid();
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
}

static void set_sane_term(void)
{
	struct termios tty;

	tcgetattr(STDIN_FILENO, &tty);

	/* set control chars */
	tty.c_cc[VINTR] = 3;	/* C-c */
	tty.c_cc[VQUIT] = 28;	/* C-\ */
	tty.c_cc[VERASE] = 127;	/* C-? */
	tty.c_cc[VKILL] = 21;	/* C-u */
	tty.c_cc[VEOF] = 4;	/* C-d */
	tty.c_cc[VSTART] = 17;	/* C-q */
	tty.c_cc[VSTOP] = 19;	/* C-s */
	tty.c_cc[VSUSP] = 26;	/* C-z */

#ifdef __linux__
	/* use line discipline 0 */
	tty.c_line = 0;
#endif

	/* Make it be sane */
#ifndef CRTSCTS
# define CRTSCTS 0
#endif
	/* added CRTSCTS to fix Debian bug 528560 */
	tty.c_cflag &= CBAUD | CBAUDEX | CSIZE | CSTOPB | PARENB | PARODD | CRTSCTS;
	tty.c_cflag |= CREAD | HUPCL | CLOCAL;

	/* input modes */
	tty.c_iflag = ICRNL | IXON | IXOFF;

	/* output modes */
	tty.c_oflag = OPOST | ONLCR;

	/* local modes */
	tty.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN;

	tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

static void console_init(char* tty)
{
#ifdef VT_OPENQRY
	int vtno;
#endif
	char *s;

    if (tty) {
        s = tty;
    } else {
		s = getenv("CONSOLE");
	    if (!s)
		    s = getenv("console");
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	    /* BSD people say their kernels do not open fd 0,1,2; they need this: */
	    if (!s)
		    s = (char*)"/dev/console";
#endif
    }
	if (s) {
		int fd = open(s, O_RDWR | O_NONBLOCK | O_NOCTTY);
		if (fd >= 0) {
			dup2(fd, STDIN_FILENO);
			dup2(fd, STDOUT_FILENO);
			if (dup2(fd, STDERR_FILENO) != STDERR_FILENO)
				printerr(errno, "cannot duplicate file descriptor");
			close(fd);
		}
	} else {
		/* Make sure fd 0,1,2 are not closed
		 * (so that they won't be used by future opens) */
		sanitize_stdio();
// Users report problems
//		/* Make sure init can't be blocked by writing to stderr */
//		fcntl(STDERR_FILENO, F_SETFL, fcntl(STDERR_FILENO, F_GETFL) | O_NONBLOCK);
	}

	s = getenv("TERM");
#ifdef VT_OPENQRY
	if (ioctl(STDIN_FILENO, VT_OPENQRY, &vtno) != 0) {
		/* Not a linux terminal, probably serial console.
		 * Force the TERM setting to vt102
		 * if TERM is set to linux (the default) */
		if (!s || strcmp(s, "linux") == 0)
			setenv((char*)"TERM", (char *)"vt102");
	} else
#endif
	if (!s)
		setenv((char*)"TERM", (char *)"linux", 1);
}

int main(int argc, char **argv, char **envp)
{
    if (argc < 3)
        printerr(1, "usage: %s TTYPATH CMD [ARG1] [ARG2] ...\n", argv[0]);

    char *cmdargsptr[argc - 1];

	cmdargsptr[0] = argv[2];
    if (argc > 3)
		for (size_t i = 3; i < argc; i++)
        	cmdargsptr[i - 2] = argv[i];
	cmdargsptr[argc - 2] = NULL;

    console_init(argv[1]);
    set_sane_term();
	setsid();

	int exc = execve(argv[2], cmdargsptr, envp);
    if (exc != 0)
		fprintf(stderr, "exec error: %s\n", strerror(exc));
    return exc;
}