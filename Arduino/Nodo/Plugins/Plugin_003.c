  
//#######################################################################################################
//#################################### Plugin-03: NewKAKU ###############################################
//#######################################################################################################

/*********************************************************************************************\
 * Dit protocol zorgt voor ontvangst Klik-Aan-Klik-Uit zenders
 * die werken volgens de automatische codering (Ontvangers met leer-knop) Dit protocol is 
 * eveneens bekend onder de naam HomeEasy. Het protocol ondersteunt eveneens dim funktionaliteit.
 * 
 * Auteur             : Nodo-team (P.K.Tonkes) www.nodo-domotca.nl
 * Support            : www.nodo-domotica.nl
 * Datum              : Jan.2013
 * Versie             : 1.1
 * Nodo productnummer : n.v.t. meegeleverd met Nodo code.
 * Compatibiliteit    : Vanaf Nodo build nummer 508
 * Compiled size      : ??? bytes voor een Mega, ??? voor een Small
 * Syntax             : "NewKAKU <Adres>,<On|Off|dimlevel 1..15>
 ***********************************************************************************************
 * Vereiste library   : - geen -
 * Externe funkties   : strcat(),  strcpy(),  cmd2str(),  tolower(),  GetArgv(),  str2cmd()
 *
 * Pulse (T) is 275us PDM
 * 0 = T,T,T,4T, 1 = T,4T,T,T, dim = T,T,T,T op bit 27
 *
 * NewKAKU ondersteund:
 *   on/off       ---- 000x Off/On
 *   all on/off   ---- 001x AllOff/AllOn
 *   dim absolute xxxx 0110 Dim16        // dim op bit 27 + 4 extra bits voor dim level
 *
 *  NewKAKU bitstream= (First sent) AAAAAAAAAAAAAAAAAAAAAAAAAACCUUUU(LLLL) -> A=KAKU_adres, C=commando, U=KAKU-Unit, L=extra dimlevel bits (optioneel)
 *
 * Interne gebruik van de parameters in het Nodo event:
 * 
 * Cmd  : Hier zit het commando SendNewKAKU of het event NewKAKU in. Deze gebruiken we verder niet.
 * Par1 : Commando VALUE_ON, VALUE_OFF of dim niveau [1..15]
 * Par2 : Adres
 *
 \*********************************************************************************************/

#define PLUGIN_ID 03
#define PLUGIN_NAME "NewKAKU"
#define NewKAKU_RawSignalLength      132
#define NewKAKUdim_RawSignalLength   148
#define NewKAKU_1T                   275        // us
#define NewKAKU_mT                   500        // us, midden tussen 1T en 4T 
#define NewKAKU_4T                  1100        // us
#define NewKAKU_8T                  2200        // us, Tijd van de space na de startbit

boolean Plugin_003(byte function, struct NodoEventStruct *event, char *string)
  {
  boolean success=false;
  
  switch(function)
    {
    #ifdef PLUGIN_003_CORE
    case PLUGIN_RAWSIGNAL_IN:
      {
      unsigned long bitstream=0L;
      boolean Bit;
      int i;
      int P0,P1,P2,P3;
      event->Par1=0;
      
      // nieuwe KAKU bestaat altijd uit start bit + 32 bits + evt 4 dim bits. Ongelijk, dan geen NewKAKU
      if (RawSignal.Number==NewKAKU_RawSignalLength || RawSignal.Number==NewKAKUdim_RawSignalLength)
        {
        // RawSignal.Number bevat aantal pulsRawSignal.Multiplyen * 2  => negeren
        // RawSignal.Pulses[1] bevat startbit met tijdsduur van 1T => negeren
        // RawSignal.Pulses[2] bevat lange space na startbit met tijdsduur van 8T => negeren
        i=3; // RawSignal.Pulses[3] is de eerste van een T,xT,T,xT combinatie
        
        do 
          {
          P0=RawSignal.Pulses[i]    * RawSignal.Multiply;
          P1=RawSignal.Pulses[i+1]  * RawSignal.Multiply;
          P2=RawSignal.Pulses[i+2]  * RawSignal.Multiply;
          P3=RawSignal.Pulses[i+3]  * RawSignal.Multiply;
          
          if     (P0<NewKAKU_mT && P1<NewKAKU_mT && P2<NewKAKU_mT && P3>NewKAKU_mT)Bit=0; // T,T,T,4T
          else if(P0<NewKAKU_mT && P1>NewKAKU_mT && P2<NewKAKU_mT && P3<NewKAKU_mT)Bit=1; // T,4T,T,T
          else if(P0<NewKAKU_mT && P1<NewKAKU_mT && P2<NewKAKU_mT && P3<NewKAKU_mT)       // T,T,T,T Deze hoort te zitten op i=111 want: 27e NewKAKU bit maal 4 plus 2 posities voor startbit
            {
            if(RawSignal.Number!=NewKAKUdim_RawSignalLength) // als de dim-bits er niet zijn
              return false;
            }
          else
            return false; // andere mogelijkheden zijn niet geldig in NewKAKU signaal.  
            
          if(i<130) // alle bits die tot de 32-bit pulstrein behoren 32bits * 4posities per bit + pulse/space voor startbit
            bitstream=(bitstream<<1) | Bit;
          else // de resterende vier bits die tot het dimlevel behoren 
            event->Par1=(event->Par1<<1) | Bit;
       
          i+=4;// volgende pulsenquartet
          }while(i<RawSignal.Number-2); //-2 omdat de space/pulse van de stopbit geen deel meer van signaal uit maakt.
            
        // Adres deel:
        if(bitstream>0xffff)                         // Is het signaal van een originele KAKU zender afkomstig, of van een Nodo ingegeven door de gebruiker ?
          // Oude Nodo compatibel
          event->Par2=bitstream &0x0FFFFFCF;         // Op hoogste nibble zat vroeger het signaaltype. 
          // event->Par2=bitstream &0xFFFFFFCF;         // dan hele adres incl. unitnummer overnemen. Alleen de twee commando-bits worden er uit gefilterd
        
        else                                         // Het is van een andere Nodo afkomstig. 
          event->Par2=(bitstream>>6)&0xff;           // Neem dan alleen 8bit v/h adresdeel van KAKU signaal over
          
        // Commando en Dim deel
        if(i>140)
          event->Par1++;                             // Dim level. +1 omdat gebruiker dim level begint bij Ã©Ã©n.
        else
          event->Par1=((bitstream>>4)&0x01)?VALUE_ON:VALUE_OFF; // On/Off bit omzetten naar een Nodo waarde. 
        event->SourceUnit    = 0;                     // Komt niet van een Nodo unit af, dus unit op nul zetten
        RawSignal.Repeats    = true;                  // het is een herhalend signaal. Bij ontvangst herhalingen onderdrukken.
        event->Type          = NODO_TYPE_PLUGIN_EVENT;
        event->Command       = 3; // nummer van dit device
        success=true;
        }   
      break;
      }
      
    case PLUGIN_COMMAND:
      break;
    #endif // CORE
      
    #if NODO_MEGA
    case PLUGIN_MMI_IN:
      {
      char* str=(char*)malloc(INPUT_COMMAND_SIZE);
    
      if(GetArgv(string,str,1))
        {
        if(strcasecmp(str,PLUGIN_NAME)==0)
          {
          if(GetArgv(string,str,2))
            {
            event->Par2=str2int(str);    
            if(GetArgv(string,str,3))
              {
              event->Type=NODO_TYPE_PLUGIN_EVENT;
              // Vul Par1 met het KAKU commando. Dit kan zijn: VALUE_ON, VALUE_OFF, 1..16. Andere waarden zijn ongeldig.
              
              // haal uit de tweede parameter een 'On' of een 'Off'.
              if(event->Par1=str2cmd(str))
                success=true;
                
              // als dit niet is gelukt, dan uit de tweede parameter de dimwaarde halen.
              else
                {
                event->Par1=str2int(str);             // zet string om in integer waarde
                if(event->Par1>=1 && event->Par1<=16) // geldig dim bereik 1..16 ?
                   success=true;
                }
              event->Command = 3; // Plugin nummer  
              }
            }
          }
        }
      free(str);
      break;
      }

    case PLUGIN_MMI_OUT:
      {
      strcpy(string,PLUGIN_NAME);            // Eerste argument=het commando deel
      strcat(string," ");
    
      // In Par3 twee mogelijkheden: Het bevat een door gebruiker ingegeven adres 0..255 of een volledig NewKAKU adres.
      if(event->Par2>=0x0ff)
        strcat(string,int2strhex(event->Par2)); 
      else
        strcat(string,int2str(event->Par2)); 
      
      strcat(string,",");
      
      if(event->Par1==VALUE_ON)
        strcat(string,"On");  
      else if(event->Par1==VALUE_OFF)
        strcat(string,"Off");
      else
        strcat(string,int2str(event->Par1));

      break;
      }
    #endif //MMI
    }      
  return success;
  }

