/**
********************************************************************************
\file   timer-linuxuser.c

\brief  Implementation of user timer module for Linux userspace

This file contains the implementation of the user timer module for Linux
userspace. This implementation uses the posix timer interface.

\ingroup module_timeru
*******************************************************************************/

/*------------------------------------------------------------------------------
Copyright (c) 2013, Bernecker+Rainer Industrie-Elektronik Ges.m.b.H. (B&R)
Copyright (c) 2013, SYSTEC electronic GmbH
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
#include <user/timeru.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <semaphore.h>

#include <signal.h>

//============================================================================//
//            G L O B A L   D E F I N I T I O N S                             //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// module global vars
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// global function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P R I V A T E   D E F I N I T I O N S                           //
//============================================================================//

//------------------------------------------------------------------------------
// const defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// local types
//------------------------------------------------------------------------------
typedef struct sTimeruData tTimeruData;

struct sTimeruData
{
    timer_t             timer;
    tTimerArg           timerArgument;
    tTimeruData         *pNextTimer;
    tTimeruData         *pPrevTimer;
};

typedef struct
{
    pthread_t           processThread;
    pthread_mutex_t     mutex;
    tTimeruData*        pFirstTimer;
    tTimeruData*        pLastTimer;
    tTimeruData*        pCurrentTimer;
} tTimeruInstance;

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static tTimeruInstance timeruInstance_g;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static void cbTimer(ULONG parameter_p);
static void* processThread(void *pArgument_p);
static void addTimer(tTimeruData *pData_p);
static void removeTimer(tTimeruData *pData_p);
static void resetTimerList(void);
static tTimeruData* getNextTimer(void);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Initialize user timers

The function initializes the user timer module.

\return The function returns a tOplkError error code.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
tOplkError timeru_init(void)
{
    return timeru_addInstance();
}

//------------------------------------------------------------------------------
/**
\brief  Add user timer instance

The function adds a user timer instance.

\return The function returns a tOplkError error code.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
tOplkError timeru_addInstance(void)
{
    struct sched_param          schedParam;
    INT                         retVal;

    // reset instance structure
    timeruInstance_g.processThread = 0;
    timeruInstance_g.pFirstTimer = NULL;
    timeruInstance_g.pLastTimer = NULL;

    if (pthread_mutex_init(&timeruInstance_g.mutex, NULL) != 0)
    {
        DEBUG_LVL_ERROR_TRACE("%s() couldn't init mutex!\n", __func__);
        return kEplNoResource;
    }

    if ((retVal = pthread_create(&timeruInstance_g.processThread, NULL,
                                 processThread,  &timeruInstance_g)) != 0)
    {
        DEBUG_LVL_ERROR_TRACE("%s() couldn't create timer thread! (%d)\n",
                                __func__, retVal);
        pthread_mutex_destroy(&timeruInstance_g.mutex);
        return kEplNoResource;
    }

    schedParam.__sched_priority = EPL_THREAD_PRIORITY_LOW;
    if (pthread_setschedparam(timeruInstance_g.processThread, SCHED_RR,
                              &schedParam) != 0)
    {
        DEBUG_LVL_ERROR_TRACE("%s() couldn't set thread scheduling parameters!\n",
                                __func__);
    }

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Delete user timer instance

The function deletes a user timer instance.

\return The function returns a tOplkError error code.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
tOplkError timeru_delInstance(void)
{
    tTimeruData*     pTimer;

    /* cancel thread */
    pthread_cancel(timeruInstance_g.processThread);
    DEBUG_LVL_TIMERU_TRACE("%s() Waiting for thread to exit...\n", __func__);

    /* wait for thread to terminate */
    pthread_join(timeruInstance_g.processThread, NULL);
    DEBUG_LVL_TIMERU_TRACE("%s()Thread exited\n", __func__);

    /* free up timer list */
    resetTimerList();
    while ((pTimer = getNextTimer()) != NULL)
    {
        removeTimer(pTimer);
        EPL_FREE(pTimer);
    }

    pthread_mutex_destroy(&timeruInstance_g.mutex);

    timeruInstance_g.pFirstTimer = NULL;
    timeruInstance_g.pLastTimer = NULL;

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  User timer process function

This function must be called repeatedly from within the application. It checks
whether a timer has expired.

\note The function is not used in the Linux userspace implementation!

\return The function returns a tOplkError error code.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
tOplkError timeru_process(void)
{
    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Create and set a timer

This function creates a timer, sets up the timeout and saves the
corresponding timer handle.

\param  pTimerHdl_p     Pointer to store the timer handle.
\param  timeInMs_p      Timeout in milliseconds.
\param  argument_p      User definable argument for timer.

\return The function returns a tOplkError error code.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
tOplkError timeru_setTimer(tTimerHdl* pTimerHdl_p, ULONG timeInMs_p, tTimerArg argument_p)
{
    tTimeruData*        pData;
    struct itimerspec   relTime;
    struct itimerspec   curTime;
    struct sigevent     sev;

    if(pTimerHdl_p == NULL)
        return kEplTimerInvalidHandle;

    pData = (tTimeruData*) EPL_MALLOC(sizeof (tTimeruData));
    if (pData == NULL)
        return kEplNoResource;

    EPL_MEMCPY(&pData->timerArgument, &argument_p, sizeof(tTimerArg));

    addTimer(pData);

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &pData->timer;
    if (timer_create(CLOCK_MONOTONIC, &sev, &pData->timer) == -1)
    {
        DEBUG_LVL_ERROR_TRACE("%s() Error creating timer!\n", __func__);
        EPL_FREE(pData);
        return kEplNoResource;
    }

    if (timeInMs_p >= 1000)
    {
        relTime.it_value.tv_sec = (timeInMs_p / 1000);
        relTime.it_value.tv_nsec = (timeInMs_p % 1000) * 1000000;
    }
    else
    {
        relTime.it_value.tv_sec = 0;
        relTime.it_value.tv_nsec = timeInMs_p * 1000000;
    }

    /*DEBUG_LVL_TIMERU_TRACE("%s() Set timer: %p, timeInMs_p=%ld\n",
                             __func__, (void *)pData, timeInMs_p); */

    relTime.it_interval.tv_sec = 0;
    relTime.it_interval.tv_nsec = 0;

    if (timer_settime(pData->timer, 0, &relTime, &curTime) < 0)
    {
        DEBUG_LVL_ERROR_TRACE("%s() Error timer_settime!\n", __func__);
        return kEplTimerNoTimerCreated;
    }

    *pTimerHdl_p = (tTimerHdl) pData;
    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Modifies an existing timer

This function modifies an existing timer. If the timer was not yet created
it creates the timer and stores the new timer handle at \p pTimerHdl_p.

\param  pTimerHdl_p     Pointer to store the timer handle.
\param  timeInMs_p      Timeout in milliseconds.
\param  argument_p      User definable argument for timer.

\return The function returns a tOplkError error code.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
tOplkError timeru_modifyTimer(tTimerHdl* pTimerHdl_p, ULONG timeInMs_p, tTimerArg argument_p)
{
    tTimeruData*        pData;
    struct itimerspec   relTime, curTime;

    if(pTimerHdl_p == NULL)
        return kEplTimerInvalidHandle;

    // check handle itself, i.e. was the handle initialized before
    if (*pTimerHdl_p == 0)
    {
        return timeru_setTimer(pTimerHdl_p, timeInMs_p, argument_p);
    }
    pData = (tTimeruData*) *pTimerHdl_p;

    if (timeInMs_p >= 1000)
    {
        relTime.it_value.tv_sec = (timeInMs_p / 1000);
        relTime.it_value.tv_nsec = (timeInMs_p % 1000) * 1000000;
    }
    else
    {
        relTime.it_value.tv_sec = 0;
        relTime.it_value.tv_nsec = timeInMs_p * 1000000;
    }

    /* DEBUG_LVL_TIMERU_TRACE("%s() Modify timer:%08x timeInMs_p=%ld\n",
                             __func__, *pTimerHdl_p, timeInMs_p); */

    relTime.it_interval.tv_sec = 0;
    relTime.it_interval.tv_nsec = 0;
    if (timer_settime(pData->timer, 0, &relTime, &curTime) != 0)
    {
        DEBUG_LVL_ERROR_TRACE("%s() Error timer_settime!\n", __func__);
        return kEplTimerNoTimerCreated;
    }

    // copy the TimerArg after the timer is restarted,
    // so that a timer occurred immediately before timer_settime
    // won't use the new TimerArg and
    // therefore the old timer cannot be distinguished from the new one.
    // But if the new timer is too fast, it may get lost.
    EPL_MEMCPY(&pData->timerArgument, &argument_p, sizeof(tTimerArg));

    return kEplSuccessful;
}

//------------------------------------------------------------------------------
/**
\brief  Delete a timer

This function deletes an existing timer.

\param  pTimerHdl_p     Pointer to timer handle of timer to delete.

\return The function returns a tOplkError error code.
\retval kEplTimerInvalidHandle  If an invalid timer handle was specified.
\retval kEplSuccessful          If the timer is deleted.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
tOplkError timeru_deleteTimer(tTimerHdl* pTimerHdl_p)
{
    tTimeruData*        pData;

    if(pTimerHdl_p == NULL)
        return kEplTimerInvalidHandle;

    // check handle itself, i.e. was the handle initialized before
    if (*pTimerHdl_p == 0)
    {
        return kEplSuccessful;
    }
    pData = (tTimeruData*) *pTimerHdl_p;

    timer_delete (pData->timer);
    removeTimer(pData);
    EPL_FREE(pData);

    // uninitialize handle
    *pTimerHdl_p = 0;
    return kEplSuccessful;

}

//------------------------------------------------------------------------------
/**
\brief  Check for an active timer

This function checks if a timer is active (is running).

\param  timerHdl_p     Handle of timer to check.

\return The function returns TRUE if the timer is active, otherwise FALSE.

\ingroup module_timeru
*/
//------------------------------------------------------------------------------
BOOL timeru_isActive(tTimerHdl timerHdl_p)
{
    tTimeruData*        pData;
    struct itimerspec   remaining;

    // check handle itself, i.e. was the handle initialized before
    if (timerHdl_p == 0)
    {   // timer was not created yet, so it is not active
        return FALSE;
    }
    pData = (tTimeruData*) timerHdl_p;

    // check if timer is running
    timer_gettime(pData->timer, &remaining);

    if ((remaining.it_value.tv_sec == 0) && (remaining.it_value.tv_nsec == 0))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{

//------------------------------------------------------------------------------
/**
\brief  Timer thread function

This function implements the timer thread function which will be started as
thread and is responsible for processing expired timers.

\param  pArgument_p     Thread argument. Not used!

\return The function returns a thread exit value (always NULL)
*/
//------------------------------------------------------------------------------
static void* processThread(void *pArgument_p)
{
    tTimeruData*    pTimer;
    sigset_t        awaitedSignal;
    siginfo_t       signalInfo;

    UNUSED_PARAMETER(pArgument_p);

    DEBUG_LVL_TIMERU_TRACE("%s() ThreadId:%d\n", __func__, syscall(SYS_gettid));

    sigemptyset(&awaitedSignal);
    sigaddset(&awaitedSignal, SIGRTMIN);
    pthread_sigmask(SIG_BLOCK, &awaitedSignal, NULL);

    /* loop forever until thread will be canceled */
    while (1)
    {
        if (sigwaitinfo(&awaitedSignal, &signalInfo) > 0)
        {
            pTimer = (tTimeruData *)signalInfo.si_value.sival_ptr;
            /* call callback function of timer */
            cbTimer((ULONG)pTimer);
        }
    }

    DEBUG_LVL_TIMERU_TRACE("%s() Exiting!\n", __func__);
    return NULL;
}

//------------------------------------------------------------------------------
/**
\brief  Timer callback function

This function is registered if a timer is started and therefore will be called
by the timer when it expires.

\param  parameter_p     The user defined parameter supplied when starting the
                        timer.
*/
//------------------------------------------------------------------------------
static void cbTimer(ULONG parameter_p)
{
    tTimeruData*        pData;
    tEplEvent           event;
    tTimerEventArg      timerEventArg;

    pData = (tTimeruData*) parameter_p;

    // call event function
    timerEventArg.timerHdl = (tTimerHdl)pData;
    EPL_MEMCPY(&timerEventArg.m_Arg, &pData->timerArgument.m_Arg,
               sizeof(timerEventArg.m_Arg));

    event.m_EventSink = pData->timerArgument.eventSink;
    event.m_EventType = kEplEventTypeTimer;
    EPL_MEMSET(&event.m_NetTime, 0x00, sizeof(tEplNetTime));
    event.m_pArg = &timerEventArg;
    event.m_uiSize = sizeof(timerEventArg);

    eventu_postEvent(&event);
}

//------------------------------------------------------------------------------
/**
\brief  Add a timer to the timer list

This function adds a new timer to the timer list.

\param  pData_p         Pointer to the timer structure.
*/
//------------------------------------------------------------------------------
static void addTimer(tTimeruData *pData_p)
{
    tTimeruData          *pTimerData;

    pthread_mutex_lock(&timeruInstance_g.mutex);

    if (timeruInstance_g.pFirstTimer == NULL)
    {
        timeruInstance_g.pFirstTimer = pData_p;
        timeruInstance_g.pLastTimer = pData_p;

        pData_p->pPrevTimer = NULL;
        pData_p->pNextTimer = NULL;
    }
    else
    {
        pTimerData = timeruInstance_g.pLastTimer;
        pTimerData->pNextTimer = pData_p;
        pData_p->pPrevTimer = pTimerData;
        pData_p->pNextTimer = NULL;
        timeruInstance_g.pLastTimer = pData_p;
    }

    pthread_mutex_unlock(&timeruInstance_g.mutex);
}

//------------------------------------------------------------------------------
/**
\brief  Remove a timer from the timer list

This function removes a new timer from the timer list.

\param  pData_p         Pointer to the timer structure.
*/
//------------------------------------------------------------------------------
static void removeTimer(tTimeruData *pData_p)
{
    tTimeruData          *pTimerData;

    pthread_mutex_lock(&timeruInstance_g.mutex);

    if (pData_p->pPrevTimer == NULL)          // first one
    {
        timeruInstance_g.pFirstTimer = pData_p->pNextTimer;
        pTimerData = pData_p->pNextTimer;
        if (pTimerData != NULL)
        {
            pTimerData->pPrevTimer = NULL;
        }
    }
    else if (pData_p->pNextTimer == NULL)     // last one
    {
        timeruInstance_g.pLastTimer = pData_p->pPrevTimer;
        pTimerData = pData_p->pPrevTimer;
        pTimerData->pNextTimer = NULL;
    }
    else
    {
        pData_p->pPrevTimer->pNextTimer = pData_p->pNextTimer;
        pData_p->pNextTimer->pPrevTimer = pData_p->pPrevTimer;
    }

    pthread_mutex_unlock(&timeruInstance_g.mutex);
}

//------------------------------------------------------------------------------
/**
\brief  Reset the timer list

This function resets the timer list.
*/
//------------------------------------------------------------------------------
static void resetTimerList(void)
{
    timeruInstance_g.pCurrentTimer = timeruInstance_g.pFirstTimer;
}

//------------------------------------------------------------------------------
/**
\brief  Get next timer from the list

This function gets the next timer from the timer list.

\return     The function returns a pointer to the next timer in the timer list.
*/
//------------------------------------------------------------------------------
static tTimeruData* getNextTimer(void)
{
    tTimeruData*    pTimer;

    pTimer = timeruInstance_g.pCurrentTimer;
    if (pTimer != NULL)
    {
        timeruInstance_g.pCurrentTimer = pTimer->pNextTimer;
    }
    return pTimer;
}

///\}

