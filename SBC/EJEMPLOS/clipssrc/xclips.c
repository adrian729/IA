   /*******************************************************/
   /*      "C" Language Integrated Production System      */
   /*                                                     */
   /*           X Windows Version 2.01  06/15/03          */ 
   /*                                                     */
   /*                    XCLIPS MODULE                    */
   /*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Bebe Ly                                              */
/*      Daniel J. McCoy                                      */
/*                                                           */
/* Contributing Programmer(s):                               */
/*      Gary D. Riley                                        */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*************************************************************/

#define _XCLIPS_SOURCE_

#include <stdio.h>
#include <stdlib.h>

#include "xsetup.h"
#include "xclips.h"
#include "xmenu.h"
#include "xmenu_wind.h"
#include "xmenu_opt.h"
#include "xclipstext.h"
#include "xmain.h"

#include "setup.h"
#include "agenda.h"
#include "bmathfun.h"
#include "classcom.h"
#include "commline.h"
#include "crstrtgy.h"
#include "defins.h"
#include "dffctdef.h"
#include "dffnxfun.h"
#include "engine.h"
#include "exprnpsr.h"
#include "facthsh.h"
#include "factmngr.h"
#include "filecom.h"
#include "genrccom.h"
#include "globlcom.h"
#include "globldef.h"
#include "incrrset.h"
#include "inscom.h"
#include "insfun.h"
#include "router.h"
#include "utility.h"

/********* Local functions visible outside this file **********/

static void PeriodicUpdate(void);
static void UpdateWindowsMenu(void);
static int GetEvent(void *);
static int InitalizeLogTable(void);
static void ReleaseLogTable(void);
static LogNamePtr LogInsert(char *);
static LogNamePtr LogLookup(char *);
static int GetHashValue(char *,int);

/********* Global variables **********/

char unget_buf[MAX_CHAR_IN_BUF];
int char_counter = 0;
int send_clips_command = True;
Boolean periodicChecking = False;
LogNamePtr log_table[LOG_TABLE_SIZE];
LogNamePtr stdin_log, xclips_log, 
           xagenda_log, xfacts_log,
           xinstances_log, xglobals_log,
           xfocus_log;


/*******************************************************************************
          Name:        InitializeInterface
          Description: initializes the the X window interface
          Arguments:   None
          Returns:     None
*******************************************************************************/
void InitializeInterface()
  {
   if (! InitalizeLogTable())
     {
      PrintRouter("werror", "Could not initialize logical name hash table\n");
      XclipsExit(0);
      ExitRouter(0);
     }
     
   //SetDribbleStatusFunction(GetCurrentEnvironment(),NULL);

   /*==================================================================*/
   /* Tell CLIPS which GetEvent function to call when an event needed. */
   /*==================================================================*/

   SetEventFunction(GetCurrentEnvironment(),GetEvent);

   /*  SetMemoryStatusFunction((int(*)())ActivateTheMenus());*/

   /*=================================================*/
   /* Tell CLIPS which function to call periodically. */
   /*=================================================*/
  
   AddPeriodicFunction("PeriodicUpdate",PeriodicUpdate,90);
   
   /*========================*/
   /* Add a main I/O router. */
   /*========================*/
   
   if (! AddRouter("wclips", 10, XclipsQuery, XclipsPrint, XclipsGetc, XclipsUngetc,
                XclipsExit))
     {
      printf("Could not allocate xclips router!\n");
      XclipsExit(0);
      ExitRouter(0);
     }
     
   PrintRouter("wclips", "                  XCLIPS for:");
  }

/*******************************************************************************
          Name:        XclipsQuery
          Description: This function verify the router's logical name
          Arguments:   log_name - logical name of the router
          Returns:      
*******************************************************************************/
int XclipsQuery(
  char *log_name)
  {
   return((LogLookup(log_name) != NULL) ? TRUE : FALSE);
  }

/*******************************************************************************
          Name:        XclipsPrint
          Description: This function will print a string to the window which 
                       has the same logical name as the argument log_name
          Arguments:   log_name - the logical name
                       str - string to be printed
          Returns:     unused
*******************************************************************************/
int XclipsPrint(
  char *log_name, 
  char *str)
  {
   String str_list[1];
   int num = 1;
   LogNamePtr lptr;

   if ((lptr = LogLookup(log_name)) == NULL)
     { return(False); }
     
   if (lptr == stdin_log)  /* standard IO : print to the main dialog */
     {
      str_list[0] = str;
      InsertClipsString(dialog_text, &TheEvent, str_list,(Cardinal *) &num);
     }
     
   /*================*/
   /* Agenda window. */
   /*================*/

   else if ((lptr == xagenda_log)&&(agenda_text != NULL))
     {
      str_list[0] = str;
      InsertClipsString(agenda_text, &TheEvent, str_list,(Cardinal *) &num);
      }

   /*==============*/
   /* Fact window. */
   /*==============*/

   else if ((lptr == xfacts_log)&&(facts_text != NULL))
     {
      str_list[0] = str;
      InsertClipsString(facts_text, &TheEvent, str_list,(Cardinal *) &num);
     }
     
   /*===================*/
   /* Instances window. */
   /*===================*/

   else if ((lptr == xinstances_log) && (instances_text != NULL))
     {
      str_list[0] = str;
      InsertClipsString(instances_text, &TheEvent, str_list,(Cardinal *) &num);  
     }
     
   /*====================*/
   /* Defglobals window. */  
   /*====================*/

   else if ((lptr == xglobals_log) && (globals_text != NULL))
     {
      str_list[0] = str;
      InsertClipsString(globals_text, &TheEvent, str_list,(Cardinal *) &num);
     }

   /*===============*/
   /* Focus window. */
   /*===============*/

   else if ((lptr == xfocus_log) && (focus_text != NULL))
     {
      str_list[0] = str;
      InsertClipsString(focus_text,&TheEvent,str_list,(Cardinal *) &num);
     }
   
   /*=======================*/
   /* Print to main dialog. */
   /*=======================*/

   else if ((lptr != xagenda_log) && (lptr != xfacts_log) &&
            (lptr != xinstances_log) && (lptr != xglobals_log)&&
            (lptr != stdin_log)) 
     {
      str_list[0] = str;
      InsertClipsString(dialog_text, &TheEvent, str_list,(Cardinal *) &num);
     }
   
   return(0);
  }

/*******************************************************************************
          Name:        XclipsGetc
          Description: Get a character input from user
          Arguments:   log_name -
          Returns:       
*******************************************************************************/
int XclipsGetc(
  char *log_name)
  {
   int quit = False;
   char ch[2];
   String str_list[1];
   int num = 1;
  
   ch[1] = 0;

   /* if unget_buf is not empty return the last character in the buf */

   if (char_counter > 0)
     { return(unget_buf[--char_counter]); }
     
   if (LogLookup(log_name) == NULL)
     { return(EOS); }
 
   while (! quit)
     {
      /* Get an event and if it is a legalimate (cahracter)key press */ 
      /* event print it to the main window, return the character     */
      /* (out of the while loop).                                    */
      /* else, dispatch the event and get another event              */

      XtAppNextEvent(app_con, &TheEvent); /* Get an event */
      XLookupString(&TheEvent.xkey, ch, 1, &TheKeysym, &compose_status);
      if ((TheEvent.type == KeyPress) &&
          (TheEvent.xproperty.window == dialog_text->core.window))
        {
         if ((TheKeysym >= XK_space) && (TheKeysym <= XK_asciitilde))
           {
            str_list[0] = ch;
            InsertClipsString(dialog_text, &TheEvent, str_list,(Cardinal *) &num);
            quit = True;
           }
         else if ((TheKeysym != XK_Linefeed) ? (TheKeysym == XK_Return) : TRUE)
           {
            ch[0] = NEWLINE;
            quit = True;
            XtDispatchEvent(&TheEvent);
           }
         else if ((TheKeysym != XK_Delete) ? (TheKeysym == XK_BackSpace) :TRUE)
           {
            if (RouterData(GetCurrentEnvironment())->CommandBufferInputCount != 0)
              {
               ch[0] = BACKSPACE;
               quit = True;
               XtDispatchEvent(&TheEvent);
              }
           }
        }
      else
        { XtDispatchEvent(&TheEvent); }
     }
     
   return((int)ch[0]);
  }

/*******************************************************************************
          Name:        XclipsUngetc
          Description: unget an input
          Arguments:   ch - the character to be save
                       log_name - logical name
          Returns:       
*******************************************************************************/
int XclipsUngetc(
  int ch,
  char *log_name)
  {
   if (char_counter < MAX_CHAR_IN_BUF)
     { unget_buf[char_counter++] = ch; }

   return(0);
  }

/*******************************************************************************
          Name:        XclipsExit
          Description: exit CLIPS in case of abnormal exit 
          Arguments:   num - unused
          Returns:       
*******************************************************************************/
int XclipsExit(
  int num)
  {
   DribbleOff();
   DeleteRouter("wclips");
   XtDestroyApplicationContext(app_con);
   ReleaseLogTable();
   exit(0);
  }

/*******************************************************************************
          Name:        PrintChangedAgenda
          Description: Update the agenda window
          Arguments:   None
          Returns:      
*******************************************************************************/
int PrintChangedAgenda()
  {
   VOID *rule_ptr;
   char buffer[MAX_CHAR_IN_BUF];
   char *name, labelBuffer[MAX_CHAR_IN_BUF];
   Window AgendaWin;
   Display *theDisplay;
   struct defmodule* theModule = (struct defmodule *)GetCurrentModule();

   /*======================================================*/
   /* Change the name of the window to the current module. */
   /*======================================================*/
   
   AgendaWin = XtWindow(agenda);
   theDisplay = XtDisplay(agenda);
   if (theModule  != NULL)
     {
      name = GetDefmoduleName(theModule);
      strcpy(labelBuffer,"Agenda Window(");
      strcat(labelBuffer,name);
      strcat(labelBuffer,")");
     }
    else
     {
      strcpy(labelBuffer,"Agenda Window");
     }
   
   XStoreName(theDisplay,AgendaWin,labelBuffer);
   
   /*============================*/
   /* Wipe out the old contents. */
   /*============================*/

   XtSetArg(TheArgs[0], XtNstring, "");
   XtSetValues(agenda_text, TheArgs, 1);
   XawAsciiSourceFreeString(XawTextGetSource(agenda_text));

   /*============================*/
   /* Print the new agenda list. */
   /*============================*/

   rule_ptr = GetNextActivation(NULL);
   while (rule_ptr != NULL)
     {
      GetActivationPPForm(buffer,MAX_CHAR_IN_BUF - 1,rule_ptr);
      PrintRouter("xagenda",buffer);
      PrintRouter("xagenda", "\n");
      rule_ptr = GetNextActivation(rule_ptr);
     }
    
   return 0;
  }

/*******************************************************************************
          Name:        PrintChangedFacts
          Description: Update the fact window
          Arguments:   None
          Returns:      
*******************************************************************************/
int PrintChangedFacts()
  {
  VOID *fact_ptr;
  char buffer[MAX_CHAR_IN_BUF];
  char *name,labelBuffer[MAX_CHAR_IN_BUF];
  Window FactWin;
  Display *theDisplay;
  struct defmodule* theModule = (struct defmodule *)GetCurrentModule();


 /* Change the name of the window to the current module */

  FactWin = XtWindow(facts);
  theDisplay = XtDisplay(facts);
  if(theModule  != NULL)
     {
       name = GetDefmoduleName(theModule);
       strcpy(labelBuffer,"Fact Window(");
       strcat(labelBuffer,name);
       strcat(labelBuffer,")");
     }
    else
     {
       strcpy(labelBuffer,"Fact Window");
     }
  XStoreName(theDisplay,FactWin,labelBuffer);


  /* Clear the old contents */

  XtSetArg(TheArgs[0], XtNstring, "");
  XtSetValues(facts_text, TheArgs, 1);
  XawAsciiSourceFreeString(XawTextGetSource(facts_text));

  /* Print the new fact list */

  fact_ptr = GetNextFact(NULL);
  while (fact_ptr != NULL)
    {
    GetFactPPForm(buffer,MAX_CHAR_IN_BUF - 1,fact_ptr);
    PrintRouter("xfacts",buffer);
    PrintRouter("xfacts", "\n");
    fact_ptr = GetNextFact(fact_ptr);
    }
  return 0;
  }

/*******************************************************************************
          Name:        PrintChangedInstances
          Description: Update the instances window
          Arguments:   None
          Returns:
*******************************************************************************/
int PrintChangedInstances()
{
   int   n = 0;
   VOID *instancePtr;
   char buffer[MAX_CHAR_IN_BUF];
   char *name,labelBuffer[MAX_CHAR_IN_BUF];
   Window InstanceWin;
   Display *theDisplay;
   struct defmodule* theModule = (struct defmodule *)GetCurrentModule();


 /* Change the name of the window to the current module */

    InstanceWin = XtWindow(instances);
    theDisplay = XtDisplay(instances);
    if(theModule  != NULL)
     {
       name = GetDefmoduleName(theModule);
       strcpy(labelBuffer,"Instances Window(");
       strcat(labelBuffer,name);
       strcat(labelBuffer,")");
     }
     else
     {
       strcpy(labelBuffer,"Instances Window");
     }
    XStoreName(theDisplay,InstanceWin,labelBuffer);


   /* Clear the old contents */

   XtSetArg(TheArgs[n],XtNstring,"");n++;
   XtSetValues(instances_text,TheArgs,n);
   XawAsciiSourceFreeString(XawTextGetSource(instances_text));
   /* Print the new instance list */
   instancePtr = (VOID*)GetNextInstance(NULL);
   while(instancePtr != NULL)
    {
       GetInstancePPForm(buffer,MAX_CHAR_IN_BUF - 1,instancePtr);
       PrintRouter("xinstances",buffer);
       PrintRouter("xinstances","\n");
       instancePtr = (VOID*)GetNextInstance(instancePtr);
    }
   return 0;
}

/*******************************************************************************
          Name:        PrintChangedGlobals
          Description: Update the global window
          Arguments:   None
          Returns:
*******************************************************************************/
int PrintChangedGlobals()
{
   VOID *dgPtr;
   int n;
   char *buffer;
   char *name,labelBuffer[MAX_CHAR_IN_BUF];
   Window GlobalWin;
   Display *theDisplay;
   struct defmodule* theModule = (struct defmodule *)GetCurrentModule();


 /* Change the name of the window to the current module */

    GlobalWin = XtWindow(globals);
    theDisplay = XtDisplay(globals);
    if(theModule  != NULL)
     {
       name = GetDefmoduleName(theModule);
       strcpy(labelBuffer,"Globals Window(");
       strcat(labelBuffer,name);
       strcat(labelBuffer,")");
     }
     else
     {
       strcpy(labelBuffer,"Globals Window");
     }
    XStoreName(theDisplay,GlobalWin,labelBuffer);

   /* Clear the old contents */

   n = 0;
   XtSetArg(TheArgs[n],XtNstring,"");n++;
   XtSetValues(globals_text,TheArgs,n);
   XawAsciiSourceFreeString(XawTextGetSource(globals_text));

   /* Print the new defglobal list */

   dgPtr = GetNextDefglobal(NULL);
   while(dgPtr != NULL)
    {
      buffer = (char *) GetDefglobalPPForm((struct constructHeader *) dgPtr);
      PrintRouter("xglobals",buffer);
      PrintRouter("xglobals","\n");
      dgPtr = GetNextDefglobal(dgPtr);
    }
   return 0;
}

/*******************************************************************************
          Name:        PrintChangedFocus
          Description: Update the Focus window
          Arguments:   None
          Returns:
*******************************************************************************/
int PrintChangedFocus()
{
   VOID *FocusPtr;
   int n;
   char *buffer;

   /* Clear the old contents */
   n = 0;
   XtSetArg(TheArgs[n],XtNstring,"");n++;
   XtSetValues(focus_text,TheArgs,n);
   XawAsciiSourceFreeString(XawTextGetSource(focus_text));

   /* Print the new focus list */

   FocusPtr = GetNextFocus(GetCurrentEnvironment(),NULL);
   while(FocusPtr != NULL)
    {
      buffer = GetDefmoduleName(((struct focus*)FocusPtr)->theModule);
      PrintRouter("xfocus",buffer);
      PrintRouter("xfocus","\n");
      FocusPtr = GetNextFocus(GetCurrentEnvironment(),FocusPtr);
    }
   return 0;
}


/*******************************************************************************
          Name:        UpdateMenus
          Description: Updates the windows if necessary
          Arguments:   None 
          Returns:     None
*******************************************************************************/
void UpdateMenus()
  {
  static long LastModuleIndex = -1; 
  Boolean UpdateAll = False;

  UpdateWindowsMenu();
  if(LastModuleIndex != DefmoduleData(GetCurrentEnvironment())->ModuleChangeIndex)
   {
     UpdateAll = True;
     LastModuleIndex = DefmoduleData(GetCurrentEnvironment())->ModuleChangeIndex;
   }
#if DEFRULE_CONSTRUCT
  if((Browse_status[AGENDA_WIN] == True ) ? (UpdateAll || (GetAgendaChanged() == CLIPS_TRUE)) : FALSE)
      {
      SetAgendaChanged(CLIPS_FALSE);
      PrintChangedAgenda();
      }

 if((Browse_status[FOCUS_WIN] == True) ? ( UpdateAll || (GetFocusChanged(GetCurrentEnvironment()) == CLIPS_TRUE)) : FALSE)
     {
     SetFocusChanged(GetCurrentEnvironment(),CLIPS_FALSE);
     PrintChangedFocus();
     }

#endif

#if DEFTEMPLATE_CONSTRUCT
  if((Browse_status[FACT_WIN] == True) ? ( UpdateAll || (GetFactListChanged() == CLIPS_TRUE)) : FALSE)
      {
      SetFactListChanged(CLIPS_FALSE);
      PrintChangedFacts();
      }
#endif

#if OBJECT_SYSTEM
 if((Browse_status[INSTANCE_WIN]) ? (UpdateAll || (GetInstancesChanged() == CLIPS_TRUE)) : FALSE)
      {
      SetInstancesChanged(CLIPS_FALSE);
      PrintChangedInstances();
      }
#endif

#ifdef DEFGLOBAL_CONSTRUCT
 if((Browse_status[GLOBAL_WIN] == True) ? (UpdateAll || (GetGlobalsChanged() == CLIPS_TRUE)) : FALSE)
      {
      SetGlobalsChanged(CLIPS_FALSE);
      PrintChangedGlobals();
      }

#endif
  }

/*******************************************************************************
          Name:        UpdateOptionsMenu
          Description: Set menu item mark on options selected
          Arguments:    None
          Returns:     None
*******************************************************************************/
void UpdateOptionsMenu()
  {
  int i;
  unsigned n =  0;

  XtSetArg(TheArgs[n], XtNleftBitmap, None);n++;
  for(i = 0; i <= RANDOM_STRATEGY;i++) 
    XtSetValues(strategy_widgets[i], TheArgs, n);
  for(i = 0; i <= EVERY_CYCLE;i++)
    XtSetValues(sal_opt_widgets[i],TheArgs,n);
  n = 0;
  XtSetArg(TheArgs[n], XtNleftBitmap, checker);n++;
  XtSetValues(strategy_widgets[GetStrategy()],TheArgs,n);
  XtSetValues(sal_opt_widgets[GetSalienceEvaluation()],TheArgs,n);
  n = 0;
  if (GetFactDuplication())
    {
    XtSetArg(TheArgs[n], XtNstate,True);n++;
    XtSetValues(option_widgets[INT_FACT_DUPLICATION], TheArgs, n);
    }
  else
    {
    XtSetArg(TheArgs[n], XtNstate,False);n++;
    XtSetValues(option_widgets[INT_FACT_DUPLICATION], TheArgs, n);
    }
  n = 0;
  if (GetDynamicConstraintChecking())
    {
    XtSetArg(TheArgs[n], XtNstate,True);n++;
    XtSetValues(option_widgets[INT_DYN_CONSTRAINT_CHK], TheArgs, n);
    }
  else
    {
    XtSetArg(TheArgs[n], XtNstate,False);n++;
    XtSetValues(option_widgets[INT_DYN_CONSTRAINT_CHK], TheArgs, n);
    }
  n = 0;
  if (GetStaticConstraintChecking())
    {
    XtSetArg(TheArgs[n], XtNstate,True);n++;
    XtSetValues(option_widgets[INT_STA_CONSTRAINT_CHK], TheArgs, n);
    }
  else
    {
    XtSetArg(TheArgs[n], XtNstate,False);n++;
    XtSetValues(option_widgets[INT_STA_CONSTRAINT_CHK], TheArgs, n);
    }
  n = 0;
  if (GetSequenceOperatorRecognition())
    {
    XtSetArg(TheArgs[n], XtNstate,True);n++;
    XtSetValues(option_widgets[INT_SEQUENCE_OPT_REG], TheArgs, n);
    }
  else
    {
    XtSetArg(TheArgs[n], XtNstate,False);n++;
    XtSetValues(option_widgets[INT_SEQUENCE_OPT_REG], TheArgs, n);
    }

  n = 0;
  if (GetAutoFloatDividend())
    {
    XtSetArg(TheArgs[n], XtNstate,True);n++;
    XtSetValues(option_widgets[INT_AUTO_FLOAT_DIV], TheArgs, n);
    }
  else
    {
    XtSetArg(TheArgs[n], XtNstate,False);n++;
    XtSetValues(option_widgets[INT_AUTO_FLOAT_DIV], TheArgs, n);
    }
  n =  0;
  if (GetIncrementalReset())
    {
    XtSetArg(TheArgs[n], XtNstate,True);n++;
    XtSetValues(option_widgets[INT_INCREMENTAL_RESET], TheArgs, n);
    }
  else
    {
    XtSetArg(TheArgs[n], XtNstate,False);n++;
    XtSetValues(option_widgets[INT_INCREMENTAL_RESET], TheArgs, n);
    }
  n =  0;
  if (GetResetGlobals())
    {
    XtSetArg(TheArgs[n], XtNstate,True);n++;
    XtSetValues(option_widgets[INT_RESET_GLOBALS], TheArgs, n);
    }
  else
    {
    XtSetArg(TheArgs[n], XtNstate,False);n++;
    XtSetValues(option_widgets[INT_RESET_GLOBALS], TheArgs, n);
    }
  }

/*******************************************************************************
          Name:        UpdateWindowsMenu
          Description: Sets manager menu items to sensitive  or unsensitive
          Arguments:   None
          Returns:     None
*******************************************************************************/
static void UpdateWindowsMenu()
  {
  /* ==================================================== */
  /*   Refresh the manager window if nessessary           */
  /* ==================================================== */
  if(list_change || list1_change )
      RefreshMngrList();

  /* =================================================================== */
  /*  Set the sensitive state to defrule manager item in the browse menu */
  /* =================================================================== */
  if(GetNextDefrule(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(defrule_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(defrule_manager, TheArgs, 1);
    }

  /* ===================================-================================ */
  /*  Set the sensitive state to deffacts manager item in the browse menu */
  /* ==================================================================== */
  if(GetNextDeffacts(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(deffact_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(deffact_manager, TheArgs, 1);
     }

  /* ======================================================================= */
  /*  Set the sensitive state to deftemplate manager item in the browse menu */
  /* ======================================================================= */
  if(GetNextDeftemplate(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(deftemplate_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(deftemplate_manager, TheArgs, 1);
    }

  /* ======================================================================= */
  /*  Set the sensitive state to deffunction manager item in the browse menu */
  /* ======================================================================= */
  if(GetNextDeffunction(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(deffunction_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(deffunction_manager, TheArgs, 1);
    }

  /* ===================================================================== */
  /*  Set the sensitive state to defglobal manager item in the browse menu */
  /* ===================================================================== */

  if(GetNextDefglobal(NULL))
   {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(defglobal_manager,TheArgs,1);
   }
  else
   {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(defglobal_manager,TheArgs,1);
   }

  /* ====================================================================== */
  /*  Set the sensitive state to defgeneric manager item in the browse menu */
  /* ====================================================================== */

  if(GetNextDefgeneric(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(defgeneric_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(defgeneric_manager, TheArgs, 1);
    }

  /* ======================================================================== */
  /*  Set the sensitive state to definstances manager item in the browse menu */
  /* ======================================================================== */

  if(GetNextDefinstances(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(definstances_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(definstances_manager, TheArgs, 1);
    }

  /* ==================================================================== */
  /*  Set the sensitive state to defclass manager item in the browse menu */
  /* ==================================================================== */

  if(GetNextDefclass(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(defclass_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(defclass_manager, TheArgs, 1);
    }

  /* =================================================================== */
  /*  Set the sensitive state to agenda manager item in the browse menu */
  /* =================================================================== */

  if(GetNextActivation(NULL))
    {
    XtSetArg(TheArgs[0], XtNsensitive, True);
    XtSetValues(agenda_manager, TheArgs, 1);
    }
  else
    {
    XtSetArg(TheArgs[0], XtNsensitive, False);
    XtSetValues(agenda_manager, TheArgs, 1);
    }
  }

/*******************************************************************************
          Name:        GetEvent
          Description: This function will get the events and dispatch 
                       them to the the appropriate event handler.
          Arguments:   None 
          Returns:       
*******************************************************************************/
static int GetEvent(
  void *theEnv)
  {
   char ch[2];
   int i;

   ch[1] = 0;
   quit_get_event = False;

   /*========================================================*/
   /* Set the menu items to sensitive before getting events. */
   /*========================================================*/

   XtSetArg(TheArgs[0],XtNsensitive,True);
   for (i  = 0; i < 7; i++)
     { XtSetValues(FileItemWidgets[i],TheArgs,1); }
    
   for (i = 0; i < 5; i++)
     { XtSetValues(ExecItemWidgets[i],TheArgs,1); }

   /*======================*/
   /* Refresh the windows. */
   /*======================*/
  
   UpdateMenus(); 

   while (! quit_get_event)
     {
      /*==============================================================*/
      /* There are two types of events (1) middle button press        */
      /* (paste event) inside  the  dialog window, (2) others.        */
      /* For (1), set the send_to_clips flag to true before dispatch. */
      /* This flag notify the event handler that the paste event was  */
      /* inside the main dialog, so send the pasted string to CLIPS   */
      /*==============================================================*/

      XtAppNextEvent(app_con, &TheEvent);
      if (TheEvent.type == ButtonPress)
        {
         if ((TheEvent.xproperty.window == dialog_text->core.window) &&
             (!GetManagerList())&& (TheEvent.xbutton.button == Button2))
           { send_to_clips = True; }
         XtDispatchEvent(&TheEvent);
        }
      else 
        {
         XtDispatchEvent(&TheEvent);
         set_clips_command(True);
        }
     }

   /*===================================*/
   /* Deactivate some of the menu items */
   /* while CLIPS processes a command.  */
   /*===================================*/

   XtSetArg(TheArgs[0],XtNsensitive,False);
   for (i  = 0; i < 7; i++)
     { XtSetValues(FileItemWidgets[i],TheArgs,1); }
     
   for (i = 0; i < 5; i++)
     { XtSetValues(ExecItemWidgets[i],TheArgs,1); }
     
   XtSetValues(defrule_manager,TheArgs,1);
   XtSetValues(deffact_manager,TheArgs,1);
   XtSetValues(deftemplate_manager,TheArgs,1);
   XtSetValues(deffunction_manager,TheArgs,1);
   XtSetValues(defgeneric_manager,TheArgs,1);
   XtSetValues(definstances_manager,TheArgs,1);
   XtSetValues(defclass_manager,TheArgs,1);
   XtSetValues(agenda_manager,TheArgs,1);
   
   return 0;
  }


/*******************************************************************************
          Name:        InitalizeLogTable
          Description:  
          Arguments:   None 
          Returns:       
*******************************************************************************/
static int InitalizeLogTable()
  {
   register int i;
    
   for (i = 0; i < LOG_TABLE_SIZE; i++)
     { log_table[i] = NULL; }

   if ((xclips_log = LogInsert("wclips")) == NULL)
     { return(FALSE); }
   
   if (LogInsert("wdialog") == NULL)
     { return(FALSE); }
   
   if (LogInsert("wdisplay") == NULL)
     return(FALSE);
   
   if (LogInsert("wwarning") == NULL)
     return(FALSE);
   
   if (LogInsert("werror") == NULL)
     return(FALSE);
   
   if (LogInsert("wtrace") == NULL)
     return(FALSE);
   
   if (LogInsert("wagenda") == NULL)
     return(FALSE);
   
   if ((stdin_log = LogInsert("stdin")) == NULL)
     return(FALSE);
   
   if (LogInsert("stdout") == NULL)
     return(FALSE);
   
   if ((xagenda_log = LogInsert("xagenda")) == NULL)
     return(FALSE);
   
   if ((xfacts_log = LogInsert("xfacts")) == NULL)
     return(FALSE);
   
   if ((xinstances_log =  LogInsert("xinstances")) == NULL)
     return(FALSE);
   
   if ((xglobals_log =  LogInsert("xglobals")) == NULL)
     return(FALSE);
   
   if ((xfocus_log =  LogInsert("xfocus")) == NULL)
     return(FALSE);
   
   return(TRUE);
  }

/*******************************************************************************
          Name:        ReleaseLogTable
          Description: 
          Arguments:   None 
          Returns:       
*******************************************************************************/
static void ReleaseLogTable()
  {
  register int i;
  LogNamePtr ptr1, ptr2;

  for(i = 0; i < LOG_TABLE_SIZE; i++)
    {
    ptr1 = log_table[i];
    while(ptr1 != NULL)
      {
      ptr2 = ptr1;
      ptr1 = ptr1->next;
      release(ptr2->name);
      release(ptr2);
      } 
    log_table[i] = NULL;
    }
  }

/*******************************************************************************
          Name:        LogInsert
          Description: Insert a new logical name in the log table
          Arguments:   logname -
          Returns:       
*******************************************************************************/
static LogNamePtr LogInsert(
  char *logname)
  {
  register int i;
  LogNamePtr node, ptr;

  i = GetHashValue(logname, LOG_TABLE_SIZE);

  if((node = balloc(1, LogName)) == NULL)
    return(NULL);
  if((node->name = balloc (strlen(logname)+1, char)) == NULL)
    {
    release(node);
    return(NULL);
    }
  strcpy(node->name, logname);
  node->next = NULL;
  if(log_table[i] == NULL)
    log_table[i] = node;
  else
    {
    ptr = log_table[i];
    while(ptr != NULL)
      {
      if(strcmp(ptr->name, logname) == 0)
        {
        release(node->name);
        release(node);
        return(NULL);
        }
      if(ptr->next == NULL)
      break;
      ptr = ptr->next;
      }
    ptr->next = node;
    }
  return(node);
  }

/*******************************************************************************
          Name:        LogLookup
          Description: Lookup in the log table for the recognizable logical name
          Arguments:   logname -
          Returns:       
*******************************************************************************/
static LogNamePtr LogLookup(
  char *logname)
  {
   LogNamePtr ptr;

   ptr = log_table[GetHashValue(logname, LOG_TABLE_SIZE)];
   while (ptr != NULL)
     {
     if(strcmp(logname, ptr->name) == 0)
       return(ptr);
     ptr = ptr->next;
     }
  return(NULL);
 }

/*******************************************************************************
          Name:        GetHashValue
          Description: Hash generates an index into the hash table for a string
                       by the following algorithm:
                            For each character in the string, take its ascii
                            value multiplied by its position in the string, and
                            then add all these values together.  The hash index
                            is this final sum mod the hash table size.
          Arguments:   str
                       size 
          Returns:     hvalue - 
*******************************************************************************/
static int GetHashValue(
  char *str,
  int size)
  {
  register unsigned i;
  unsigned hvalue;

  for(i = 0, hvalue = 0; str[i] != EOS; i++)
    hvalue += ((unsigned)str[i])*(i+1);
  hvalue %= (unsigned)size;
  return((int)hvalue);
  }

/*******************************************************************************
          Name:        set_clips_command
          Description:  
          Arguments:   flag -
          Returns:       
*******************************************************************************/
int set_clips_command(
  int flag)
  {
  send_clips_command = flag;
  return 0;
  }

/*******************************************************************************
          Name:        get_clips_command
          Description:  
          Arguments:   None 
          Returns:       
*******************************************************************************/
int get_clips_command()
  {
  return(send_clips_command);
  }


/******************************************************************************
 * PeriodicUpdate:
 * Description : This function will be called by CLIPS periodically 
 *               while it processes a command. This will allow user
 *               to halt the execution, change options, turn on/off watch,
 *               update the the windows during CLIPS process
 ******************************************************************************
 */
static void PeriodicUpdate()
{
  periodicChecking = True;
  while(XtAppPending(app_con) != 0)
   {
     XtAppNextEvent(app_con, &TheEvent);
     XtDispatchEvent(&TheEvent);
   }

  periodicChecking = False;

  if((Browse_status[AGENDA_WIN] == True) ? (GetAgendaChanged() == CLIPS_TRUE) : FALSE)
      {
      PrintChangedAgenda();
      SetAgendaChanged(CLIPS_FALSE);
      }
  if((Browse_status[FACT_WIN] == True) ? (GetFactListChanged() == CLIPS_TRUE) : FALSE)
      {
      PrintChangedFacts();
      SetFactListChanged(CLIPS_FALSE);
      }
 if((Browse_status[INSTANCE_WIN] == True) ? (GetInstancesChanged() == CLIPS_TRUE) : FALSE)
      {
      PrintChangedInstances();
      SetInstancesChanged(CLIPS_FALSE);
      }
 if((Browse_status[GLOBAL_WIN] == True) ? (GetGlobalsChanged() == CLIPS_TRUE) : FALSE)
      {
      PrintChangedGlobals();
      SetGlobalsChanged(CLIPS_FALSE);
      }
 if((Browse_status[FOCUS_WIN] == True) ? (GetFocusChanged(GetCurrentEnvironment()) == CLIPS_TRUE) :FALSE)
     {
      PrintChangedFocus();
      SetFocusChanged(GetCurrentEnvironment(),CLIPS_FALSE);
     }

}

