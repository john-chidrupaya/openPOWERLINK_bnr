/**
********************************************************************************
\file   QtApiGlobal.h

\brief  Global definitions
*******************************************************************************/

#ifndef _PLKQTAPI_GLOBAL_H_
#define _PLKQTAPI_GLOBAL_H_

#include <QtCore/qglobal.h>

#if defined(PLKQTAPI_LIB)
#  define PLKQTAPI_EXPORT Q_DECL_EXPORT
#else
#  define PLKQTAPI_EXPORT
#endif

#endif // _PLKQTAPI_GLOBAL_H_