
package CLIPSJNI;

import CLIPSJNI.Environment;

public class TestHarness
  {
   /****************/
   /* TestHarness: */
   /****************/
   public TestHarness()
     {
      super();
     }

     
   /*********/
   /* main: */
   /*********/
   public static void main(String args[])
     {   
      for (int j = 0; j < 1000; j++) // 100
        {
         System.out.println("Iteraction: "+j);
         Environment clips = new Environment();
         
         for (int i = 0; i < 10000; i++) // 1000 
           {
            PrimitiveValue fv; 
            
            fv = clips.eval("(assert (dummy-fact " + i + "))");
                        
            String evalStr = "(find-all-facts ((?f dummy-fact)) TRUE)";
            PrimitiveValue pv = clips.eval(evalStr);
            
           if (i%10 == 0)
             { clips.reset(); }
           }
           
         clips.destroy();
        }

/*
      for (int i = 0; i < 10000; i++)
        {
         Environment clips = new Environment(); 
         clips.load("mabnp.clp");
         clips.reset();
         clips.run();
         clips.clear();
         clips.destroy();
        }
*/
     }  
  }
