/**
********************************************************************************
\file   parser.c

\brief  {BRIEF_DESCRIPTION_OF_THE_FILE}

{DETAILED_DESCRIPTION_OF_THE_FILE}

\ingroup {MODULE_GROUP}
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
#include <Epl.h>
#include <console/console.h>
#include "local-types.h"
#include "parser.h"
#include "commands.h"

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

//------------------------------------------------------------------------------
// local function prototypes
//------------------------------------------------------------------------------

//============================================================================//
//            P U B L I C   F U N C T I O N S                                 //
//============================================================================//

//------------------------------------------------------------------------------
/**
\brief  Parse for commands

*/
//------------------------------------------------------------------------------
BOOL parseCommand(tCmdTbl* pCommands_p)
{
    char        command[120];
    tCmdTbl*    pCmdEntry;

    fgets (command, 120, stdin);
    command[strlen(command)-1] = '\0';
    pCmdEntry = pCommands_p;
    while (pCmdEntry->cmd != NULL)
    {
        if (strcmp(pCmdEntry->cmd, command) == 0)
        {
            return (pCmdEntry->cmdFunc());
        }
        pCmdEntry++;
    }
    printHelp();
    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Print command help

*/
//------------------------------------------------------------------------------
BOOL printHelp(void)
{
    tCmdTbl*        pCmdEntry;

    printf ("\nAvailable commands:\n-------------------\n");

    pCmdEntry = getCommands();
    while(pCmdEntry->cmd != NULL)
    {
        printf ("%s - %s\n", pCmdEntry->cmd, pCmdEntry->description);
        pCmdEntry++;
    }
    printf ("\n");
    return FALSE;
}

//------------------------------------------------------------------------------
/**
\brief  Read unsigned integer

*/
//------------------------------------------------------------------------------
UINT32 readUint(char* strName_p, UINT32 low_p, UINT32 high_p, int base_p)
{
    UINT32      input = 0;
    UINT32      fInputValid;
    char        buffer[30];
    char*       pEnd;

    fInputValid  = 0;
    do
    {
        printf("Enter %s: ", strName_p);
        fgets(buffer, sizeof(buffer), stdin);
        input = (UINT32)strtoul(buffer, &pEnd, base_p);
        if ((input >= low_p) && (input <= high_p))
        {
            fInputValid  = 1;
        }
        else
        {
            if (base_p == 10)
                printf ("Please enter a number between %d and %d\n", low_p, high_p);
            else
                printf ("Please enter a number between 0x%x and 0x%x\n", low_p, high_p);
        }
    } while(fInputValid != 1);

    return  input;
}

//============================================================================//
//            P R I V A T E   F U N C T I O N S                               //
//============================================================================//
/// \name Private Functions
/// \{


///\}







