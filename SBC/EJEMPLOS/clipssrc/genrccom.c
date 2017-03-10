   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*               CLIPS Version 6.21  06/15/03          */
   /*                                                     */
   /*                                                     */
   /*******************************************************/

/*************************************************************/
/* Purpose: Generic Functions Interface Routines             */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Brian L. Donnell                                     */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*************************************************************/

/* =========================================
   *****************************************
               EXTERNAL DEFINITIONS
   =========================================
   ***************************************** */
#include "setup.h"

#if DEFGENERIC_CONSTRUCT

#include <string.h>

#if DEFRULE_CONSTRUCT
#include "network.h"
#endif

#if BLOAD || BLOAD_AND_BSAVE
#include "bload.h"
#endif

#if BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE
#include "genrcbin.h"
#endif

#if CONSTRUCT_COMPILER
#include "genrccmp.h"
#endif

#if (! BLOAD_ONLY) && (! RUN_TIME)
#include "constrct.h"
#include "genrcpsr.h"
#endif

#if OBJECT_SYSTEM
#include "classcom.h"
#include "inscom.h"
#endif

#if DEBUGGING_FUNCTIONS
#include "watch.h"
#endif

#include "argacces.h"
#include "cstrcpsr.h"
#include "envrnmnt.h"
#include "extnfunc.h"
#include "genrcexe.h"
#include "memalloc.h"
#include "modulpsr.h"
#include "multifld.h"
#include "router.h"

#define _GENRCCOM_SOURCE_
#include "genrccom.h"

/* =========================================
   *****************************************
      INTERNALLY VISIBLE FUNCTION HEADERS
   =========================================
   ***************************************** */

static void PrintGenericCall(void *,char *,void *);
static BOOLEAN EvaluateGenericCall(void *,void *,DATA_OBJECT *);
static void DecrementGenericBusyCount(void *,void *);
static void IncrementGenericBusyCount(void *,void *);
static void DeallocateDefgenericData(void *);
static void DestroyDefgenericAction(void *,struct constructHeader *,void *);

#if (! BLOAD_ONLY) && (! RUN_TIME)

static void SaveDefgenerics(void *,void *,char *);
static void SaveDefmethods(void *,void *,char *);
static void SaveDefmethodsForDefgeneric(void *,struct constructHeader *,void *);
static void RemoveDefgenericMethod(void *,DEFGENERIC *,int);

#endif

#if DEBUGGING_FUNCTIONS
static long ListMethodsForGeneric(void *,char *,DEFGENERIC *);
static unsigned DefgenericWatchAccess(void *,int,unsigned,EXPRESSION *);
static unsigned DefgenericWatchPrint(void *,char *,int,EXPRESSION *);
static unsigned DefmethodWatchAccess(void *,int,unsigned,EXPRESSION *);
static unsigned DefmethodWatchPrint(void *,char *,int,EXPRESSION *);
static unsigned DefmethodWatchSupport(void *,char *,char *,unsigned,
                                     void (*)(void *,char *,void *,unsigned),
                                     void (*)(void *,unsigned,void *,unsigned),
                                     EXPRESSION *);
static void PrintMethodWatchFlag(void *,char *,void *,unsigned);
#endif

/* =========================================
   *****************************************
          EXTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***********************************************************
  NAME         : SetupGenericFunctions
  DESCRIPTION  : Initializes all generic function
                   data structures, constructs and functions
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Generic function H/L functions set up
  NOTES        : None
 ***********************************************************/
globle void SetupGenericFunctions(
  void *theEnv)
  {
   ENTITY_RECORD genericEntityRecord =
                     { "GCALL", GCALL,0,0,1,
                       PrintGenericCall,PrintGenericCall,
                       NULL,EvaluateGenericCall,NULL,
                       DecrementGenericBusyCount,IncrementGenericBusyCount,
                       NULL,NULL,NULL,NULL };
   
   AllocateEnvironmentData(theEnv,DEFGENERIC_DATA,sizeof(struct defgenericData),DeallocateDefgenericData);
   memcpy(&DefgenericData(theEnv)->GenericEntityRecord,&genericEntityRecord,sizeof(struct entityRecord));   

   InstallPrimitive(theEnv,&DefgenericData(theEnv)->GenericEntityRecord,GCALL);

   DefgenericData(theEnv)->DefgenericModuleIndex =
                RegisterModuleItem(theEnv,"defgeneric",
#if (! RUN_TIME)
                                    AllocateDefgenericModule,FreeDefgenericModule,
#else
                                    NULL,NULL,
#endif
#if BLOAD_AND_BSAVE || BLOAD || BLOAD_ONLY
                                    BloadDefgenericModuleReference,
#else
                                    NULL,
#endif
#if CONSTRUCT_COMPILER && (! RUN_TIME)
                                    DefgenericCModuleReference,
#else
                                    NULL,
#endif
                                    EnvFindDefgeneric);

   DefgenericData(theEnv)->DefgenericConstruct =  AddConstruct(theEnv,"defgeneric","defgenerics",
#if (! BLOAD_ONLY) && (! RUN_TIME)
                                       ParseDefgeneric,
#else
                                       NULL,
#endif
                                       EnvFindDefgeneric,
                                       GetConstructNamePointer,GetConstructPPForm,
                                       GetConstructModuleItem,EnvGetNextDefgeneric,
                                       SetNextConstruct,EnvIsDefgenericDeletable,
                                       EnvUndefgeneric,
#if (! BLOAD_ONLY) && (! RUN_TIME)
                                       RemoveDefgeneric
#else
                                       NULL
#endif
                                       );

#if ! RUN_TIME
   AddClearReadyFunction(theEnv,"defgeneric",ClearDefgenericsReady,0);

#if BLOAD || BLOAD_ONLY || BLOAD_AND_BSAVE
   SetupGenericsBload(theEnv);
#endif

#if CONSTRUCT_COMPILER
   SetupGenericsCompiler(theEnv);
#endif

#if ! BLOAD_ONLY
#if DEFMODULE_CONSTRUCT
   AddPortConstructItem(theEnv,"defgeneric",SYMBOL);
#endif
   AddConstruct(theEnv,"defmethod","defmethods",ParseDefmethod,
                NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

  /* ================================================================
     Make sure defmethods are cleared last, for other constructs may
       be using them and need to be cleared first

     Need to be cleared in two stages so that mutually dependent
       constructs (like classes) can be cleared
     ================================================================ */
   AddSaveFunction(theEnv,"defgeneric",SaveDefgenerics,1000);
   AddSaveFunction(theEnv,"defmethod",SaveDefmethods,-1000);
   EnvDefineFunction2(theEnv,"undefgeneric",'v',PTIEF UndefgenericCommand,"UndefgenericCommand","11w");
   EnvDefineFunction2(theEnv,"undefmethod",'v',PTIEF UndefmethodCommand,"UndefmethodCommand","22*wg");
#endif

#if IMPERATIVE_METHODS
   EnvDefineFunction2(theEnv,"call-next-method",'u',PTIEF CallNextMethod,"CallNextMethod","00");
   FuncSeqOvlFlags(theEnv,"call-next-method",TRUE,FALSE);
   EnvDefineFunction2(theEnv,"call-specific-method",'u',PTIEF CallSpecificMethod,
                   "CallSpecificMethod","2**wi");
   FuncSeqOvlFlags(theEnv,"call-specific-method",TRUE,FALSE);
   EnvDefineFunction2(theEnv,"override-next-method",'u',PTIEF OverrideNextMethod,
                   "OverrideNextMethod",NULL);
   FuncSeqOvlFlags(theEnv,"override-next-method",TRUE,FALSE);
   EnvDefineFunction2(theEnv,"next-methodp",'b',PTIEF NextMethodP,"NextMethodP","00");
   FuncSeqOvlFlags(theEnv,"next-methodp",TRUE,FALSE);
#endif
   EnvDefineFunction2(theEnv,"(gnrc-current-arg)",'u',PTIEF GetGenericCurrentArgument,
                   "GetGenericCurrentArgument",NULL);

#if DEBUGGING_FUNCTIONS
   EnvDefineFunction2(theEnv,"ppdefgeneric",'v',PTIEF PPDefgenericCommand,"PPDefgenericCommand","11w");
   EnvDefineFunction2(theEnv,"list-defgenerics",'v',PTIEF ListDefgenericsCommand,"ListDefgenericsCommand","01");
   EnvDefineFunction2(theEnv,"ppdefmethod",'v',PTIEF PPDefmethodCommand,"PPDefmethodCommand","22*wi");
   EnvDefineFunction2(theEnv,"list-defmethods",'v',PTIEF ListDefmethodsCommand,"ListDefmethodsCommand","01w");
   EnvDefineFunction2(theEnv,"preview-generic",'v',PTIEF PreviewGeneric,"PreviewGeneric","1**w");
#endif

   EnvDefineFunction2(theEnv,"get-defgeneric-list",'m',PTIEF GetDefgenericListFunction,
                   "GetDefgenericListFunction","01");
   EnvDefineFunction2(theEnv,"get-defmethod-list",'m',PTIEF GetDefmethodListCommand,
                   "GetDefmethodListCommand","01w");
   EnvDefineFunction2(theEnv,"get-method-restrictions",'m',PTIEF GetMethodRestrictionsCommand,
                   "GetMethodRestrictionsCommand","22iw");
   EnvDefineFunction2(theEnv,"defgeneric-module",'w',PTIEF GetDefgenericModuleCommand,
                   "GetDefgenericModuleCommand","11w");

#if OBJECT_SYSTEM
   EnvDefineFunction2(theEnv,"type",'u',PTIEF ClassCommand,"ClassCommand","11u");
#else
   EnvDefineFunction2(theEnv,"type",'u',PTIEF TypeCommand,"TypeCommand","11u");
#endif

#endif

#if DEBUGGING_FUNCTIONS
   AddWatchItem(theEnv,"generic-functions",0,&DefgenericData(theEnv)->WatchGenerics,34,
                DefgenericWatchAccess,DefgenericWatchPrint);
   AddWatchItem(theEnv,"methods",0,&DefgenericData(theEnv)->WatchMethods,33,
                DefmethodWatchAccess,DefmethodWatchPrint);
#endif
  }
  
/*****************************************************/
/* DeallocateDefgenericData: Deallocates environment */
/*    data for the defgeneric construct.             */
/*****************************************************/
static void DeallocateDefgenericData(
  void *theEnv)
  {
#if ! RUN_TIME
   struct defgenericModule *theModuleItem;
   void *theModule;

#if BLOAD || BLOAD_AND_BSAVE
   if (Bloaded(theEnv)) return;
#endif

   DoForAllConstructs(theEnv,DestroyDefgenericAction,DefgenericData(theEnv)->DefgenericModuleIndex,FALSE,NULL); 

   for (theModule = EnvGetNextDefmodule(theEnv,NULL);
        theModule != NULL;
        theModule = EnvGetNextDefmodule(theEnv,theModule))
     {
      theModuleItem = (struct defgenericModule *)
                      GetModuleItem(theEnv,(struct defmodule *) theModule,
                                    DefgenericData(theEnv)->DefgenericModuleIndex);

      rtn_struct(theEnv,defgenericModule,theModuleItem);
     }
#else
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif
#endif
  }
  
/****************************************************/
/* DestroyDefgenericAction: Action used to remove   */
/*   defgenerics as a result of DestroyEnvironment. */
/****************************************************/
#if IBM_TBC
#pragma argsused
#endif
static void DestroyDefgenericAction(
  void *theEnv,
  struct constructHeader *theConstruct,
  void *buffer)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(buffer)
#endif
#if (! BLOAD_ONLY) && (! RUN_TIME)
   struct defgeneric *theDefgeneric = (struct defgeneric *) theConstruct;
   unsigned i;
   
   if (theDefgeneric == NULL) return;

   for (i = 0 ; i < theDefgeneric->mcnt ; i++)
     { DestroyMethodInfo(theEnv,theDefgeneric,&theDefgeneric->methods[i]); }

   if (theDefgeneric->mcnt != 0)
     rm(theEnv,(void *) theDefgeneric->methods,(sizeof(DEFMETHOD) * theDefgeneric->mcnt));

   DestroyConstructHeader(theEnv,&theDefgeneric->header);

   rtn_struct(theEnv,defgeneric,theDefgeneric);
#else
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv,theConstruct)
#endif
#endif
  }

/***************************************************
  NAME         : EnvFindDefgeneric
  DESCRIPTION  : Searches for a generic
  INPUTS       : The name of the generic
                 (possibly including a module name)
  RETURNS      : Pointer to the generic if
                 found, otherwise NULL
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
globle void *EnvFindDefgeneric(
  void *theEnv,
  char *genericModuleAndName)
  {
   return(FindNamedConstruct(theEnv,genericModuleAndName,DefgenericData(theEnv)->DefgenericConstruct));
  }

/***************************************************
  NAME         : LookupDefgenericByMdlOrScope
  DESCRIPTION  : Finds a defgeneric anywhere (if
                 module is specified) or in current
                 or imported modules
  INPUTS       : The defgeneric name
  RETURNS      : The defgeneric (NULL if not found)
  SIDE EFFECTS : Error message printed on
                  ambiguous references
  NOTES        : None
 ***************************************************/
globle DEFGENERIC *LookupDefgenericByMdlOrScope(
  void *theEnv,
  char *defgenericName)
  {
   return((DEFGENERIC *) LookupConstruct(theEnv,DefgenericData(theEnv)->DefgenericConstruct,defgenericName,TRUE));
  }

/***************************************************
  NAME         : LookupDefgenericInScope
  DESCRIPTION  : Finds a defgeneric in current or
                   imported modules (module
                   specifier is not allowed)
  INPUTS       : The defgeneric name
  RETURNS      : The defgeneric (NULL if not found)
  SIDE EFFECTS : Error message printed on
                  ambiguous references
  NOTES        : None
 ***************************************************/
globle DEFGENERIC *LookupDefgenericInScope(
  void *theEnv,
  char *defgenericName)
  {
   return((DEFGENERIC *) LookupConstruct(theEnv,DefgenericData(theEnv)->DefgenericConstruct,defgenericName,FALSE));
  }

/***********************************************************
  NAME         : EnvGetNextDefgeneric
  DESCRIPTION  : Finds first or next generic function
  INPUTS       : The address of the current generic function
  RETURNS      : The address of the next generic function
                   (NULL if none)
  SIDE EFFECTS : None
  NOTES        : If ptr == NULL, the first generic function
                    is returned.
 ***********************************************************/
globle void *EnvGetNextDefgeneric(
  void *theEnv,
  void *ptr)
  {
   return((void *) GetNextConstructItem(theEnv,(struct constructHeader *) ptr,DefgenericData(theEnv)->DefgenericModuleIndex));
  }

/***********************************************************
  NAME         : EnvGetNextDefmethod
  DESCRIPTION  : Find the next method for a generic function
  INPUTS       : 1) The generic function address
                 2) The index of the current method
  RETURNS      : The index of the next method
                    (0 if none)
  SIDE EFFECTS : None
  NOTES        : If index == 0, the index of the first
                   method is returned
 ***********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle unsigned EnvGetNextDefmethod(
  void *theEnv,
  void *ptr,
  unsigned theIndex)
  {
   DEFGENERIC *gfunc;
   int mi;
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   gfunc = (DEFGENERIC *) ptr;
   if (theIndex == 0)
     {
      if (gfunc->methods != NULL)
        return(gfunc->methods[0].index);
      return(0);
     }
   mi = FindMethodByIndex(gfunc,theIndex);
   if ((mi+1) == (int) gfunc->mcnt)
     return(0);
   return(gfunc->methods[mi+1].index);
  }

/*****************************************************
  NAME         : GetDefmethodPointer
  DESCRIPTION  : Returns a pointer to a method
  INPUTS       : 1) Pointer to a defgeneric
                 2) Array index of method in generic's
                    method array (+1)
  RETURNS      : Pointer to the method.
  SIDE EFFECTS : None
  NOTES        : None
 *****************************************************/
globle DEFMETHOD *GetDefmethodPointer(
  void *ptr,
  unsigned theIndex)
  {
   return(&((DEFGENERIC *) ptr)->methods[theIndex-1]);
  }

/***************************************************
  NAME         : EnvIsDefgenericDeletable
  DESCRIPTION  : Determines if a generic function
                   can be deleted
  INPUTS       : Address of the generic function
  RETURNS      : TRUE if deletable, FALSE otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
globle int EnvIsDefgenericDeletable(
  void *theEnv,
  void *ptr)
  {
#if (MAC_MPW || MAC_MCW || IBM_MCW) && (RUN_TIME || BLOAD_ONLY)
#pragma unused(theEnv,ptr)
#endif

#if BLOAD_ONLY || RUN_TIME
   return(FALSE);
#else
#if BLOAD || BLOAD_AND_BSAVE

   if (Bloaded(theEnv))
     return(FALSE);
#endif
   return ((((DEFGENERIC *) ptr)->busy == 0) ? TRUE : FALSE);
#endif
  }

/***************************************************
  NAME         : EnvIsDefmethodDeletable
  DESCRIPTION  : Determines if a generic function
                   method can be deleted
  INPUTS       : 1) Address of the generic function
                 2) Index of the method
  RETURNS      : TRUE if deletable, FALSE otherwise
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
globle int EnvIsDefmethodDeletable(
  void *theEnv,
  void *ptr,
  unsigned theIndex)
  {
#if (MAC_MPW || MAC_MCW || IBM_MCW) && (RUN_TIME || BLOAD_ONLY)
#pragma unused(theEnv,ptr,theIndex)
#endif

#if BLOAD_ONLY || RUN_TIME
   return(FALSE);
#else
#if BLOAD || BLOAD_AND_BSAVE

   if (Bloaded(theEnv))
     return(FALSE);
#endif
   if (((DEFGENERIC *) ptr)->methods[FindMethodByIndex((DEFGENERIC *) ptr,theIndex)].system)
     return(FALSE);
   return((MethodsExecuting((DEFGENERIC *) ptr) == FALSE) ? TRUE : FALSE);
#endif
  }

/**********************************************************
  NAME         : UndefgenericCommand
  DESCRIPTION  : Deletes all methods for a generic function
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : methods deallocated
  NOTES        : H/L Syntax: (undefgeneric <name> | *)
 **********************************************************/
globle void UndefgenericCommand(
  void *theEnv)
  {
   UndefconstructCommand(theEnv,"undefgeneric",DefgenericData(theEnv)->DefgenericConstruct);
  }

/****************************************************************
  NAME         : GetDefgenericModuleCommand
  DESCRIPTION  : Determines to which module a defgeneric belongs
  INPUTS       : None
  RETURNS      : The symbolic name of the module
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (defgeneric-module <generic-name>)
 ****************************************************************/
globle SYMBOL_HN *GetDefgenericModuleCommand(
  void *theEnv)
  {
   return(GetConstructModuleCommand(theEnv,"defgeneric-module",DefgenericData(theEnv)->DefgenericConstruct));
  }

/**************************************************************
  NAME         : UndefmethodCommand
  DESCRIPTION  : Deletes one method for a generic function
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : methods deallocated
  NOTES        : H/L Syntax: (undefmethod <name> <index> | *)
 **************************************************************/
globle void UndefmethodCommand(
  void *theEnv)
  {
   DATA_OBJECT temp;
   DEFGENERIC *gfunc;
   unsigned mi;

   if (EnvArgTypeCheck(theEnv,"undefmethod",1,SYMBOL,&temp) == FALSE)
     return;
   gfunc = LookupDefgenericByMdlOrScope(theEnv,DOToString(temp));
   if ((gfunc == NULL) ? (strcmp(DOToString(temp),"*") != 0) : FALSE)
     {
      PrintErrorID(theEnv,"GENRCCOM",1,FALSE);
      EnvPrintRouter(theEnv,WERROR,"No such generic function ");
      EnvPrintRouter(theEnv,WERROR,DOToString(temp));
      EnvPrintRouter(theEnv,WERROR," in function undefmethod.\n");
      return;
     }
   EnvRtnUnknown(theEnv,2,&temp);
   if (temp.type == SYMBOL)
     {
      if (strcmp(DOToString(temp),"*") != 0)
        {
         PrintErrorID(theEnv,"GENRCCOM",2,FALSE);
         EnvPrintRouter(theEnv,WERROR,"Expected a valid method index in function undefmethod.\n");
         return;
        }
      mi = 0;
     }
   else if (temp.type == INTEGER)
     {
      mi = (unsigned) DOToInteger(temp);
      if (mi == 0)
        {
         PrintErrorID(theEnv,"GENRCCOM",2,FALSE);
         EnvPrintRouter(theEnv,WERROR,"Expected a valid method index in function undefmethod.\n");
         return;
        }
     }
   else
     {
      PrintErrorID(theEnv,"GENRCCOM",2,FALSE);
      EnvPrintRouter(theEnv,WERROR,"Expected a valid method index in function undefmethod.\n");
      return;
     }
   EnvUndefmethod(theEnv,(void *) gfunc,mi);
  }

/**************************************************************
  NAME         : EnvUndefgeneric
  DESCRIPTION  : Deletes all methods for a generic function
  INPUTS       : The generic-function address (NULL for all)
  RETURNS      : TRUE if generic successfully deleted,
                 FALSE otherwise
  SIDE EFFECTS : methods deallocated
  NOTES        : None
 **************************************************************/
globle BOOLEAN EnvUndefgeneric(
  void *theEnv,
  void *vptr)
  {
#if (MAC_MPW || MAC_MCW || IBM_MCW) && (RUN_TIME || BLOAD_ONLY)
#pragma unused(theEnv,vptr)
#endif

#if RUN_TIME || BLOAD_ONLY
   return(FALSE);
#else
   DEFGENERIC *gfunc;
   int success = TRUE;

   gfunc = (DEFGENERIC *) vptr;
   if (gfunc == NULL)
     {
      if (ClearDefmethods(theEnv) == FALSE)
        success = FALSE;
      if (ClearDefgenerics(theEnv) == FALSE)
        success = FALSE;
      return(success);
     }
   if (EnvIsDefgenericDeletable(theEnv,vptr) == FALSE)
     return(FALSE);
   RemoveConstructFromModule(theEnv,(struct constructHeader *) vptr);
   RemoveDefgeneric(theEnv,gfunc);
   return(TRUE);
#endif
  }

/**************************************************************
  NAME         : EnvUndefmethod
  DESCRIPTION  : Deletes one method for a generic function
  INPUTS       : 1) Address of generic function (can be NULL)
                 2) Method index (0 for all)
  RETURNS      : TRUE if method deleted successfully,
                 FALSE otherwise
  SIDE EFFECTS : methods deallocated
  NOTES        : None
 **************************************************************/
globle BOOLEAN EnvUndefmethod(
  void *theEnv,
  void *vptr,
  unsigned mi)
  {
   DEFGENERIC *gfunc;

#if RUN_TIME || BLOAD_ONLY
   gfunc = (DEFGENERIC *) vptr;
   PrintErrorID(theEnv,"PRNTUTIL",4,FALSE);
   EnvPrintRouter(theEnv,WERROR,"Unable to delete method ");
   if (gfunc != NULL)
     {
      PrintGenericName(theEnv,WERROR,gfunc);
      EnvPrintRouter(theEnv,WERROR," #");
      PrintLongInteger(theEnv,WERROR,(long) mi);
     }
   else
     EnvPrintRouter(theEnv,WERROR,"*");
   EnvPrintRouter(theEnv,WERROR,".\n");
   return(FALSE);
#else
   int nmi;

   gfunc = (DEFGENERIC *) vptr;
#if BLOAD || BLOAD_AND_BSAVE
   if (Bloaded(theEnv) == TRUE)
     {
      PrintErrorID(theEnv,"PRNTUTIL",4,FALSE);
      EnvPrintRouter(theEnv,WERROR,"Unable to delete method ");
      if (gfunc != NULL)
        {
         EnvPrintRouter(theEnv,WERROR,EnvGetDefgenericName(theEnv,(void *) gfunc));
         EnvPrintRouter(theEnv,WERROR," #");
         PrintLongInteger(theEnv,WERROR,(long) mi);
        }
      else
        EnvPrintRouter(theEnv,WERROR,"*");
      EnvPrintRouter(theEnv,WERROR,".\n");
      return(FALSE);
     }
#endif
   if (gfunc == NULL)
     {
      if (mi != 0)
        {
         PrintErrorID(theEnv,"GENRCCOM",3,FALSE);
         EnvPrintRouter(theEnv,WERROR,"Incomplete method specification for deletion.\n");
         return(FALSE);
        }
      return(ClearDefmethods(theEnv));
     }
   if (MethodsExecuting(gfunc))
     {
      MethodAlterError(theEnv,gfunc);
      return(FALSE);
     }
   if (mi == 0)
     RemoveAllExplicitMethods(theEnv,gfunc);
   else
     {
      nmi = CheckMethodExists(theEnv,"undefmethod",gfunc,(int) mi);
      if (nmi == -1)
        return(FALSE);
      RemoveDefgenericMethod(theEnv,gfunc,nmi);
     }
   return(TRUE);
#endif
  }

#if DEBUGGING_FUNCTIONS

/*****************************************************
  NAME         : EnvGetDefmethodDescription
  DESCRIPTION  : Prints a synopsis of method parameter
                   restrictions into caller's buffer
  INPUTS       : 1) Caller's buffer
                 2) Buffer size (not including space
                    for terminating '\0')
                 3) Address of generic function
                 4) Index of method
  RETURNS      : Nothing useful
  SIDE EFFECTS : Caller's buffer written
  NOTES        : Terminating '\n' not written
 *****************************************************/
globle void EnvGetDefmethodDescription(
  void *theEnv,
  char *buf,
  int buflen,
  void *ptr,
  unsigned theIndex)
  {
   DEFGENERIC *gfunc;
   int mi;
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   gfunc = (DEFGENERIC *) ptr;
   mi = FindMethodByIndex(gfunc,theIndex);
   PrintMethod(theEnv,buf,buflen,&gfunc->methods[mi]);
  }

/*********************************************************
  NAME         : EnvGetDefgenericWatch
  DESCRIPTION  : Determines if trace messages are
                 gnerated when executing generic function
  INPUTS       : A pointer to the generic
  RETURNS      : TRUE if a trace is active,
                 FALSE otherwise
  SIDE EFFECTS : None
  NOTES        : None
 *********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle unsigned EnvGetDefgenericWatch(
  void *theEnv,
  void *theGeneric)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   return(((DEFGENERIC *) theGeneric)->trace);
  }

/*********************************************************
  NAME         : EnvSetDefgenericWatch
  DESCRIPTION  : Sets the trace to ON/OFF for the
                 generic function
  INPUTS       : 1) TRUE to set the trace on,
                    FALSE to set it off
                 2) A pointer to the generic
  RETURNS      : Nothing useful
  SIDE EFFECTS : Watch flag for the generic set
  NOTES        : None
 *********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle void EnvSetDefgenericWatch(
  void *theEnv,
  unsigned newState,
  void *theGeneric)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   ((DEFGENERIC *) theGeneric)->trace = newState;
  }

/*********************************************************
  NAME         : EnvGetDefmethodWatch
  DESCRIPTION  : Determines if trace messages for calls
                 to this method will be generated or not
  INPUTS       : 1) A pointer to the generic
                 2) The index of the method
  RETURNS      : TRUE if a trace is active,
                 FALSE otherwise
  SIDE EFFECTS : None
  NOTES        : None
 *********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle unsigned EnvGetDefmethodWatch(
  void *theEnv,
  void *theGeneric,
  unsigned theIndex)
  {
   DEFGENERIC *gfunc;
   int mi;
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   gfunc = (DEFGENERIC *) theGeneric;
   mi = FindMethodByIndex(gfunc,theIndex);
   return(gfunc->methods[mi].trace);
  }

/*********************************************************
  NAME         : EnvSetDefmethodWatch
  DESCRIPTION  : Sets the trace to ON/OFF for the
                 calling of the method
  INPUTS       : 1) TRUE to set the trace on,
                    FALSE to set it off
                 2) A pointer to the generic
                 3) The index of the method
  RETURNS      : Nothing useful
  SIDE EFFECTS : Watch flag for the method set
  NOTES        : None
 *********************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle void EnvSetDefmethodWatch(
  void *theEnv,
  unsigned newState,
  void *theGeneric,
  unsigned theIndex)
  {
   DEFGENERIC *gfunc;
   int mi;
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   gfunc = (DEFGENERIC *) theGeneric;
   mi = FindMethodByIndex(gfunc,theIndex);
   gfunc->methods[mi].trace = newState;
  }


/********************************************************
  NAME         : PPDefgenericCommand
  DESCRIPTION  : Displays the pretty-print form of
                  a generic function header
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (ppdefgeneric <name>)
 ********************************************************/
globle void PPDefgenericCommand(
  void *theEnv)
  {
   PPConstructCommand(theEnv,"ppdefgeneric",DefgenericData(theEnv)->DefgenericConstruct);
  }

/**********************************************************
  NAME         : PPDefmethodCommand
  DESCRIPTION  : Displays the pretty-print form of
                  a method
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (ppdefmethod <name> <index>)
 **********************************************************/
globle void PPDefmethodCommand(
  void *theEnv)
  {
   DATA_OBJECT temp;
   char *gname;
   DEFGENERIC *gfunc;
   int gi;
   
   if (EnvArgTypeCheck(theEnv,"ppdefmethod",1,SYMBOL,&temp) == FALSE)
     return;
   gname = DOToString(temp);
   if (EnvArgTypeCheck(theEnv,"ppdefmethod",2,INTEGER,&temp) == FALSE)
     return;
   gfunc = CheckGenericExists(theEnv,"ppdefmethod",gname);
   if (gfunc == NULL)
     return;
   gi = CheckMethodExists(theEnv,"ppdefmethod",gfunc,DOToInteger(temp));
   if (gi == -1)
     return;
   if (gfunc->methods[gi].ppForm != NULL)
     PrintInChunks(theEnv,WDISPLAY,gfunc->methods[gi].ppForm);
  }

/******************************************************
  NAME         : ListDefmethodsCommand
  DESCRIPTION  : Lists a brief description of methods
                   for a particular generic function
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (list-defmethods <name>)
 ******************************************************/
globle void ListDefmethodsCommand(
  void *theEnv)
  {
   DATA_OBJECT temp;
   DEFGENERIC *gfunc;
   
   if (EnvRtnArgCount(theEnv) == 0)
     EnvListDefmethods(theEnv,WDISPLAY,NULL);
   else
     {
      if (EnvArgTypeCheck(theEnv,"list-defmethods",1,SYMBOL,&temp) == FALSE)
        return;
      gfunc = CheckGenericExists(theEnv,"list-defmethods",DOToString(temp));
      if (gfunc != NULL)
        EnvListDefmethods(theEnv,WDISPLAY,(void *) gfunc);
     }
  }

/***************************************************************
  NAME         : EnvGetDefmethodPPForm
  DESCRIPTION  : Getsa generic function method pretty print form
  INPUTS       : 1) Address of the generic function
                 2) Index of the method
  RETURNS      : Method ppform
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************************/
#if IBM_TBC
#pragma argsused
#endif
globle char *EnvGetDefmethodPPForm(
  void *theEnv,
  void *ptr,
  unsigned theIndex)
  {
   DEFGENERIC *gfunc;
   int mi;
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif

   gfunc = (DEFGENERIC *) ptr;
   mi = FindMethodByIndex(gfunc,theIndex);
   return(gfunc->methods[mi].ppForm);
  }

/***************************************************
  NAME         : ListDefgenericsCommand
  DESCRIPTION  : Displays all defgeneric names
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defgeneric names printed
  NOTES        : H/L Interface
 ***************************************************/
globle void ListDefgenericsCommand(
  void *theEnv)
  {
   ListConstructCommand(theEnv,"list-defgenerics",DefgenericData(theEnv)->DefgenericConstruct);
  }

/***************************************************
  NAME         : EnvListDefgenerics
  DESCRIPTION  : Displays all defgeneric names
  INPUTS       : 1) The logical name of the output
                 2) The module
  RETURNS      : Nothing useful
  SIDE EFFECTS : Defgeneric names printed
  NOTES        : C Interface
 ***************************************************/
globle void EnvListDefgenerics(
  void *theEnv,
  char *logicalName,
  struct defmodule *theModule)
  {
   ListConstruct(theEnv,DefgenericData(theEnv)->DefgenericConstruct,logicalName,theModule);
  }

/******************************************************
  NAME         : EnvListDefmethods
  DESCRIPTION  : Lists a brief description of methods
                   for a particular generic function
  INPUTS       : 1) The logical name of the output
                 2) Generic function to list methods for
                    (NULL means list all methods)
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 ******************************************************/
globle void EnvListDefmethods(
  void *theEnv,
  char *logicalName,
  void *vptr)
  {
   DEFGENERIC *gfunc;
   long count;
   if (vptr != NULL)
     count = ListMethodsForGeneric(theEnv,logicalName,(DEFGENERIC *) vptr);
   else
     {
      count = 0L;
      for (gfunc = (DEFGENERIC *) EnvGetNextDefgeneric(theEnv,NULL) ;
           gfunc != NULL ;
           gfunc = (DEFGENERIC *) EnvGetNextDefgeneric(theEnv,(void *) gfunc))
        {
         count += ListMethodsForGeneric(theEnv,logicalName,gfunc);
         if (EnvGetNextDefgeneric(theEnv,(void *) gfunc) != NULL)
           EnvPrintRouter(theEnv,logicalName,"\n");
        }
     }
   PrintTally(theEnv,logicalName,count,"method","methods");
  }

#endif

/***************************************************************
  NAME         : GetDefgenericListFunction
  DESCRIPTION  : Groups all defgeneric names into
                 a multifield list
  INPUTS       : A data object buffer to hold
                 the multifield result
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield allocated and filled
  NOTES        : H/L Syntax: (get-defgeneric-list [<module>])
 ***************************************************************/
globle void GetDefgenericListFunction(
  void *theEnv,
  DATA_OBJECT*returnValue)
  {
   GetConstructListFunction(theEnv,"get-defgeneric-list",returnValue,DefgenericData(theEnv)->DefgenericConstruct);
  }

/***************************************************************
  NAME         : EnvGetDefgenericList
  DESCRIPTION  : Groups all defgeneric names into
                 a multifield list
  INPUTS       : 1) A data object buffer to hold
                    the multifield result
                 2) The module from which to obtain defgenerics
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield allocated and filled
  NOTES        : External C access
 ***************************************************************/
globle void EnvGetDefgenericList(
  void *theEnv,
  DATA_OBJECT *returnValue,
  struct defmodule *theModule)
  {
   GetConstructList(theEnv,returnValue,DefgenericData(theEnv)->DefgenericConstruct,theModule);
  }

/***********************************************************
  NAME         : GetDefmethodListCommand
  DESCRIPTION  : Groups indices of all methdos for a generic
                 function into a multifield variable
                 (NULL means get methods for all generics)
  INPUTS       : A data object buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield set to list of method indices
  NOTES        : None
 ***********************************************************/
globle void GetDefmethodListCommand(
  void *theEnv,
  DATA_OBJECT_PTR returnValue)
  {
   DATA_OBJECT temp;
   DEFGENERIC *gfunc;
   
   if (EnvRtnArgCount(theEnv) == 0)
     EnvGetDefmethodList(theEnv,NULL,returnValue);
   else
     {
      if (EnvArgTypeCheck(theEnv,"get-defmethod-list",1,SYMBOL,&temp) == FALSE)
        {
         EnvSetMultifieldErrorValue(theEnv,returnValue);
         return;
        }
      gfunc = CheckGenericExists(theEnv,"get-defmethod-list",DOToString(temp));
      if (gfunc != NULL)
        EnvGetDefmethodList(theEnv,(void *) gfunc,returnValue);
      else
        EnvSetMultifieldErrorValue(theEnv,returnValue);
     }
  }

/***********************************************************
  NAME         : EnvGetDefmethodList
  DESCRIPTION  : Groups indices of all methdos for a generic
                 function into a multifield variable
                 (NULL means get methods for all generics)
  INPUTS       : 1) A pointer to a generic function
                 2) A data object buffer
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield set to list of method indices
  NOTES        : None
 ***********************************************************/
globle void EnvGetDefmethodList(
  void *theEnv,
  void *vgfunc,
  DATA_OBJECT_PTR returnValue)
  {
   DEFGENERIC *gfunc,*svg,*svnxt;
   unsigned i,j;
   unsigned long count;
   MULTIFIELD_PTR theList;

   if (vgfunc != NULL)
     {
      gfunc = (DEFGENERIC *) vgfunc;
      svnxt = (DEFGENERIC *) EnvGetNextDefgeneric(theEnv,vgfunc);
      SetNextDefgeneric(vgfunc,NULL);
     }
   else
     {
      gfunc = (DEFGENERIC *) EnvGetNextDefgeneric(theEnv,NULL);
      svnxt = (gfunc != NULL) ? (DEFGENERIC *) EnvGetNextDefgeneric(theEnv,(void *) gfunc) : NULL;
     }
   count = 0;
   for (svg = gfunc ;
        gfunc != NULL ;
        gfunc = (DEFGENERIC *) EnvGetNextDefgeneric(theEnv,(void *) gfunc))
     count += (unsigned long) gfunc->mcnt;
   count *= 2;
   SetpType(returnValue,MULTIFIELD);
   SetpDOBegin(returnValue,1);
   SetpDOEnd(returnValue,count);
   theList = (MULTIFIELD_PTR) EnvCreateMultifield(theEnv,count);
   SetpValue(returnValue,theList);
   for (gfunc = svg , i = 1 ;
        gfunc != NULL ;
        gfunc = (DEFGENERIC *) EnvGetNextDefgeneric(theEnv,(void *) gfunc))
     {
      for (j = 0 ; j < gfunc->mcnt ; j++)
        {
         SetMFType(theList,i,SYMBOL);
         SetMFValue(theList,i++,GetDefgenericNamePointer((void *) gfunc));
         SetMFType(theList,i,INTEGER);
         SetMFValue(theList,i++,EnvAddLong(theEnv,(long) gfunc->methods[j].index));
        }
     }
   if (svg != NULL)
     SetNextDefgeneric((void *) svg,(void *) svnxt);
  }

/***********************************************************************************
  NAME         : GetMethodRestrictionsCommand
  DESCRIPTION  : Stores restrictions of a method in multifield
  INPUTS       : A data object buffer to hold a multifield
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield created (length zero on errors)
  NOTES        : Syntax: (get-method-restrictions <generic-function> <method-index>)
 ***********************************************************************************/
globle void GetMethodRestrictionsCommand(
  void *theEnv,
  DATA_OBJECT *result)
  {
   DATA_OBJECT temp;
   DEFGENERIC *gfunc;

   if (EnvArgTypeCheck(theEnv,"get-method-restrictions",1,SYMBOL,&temp) == FALSE)
     {
      EnvSetMultifieldErrorValue(theEnv,result);
      return;
     }
   gfunc = CheckGenericExists(theEnv,"get-method-restrictions",DOToString(temp));
   if (gfunc == NULL)
     {
      EnvSetMultifieldErrorValue(theEnv,result);
      return;
     }
   if (EnvArgTypeCheck(theEnv,"get-method-restrictions",2,INTEGER,&temp) == FALSE)
     {
      EnvSetMultifieldErrorValue(theEnv,result);
      return;
     }
   if (CheckMethodExists(theEnv,"get-method-restrictions",gfunc,DOToInteger(temp)) == -1)
     {
      EnvSetMultifieldErrorValue(theEnv,result);
      return;
     }
   EnvGetMethodRestrictions(theEnv,(void *) gfunc,(unsigned) DOToInteger(temp),result);
  }

/***********************************************************************
  NAME         : EnvGetMethodRestrictions
  DESCRIPTION  : Stores restrictions of a method in multifield
  INPUTS       : 1) Pointer to the generic function
                 2) The method index
                 3) A data object buffer to hold a multifield
  RETURNS      : Nothing useful
  SIDE EFFECTS : Multifield created (length zero on errors)
  NOTES        : The restrictions are stored in the multifield
                 in the following format:

                 <min-number-of-arguments>
                 <max-number-of-arguments> (-1 if wildcard allowed)
                 <restriction-count>
                 <index of 1st restriction>
                       .
                       .
                 <index of nth restriction>
                 <restriction 1>
                     <query TRUE/FALSE>
                     <number-of-classes>
                     <class 1>
                        .
                        .
                     <class n>
                    .
                    .
                    .
                  <restriction n>

                  Thus, for the method
                  (defmethod foo ((?a NUMBER SYMBOL) (?b (= 1 1)) $?c))
                  (get-method-restrictions foo 1) would yield

                  (2 -1 3 7 11 13 FALSE 2 NUMBER SYMBOL TRUE 0 FALSE 0)
 ***********************************************************************/
globle void EnvGetMethodRestrictions(
  void *theEnv,
  void *vgfunc,
  unsigned mi,
  DATA_OBJECT *result)
  {
   register unsigned i,j;
   register DEFMETHOD *meth;
   register RESTRICTION *rptr;
   unsigned count;
   int roffset,rstrctIndex;
   MULTIFIELD_PTR theList;

   meth = ((DEFGENERIC *) vgfunc)->methods + FindMethodByIndex((DEFGENERIC *) vgfunc,mi);
   count = 3;
   for (i = 0 ; i < (unsigned) meth->restrictionCount ; i++)
     count += meth->restrictions[i].tcnt + 3;
   theList = (MULTIFIELD_PTR) EnvCreateMultifield(theEnv,count);
   SetpType(result,MULTIFIELD);
   SetpValue(result,theList);
   SetpDOBegin(result,1);
   SetpDOEnd(result,count);
   SetMFType(theList,1,INTEGER);
   SetMFValue(theList,1,EnvAddLong(theEnv,(long) meth->minRestrictions));
   SetMFType(theList,2,INTEGER);
   SetMFValue(theList,2,EnvAddLong(theEnv,(long) meth->maxRestrictions));
   SetMFType(theList,3,INTEGER);
   SetMFValue(theList,3,EnvAddLong(theEnv,(long) meth->restrictionCount));
   roffset = 3 + meth->restrictionCount + 1;
   rstrctIndex = 4;
   for (i = 0 ; i < (unsigned) meth->restrictionCount ; i++)
     {
      rptr = meth->restrictions + i;
      SetMFType(theList,rstrctIndex,INTEGER);
      SetMFValue(theList,rstrctIndex++,EnvAddLong(theEnv,(long) roffset));
      SetMFType(theList,roffset,SYMBOL);
      SetMFValue(theList,roffset++,(rptr->query != NULL) ? SymbolData(theEnv)->TrueSymbol : SymbolData(theEnv)->FalseSymbol);
      SetMFType(theList,roffset,INTEGER);
      SetMFValue(theList,roffset++,EnvAddLong(theEnv,(long) rptr->tcnt));
      for (j = 0 ; j < rptr->tcnt ; j++)
        {
         SetMFType(theList,roffset,SYMBOL);
#if OBJECT_SYSTEM
         SetMFValue(theList,roffset++,EnvAddSymbol(theEnv,EnvGetDefclassName(theEnv,rptr->types[j])));
#else
         SetMFValue(theList,roffset++,EnvAddSymbol(theEnv,TypeName(theEnv,ValueToInteger(rptr->types[j]))));
#endif
        }
     }
  }

/* =========================================
   *****************************************
          INTERNALLY VISIBLE FUNCTIONS
   =========================================
   ***************************************** */

/***************************************************
  NAME         : PrintGenericCall
  DESCRIPTION  : PrintExpression() support function
                 for generic function calls
  INPUTS       : 1) The output logical name
                 2) The generic function
  RETURNS      : Nothing useful
  SIDE EFFECTS : Call expression printed
  NOTES        : None
 ***************************************************/
#if IBM_TBC && (! DEVELOPER)
#pragma argsused
#endif
static void PrintGenericCall(
  void *theEnv,
  char *log,
  void *value)
  {
#if DEVELOPER

   EnvPrintRouter(theEnv,log,"(");
   EnvPrintRouter(theEnv,log,EnvGetDefgenericName(theEnv,value));
   if (GetFirstArgument() != NULL)
     {
      EnvPrintRouter(theEnv,log," ");
      PrintExpression(theEnv,log,GetFirstArgument());
     }
   EnvPrintRouter(theEnv,log,")");
#else
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#pragma unused(log)
#pragma unused(value)
#endif
#endif
  }

/*******************************************************
  NAME         : EvaluateGenericCall
  DESCRIPTION  : Primitive support function for
                 calling a generic function
  INPUTS       : 1) The generic function
                 2) A data object buffer to hold
                    the evaluation result
  RETURNS      : FALSE if the generic function
                 returns the symbol FALSE,
                 TRUE otherwise
  SIDE EFFECTS : Data obejct buffer set and any
                 side-effects of calling the generic
  NOTES        : None
 *******************************************************/
static BOOLEAN EvaluateGenericCall(
  void *theEnv,
  void *value,
  DATA_OBJECT *result)
  {
   GenericDispatch(theEnv,(DEFGENERIC *) value,NULL,NULL,GetFirstArgument(),result);
   if ((GetpType(result) == SYMBOL) &&
       (GetpValue(result) == SymbolData(theEnv)->FalseSymbol))
     return(FALSE);
   return(TRUE);
  }

/***************************************************
  NAME         : DecrementGenericBusyCount
  DESCRIPTION  : Lowers the busy count of a
                 generic function construct
  INPUTS       : The generic function
  RETURNS      : Nothing useful
  SIDE EFFECTS : Busy count decremented if a clear
                 is not in progress (see comment)
  NOTES        : None
 ***************************************************/
static void DecrementGenericBusyCount(
  void *theEnv,
  void *value)
  {
   /* ==============================================
      The generics to which expressions in other
      constructs may refer may already have been
      deleted - thus, it is important not to modify
      the busy flag during a clear.
      ============================================== */
   if (! ConstructData(theEnv)->ClearInProgress)
     ((DEFGENERIC *) value)->busy--;
  }

/***************************************************
  NAME         : IncrementGenericBusyCount
  DESCRIPTION  : Raises the busy count of a
                 generic function construct
  INPUTS       : The generic function
  RETURNS      : Nothing useful
  SIDE EFFECTS : Busy count incremented
  NOTES        : None
 ***************************************************/
#if IBM_TBC
#pragma argsused
#endif
static void IncrementGenericBusyCount(
  void *theEnv,
  void *value)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(theEnv)
#endif
   ((DEFGENERIC *) value)->busy++;
  }

#if (! BLOAD_ONLY) && (! RUN_TIME)

/**********************************************************************
  NAME         : SaveDefgenerics
  DESCRIPTION  : Outputs pretty-print forms of generic function headers
  INPUTS       : The logical name of the output
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 **********************************************************************/
static void SaveDefgenerics(
  void *theEnv,
  void *theModule,
  char *log)
  {
   SaveConstruct(theEnv,theModule,log,DefgenericData(theEnv)->DefgenericConstruct);
  }

/**********************************************************************
  NAME         : SaveDefmethods
  DESCRIPTION  : Outputs pretty-print forms of generic function methods
  INPUTS       : The logical name of the output
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 **********************************************************************/
static void SaveDefmethods(
  void *theEnv,
  void *theModule,
  char *log)
  {
   DoForAllConstructsInModule(theEnv,theModule,SaveDefmethodsForDefgeneric,
                              DefgenericData(theEnv)->DefgenericModuleIndex,
                              FALSE,(void *) log);
  }

/***************************************************
  NAME         : SaveDefmethodsForDefgeneric
  DESCRIPTION  : Save the pretty-print forms of
                 all methods for a generic function
                 to a file
  INPUTS       : 1) The defgeneric
                 2) The logical name of the output
  RETURNS      : Nothing useful
  SIDE EFFECTS : Methods written
  NOTES        : None
 ***************************************************/
static void SaveDefmethodsForDefgeneric(
  void *theEnv,
  struct constructHeader *theDefgeneric,
  void *userBuffer)
  {
   DEFGENERIC *gfunc = (DEFGENERIC *) theDefgeneric;
   char *log = (char *) userBuffer;
   register unsigned i;

   for (i = 0 ; i < gfunc->mcnt ; i++)
     {
      if (gfunc->methods[i].ppForm != NULL)
        {
         PrintInChunks(theEnv,log,gfunc->methods[i].ppForm);
         EnvPrintRouter(theEnv,log,"\n");
        }
     }
  }

/****************************************************
  NAME         : RemoveDefgenericMethod
  DESCRIPTION  : Removes a generic function method
                   from the array and removes the
                   generic too if its the last method
  INPUTS       : 1) The generic function
                 2) The array index of the method
  RETURNS      : Nothing useful
  SIDE EFFECTS : List adjusted
                 Nodes deallocated
  NOTES        : Assumes deletion is safe
 ****************************************************/
static void RemoveDefgenericMethod(
  void *theEnv,
  DEFGENERIC *gfunc,
  int gi)
  {
   DEFMETHOD *narr;
   register unsigned b,e;

   if (gfunc->methods[gi].system)
     {
      SetEvaluationError(theEnv,TRUE);
      PrintErrorID(theEnv,"GENRCCOM",4,FALSE);
      EnvPrintRouter(theEnv,WERROR,"Cannot remove implicit system function method for generic function ");
      EnvPrintRouter(theEnv,WERROR,EnvGetDefgenericName(theEnv,(void *) gfunc));
      EnvPrintRouter(theEnv,WERROR,".\n");
      return;
     }
   DeleteMethodInfo(theEnv,gfunc,&gfunc->methods[gi]);
   if (gfunc->mcnt == 1)
     {
      rm(theEnv,(void *) gfunc->methods,(int) sizeof(DEFMETHOD));
      gfunc->mcnt = 0;
      gfunc->methods = NULL;
     }
   else
     {
      gfunc->mcnt--;
      narr = (DEFMETHOD *) gm2(theEnv,(sizeof(DEFMETHOD) * gfunc->mcnt));
      for (b = e = 0 ; b < gfunc->mcnt ; b++ , e++)
        {
         if (((int) b) == gi)
           e++;
         GenCopyMemory(DEFMETHOD,1,&narr[b],&gfunc->methods[e]);
        }
      rm(theEnv,(void *) gfunc->methods,(sizeof(DEFMETHOD) * (gfunc->mcnt+1)));
      gfunc->methods = narr;
     }
  }

#endif

#if DEBUGGING_FUNCTIONS

/******************************************************
  NAME         : ListMethodsForGeneric
  DESCRIPTION  : Lists a brief description of methods
                   for a particular generic function
  INPUTS       : 1) The logical name of the output
                 2) Generic function to list methods for
  RETURNS      : The number of methods printed
  SIDE EFFECTS : None
  NOTES        : None
 ******************************************************/
static long ListMethodsForGeneric(
  void *theEnv,
  char *logicalName,
  DEFGENERIC *gfunc)
  {
   unsigned gi;
   char buf[256];

   for (gi = 0 ; gi < gfunc->mcnt ; gi++)
     {
      EnvPrintRouter(theEnv,logicalName,EnvGetDefgenericName(theEnv,(void *) gfunc));
      EnvPrintRouter(theEnv,logicalName," #");
      PrintMethod(theEnv,buf,255,&gfunc->methods[gi]);
      EnvPrintRouter(theEnv,logicalName,buf);
      EnvPrintRouter(theEnv,logicalName,"\n");
     }
   return((long) gfunc->mcnt);
  }

/******************************************************************
  NAME         : DefgenericWatchAccess
  DESCRIPTION  : Parses a list of generic names passed by
                 AddWatchItem() and sets the traces accordingly
  INPUTS       : 1) A code indicating which trace flag is to be set
                    Ignored
                 2) The value to which to set the trace flags
                 3) A list of expressions containing the names
                    of the generics for which to set traces
  RETURNS      : TRUE if all OK, FALSE otherwise
  SIDE EFFECTS : Watch flags set in specified generics
  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
#if IBM_TBC
#pragma argsused
#endif
static unsigned DefgenericWatchAccess(
  void *theEnv,
  int code,
  unsigned newState,
  EXPRESSION *argExprs)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(code)
#endif

   return(ConstructSetWatchAccess(theEnv,DefgenericData(theEnv)->DefgenericConstruct,newState,argExprs,
                                    EnvGetDefgenericWatch,EnvSetDefgenericWatch));
  }

/***********************************************************************
  NAME         : DefgenericWatchPrint
  DESCRIPTION  : Parses a list of generic names passed by
                 AddWatchItem() and displays the traces accordingly
  INPUTS       : 1) The logical name of the output
                 2) A code indicating which trace flag is to be examined
                    Ignored
                 3) A list of expressions containing the names
                    of the generics for which to examine traces
  RETURNS      : TRUE if all OK, FALSE otherwise
  SIDE EFFECTS : Watch flags displayed for specified generics
  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
#if IBM_TBC
#pragma argsused
#endif
static unsigned DefgenericWatchPrint(
  void *theEnv,
  char *log,
  int code,
  EXPRESSION *argExprs)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(code)
#endif

   return(ConstructPrintWatchAccess(theEnv,DefgenericData(theEnv)->DefgenericConstruct,log,argExprs,
                                    EnvGetDefgenericWatch,EnvSetDefgenericWatch));
  }

/******************************************************************
  NAME         : DefmethodWatchAccess
  DESCRIPTION  : Parses a list of methods passed by
                 AddWatchItem() and sets the traces accordingly
  INPUTS       : 1) A code indicating which trace flag is to be set
                    Ignored
                 2) The value to which to set the trace flags
                 3) A list of expressions containing the methods
                   for which to set traces
  RETURNS      : TRUE if all OK, FALSE otherwise
  SIDE EFFECTS : Watch flags set in specified methods
  NOTES        : Accessory function for AddWatchItem()
 ******************************************************************/
#if IBM_TBC
#pragma argsused
#endif
static unsigned DefmethodWatchAccess(
  void *theEnv,
  int code,
  unsigned newState,
  EXPRESSION *argExprs)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(code)
#endif
   return(DefmethodWatchSupport(theEnv,(char *) (newState ? "watch" : "unwatch"),NULL,
                                newState,NULL,EnvSetDefmethodWatch,argExprs));
  }

/***********************************************************************
  NAME         : DefmethodWatchPrint
  DESCRIPTION  : Parses a list of methods passed by
                 AddWatchItem() and displays the traces accordingly
  INPUTS       : 1) The logical name of the output
                 2) A code indicating which trace flag is to be examined
                    Ignored
                 3) A list of expressions containing the methods for
                    which to examine traces
  RETURNS      : TRUE if all OK, FALSE otherwise
  SIDE EFFECTS : Watch flags displayed for specified methods
  NOTES        : Accessory function for AddWatchItem()
 ***********************************************************************/
#if IBM_TBC
#pragma argsused
#endif
static unsigned DefmethodWatchPrint(
  void *theEnv,
  char *log,
  int code,
  EXPRESSION *argExprs)
  {
#if MAC_MPW || MAC_MCW || IBM_MCW
#pragma unused(code)
#endif
   return(DefmethodWatchSupport(theEnv,"list-watch-items",log,0,
                                PrintMethodWatchFlag,NULL,argExprs));
  }

/*******************************************************
  NAME         : DefmethodWatchSupport
  DESCRIPTION  : Sets or displays methods specified
  INPUTS       : 1) The calling function name
                 2) The logical output name for displays
                    (can be NULL)
                 3) The new set state
                 4) The print function (can be NULL)
                 5) The trace function (can be NULL)
                 6) The methods expression list
  RETURNS      : TRUE if all OK,
                 FALSE otherwise
  SIDE EFFECTS : Method trace flags set or displayed
  NOTES        : None
 *******************************************************/
static unsigned DefmethodWatchSupport(
  void *theEnv,
  char *funcName,
  char *log,
  unsigned newState,
  void (*printFunc)(void *,char *,void *,unsigned),
  void (*traceFunc)(void *,unsigned,void *,unsigned),
  EXPRESSION *argExprs)
  {
   void *theGeneric;
   unsigned theMethod;
   int argIndex = 2;
   DATA_OBJECT genericName,methodIndex;
   struct defmodule *theModule;

   /* ==============================
      If no methods are specified,
      show the trace for all methods
      in all generics
      ============================== */
   if (argExprs == NULL)
     {
      SaveCurrentModule(theEnv);
      theModule = (struct defmodule *) EnvGetNextDefmodule(theEnv,NULL);
      while (theModule != NULL)
        {
         EnvSetCurrentModule(theEnv,(void *) theModule);
         if (traceFunc == NULL)
           {
            EnvPrintRouter(theEnv,log,EnvGetDefmoduleName(theEnv,(void *) theModule));
            EnvPrintRouter(theEnv,log,":\n");
           }
         theGeneric = EnvGetNextDefgeneric(theEnv,NULL);
         while (theGeneric != NULL)
            {
             theMethod = EnvGetNextDefmethod(theEnv,theGeneric,0);
             while (theMethod != 0)
               {
                if (traceFunc != NULL)
                  (*traceFunc)(theEnv,newState,theGeneric,theMethod);
                else
                  {
                   EnvPrintRouter(theEnv,log,"   ");
                   (*printFunc)(theEnv,log,theGeneric,theMethod);
                  }
                theMethod = EnvGetNextDefmethod(theEnv,theGeneric,theMethod);
               }
             theGeneric = EnvGetNextDefgeneric(theEnv,theGeneric);
            }
         theModule = (struct defmodule *) EnvGetNextDefmodule(theEnv,(void *) theModule);
        }
      RestoreCurrentModule(theEnv);
      return(TRUE);
     }

   /* =========================================
      Set the traces for every method specified
      ========================================= */
   while (argExprs != NULL)
     {
      if (EvaluateExpression(theEnv,argExprs,&genericName))
        return(FALSE);
      if ((genericName.type != SYMBOL) ? TRUE :
          ((theGeneric = (void *)
              LookupDefgenericByMdlOrScope(theEnv,DOToString(genericName))) == NULL))
        {
         ExpectedTypeError1(theEnv,funcName,argIndex,"generic function name");
         return(FALSE);
        }
      if (GetNextArgument(argExprs) == NULL)
        theMethod = 0;
      else
        {
         argExprs = GetNextArgument(argExprs);
         argIndex++;
         if (EvaluateExpression(theEnv,argExprs,&methodIndex))
           return(FALSE);
         if ((methodIndex.type != INTEGER) ? FALSE :
             ((DOToInteger(methodIndex) <= 0) ? FALSE :
              (FindMethodByIndex((DEFGENERIC *) theGeneric,theMethod) != -1)))
           theMethod = (unsigned) DOToInteger(methodIndex);
         else
           {
            ExpectedTypeError1(theEnv,funcName,argIndex,"method index");
            return(FALSE);
           }
        }
      if (theMethod == 0)
        {
         theMethod = EnvGetNextDefmethod(theEnv,theGeneric,0);
         while (theMethod != 0)
           {
            if (traceFunc != NULL)
              (*traceFunc)(theEnv,newState,theGeneric,theMethod);
            else
              (*printFunc)(theEnv,log,theGeneric,theMethod);
            theMethod = EnvGetNextDefmethod(theEnv,theGeneric,theMethod);
           }
        }
      else
        {
         if (traceFunc != NULL)
           (*traceFunc)(theEnv,newState,theGeneric,theMethod);
         else
           (*printFunc)(theEnv,log,theGeneric,theMethod);
        }
      argExprs = GetNextArgument(argExprs);
      argIndex++;
     }
   return(TRUE);
  }

/***************************************************
  NAME         : PrintMethodWatchFlag
  DESCRIPTION  : Displays trace value for method
  INPUTS       : 1) The logical name of the output
                 2) The generic function
                 3) The method index
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : None
 ***************************************************/
static void PrintMethodWatchFlag(
  void *theEnv,
  char *log,
  void *theGeneric,
  unsigned theMethod)
  {
   char buf[60];

   EnvPrintRouter(theEnv,log,EnvGetDefgenericName(theEnv,theGeneric));
   EnvPrintRouter(theEnv,log," ");
   EnvGetDefmethodDescription(theEnv,buf,59,theGeneric,theMethod);
   EnvPrintRouter(theEnv,log,buf);
   EnvPrintRouter(theEnv,log,(char *) (EnvGetDefmethodWatch(theEnv,theGeneric,theMethod) ? " = on\n" : " = off\n"));
  }

#endif

#if ! OBJECT_SYSTEM

/***************************************************
  NAME         : TypeCommand
  DESCRIPTION  : Works like "class" in COOL
  INPUTS       : None
  RETURNS      : Nothing useful
  SIDE EFFECTS : None
  NOTES        : H/L Syntax: (type <primitive>)
 ***************************************************/
globle void TypeCommand(
  void *theEnv,
  DATA_OBJECT *result)
  {
   EvaluateExpression(theEnv,GetFirstArgument(),result);
   result->value = (void *) EnvAddSymbol(theEnv,TypeName(theEnv,result->type));
   result->type = SYMBOL;
  }

#endif

#endif

