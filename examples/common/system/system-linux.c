/**
********************************************************************************
\file   system-linux.c

\brief  Sytem specific functions for Linux

The file implements the system specific funtions for Linux used by the
openPOWERLINK demo applications.

\ingroup module_app_common
*******************************************************************************/

/*------------------------------------------------------------------------------
Copyright (c) 2013, Bernecker+Rainer Industrie-Elektronik Ges.m.b.H. (B&R)
Copyright (c) 2013, SYSTEC electronic GmbH
Copyright (c) 2013, Kalycito Infotech Private Ltd.All rights reserved.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holders nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
------------------------------------------------------------------------------*/

//------------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------------
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include <Epl.h>

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------
#define SET_CPU_AFFINITY
#define MAIN_THREAD_PRIORITY            20

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------
static BOOL    fTermSignalReceived_g = FALSE;

//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

#if defined(CONFIG_USE_SYNCTHREAD)
//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------
typedef struct
{
    tEplSyncCb      pfnSyncCb;
} tSyncThreadInstance;

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static pthread_t                syncThreadId_l;
static tSyncThreadInstance      syncThreadInstance_l;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
void *powerlinkSyncThread(void * arg);
#endif

void system_handleTermSignal(int signum);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Initialize system

The function initializes important stuff on the system for openPOWERLINK to
work correctly.

\ingroup module_app_common
*/
//------------------------------------------------------------------------------
int initSystem(void)
{
    struct sched_param          schedParam;

    /* adjust process priority */
    if (nice (-20) == -1)         // push nice level in case we have no RTPreempt
    {
        EPL_DBGLVL_ERROR_TRACE("%s() couldn't set nice value! (%s)\n", __func__, strerror(errno));
    }
    schedParam.__sched_priority = MAIN_THREAD_PRIORITY;
    if (pthread_setschedparam(pthread_self(), SCHED_RR, &schedParam) != 0)
    {
        EPL_DBGLVL_ERROR_TRACE("%s() couldn't set thread scheduling parameters! %d\n",
                __func__, schedParam.__sched_priority);
    }

    // Register termination handler for signals with termination semantics
    struct sigaction new_action;

    new_action.sa_handler = system_handleTermSignal;
    (void) sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    (void) sigaction(SIGINT,  &new_action, NULL);    // Sent via CTRL-C
    (void) sigaction(SIGTERM, &new_action, NULL);    // Generic signal used to cause program termination.
    (void) sigaction(SIGQUIT, &new_action, NULL);    // Terminate because of abnormal condition

    /* Initialize target specific stuff */
    target_init();

#ifdef SET_CPU_AFFINITY
    {
        /* binds all openPOWERLINK threads to the second CPU core */
        cpu_set_t                   affinity;

        CPU_ZERO(&affinity);
        CPU_SET(1, &affinity);
        sched_setaffinity(0, sizeof(cpu_set_t), &affinity);
    }
#endif

    /* Enabling ftrace for debugging */
    FTRACE_OPEN();
    FTRACE_ENABLE(TRUE);

    return 0;
}

//------------------------------------------------------------------------------
/**
\brief  Shutdown system

The function shuts-down the system.

\ingroup module_app_common
*/
//------------------------------------------------------------------------------
void shutdownSystem(void)
{
    /* Disable ftrace debugging */
    FTRACE_ENABLE(FALSE);
}

#if defined(CONFIG_USE_SYNCTHREAD)
//------------------------------------------------------------------------------
/**
\brief  Start synchronous data thread

The function starts the thread used for synchronous data handling.

\param  pfnSync_p           Pointer to sync callback function

\ingroup module_app_common
*/
//------------------------------------------------------------------------------
void startSyncThread(tEplSyncCb pfnSync_p)
{
    syncThreadInstance_l.pfnSyncCb = pfnSync_p;

    // create sync thread
    if (pthread_create(&syncThreadId_l, NULL, &powerlinkSyncThread,
                       &syncThreadInstance_l) != 0)
    {
        return;
    }
}
#endif

//------------------------------------------------------------------------------
/**
\brief  Return true if a termination signal has been received

The function can be used by the application to react on termination request.

\ingroup module_app_common
*/
//------------------------------------------------------------------------------
BOOL system_getTermSignalState(void)
{
    return fTermSignalReceived_g;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{

#if defined(CONFIG_USE_SYNCTHREAD)
//------------------------------------------------------------------------------
/**
\brief  Synchronous application thread

This function implements the synchronous application thread.

\param  arg             Needed for thread interface not used
*/
//------------------------------------------------------------------------------
void *powerlinkSyncThread(void* arg)
{
    tSyncThreadInstance*     pSyncThreadInstance = (tSyncThreadInstance*)arg;

    while (1)
    {
        pSyncThreadInstance->pfnSyncCb();
    }
    return NULL;
}
#endif

//------------------------------------------------------------------------------
/**
\brief  Handle termination requests

This functions can be used to react on signals with termination semantics,
and remembers in a flag that the user or the system asked to program to shut down.
The application can than check this flag.
*/
//------------------------------------------------------------------------------
void system_handleTermSignal(int signum)
{
    switch (signum)
    {
        case SIGINT:    // Signals with termination semantics
        case SIGTERM:   // trigger a flag change
        case SIGQUIT:   fTermSignalReceived_g = TRUE;
                        break;

        default:        // All other signals are ignored by this handler
                        break;
    }
}

///\}







