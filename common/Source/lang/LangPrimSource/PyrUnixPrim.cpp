/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
/*

Primitives for Unix.

*/

#include <cstring>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "PyrPrimitive.h"
#include "PyrObject.h"
#include "PyrKernel.h"
#include "PyrSched.h"
#include "VMGlobals.h"
#include "GC.h"
#include "SC_RGen.h"
#include "SC_DirUtils.h"
#include "sc_popen.h"

#ifdef SC_WIN32
#include "SC_Win32Utils.h"
#else
#include <libgen.h>
#endif

extern bool compiledOK;
PyrSymbol* s_unixCmdAction;

int prString_System(struct VMGlobals *g, int numArgsPushed);
int prString_System(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a = g->sp;

	char cmdline[1024];
	int err = slotStrVal(a, cmdline, 1023);
	if (err) return err;

	int res = system(cmdline);
	SetInt(a, res);

	return errNone;
}

int prString_Basename(struct VMGlobals *g, int numArgsPushed);
int prString_Basename(struct VMGlobals *g, int numArgsPushed)
{
        PyrSlot *a = g->sp;

        char path[PATH_MAX];
        int err = slotStrVal(a, path, PATH_MAX);
        if (err) return err;

		char *basename0 = basename(path);

        int size = strlen(basename0);
        PyrString *strobj = newPyrStringN(g->gc, size, 0, true);
        memcpy(strobj->s, basename0, size);

        SetObject(a, strobj);

        return errNone;
}

int prString_Dirname(struct VMGlobals *g, int numArgsPushed);
int prString_Dirname(struct VMGlobals *g, int numArgsPushed)
{
        PyrSlot *a = g->sp;

        char path[PATH_MAX];
        int err = slotStrVal(a, path, PATH_MAX);
        if (err) return err;

        char *dirname0 = dirname(path);

        int size = strlen(dirname0);
        PyrString *strobj = newPyrStringN(g->gc, size, 0, true);
        memcpy(strobj->s, dirname0, size);

        SetObject(a, strobj);

        return errNone;
}

struct sc_process {
	pid_t pid;
	FILE *stream;
	bool postOutput;
};

void* string_popen_thread_func(void *data);
void* string_popen_thread_func(void *data)
{
	struct sc_process *process = (struct sc_process *)data;
	FILE *stream = process->stream;
	pid_t pid = process->pid;
	char buf[1024];

	while (process->postOutput) {
		char *string = fgets(buf, 1024, stream);
		if (!string) break;
		postText(string, strlen(string));
	}

	int res;
	res = sc_pclose(stream, pid);
	res = WEXITSTATUS(res);

	if(process->postOutput)
		post("RESULT = %d\n", res);

	free(process);

    pthread_mutex_lock (&gLangMutex);
    if(compiledOK) {
		VMGlobals *g = gMainVMGlobals;
		g->canCallOS = true;
		++g->sp;  SetObject(g->sp, class_string);
		++g->sp; SetInt(g->sp, res);
		++g->sp; SetInt(g->sp, pid);
		runInterpreter(g, s_unixCmdAction, 3);
		g->canCallOS = false;
	}
    pthread_mutex_unlock (&gLangMutex);

	return 0;
}

int prString_POpen(struct VMGlobals *g, int numArgsPushed);
int prString_POpen(struct VMGlobals *g, int numArgsPushed)
{
	struct sc_process *process;
	PyrSlot *a = g->sp - 1;
	PyrSlot *b = g->sp;
	int err;

	if (!isKindOfSlot(a, class_string)) return errWrongType;

	char *cmdline = (char*)malloc(slotRawObject(a)->size + 1);
	err = slotStrVal(a, cmdline, slotRawObject(a)->size + 1);
	if(err) {
		free(cmdline);
		return errFailed;
	}

#ifdef SC_IPHONE
	SetInt(a, 0);
	return errNone;
#endif

	process = (struct sc_process *)malloc(sizeof(struct sc_process));
	process->stream = sc_popen(cmdline, &process->pid, "r");
	setvbuf(process->stream, 0, _IONBF, 0);

	process->postOutput = IsTrue(b);

	free(cmdline);

	if(process->stream == NULL) {
		free(process);
		return errFailed;
	}

	pthread_t thread;
	pthread_create(&thread, NULL, string_popen_thread_func, (void*)process);
	pthread_detach(thread);

	SetInt(a, process->pid);
	return errNone;
}

int prPidRunning(VMGlobals *g, int numArgsPushed);
int prPidRunning(VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a;

	a = g->sp;

#ifdef SC_WIN32
	HANDLE handle;

	handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, slotRawInt(a));
	if(handle) {
		unsigned long exitCode;

		if(GetExitCodeProcess(handle, &exitCode) == 0)
			SetFalse(a);
		else if(exitCode == STILL_ACTIVE)
			SetTrue(a);

		CloseHandle(handle);
	}
	else
		SetFalse(a);
#else
	if(kill(slotRawInt(a), 0) == 0)
		SetTrue(a);
	else
		SetFalse(a);
#endif

	return errNone;
}

int prUnix_Errno(struct VMGlobals *g, int numArgsPushed);
int prUnix_Errno(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a = g->sp;

	SetInt(a, errno);

	return errNone;
}

#include <time.h>

#ifndef SC_WIN32
#include <sys/time.h>
#endif

double bootSeconds();

int prLocalTime(struct VMGlobals *g, int numArgsPushed);
int prLocalTime(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a = g->sp;
	PyrSlot *slots = slotRawObject(a)->slots;

	struct timeval tv;
	gettimeofday(&tv, 0);

	struct tm* tm = localtime((const time_t*)&tv.tv_sec);

	SetInt(slots+0, tm->tm_year + 1900);
	SetInt(slots+1, tm->tm_mon + 1); // 0 based month ??
	SetInt(slots+2, tm->tm_mday);
	SetInt(slots+3, tm->tm_hour);
	SetInt(slots+4, tm->tm_min);
	SetInt(slots+5, tm->tm_sec);
	SetInt(slots+6, tm->tm_wday);
	SetFloat(slots+7, tv.tv_sec + 1e-6 * tv.tv_usec);
	SetFloat(slots+8, bootSeconds());

	return errNone;
}

int prGMTime(struct VMGlobals *g, int numArgsPushed);
int prGMTime(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a = g->sp;
	PyrSlot *slots = slotRawObject(a)->slots;

	struct timeval tv;
	gettimeofday(&tv, 0);

	struct tm* tm = gmtime((const time_t*)&tv.tv_sec);

	SetInt(slots+0, tm->tm_year + 1900);
	SetInt(slots+1, tm->tm_mon + 1);
	SetInt(slots+2, tm->tm_mday);
	SetInt(slots+3, tm->tm_hour);
	SetInt(slots+4, tm->tm_min);
	SetInt(slots+5, tm->tm_sec);
	SetInt(slots+6, tm->tm_wday);
	SetFloat(slots+7, tv.tv_sec + 1e-6 * tv.tv_usec);
	SetFloat(slots+8, bootSeconds());

	return errNone;
}

int prAscTime(struct VMGlobals *g, int numArgsPushed);
int prAscTime(struct VMGlobals *g, int numArgsPushed)
{
        PyrSlot *a = g->sp;
        PyrSlot *slots = slotRawObject(a)->slots;

        if (IsNil(slots + 0)) {
          SetNil(a);
          return errNone;
        }

        struct tm tm0;

        if (slotIntVal(slots+0, &tm0.tm_year)) return errWrongType;
        tm0.tm_year -= 1900;
        if (slotIntVal(slots+1, &tm0.tm_mon)) return errWrongType;
		tm0.tm_mon -- ;
        if (slotIntVal(slots+2, &tm0.tm_mday)) return errWrongType;
        if (slotIntVal(slots+3, &tm0.tm_hour)) return errWrongType;
        if (slotIntVal(slots+4, &tm0.tm_min)) return errWrongType;
        if (slotIntVal(slots+5, &tm0.tm_sec)) return errWrongType;
        if (slotIntVal(slots+6, &tm0.tm_wday)) return errWrongType;

        const char *text = asctime(&tm0);

        int size = strlen(text) - 1; // Discard trailing newline
        PyrString *strobj = newPyrStringN(g->gc, size, 0, true);
        memcpy(strobj->s, text, size);

        SetObject(a, strobj);

        return errNone;
}

int prStrFTime(struct VMGlobals *g, int numArgsPushed);
int prStrFTime(struct VMGlobals *g, int numArgsPushed)
{
        PyrSlot *a = g->sp - 1;
        PyrSlot *b = g->sp;

        PyrSlot *slots = slotRawObject(a)->slots;

        if (IsNil(slots + 0)) {
          SetNil(a);
          return errNone;
        }

        struct tm tm0;

        if (slotIntVal(slots+0, &tm0.tm_year)) return errWrongType;
        tm0.tm_year -= 1900;
        if (slotIntVal(slots+1, &tm0.tm_mon)) return errWrongType;
		tm0.tm_mon --;
        if (slotIntVal(slots+2, &tm0.tm_mday)) return errWrongType;
        if (slotIntVal(slots+3, &tm0.tm_hour)) return errWrongType;
        if (slotIntVal(slots+4, &tm0.tm_min)) return errWrongType;
        if (slotIntVal(slots+5, &tm0.tm_sec)) return errWrongType;
        if (slotIntVal(slots+6, &tm0.tm_wday)) return errWrongType;

        char format[1024];
        if (slotStrVal(b, format, 1024)) return errWrongType;

        char buffer[1024];
        if (strftime(buffer, 1024, format, &tm0) != 0) {
          int size = strlen(buffer);
          PyrString *strobj = newPyrStringN(g->gc, size, 0, true);
          memcpy(strobj->s, buffer, size);

          SetObject(a, strobj);
        } else {
          error("could not convert the date to string with the give format");
          return errFailed;
        }
        return errNone;
}

int32 timeseed();

int prTimeSeed(struct VMGlobals *g, int numArgsPushed);
int prTimeSeed(struct VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a = g->sp;
	SetInt(a, timeseed());
	return errNone;
}

int prGetPid(VMGlobals *g, int numArgsPushed);
int prGetPid(VMGlobals *g, int numArgsPushed)
{
	PyrSlot *a = g->sp;
	SetInt(a,
#ifndef SC_WIN32
		getpid()
#else
		GetCurrentThreadId()
#endif
		);
	return errNone;
}


void initUnixPrimitives();
void initUnixPrimitives()
{
	int base, index = 0;

	base = nextPrimitiveIndex();

    s_unixCmdAction = getsym("doUnixCmdAction");

	definePrimitive(base, index++, "_String_System", prString_System, 1, 0);
	definePrimitive(base, index++, "_String_Basename", prString_Basename, 1, 0);
	definePrimitive(base, index++, "_String_Dirname", prString_Dirname, 1, 0);
	definePrimitive(base, index++, "_String_POpen", prString_POpen, 2, 0);
	definePrimitive(base, index++, "_Unix_Errno", prUnix_Errno, 1, 0);
	definePrimitive(base, index++, "_LocalTime", prLocalTime, 1, 0);
	definePrimitive(base, index++, "_GMTime", prGMTime, 1, 0);
	definePrimitive(base, index++, "_AscTime", prAscTime, 1, 0);
	definePrimitive(base, index++, "_prStrFTime", prStrFTime, 2, 0);
	definePrimitive(base, index++, "_TimeSeed", prTimeSeed, 1, 0);
	definePrimitive(base, index++, "_PidRunning", prPidRunning, 1, 0);
	definePrimitive(base, index++, "_GetPid", prGetPid, 1, 0);
}
