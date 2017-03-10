   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*             CLIPS Version 6.20  01/31/02            */
   /*                                                     */
   /*          DEFTEMPLATE BASIC COMMANDS MODULE          */
   /*******************************************************/

/*************************************************************/
/* Purpose: Implements core commands for the deftemplate     */
/*   construct such as clear, reset, save, undeftemplate,    */
/*   ppdeftemplate, list-deftemplates, and                   */
/*   get-deftemplate-list.                                   */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Brian L. Donnell                                     */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*************************************************************/

#define _TMPLTBSC_SOURCE_

#include "setup.h"

#if DEFTEMPLATE_CONSTRUCT

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "argacces.h"
#include "memalloc.h"
#include "scanner.h"
#include "router.h"
#include "extnfunc.h"
#include "constrct.h"
#include "cstrccom.h"
#include "factrhs.h"
#include "cstrcpsr.h"
#include "tmpltpsr.h"
#include "tmpltdef.h"
#if BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE
#include "tmpltbin.h"
#endif
#if CONSTRUCT_COMPILER && (! RUN_TIME)
#include "tmpltcmp.h"
#endif
#include "tmpltutl.h"
#include "envrnmnt.h"

#include "tmpltbsc.h"

/***************************************/
/* LOCAL INTERNAL FUNCTION DEFINITIONS */
/***************************************/

#if ! DEFFACTS_CONSTRUCT
   static void                    ResetDeftemplates(void *);
#endif
   static void                    ClearDeftemplates(void *);
   static void                    SaveDeftemplates(void *,void *,char *);

/*********************************************************************/
/* DeftemplateBasicCommands: Initializes basic deftemplate commands. */
/*********************************************************************/
globle void DeftemplateBasicCommands(
  void *theEnv)
  {
#if ! DEFFACTS_CONSTRUCT
   EnvAddResetFunction(theEnv,"deftemplate",ResetDeftemplates,0);
#endif
   EnvAddClearFunction(theEnv,"deftemplate",ClearDeftemplates,0);
   AddSaveFunction(theEnv,"deftemplate",SaveDeftemplates,10);

#if ! RUN_TIME
   EnvDefineFunction2(theEnv,"get-deftemplate-list",'m',PTIEF GetDeftemplateListFunction,"GetDeftemplateListFunction","01w");
   EnvDefineFunction2(theEnv,"undeftemplate",'v',PTIEF UndeftemplateCommand,"UndeftemplateCommand","11w");
   EnvDefineFunction2(theEnv,"deftemplate-module",'w',PTIEF DeftemplateModuleFunction,"DeftemplateModuleFunction","11w");

#if DEBUGGING_FUNCTIONS
   EnvDefineFunction2(theEnv,"list-deftemplates",'v', PTIEF ListDeftemplatesCommand,"ListDeftemplatesCommand","01w");
   EnvDefineFunction2(theEnv,"ppdeftemplate",'v',PTIEF PPDeftemplateCommand,"PPDeftemplateCommand","11w");
#endif

#if (BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE)
   DeftemplateBinarySetup(theEnv);
#endif

#if CONSTRUCT_COMPILER && (! RUN_TIME)
   DeftemplateCompilerSetup(theEnv);
#endif

#endif
  }

/*************************************************************/
/* ResetDeftemplates: Deftemplate reset routine for use with */
/*   the reset command. Asserts the initial-fact fact when   */
/*   the deffacts construct has been disabled.               */
/*************************************************************/
#if ! DEFFACTS_CONSTRUCT
static void ResetDeftemplates(
  void *theEnv)
  {
   struct fact *factPtr;

   factPtr = StringToFact(theEnv,"(initial-fact)");

   if (factPtr == NULL) return;

   Assert((void *) factPtr);
 }
#endif

/*****************************************************************/
/* ClearDeftemplates: Deftemplate clear routine for use with the */
/*   clear command. Creates the initial-facts deftemplate.       */
/*****************************************************************/
static void ClearDeftemplates(
  void *theEnv)
  {
#if (! RUN_TIME) && (! BLOAD_ONLY)

   CreateImpliedDeftemplate(theEnv,(SYMBOL_HN *) EnvAddSymbol(theEnv,"initial-fact"),FALSE);
#else
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif
#endif
  }

/**********************************************/
/* SaveDeftemplates: Deftemplate save routine */
/*   for use with the save command.           */
/**********************************************/
static void SaveDeftemplates(
  void *theEnv,
  void *theModule,
  char *logicalName)
  {   
   SaveConstruct(theEnv,theModule,logicalName,DeftemplateData(theEnv)->DeftemplateConstruct);
  }

/**********************************************/
/* UndeftemplateCommand: H/L access routine   */
/*   for the undeftemplate command.           */
/**********************************************/
globle void UndeftemplateCommand(
  void *theEnv)
  {   
   UndefconstructCommand(theEnv,"undeftemplate",DeftemplateData(theEnv)->DeftemplateConstruct); 
  }

/**************************************/
/* EnvUndeftemplate: C access routine */
/*   for the undeftemplate command.   */
/**************************************/
globle BOOLEAN EnvUndeftemplate(
  void *theEnv,
  void *theDeftemplate)
  {   
   return(Undefconstruct(theEnv,theDeftemplate,DeftemplateData(theEnv)->DeftemplateConstruct)); 
  }

/****************************************************/
/* GetDeftemplateListFunction: H/L access routine   */
/*   for the get-deftemplate-list function.         */
/****************************************************/
globle void GetDeftemplateListFunction(
  void *theEnv,
  DATA_OBJECT_PTR returnValue)
  {   
   GetConstructListFunction(theEnv,"get-deftemplate-list",returnValue,DeftemplateData(theEnv)->DeftemplateConstruct); 
  }

/***********************************************/
/* EnvGetDeftemplateList: C access routine for */
/*   the get-deftemplate-list function.        */
/***********************************************/
globle void EnvGetDeftemplateList(
  void *theEnv,
  DATA_OBJECT_PTR returnValue,
  void *theModule)
  {   
   GetConstructList(theEnv,returnValue,DeftemplateData(theEnv)->DeftemplateConstruct,(struct defmodule *) theModule); 
  }

/***************************************************/
/* DeftemplateModuleFunction: H/L access routine   */
/*   for the deftemplate-module function.          */
/***************************************************/
globle SYMBOL_HN *DeftemplateModuleFunction(
  void *theEnv)
  {   
   return(GetConstructModuleCommand(theEnv,"deftemplate-module",DeftemplateData(theEnv)->DeftemplateConstruct)); 
  }

#if DEBUGGING_FUNCTIONS

/**********************************************/
/* PPDeftemplateCommand: H/L access routine   */
/*   for the ppdeftemplate command.           */
/**********************************************/
globle void PPDeftemplateCommand(
  void *theEnv)
  {   
   PPConstructCommand(theEnv,"ppdeftemplate",DeftemplateData(theEnv)->DeftemplateConstruct); 
  }

/***************************************/
/* PPDeftemplate: C access routine for */
/*   the ppdeftemplate command.        */
/***************************************/
globle int PPDeftemplate(
  void *theEnv,
  char *deftemplateName,
  char *logicalName)
  {   
   return(PPConstruct(theEnv,deftemplateName,logicalName,DeftemplateData(theEnv)->DeftemplateConstruct)); 
  }

/*************************************************/
/* ListDeftemplatesCommand: H/L access routine   */
/*   for the list-deftemplates command.          */
/*************************************************/
globle void ListDeftemplatesCommand(
  void *theEnv)
  {    
   ListConstructCommand(theEnv,"list-deftemplates",DeftemplateData(theEnv)->DeftemplateConstruct); 
  }

/*****************************************/
/* EnvListDeftemplates: C access routine */
/*   for the list-deftemplates command.  */
/*****************************************/
globle void EnvListDeftemplates(
  void *theEnv,
  char *logicalName,
  void *theModule)
  {   
   ListConstruct(theEnv,DeftemplateData(theEnv)->DeftemplateConstruct,logicalName,(struct defmodule *) theModule); 
  }

/***********************************************************/
/* EnvGetDeftemplateWatch: C access routine for retrieving */
/*   the current watch value of a deftemplate.             */
/***********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle unsigned EnvGetDeftemplateWatch(
  void *theEnv,
  void *theTemplate)
  { 
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   return(((struct deftemplate *) theTemplate)->watch); 
  }

/*********************************************************/
/* EnvSetDeftemplateWatch:  C access routine for setting */
/*   the current watch value of a deftemplate.           */
/*********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle void EnvSetDeftemplateWatch(
  void *theEnv,
  unsigned newState,
  void *theTemplate)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   ((struct deftemplate *) theTemplate)->watch = newState; 
  }

/**********************************************************/
/* DeftemplateWatchAccess: Access routine for setting the */
/*   watch flag of a deftemplate via the watch command.   */
/**********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle unsigned DeftemplateWatchAccess(
  void *theEnv,
  int code,
  unsigned newState,
  EXPRESSION *argExprs)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(code)
#endif

   return(ConstructSetWatchAccess(theEnv,DeftemplateData(theEnv)->DeftemplateConstruct,newState,argExprs,
                                  EnvGetDeftemplateWatch,EnvSetDeftemplateWatch));
  }

/*************************************************************************/
/* DeftemplateWatchPrint: Access routine for printing which deftemplates */
/*   have their watch flag set via the list-watch-items command.         */
/*************************************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle unsigned DeftemplateWatchPrint(
  void *theEnv,
  char *log,
  int code,
  EXPRESSION *argExprs)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(code)
#endif

   return(ConstructPrintWatchAccess(theEnv,DeftemplateData(theEnv)->DeftemplateConstruct,log,argExprs,
                                    EnvGetDeftemplateWatch,EnvSetDeftemplateWatch));
  }

#endif /* DEBUGGING_FUNCTIONS */

#endif /* DEFTEMPLATE_CONSTRUCT */

