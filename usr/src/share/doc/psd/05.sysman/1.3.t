.\" Copyright (c) 1983 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)1.3.t	5.1 (Berkeley) %G%
.\"
.\" 1.3.t 5.1 86/05/08
.sh "Signals
.PP
.NH 3
Overview
.PP
The system defines a set of \fIsignals\fP that may be delivered
to a process.  Signal delivery resembles the occurrence of a hardware
interrupt: the signal is blocked from further occurrence,
the current process context is saved, and a new one
is built.  A process may specify
the \fIhandler\fP to which a signal is delivered, or specify that
the signal is to be \fIblocked\fP or \fIignored\fP.  A process may
also specify that a
\fIdefault\fP action is to be taken when signals occur.
.PP
Some signals
will cause a process to exit when they are not caught.  This
may be accompanied by creation of a \fIcore\fP image file, containing
the current memory image of the process for use in post-mortem debugging.
A process may choose to have signals delivered on a special
stack, so that sophisticated software stack manipulations are possible.
.PP
All signals have the same \fIpriority\fP.  If multiple signals
are pending simultaneously, the order in which they are delivered
to a process is implementation specific.  Signal routines execute
with the signal that caused their invocation \fIblocked\fP, but other
signals may yet occur.  Mechanisms are provided whereby critical sections
of code may protect themselves against the occurrence of specified signals.
.NH 3
Signal types
.PP
The signals defined by the system fall into one of
five classes: hardware conditions,
software conditions, input/output notification, process control, or
resource control.
The set of signals is defined in the file <signal.h>.
.PP
Hardware signals are derived from exceptional conditions which
may occur during
execution.  Such signals include SIGFPE representing floating
point and other arithmetic exceptions, SIGILL for illegal instruction
execution, SIGSEGV for addresses outside the currently assigned
area of memory, and SIGBUS for accesses that violate memory
protection constraints.
Other, more cpu-specific hardware signals exist,
such as those for the various customer-reserved instructions on
the VAX (SIGIOT, SIGEMT, and SIGTRAP). 
.PP
Software signals reflect interrupts generated by user request:
SIGINT for the normal interrupt signal; SIGQUIT for the more
powerful \fIquit\fP signal, that normally causes a core image
to be generated; SIGHUP and SIGTERM that cause graceful
process termination, either because a user has ``hung up'', or
by user or program request; and SIGKILL, a more powerful termination
signal which a process cannot catch or ignore.
Other software signals (SIGALRM, SIGVTALRM, SIGPROF)
indicate the expiration of interval timers.
.PP
A process can request notification via a SIGIO signal
when input or output is possible
on a descriptor, or when a \fInon-blocking\fP operation completes.
A process may request to receive a SIGURG signal when an
urgent condition arises. 
.PP
A process may be \fIstopped\fP by a signal sent to it or the members
of its process group.  The SIGSTOP signal is a powerful stop
signal, because it cannot be caught.  Other stop signals
SIGTSTP, SIGTTIN, and SIGTTOU are used when a user request, input
request, or output request respectively is the reason the process
is being stopped.  A SIGCONT signal is sent to a process when it is
continued from a stopped state.
Processes may receive notification with a SIGCHLD signal when
a child process changes state, either by stopping or by terminating.
.PP
Exceeding resource limits may cause signals to be generated.
SIGXCPU occurs when a process nears its CPU time limit and SIGXFSZ
warns that the limit on file size creation has been reached.
.NH 3
Signal handlers
.PP
A process has a handler associated with each signal that
controls the way the signal is delivered.
The call
.DS
#include <signal.h>

._f
struct sigvec {
	int	(*sv_handler)();
	int	sv_mask;
	int	sv_onstack;
};

sigvec(signo, sv, osv)
int signo; struct sigvec *sv; result struct sigvec *osv;
.DE
assigns interrupt handler address \fIsv_handler\fP to signal \fIsigno\fP.
Each handler address
specifies either an interrupt routine for the signal, that the
signal is to be ignored,
or that a default action (usually process termination) is to occur
if the signal occurs.
The constants
SIG_IGN and SIG_DEF used as values for \fIsv_handler\fP
cause ignoring or defaulting of a condition.
The \fIsv_mask\fP and \fIsv_onstack\fP values specify the
signal mask to be used when the handler is invoked and
whether the handler should operate on the normal run-time
stack or a special signal stack (see below).  If \fIosv\fP
is non-zero, the previous signal vector is returned.
.PP
When a signal condition arises for a process, the signal
is added to a set of signals pending for the process.
If the signal is not currently \fIblocked\fP by the process
then it will be delivered.  The process of signal delivery
adds the signal to be delivered and those signals
specified in the associated signal
handler's \fIsv_mask\fP to a set of those \fImasked\fP
for the process, saves the current process context,
and places the process in the context of the signal
handling routine.  The call is arranged so that if the signal
handling routine exits normally the signal mask will be restored
and the process will resume execution in the original context.
If the process wishes to resume in a different context, then
it must arrange to restore the signal mask itself.
.PP
The mask of \fIblocked\fP signals is independent of handlers for
signals.  It prevents signals from being delivered much as a
raised hardware interrupt priority level prevents hardware interrupts.
Preventing an interrupt from occurring by changing the handler is analogous to
disabling a device from further interrupts.
.PP
The signal handling routine \fIsv_handler\fP is called by a C call
of the form
.DS
(*sv_handler)(signo, code, scp);
int signo; long code; struct sigcontext *scp;
.DE
The \fIsigno\fP gives the number of the signal that occurred, and
the \fIcode\fP, a word of information supplied by the hardware.
The \fIscp\fP parameter is a pointer to a machine-dependent
structure containing the information for restoring the
context before the signal.
.NH 3
Sending signals
.PP
A process can send a signal to another process or group of processes
with the calls:
.DS
kill(pid, signo)
int pid, signo;

killpgrp(pgrp, signo)
int pgrp, signo;
.DE
Unless the process sending the signal is privileged,
it and the process receiving the signal must have the same effective user id.
.PP
Signals are also sent implicitly from a terminal device to the
process group associated with the terminal when certain input characters
are typed.
.NH 3
Protecting critical sections
.PP
To block a section of code against one or more signals, a \fIsigblock\fP
call may be used to add a set of signals to the existing mask, returning
the old mask:
.DS
oldmask = sigblock(mask);
result long oldmask; long mask;
.DE
The old mask can then be restored later with \fIsigsetmask\fP\|,
.DS
oldmask = sigsetmask(mask);
result long oldmask; long mask;
.DE
The \fIsigblock\fP call can be used to read the current mask
by specifying an empty \fImask\fP\|.
.PP
It is possible to check conditions with some signals blocked,
and then to pause waiting for a signal and restoring the mask, by using:
.DS
sigpause(mask);
long mask;
.DE
.NH 3
Signal stacks
.PP
Applications that maintain complex or fixed size stacks can use
the call
.DS
._f
struct sigstack {
	caddr_t	ss_sp;
	int	ss_onstack;
};

sigstack(ss, oss)
struct sigstack *ss; result struct sigstack *oss;
.DE
to provide the system with a stack based at \fIss_sp\fP for delivery
of signals. The value \fIss_onstack\fP indicates whether the
process is currently on the signal stack,
a notion maintained in software by the system.
.PP
When a signal is to be delivered, the system checks whether
the process is on a signal stack.  If not, then the process is switched
to the signal stack for delivery, with the return from the signal
arranged to restore the previous stack.
.PP
If the process wishes to take a non-local exit from the signal routine,
or run code from the signal stack that uses a different stack,
a \fIsigstack\fP call should be used to reset the signal stack.
