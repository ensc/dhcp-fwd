/***
  Copyright 2010 Lennart Poettering

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
***/

#include <sys/types.h>
#include <inttypes.h>

#ifndef _sd_printf_attr_
#  if __GNUC__ >= 4
#    define _sd_printf_attr_(a,b) __attribute__ ((format (printf, a, b)))
#  else
#    define _sd_printf_attr_(a,b)
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif

#ifdef USE_SD_NOTIFY
int sd_notify_supported(void);
#else
inline static int sd_notify_supported(void)
{
	return 0;
}
#endif
/*
  Informs systemd about changed daemon state. This takes a number of
  newline separated environment-style variable assignments in a
  string. The following variables are known:

     READY=1      Tells systemd that daemon startup is finished (only
                  relevant for services of Type=notify). The passed
                  argument is a boolean "1" or "0". Since there is
                  little value in signaling non-readiness the only
                  value daemons should send is "READY=1".

     STATUS=...   Passes a single-line status string back to systemd
                  that describes the daemon state. This is free-from
                  and can be used for various purposes: general state
                  feedback, fsck-like programs could pass completion
                  percentages and failing programs could pass a human
                  readable error message. Example: "STATUS=Completed
                  66% of file system check..."

     ERRNO=...    If a daemon fails, the errno-style error code,
                  formatted as string. Example: "ERRNO=2" for ENOENT.

     BUSERROR=... If a daemon fails, the D-Bus error-style error
                  code. Example: "BUSERROR=org.freedesktop.DBus.Error.TimedOut"

     MAINPID=...  The main pid of a daemon, in case systemd did not
                  fork off the process itself. Example: "MAINPID=4711"

     WATCHDOG=1   Tells systemd to update the watchdog timestamp.
                  Services using this feature should do this in
                  regular intervals. A watchdog framework can use the
                  timestamps to detect failed services.

  Daemons can choose to send additional variables. However, it is
  recommended to prefix variable names not listed above with X_.

  Returns a negative errno-style error code on failure. Returns > 0
  if systemd could be notified, 0 if it couldn't possibly because
  systemd is not running.

  Example: When a daemon finished starting up, it could issue this
  call to notify systemd about it:

     sd_notify(0, "READY=1");

  See sd_notifyf() for more complete examples.

  See sd_notify(3) for more information.
*/

#ifdef USE_SD_NOTIFY
int sd_notify(int unset_environment, const char *state);
#else
inline static void sd_notify(int unset_environment, const char *state)
{
	(void)unset_environment;
	(void)state;
	return;
}
#endif

#if 0
{
#endif
#ifdef __cplusplus
}
#endif
