/**
********************************************************************************
\file   app.c

\brief  Generic application
*******************************************************************************/
/*------------------------------------------------------------------------------
Copyright (c) 2013, Bernecker+Rainer Industrie-Elektronik Ges.m.b.H. (B&R)
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
#include <Epl.h>
#include "app.h"
#include "local-types.h"

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

//------------------------------------------------------------------------------
// local vars
//------------------------------------------------------------------------------
static UINT8*                       pProcessImageIn_g;
static UINT8*                       pProcessImageOut_g;
static tEplApiProcessImageCopyJob   AppProcessImageCopyJob_g;
static BOOL                         fAppRun_l;

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------
static tEplKernel initProcessImage(size_t inSize_p, size_t outSize_p);

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Initialize the synchronous data application

The function initializes the synchronous data application

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
tEplKernel initApp(DWORD inSize_p, DWORD outSize_p)
{
    tEplKernel ret = kEplSuccessful;

    /*******************************************************/
    /* place initialization of application variables here! */


    /*******************************************************/
    ret = initProcessImage(inSize_p, outSize_p);

    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Shutdown the synchronous data application

The function shut's down the synchronous data application

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
void shutdownApp (void)
{
    EplApiProcessImageFree();

    EPL_FREE(pProcessImageIn_g);
    EPL_FREE(pProcessImageOut_g);
}


//------------------------------------------------------------------------------
/**
\brief  Run application code

The function sets up the run flag of the application and therefore controls
if the application runs.
*/
//------------------------------------------------------------------------------
void runApp(BOOL run_p)
{
    fAppRun_l = run_p;
}

//------------------------------------------------------------------------------
/**
\brief  Synchronous data handler

The function implements the synchronous data handler.

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
tEplKernel processSync(void)
{
    tEplKernel          ret = kEplSuccessful;

    ret = EplApiProcessImageExchange(&AppProcessImageCopyJob_g);
    if (ret != kEplSuccessful)
        return ret;

    /**************************************************************************/
    // read inputs from process image
    // e.g. nodeVar_l[0].input = AppProcessImageOut_g.CN1_M00_Digital_Input_8_Bit_Byte_1;
    /**************************************************************************/

    /**************************************************************************/
    // process your data
    /**************************************************************************/

    /**************************************************************************/
    // store your outputs into process image
    // AppProcessImageIn_g.CN1_M00_Digital_Ouput_8_Bit_Byte_1 = nodeVar_l[0].leds;
    /**************************************************************************/

    return ret;
}

//------------------------------------------------------------------------------
/**
\brief  Get input image

The function returns a pointer to the input process image.
*/
//------------------------------------------------------------------------------
char* getInputImage(void)
{
    return (char *)pProcessImageIn_g;
}

//------------------------------------------------------------------------------
/**
\brief  Get input image

The function returns a pointer to the input process image.
*/
//------------------------------------------------------------------------------
char* getOutputImage(void)
{
    return (char *)pProcessImageOut_g;
}

//------------------------------------------------------------------------------
/**
\brief  Get size of output image

The function returns the size of the output image
*/
//------------------------------------------------------------------------------
UINT32 getOutputSize(void)
{
    return 0;
}

//------------------------------------------------------------------------------
/**
\brief  Get size of output image

The function returns the size of the output image
*/
//------------------------------------------------------------------------------
UINT32 getInputSize(void)
{
    return 0;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{

//------------------------------------------------------------------------------
/**
\brief  Initialize process image

The function initializes the process image of the application.

\return The function returns a tEplKernel error code.
*/
//------------------------------------------------------------------------------
static tEplKernel initProcessImage(size_t inSize_p, size_t outSize_p)
{
    tEplKernel      ret = kEplSuccessful;

    printf("Initializing process image...\n");
    printf("Size of input process image: %ld\n", inSize_p);
    printf("Size of output process image: %ld\n", outSize_p);

    if ((pProcessImageIn_g = EPL_MALLOC(inSize_p)) == NULL)
        return kEplNoResource;
    EPL_MEMSET(pProcessImageIn_g, 0, inSize_p);

    if ((pProcessImageOut_g = EPL_MALLOC(outSize_p)) == NULL)
        return kEplNoResource;

    EPL_MEMSET(pProcessImageOut_g, 0, outSize_p);

    AppProcessImageCopyJob_g.m_fNonBlocking = FALSE;
    AppProcessImageCopyJob_g.m_uiPriority = 0;
    AppProcessImageCopyJob_g.m_In.m_pPart = pProcessImageIn_g;
    AppProcessImageCopyJob_g.m_In.m_uiOffset = 0;
    AppProcessImageCopyJob_g.m_In.m_uiSize = inSize_p;
    AppProcessImageCopyJob_g.m_Out.m_pPart = pProcessImageOut_g;
    AppProcessImageCopyJob_g.m_Out.m_uiOffset = 0;
    AppProcessImageCopyJob_g.m_Out.m_uiSize = outSize_p;

    ret = EplApiProcessImageAlloc(inSize_p, outSize_p, 2, 2);
    if (ret != kEplSuccessful)
    {
        return ret;
    }

    ret = EplApiProcessImageSetup();

    return ret;
}

///\}

