/************************************************************************************

Filename    :   Logging_Tools.cpp
Content     :   Tools for Logging
Created     :   Oct 26, 2015
Authors     :   Chris Taylor

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.3 (the "License"); 
you may not use the Oculus VR Rift SDK except in compliance with the License, 
which is provided at the time of installation or download, or which 
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.3 

Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/

#ifdef _MSC_VER
    #pragma warning(disable: 4530) // C++ exception handler used, but unwind semantics are not enabled
#endif

#include "Logging/Logging_Tools.h"

#include <assert.h>
#include <time.h>

#if defined(__APPLE__)
    #include <unistd.h>
    #include <sys/sysctl.h>
    #include <libproc.h>
    #include <locale>
#endif


namespace ovrlog {


//-----------------------------------------------------------------------------
// Terminator

Terminator::Terminator() :
    Terminated(false),
    TerminateEvent()
{
}

Terminator::~Terminator()
{
}

bool Terminator::Initialize()
{
    Terminated = false;

    #if defined(_WIN32)
        if (TerminateEvent.IsValid())
        {
            ::ResetEvent(TerminateEvent.Get());
            return true;
        }

        TerminateEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    #else
        // To do: Implement this.
        fprintf(stderr, __FILE__ "[%s] Not implemented.\n", __func__);
    #endif
    
    return TerminateEvent.IsValid();
}

void Terminator::Terminate()
{
    Terminated = true;

    if (TerminateEvent.IsValid())
    {
        #if defined(_WIN32)
            ::SetEvent(TerminateEvent.Get());
        #else
            // To do: Implement this.
            assert(false);
        #endif
    }
}

// Returns true if the event signaled and false on termination.
bool Terminator::WaitOn(OvrLogHandle hEvent, uint32_t timeoutMsec)
{
    if (Terminated || !TerminateEvent.IsValid())
        return false;

    #if defined(_WIN32)
        HANDLE events[2] = { hEvent, TerminateEvent.Get() };

        DWORD result = ::WaitForMultipleObjects(2, events, FALSE, timeoutMsec);

        if (Terminated)
            return false;

        if (result == WAIT_TIMEOUT)
            return false;
        if (result == WAIT_OBJECT_0)
            return true;
    #else
        // To do: Implement this.
        (void)hEvent;
        (void)timeoutMsec;
        assert(false);
    #endif
    
    return false;
}

// Returns true if the sleep interval exceeded or false on termination.
bool Terminator::WaitSleep(int milliseconds)
{
    if (Terminated || !TerminateEvent.IsValid())
        return false;

    #if defined(_WIN32)
        ::WaitForSingleObject(TerminateEvent.Get(), milliseconds); // Ignore return value
    #else
        // To do: Implement this.
        (void)milliseconds;
        assert(false);
    #endif

    
    return !Terminated;
}


//-----------------------------------------------------------------------------
// Lock

Lock::Lock() :
#if defined(_WIN32)
    cs{}
#else
	m()
#endif
{
#if defined(_WIN32)
    static const DWORD kSpinCount = 1000;
    ::InitializeCriticalSectionAndSpinCount(&cs, kSpinCount);
#endif
}

Lock::~Lock()
{
#if defined(_WIN32)
    ::DeleteCriticalSection(&cs);
#endif
}

bool Lock::TryEnter()
{
#if defined(_WIN32)
    return ::TryEnterCriticalSection(&cs) != FALSE;
#else
	return m.try_lock();
#endif
}

void Lock::Enter()
{
#if defined(_WIN32)
    ::EnterCriticalSection(&cs);
#else
	m.lock();
#endif
}

void Lock::Leave()
{
#if defined(_WIN32)
    ::LeaveCriticalSection(&cs);
#else
	m.unlock();
#endif
}


//-----------------------------------------------------------------------------
// Locker

Locker::Locker(Lock* lock) :
    TheLock(lock)
{
    if (TheLock)
        TheLock->Enter();
}

Locker::Locker(Lock& lock) :
    TheLock(&lock)
{
    if (TheLock)
        TheLock->Enter();
}

Locker::~Locker()
{
    Clear();
}

bool Locker::TrySet(Lock* lock)
{
    Clear();

    if (!lock || !lock->TryEnter())
        return false;

    TheLock = lock;
    return true;
}

bool Locker::TrySet(Lock& lock)
{
    return TrySet(&lock);
}

void Locker::Set(Lock* lock)
{
    Clear();

    if (lock)
    {
        lock->Enter();
        TheLock = lock;
    }
}

void Locker::Set(Lock& lock)
{
    return Set(&lock);
}

void Locker::Clear()
{
    if (TheLock)
    {
        TheLock->Leave();
        TheLock = nullptr;
    }
}


//-----------------------------------------------------------------------------
// AutoHandle

AutoHandle::AutoHandle(OvrLogHandle handle) :
    TheHandle(handle)
{
}

AutoHandle::~AutoHandle()
{
    Clear();
}

void AutoHandle::operator=(OvrLogHandle handle)
{
    Clear();
    TheHandle = handle;
}

void AutoHandle::Clear()
{
    if (TheHandle)
    {
        #if defined(_WIN32)
            ::CloseHandle(TheHandle);
        #else
            // To do: Implement this.
            assert(false);
        #endif
        
        TheHandle = nullptr;
    }
}


//-----------------------------------------------------------------------------
// Tools

bool IsDebuggerAttached()
{
    #if defined(_WIN32)
        return ::IsDebuggerPresent() != FALSE;
    #elif defined(__APPLE__)
        int               mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
        struct kinfo_proc info;
        size_t            size = sizeof(info);

        info.kp_proc.p_flag = 0;
        sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, nullptr, 0);

        return ((info.kp_proc.p_flag & P_TRACED) != 0);

    #elif defined(PT_TRACE_ME) && !defined(__android__)
        return (ptrace(PT_TRACE_ME, 0, 1, 0) < 0);
    #else
        // We have some platform-specific code elsewhere.
        #error "IsDebuggerAttached: Need to get platform-specific code for this."
    #endif
}

} // namespace ovrlog

#ifdef OVR_STRINGIZE
#error "This code must remain independent of LibOVR"
#endif
