
 /*********************************************************************************************\
 * wachtloop die wordt afgebroken als:
 * - <Timeout> seconden zijn voorbij. In dit geval geeft deze funktie een <false> terug.
 * - <QueueTime> opgegeven en tijd is verstreken na laatst ontvangen event
 * - <BreakNoBusyNodo>==true en alle Nodo's na een "Busy On" ook weer een "Busy Off" hebben verzonden.
 * - <BreakEvent> is opgegeven en dit event is ontvangen.
 * - de Nodo expliciet een "Queue Off" opdracht heeft ontvangen.
 \*********************************************************************************************/
boolean WaitAndQueue(int Timeout, boolean BreakNoBusyNodo, unsigned long BreakEvent)
  {
  unsigned long TimeoutTimer=millis() + (unsigned long)(Timeout)*1000;
  unsigned long TimeoutMinTimer=millis() + 500L; // deze timer nodig omdat de RF ontvangers na inschakelen tijd nodig hebben om signaal te kunnen ontvangen. Zolang blijft de loop minimaal lopen.
  unsigned long Event;
  int Port,x,y,z;
  
  Led(BLUE);
  
  #if NODO_MEGA
  if(S.Debug==VALUE_ON)
    PrintTerminal(ProgmemString(Text_24));
  #endif

  while(TimeoutTimer>millis())
    {
    if(GetEvent_IRRF(&Event,&Port))
      {
      TimeoutTimer=millis() + (unsigned long)(Timeout)*1000;
      #if NODO_MEGA
      if(S.Debug==VALUE_ON)
          PrintEvent(Event,Port,VALUE_DIRECTION_INPUT);
      #endif

      if(BreakEvent!=0 && Event==BreakEvent)
        break;

      x=(byte)((Event>>16)&0xff); // cmd
      y=(byte)((Event>>24)&0xf); // unit
      z=(byte)((Event>>8)&0xff); // par1
      
      if(x==CMD_BUSY) // command
        {
        NodoBusy(Event,false);          
        
        if(BreakNoBusyNodo && NodoBusyStatus==0)
          break; // Geen Busy Nodo meer
        }
        
      else if(x==CMD_QUEUE) // command
        {
        if(y==S.Unit) // Als commando voor deze unit bestemd
          {
          if(z==VALUE_OFF) // Par1
            break;// Queue Off commando ontgangen.
          }
        }
  
      // Het is geen Busy event of Queue commando event, dan deze in de queue plaatsen.
      #if NODO_MEGA
      else
        AddFileSDCard(ProgmemString(Text_15),int2str(Event));       // Sla event op in de queue. De Nodo-Mega slaat de queue op SDCard op.
      #else
      // sla event op in de queue. De Nodo-Mini heeft de queue is een kleine array <QueueEvent> staan.
      else if(QueuePos<EVENT_QUEUE_MAX)
        {
        QueueEvent[QueuePos]=Event;
        QueuePort[QueuePos]=Port;
        QueuePos++;           
        }       
      else
        RaiseMessage(MESSAGE_04);    
      #endif
      }    
    }   

  if(TimeoutTimer<=millis())
    return false;

  return true;
  }
  

 /*********************************************************************************************\
 * Controleert of er een Nodo busy is.
 * <Event> event t.b.v. actualiseren status.
 * <Wait> als true, dan wordt gewacht tot alle Nodo vrij zijn.
 * geeft een true terug als er nog een Nodo busy is.
 \*********************************************************************************************/
boolean NodoBusy(unsigned long Event, boolean Wait)
  {
  int PreviousNodoBusyStatus=NodoBusyStatus;
  
  if(Event)
    {
    // Check of het event een Busy xxx event is en zet de betreffende status van de Nodo
    if(((Event>>16)&0xff)==CMD_BUSY) // command
      {
      if(((Event>>8)&0xff)==VALUE_ON) // Par1
        NodoBusyStatus  |=  (1<<((Event>>24)&0xf)); // unit nummer
      else
        NodoBusyStatus  &= ~(1<<((Event>>24)&0xf)); // unit nummer
      }
    }

  if(Wait && NodoBusyStatus!=0)
    {
    #if NODO_MEGA
    // Geef weer op welke Nodo(s) de timeout heeft plaatsgevonden
    strcpy(TempString, ProgmemString(Text_06));
    for(byte x=1;x<=UNIT_MAX;x++)
      {
      if((NodoBusyStatus>>x)&0x1)
        {
        strcat(TempString, int2str(x));  
        strcat(TempString, "  ");         
        }
      }
    PrintTerminal(TempString);
//???    TransmitCode(command2event(S.Unit, CMD_MESSAGE ,MESSAGE_14,0),VALUE_SOURCE_HTTP);// geen RaiseMessage() anders weer poging om te loggen naar SDCard ==> oneindige loop
    #endif

    if(!WaitAndQueue(30,true,0))
      {
      RaiseMessage(MESSAGE_13);
      NodoBusyStatus=0;
      }
    }

  // Als de laatste Nodo vrij is, dan een korte pause zodat zeker alle ontvangers weer paraat staan
  if(NodoBusyStatus==0 && NodoBusyStatus!=PreviousNodoBusyStatus)
    delay(500);

  return NodoBusyStatus!=0;
  }


 /*********************************************************************************************\
 * Zet het hoogste nibbel van een event zodat het event het Type kenmerk krijgt
 \*********************************************************************************************/
unsigned long SetEventType(unsigned long Event, byte Type)
  {
  return (Event&0x0fffffff) | (unsigned long)(Type)<<28;
  }



 /*********************************************************************************************\
 * (re)set een timer. Eenheden in seconden. 
 * Timer 1..15. Timer=0 is een wildcard voor alle timers
 * Als de timer op 0 wordt gezet, wordt er geen event gegenereerd.
 \*********************************************************************************************/
void TimerSet(byte Timer, int Time)
  {
  if(Timer==0)// 0=wildcard voor alle timers
    {
    for(int x=0;x<TIMER_MAX;x++)
      {
      if(Time==0)
        UserTimer[x]=0L;
      else
        UserTimer[x]=millis()+(unsigned long)(Time)*1000;
      }
    }
  else
    {
    if(Time==0)
      UserTimer[Timer-1]=0L;
    else
      UserTimer[Timer-1]=millis()+(unsigned long)Time*1000;
    }
  }

 /*********************************************************************************************\
 * Haal voor het opgegeven Command de status op.
 * Let op: call by reference!
 \*********************************************************************************************/
boolean GetStatus(byte *Command, byte *Par1, byte *Par2)
 { 
  int x;
  byte xPar1=*Par1,xPar2=*Par2;
  unsigned long event,UL;

  *Par1=0;
  *Par2=0;
  switch (*Command)
    {
    case VALUE_EVENTLIST_COUNT:
      for(x=1; x<=EVENTLIST_MAX;x++)
        if(Eventlist_Read(x,&event,&UL))// event en UL worden hier gebruikt als dummy
          *Par1=*Par1+1;
      break;

    case CMD_WAITFREERF: 
      *Par1=S.WaitFreeRF_Delay;
      *Par2=S.WaitFreeRF_Window;
      break;
  
    case CMD_UNIT: 
      *Par1=S.Unit;
      break;        

    case CMD_SENDBUSY:
      *Par1=S.SendBusy;
      break;

    case CMD_DEBUG:
      *Par1=S.Debug;
      break;

    case CMD_LOCK:
      *Par1=S.Lock==0?0:0x80;// uit 16-bit combi van Par1+Par2 staat de on/off in bit 15.
      break;

    case CMD_WAITBUSY:
      *Par1=S.WaitBusy?VALUE_ALL:VALUE_OFF;
      break;
                  
    case CMD_ANALYSE_SETTINGS:
      *Par1=S.AnalyseTimeOut/1000;
      *Par2=S.AnalyseSharpness;
      break;

    case CMD_TRANSMIT_RF:
      *Par1=S.TransmitRF;
      *Par2=S.TransmitRepeatRF;
      break;
      
    case CMD_TRANSMIT_IR:
      *Par1=S.TransmitIR;
      *Par2=S.TransmitRepeatIR;
      break;

    case CMD_CLOCK_EVENT_DAYLIGHT:
      *Par1=Time.Daylight;
      break;

    case CMD_VARIABLE_SET:
      event=AnalogInt2event(UserVar[xPar1-1], xPar1, 0);
      *Par1=(event>>8)&0xff;
      *Par2=event&0xff;
      break;

    case CMD_CLOCK_DATE:
      if(!bitRead(HW_Config,HW_CLOCK))return false;
      *Par1=Time.Date;
      *Par2=Time.Month;
      break;

    case CMD_CLOCK_TIME:
      if(!bitRead(HW_Config,HW_CLOCK))return false;
      *Par1=Time.Hour;
      *Par2=Time.Minutes;
      break;

    case CMD_CLOCK_DOW:
      if(!bitRead(HW_Config,HW_CLOCK))return false;
      *Par1=Time.Day;
      break;

    case CMD_CLOCK_YEAR:
      if(!bitRead(HW_Config,HW_CLOCK))return false;
      *Par1=Time.Year/100;
      *Par2=Time.Year-2000;
      break;

    case CMD_TIMER_SET_MIN:
      *Par1=xPar1;
      if(UserTimer[xPar1-1])
        *Par2=(byte)((UserTimer[xPar1-1]-millis()+59999UL)/60000UL);
      else
        *Par2=0;
      break;

    case CMD_WIRED_PULLUP:
      *Par1=xPar1;
      *Par2=S.WiredInputPullUp[xPar1-1];
      break;

    case CMD_WIRED_ANALOG:
      // lees analoge waarde. Dit is een 10-bit waarde, unsigned 0..1023
      // vervolgens met map() omrekenen naar gekalibreerde waarde        
      x=map(analogRead(PIN_WIRED_IN_1+xPar1-1),S.WiredInput_Calibration_IL[xPar1-1],S.WiredInput_Calibration_IH[xPar1-1],S.WiredInput_Calibration_OL[xPar1-1],S.WiredInput_Calibration_OH[xPar1-1]);
      event=AnalogInt2event(x, xPar1, CMD_WIRED_ANALOG);
      *Par1=(byte)((event>>8) & 0xff);
      *Par2=(byte)(event & 0xff);
      break;

    case CMD_WIRED_THRESHOLD:
      event=AnalogInt2event(S.WiredInputThreshold[xPar1-1], xPar1,0);
      *Par1=(byte)((event>>8) & 0xff);
      *Par2=(byte)(event & 0xff);
      break;

    case CMD_WIRED_SMITTTRIGGER:
      event=AnalogInt2event(S.WiredInputSmittTrigger[xPar1-1], xPar1,0);
      *Par1=(byte)((event>>8) & 0xff);
      *Par2=(byte)(event & 0xff);
      break;

    case CMD_WIRED_IN_EVENT:
      *Par1=xPar1;
      *Par2=(WiredInputStatus[xPar1-1])?VALUE_ON:VALUE_OFF;
      break;

    case CMD_WIRED_OUT:
      *Par1=xPar1;
      *Par2=(WiredOutputStatus[xPar1-1])?VALUE_ON:VALUE_OFF;
      break;

#if NODO_MEGA
    case CMD_TRANSMIT_IP:
      *Par1=S.TransmitIP;
      if(S.TransmitIP==VALUE_SOURCE_HTTP)
        *Par2=S.HTTP_Pin;
      if(S.TransmitIP==VALUE_SOURCE_EVENTGHOST)
        *Par2=S.AutoSaveEventGhostIP;
      break;

    // pro-forma de commando's die geen fout op mogen leveren omdat deze elders in de statusafhandeling worden weergegeven
    case CMD_NODO_IP:
    case CMD_GATEWAY:
    case CMD_SUBNET:
    case CMD_DNS_SERVER:
    case CMD_PORT_SERVER:
    case CMD_PORT_CLIENT:
    case CMD_HTTP_REQUEST:
    case CMD_ID:
    case CMD_EVENTGHOST_SERVER:
      *Par1=0;
      *Par2=0;
      break;
#endif      
    default:
      return false;
    }
  return true;
  }


 /**********************************************************************************************\
 * Deze functie haalt een tekst op uit PROGMEM en geeft als string terug
 \*********************************************************************************************/
char* ProgmemString(prog_char* text)
    {
    byte x=0;
    static char buffer[40];
  
    do
      {
      buffer[x]=pgm_read_byte_near(text+x);
      }while(buffer[x++]!=0);
    return buffer;
    }
  
  
   /*********************************************************************************************\
   * Sla alle settings op in het EEPROM geheugen.
   \*********************************************************************************************/
  void SaveSettings(void)  
    {
    char ByteToSave,*pointerToByteToSave=pointerToByteToSave=(char*)&S;    //pointer verwijst nu naar startadres van de struct. 
    
    for(int x=0; x<sizeof(struct Settings) ;x++)
      {
      EEPROM.write(x,*pointerToByteToSave); 
      pointerToByteToSave++;
      }  
    }

 /*********************************************************************************************\
 * Laad de settings uit het EEPROM geheugen.
 \*********************************************************************************************/
boolean LoadSettings()
  {
  byte x;
    
  char ByteToSave,*pointerToByteToRead=(char*)&S;    //pointer verwijst nu naar startadres van de struct.

  for(int x=0; x<sizeof(struct Settings);x++)
    {
    *pointerToByteToRead=EEPROM.read(x);
    pointerToByteToRead++;// volgende byte uit de struct
    }
    
  for(x=0;x<USER_VARIABLES_MAX;x++)
    UserVar[x]=S.UserVar[x];
      
  }
 
  
 /*********************************************************************************************\
 * Alle settings van de Nodo weer op default.
 \*********************************************************************************************/
void ResetFactory(void)
  {
  Led(BLUE);
  Beep(2000,2000);
  int x,y;
  ClockRead(); 

  S.Version                    = SETTINGS_VERSION;
  S.Unit                       = UNIT_DEFAULT;
  S.Lock                       = 0;
  S.AnalyseSharpness           = 50;
  S.AnalyseTimeOut             = SIGNAL_TIMEOUT_IR;
  S.TransmitIR                 = VALUE_OFF;
  S.TransmitRF                 = VALUE_ON;
  S.TransmitIP                 = VALUE_OFF;
  S.TransmitRepeatIR           = TX_REPEATS;
  S.TransmitRepeatRF           = TX_REPEATS;
  S.SendBusy                   = VALUE_OFF;
  S.WaitBusy                   = VALUE_ON;
  S.WaitFreeRF_Delay           = (S.Unit!=1)?S.Unit:0;
  S.WaitFreeRF_Window          = (S.Unit!=1)?1:0; // 1 eenheid = 100 ms.
  S.WaitFreeRF_Window          = 0;
  S.WaitFreeRF_Delay           = 0;
  S.DaylightSaving             = Time.DaylightSaving;
  S.Debug                      = VALUE_OFF;

#if NODO_MEGA
  S.AutoSaveEventGhostIP       = VALUE_OFF;
  S.EventGhostServer_IP[0]     = 0; // IP adres van de EventGhost server
  S.EventGhostServer_IP[1]     = 0; // IP adres van de EventGhost server
  S.EventGhostServer_IP[2]     = 0; // IP adres van de EventGhost server
  S.EventGhostServer_IP[3]     = 0; // IP adres van de EventGhost server
  S.HTTPRequest[0]             = 0; // string van het HTTP adres leeg maken
  S.Client_IP[0]               = 0;
  S.Client_IP[1]               = 0;
  S.Client_IP[2]               = 0;
  S.Client_IP[3]               = 0;
  S.Nodo_IP[0]                 = 0;
  S.Nodo_IP[1]                 = 0;
  S.Nodo_IP[2]                 = 0;
  S.Nodo_IP[3]                 = 0;
  S.Gateway[0]                 = 0;
  S.Gateway[1]                 = 0;
  S.Gateway[2]                 = 0;
  S.Gateway[3]                 = 0;
  S.Subnet[0]                  = 255;
  S.Subnet[1]                  = 255;
  S.Subnet[2]                  = 255;
  S.Subnet[3]                  = 0;
  S.DnsServer[0]               = 0;
  S.DnsServer[1]               = 0;
  S.DnsServer[2]               = 0;
  S.DnsServer[3]               = 0;
  S.PortServer                 = 8080;
  S.PortClient                 = 80;
  S.ID[0]                      = 0; // string leegmaken
  S.HTTP_Pin                   = VALUE_OFF;
  S.EchoSerial                 = VALUE_ON;
  S.EchoTelnet                 = VALUE_ON;
  
  strcpy(S.Password,ProgmemString(Text_10));
#endif

  // zet analoge waarden op default
  for(x=0;x<WIRED_PORTS;x++)
    {
    S.WiredInputThreshold[x]=5000; 
    S.WiredInputSmittTrigger[x]=500;
    S.WiredInputPullUp[x]=VALUE_ON;
    S.WiredInput_Calibration_IH[x]=1023;
    S.WiredInput_Calibration_IL[x]=0;
    S.WiredInput_Calibration_OH[x]=10000;
    S.WiredInput_Calibration_OL[x]=0;
    }

  // maak alle variabelen leeg
  for(byte x=0;x<USER_VARIABLES_MAX;x++)
     S.UserVar[x]=0;

  SaveSettings();  
  FactoryEventlist();
  delay(250);// kleine pauze, anders kans fout bij seriële communicatie
  Reset();
  }
  
 /**********************************************************************************************\
 * Maak de Eventlist leeg en herstel de default inhoud
 \*********************************************************************************************/
void FactoryEventlist(void)
  {
  // maak de eventlist leeg.
  for(int x=1;x<=EVENTLIST_MAX;x++)
    Eventlist_Write(x,0L,0L);

#if NODO_MEGA
  // schrijf default regels.
  Eventlist_Write(0,command2event(S.Unit,CMD_BOOT_EVENT,S.Unit,0),command2event(S.Unit,CMD_SOUND,7,0)); // geluidssignaal na opstarten Nodo
  Eventlist_Write(0,command2event(S.Unit,CMD_COMMAND_WILDCARD,VALUE_SOURCE_IR,CMD_KAKU),command2event(S.Unit,CMD_RAWSIGNAL_SEND,0,0)); 
#else
  Eventlist_Write(0,command2event(S.Unit,CMD_BOOT_EVENT,S.Unit,0),command2event(S.Unit,CMD_SOUND,7,0)); // geluidssignaal na opstarten Nodo
#endif
  }

  

/* This function places the current value of the heap and stack pointers in the
 * variables. You can call it from any place in your code and save the data for
 * outputting or displaying later. This allows you to check at different parts of
 * your program flow.
 * The stack pointer starts at the top of RAM and grows downwards. The heap pointer
 * starts just above the static variables etc. and grows upwards. SP should always
 * be larger than HP or you'll be in big trouble! The smaller the gap, the more
 * careful you need to be. Julian Gall 6-Feb-2009.
 */
uint8_t *heapptr, *stackptr;
void FreeMemory(int x) 
  {
  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
  Serial.print("*** debug: Free RAM (");Serial.print(x,DEC);Serial.print(")=");Serial.println(stackptr-heapptr,DEC);
  }


 /**********************************************************************************************\
  * Geeft de status weer of verzendt deze.
  * verzend de status van een setting als event.
  * Par1 = Command
  * Par2 = selectie (poort, variabele, timer) Als nul, dan waldcard voor alle.
 \**********************************************************************************************/
void Status(byte Par1, byte Par2, boolean Transmit)
  {
  byte CMD_Start,CMD_End;
  byte Par1_Start,Par1_End;
  byte x,P1,P2; // in deze variabele wordt de waarde geplaats (call by reference)
  boolean s;
  
  if(Par1==0)
    return;
    
  if(Par2==VALUE_ALL)
    Par2==0;

  if(Par1==CMD_BOOT_EVENT)
    {
    PrintWelcome();
    return;
    }
    
  if(Par1==VALUE_ALL)
    {
    Par2=0;
    if(!Transmit)
      PrintWelcome();
    CMD_Start=FIRST_COMMAND;
    CMD_End=COMMAND_MAX;
    }
  else
    {
    if(!GetStatus(&Par1,&P1,&P2))// kijk of voor de opgegeven parameter de status opvraagbaar is. Zo niet dan klaar. Error in parameter???
      return;
    CMD_Start=Par1;
    CMD_End=Par1;
    }

  for(x=CMD_Start; x<=CMD_End; x++)
    {
    s=false;

#if NODO_MEGA          
    boolean dhcp=(S.Nodo_IP[0] + S.Nodo_IP[1] + S.Nodo_IP[2] + S.Nodo_IP[3])==0;
#endif

    if(!Transmit)
      {
      s=true;
      switch (x)
        {

   #if NODO_MEGA          
        case CMD_CLIENT_IP:
          sprintf(TempString,"%s %u.%u.%u.%u",cmd2str(CMD_CLIENT_IP),S.Client_IP[0],S.Client_IP[1],S.Client_IP[2],S.Client_IP[3]);
          PrintTerminal(TempString);
          break;

        case CMD_NODO_IP:
          sprintf(TempString,"%s %u.%u.%u.%u",cmd2str(CMD_NODO_IP), Ethernet.localIP()[0],Ethernet.localIP()[1],Ethernet.localIP()[2],Ethernet.localIP()[3]);
          if(dhcp)
            strcat(TempString," (DHCP)");
          PrintTerminal(TempString);
          break;

        case CMD_GATEWAY:
          // Gateway
          if(!dhcp)
            {
            sprintf(TempString,"%s %u.%u.%u.%u",cmd2str(CMD_GATEWAY),S.Gateway[0],S.Gateway[1],S.Gateway[2],S.Gateway[3]);
            PrintTerminal(TempString);
            }
          break;

        case CMD_SUBNET:
          // Subnetmask
          if(!dhcp)
            {
            sprintf(TempString,"%s %u.%u.%u.%u",cmd2str(CMD_SUBNET),S.Subnet[0],S.Subnet[1],S.Subnet[2],S.Subnet[3]);
            PrintTerminal(TempString);
            }
          break;

        case CMD_DNS_SERVER:
          if(!dhcp)
            {
            // DnsServer
            sprintf(TempString,"%s %u.%u.%u.%u",cmd2str(CMD_DNS_SERVER),S.DnsServer[0],S.DnsServer[1],S.DnsServer[2],S.DnsServer[3]);
            PrintTerminal(TempString);
            }
          break;

        case CMD_PORT_SERVER:
          sprintf(TempString,"%s %d",cmd2str(CMD_PORT_SERVER), S.PortServer);
          PrintTerminal(TempString);
          break;

        case CMD_PORT_CLIENT:
          sprintf(TempString,"%s %d",cmd2str(CMD_PORT_CLIENT), S.PortClient);
          PrintTerminal(TempString);
          break;

        case CMD_HTTP_REQUEST:
          sprintf(TempString,"%s %s",cmd2str(CMD_HTTP_REQUEST),S.HTTPRequest);
          PrintTerminal(TempString);
          break;

        case CMD_ID:
          sprintf(TempString,"%s %s",cmd2str(CMD_ID), S.ID);
          PrintTerminal(TempString);
          break;
          
        case CMD_EVENTGHOST_SERVER:
          // EvetGhost client IP
          sprintf(TempString,"%s %u.%u.%u.%u ",cmd2str(CMD_EVENTGHOST_SERVER),S.EventGhostServer_IP[0],S.EventGhostServer_IP[1],S.EventGhostServer_IP[2],S.EventGhostServer_IP[3]);
          if(S.AutoSaveEventGhostIP==VALUE_AUTO)
            strcat(TempString," (Auto)");      
          PrintTerminal(TempString);
          break;
#endif

        default:
          s=false; 
          break;
        }
      }
      
    if(!s && GetStatus(&x,&P1,&P2)) // Als het een geldige uitvraag is. let op: call by reference !
      {
      if(Par2==0) // Als in het commando 'Status Par1, Par2' Par2 niet is gevuld met een waarde
        {
        switch(x)
          {
          case CMD_WIRED_ANALOG:
          case CMD_WIRED_OUT:
          case CMD_WIRED_PULLUP:
          case CMD_WIRED_SMITTTRIGGER:
          case CMD_WIRED_THRESHOLD:
          case CMD_WIRED_IN_EVENT:
            Par1_Start=1;
            Par1_End=WIRED_PORTS;
            break;      

          case CMD_VARIABLE_SET:
            Par1_Start=1;
            Par1_End=USER_VARIABLES_MAX;
            break;

          case CMD_TIMER_SET_MIN:
            Par1_Start=1;
            Par1_End=TIMER_MAX;
            break;

          default:
            Par1_Start=0;
            Par1_End=0;
          }
        }
      else
        {
        Par1_Start=Par2;
        Par1_End=Par2;
        }

      for(byte y=Par1_Start;y<=Par1_End;y++)
          {
          P1=y;
          P2=0;
          GetStatus(&x,&P1,&P2); // haal status op. Call by Reference!
          if(Transmit)
            {
            TransmitCode(command2event(S.Unit,x,P1,P2),VALUE_ALL); // verzend als event
            }

#if NODO_MEGA
          else
            PrintTerminal(Event2str(command2event(S.Unit,x,P1,P2)));
#endif
          }
      }
    }
#if NODO_MEGA
  if(!Transmit && Par1==VALUE_ALL)
    PrintTerminal(ProgmemString(Text_22));
#endif
  }


/*********************************************************************************************\
* Deze routine parsed string en geeft het opgegeven argument nummer Argc terug in Argv
* argumenten worden van elkaar gescheiden door een komma of een spatie.
* Let op dat de ruimte in de doelstring voldoende is EN dat de bron string netjes is afgesloten 
* met een 0-byte.
\*********************************************************************************************/
boolean GetArgv(char *string, char *argv, byte argc)
  {
  int string_pos=0,argv_pos=0,argc_pos=0; 
  argc--;
  int c;

  while(string[string_pos]==32)string_pos++;   // sla voorloopspaties over
  
  while(string[string_pos]!=0 && argc_pos<=argc)
    {
    if(string[string_pos]==','|| string[string_pos]==32)
      {
      while(string[string_pos+1]==32)string_pos++;   // sla voorloopspaties over
      argc_pos++;
      }
    else if(string[string_pos]>=33 && string[string_pos]<=126 && argc_pos==argc)
      argv[argv_pos++]=string[string_pos];
      
    string_pos++;
    }    
  argv[argv_pos]=0;

  if(argv_pos>0)
    {
    argv[argv_pos]=0;
    return true;
    }
  else
    {
    argv[0]=0;
    return false;
    }
  }


/**********************************************************************************************\
 * Geef een geluidssignaal met toonhoogte van 'frequentie' in Herz en tijdsduur 'duration' in milliseconden.
 * LET OP: toonhoogte is slechts een grove indicatie. Deze routine is bedoeld als signaalfunctie
 * en is niet bruikbaar voor toepassingen waar de toonhoogte zuiver/exact moet zijn. Geen PWM.
 * Definieer de constante:
 * #define PIN_SPEAKER <LuidsprekerAansluiting>
 * Neem in setup() de volgende regel op:
 * pinMode(PIN_SPEAKER, OUTPUT);
 * Routine wordt verlaten na beeindiging van de pieptoon.
 * Revision 01, 13-02-2009, P.K.Tonkes@gmail.com
 \*********************************************************************************************/

void Beep(int frequency, int duration)//Herz,millisec 
  {
  long halfperiod=500000L/frequency;
  long loops=(long)duration*frequency/(long)1000;
  
  noInterrupts();
  for(loops;loops>0;loops--) 
    {
    digitalWrite(PIN_SPEAKER, HIGH);
    delayMicroseconds(halfperiod);
    digitalWrite(PIN_SPEAKER, LOW);
    delayMicroseconds(halfperiod);
    }
  interrupts();
  }
 
 /**********************************************************************************************\
 * Geeft een belsignaal.
 * Revision 01, 09-03-2009, P.K.Tonkes@gmail.com
 \*********************************************************************************************/
void Alarm(int Variant,int Option)
  {
   byte x,y;

   switch (Variant)
    { 
    case 1:// four beeps
      for(y=1;y<=(Option>1?Option:1);y++)
        {
        Beep(3000,30);
        delay(100);
        Beep(3000,30);
        delay(100);
        Beep(3000,30);
        delay(100);
        Beep(3000,30);
        delay(1000);
        }    
      break;

    case 2: // whoop up
      for(y=1;y<=(Option>1?Option:1);y++)
        {
        for(x=1;x<=50;x++)
            Beep(250*x/4,20);
        }          
      break;

    case 3: // whoop down
      for(y=1;y<=(Option>1?Option:1);y++)
        {
        for(x=50;x>0;x--)
            Beep(250*x/4,20);
        }          
      break;

    case 4:// S.O.S.
      for(y=1;y<=(Option>1?Option:1);y++)
        {
          Beep(1200,50);
          delay(100);
          Beep(1200,50);
          delay(100);
          Beep(1200,50);
          delay(200);
          Beep(1200,300);
          delay(100);
          Beep(1200,300);
          delay(100);
          Beep(1200,300);
          delay(200);
          Beep(1200,50);
          delay(100);
          Beep(1200,50);
          delay(100);
          Beep(1200,50);
          if(Option>1)delay(500);
        }
      break;
          
     case 5:// ding-dong
       for(x=1;x<=(Option>1?Option:1);x++)
         {
         if(x>1)delay(2000);
         Beep(1500,500);
         Beep(1200,500);
         }    
       break;

    case 6: // phone ring
      for(x=1;x<(15*(Option>1?Option:1));x++)
        {
          Beep(1000,40);
          Beep(750,40);
        }
        break;

    case 7: // boot
        Beep(1500,100);
        Beep(1000,100);
        break;

    default:// beep
       if(Variant==0)
          Variant=5; // tijdsduur
          
       if(Option==0)
         Option=20; // toonhoogte

       Beep(100*Option,Variant*10);
       break;
    }
  }


/*********************************************************************************************\
* Deze routine parsed string en zoekt naar keyword. Geeft de startpositie terug waar het keyword
* gevonden is. -1 indien niet gevonden. Niet casesensitive.
\*********************************************************************************************/
int StringFind(char *string, char *keyword)
  {
  int x,y;
  int keyword_len=strlen(keyword);
  int string_len=strlen(string);
  
  if(keyword_len>string_len) // doe geen moeite als het te vinden eyword langer is dan de string.
    return -1;
  
  for(x=0; x<=(string_len-keyword_len); x++)
    {
    y=0;
    while(y<keyword_len && (tolower(string[x+y])==tolower(keyword[y])))
      y++;

    if(y==keyword_len)
      return x;
    }
  return -1;
  }

 /**********************************************************************************************\
 * Schrijft een event in de Eventlist. Deze Eventlist bevindt zich in het EEPROM geheugen.
 \*********************************************************************************************/
boolean Eventlist_Write(int address, unsigned long Event, unsigned long Action)// LET OP: eerste adres=1
  {
  unsigned long TempEvent,TempAction;

  // als adres=0, zoek dan de eerste vrije plaats.
  if(address==0)
    {
    address++;
    while(Eventlist_Read(address,&TempEvent,&TempAction))address++;
    }
    
  if(address>EVENTLIST_MAX)
    return false;// geen geldig adres meer c.q. alles is vol.

  address--;// echte adressering begint vanaf nul. voor de user vanaf 1.
  address=address*8+sizeof(struct Settings);     // Eerste deel van het EEPROM geheugen is voor de settings. Reserveer deze bytes. Deze niet te gebruiken voor de Eventlist!

  EEPROM.write(address++,(Event>>24  & 0xFF));
  EEPROM.write(address++,(Event>>16  & 0xFF));
  EEPROM.write(address++,(Event>> 8  & 0xFF));
  EEPROM.write(address++,(Event      & 0xFF));
  EEPROM.write(address++,(Action>>24 & 0xFF));
  EEPROM.write(address++,(Action>>16 & 0xFF));
  EEPROM.write(address++,(Action>> 8 & 0xFF));
  EEPROM.write(address++,(Action     & 0xFF));

  return true;
  }
  
 /**********************************************************************************************\
 * 
 * Revision 01, 09-12-2009, P.K.Tonkes@gmail.com
 \*********************************************************************************************/
boolean Eventlist_Read(int address, unsigned long *Event, unsigned long *Action)// LET OP: eerste adres=1
  {
  if(address>EVENTLIST_MAX)return(false);

  address--;// echte adressering begint vanaf nul. voor de user vanaf 1.
  address=address*8+sizeof(struct Settings);     // Eerste deel van het EEPROM geheugen is voor de settings. Reserveer deze bytes. Deze niet te gebruiken voor de Eventlist!

  *Event  =((unsigned long)(EEPROM.read(address++))) << 24;
  *Event|= ((unsigned long)(EEPROM.read(address++))) << 16;
  *Event|= ((unsigned long)(EEPROM.read(address++))) <<  8;
  *Event|= ((unsigned long)(EEPROM.read(address++)))      ;
  *Action =((unsigned long)(EEPROM.read(address++))) << 24;
  *Action|=((unsigned long)(EEPROM.read(address++))) << 16;
  *Action|=((unsigned long)(EEPROM.read(address++))) <<  8;
  *Action|=((unsigned long)(EEPROM.read(address  )))      ;
  if(*Event==0L)
    return(false);
  else
    return(true);
  }


void RaiseMessage(byte MessageCode)
  {
  unsigned long eventcode;
  eventcode=command2event(S.Unit,CMD_MESSAGE, S.Unit, MessageCode);
  PrintEvent(eventcode,VALUE_DIRECTION_INTERNAL,VALUE_SOURCE_SYSTEM);  // geef event weer op Serial
  TransmitCode(eventcode,VALUE_ALL);
  }
    

 /**********************************************************************************************\
 * Stuur de RGB-led.
 *
 * Voor de Nodo geldt:
 *
 * Groen = Nodo in rust en wacht op een event.
 * Rood = Nodo verwerkt event of commando.
 * Blauw = Bijzondere modus Nodo waarin Nodo niet in staat is om events te ontvangen of genereren.
 \*********************************************************************************************/
void Led(byte Color)
  {
  digitalWrite(PIN_LED_RGB_R,Color==RED);
  digitalWrite(PIN_LED_RGB_B,Color==BLUE);
#if NODO_MEGA
  digitalWrite(PIN_LED_RGB_G,Color==GREEN);
#endif
  }
  
void PulseCounterISR()
   {
   // in deze interrupt service routine staat millis() stil. Dit is echter geen bezwaar voor de meting.
   PulseTime=millis()-PulseTimePrevious;
   PulseTimePrevious=millis();
   PulseCount++;   
   }     

#if NODO_MEGA
 /**********************************************************************************************\
 * Voeg een regel toe aan de logfile.
 \*********************************************************************************************/
boolean AddFileSDCard(char *FileName, char *Line)
  {
  boolean r;

  // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer SDCard chip
  digitalWrite(Ethernetshield_CS_W5100, HIGH);
  digitalWrite(EthernetShield_CS_SDCard,LOW);

  File LogFile = SD.open(FileName, FILE_WRITE);
  if(LogFile) 
    {
    r=true;
    LogFile.write((uint8_t*)Line,strlen(Line));      
    LogFile.write('\n'); // nieuwe regel
    LogFile.close();
    }
  else 
    r=false;

  // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer W510 chip
  digitalWrite(EthernetShield_CS_SDCard,HIGH);
  digitalWrite(Ethernetshield_CS_W5100, LOW);
  
  return r;
  }

 /**********************************************************************************************\
 * Voeg een regel toe aan de logfile.
 \*********************************************************************************************/
boolean SaveEventlistSDCard(char *FileName)
  {
  int x;
  boolean r=true;
  
  // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer SDCard chip
  digitalWrite(Ethernetshield_CS_W5100, HIGH);
  digitalWrite(EthernetShield_CS_SDCard,LOW);

  SD.remove(FileName); // eerst bestand wissen, anders wordt de data toegevoegd

  File EventlistFile = SD.open(FileName, FILE_WRITE);
  if(EventlistFile) 
    {
    strcpy(TempString,cmd2str(CMD_EVENTLIST_ERASE));
    EventlistFile.write((uint8_t*)TempString,strlen(TempString));      
    EventlistFile.write('\n'); // nieuwe regel

    for(x=1;x<=EVENTLIST_MAX;x++)
      {
      if(EventlistEntry2str(x,0,TempString,true))
        {
        EventlistFile.write((uint8_t*)TempString,strlen(TempString));      
        EventlistFile.write('\n'); // nieuwe regel
        }
      }
    EventlistFile.close();
    }
  else 
    {
    r=false; // niet meer weer proberen weg te schrijven.
    TransmitCode(command2event(S.Unit,CMD_MESSAGE,MESSAGE_03,0),VALUE_ALL);
    }

  // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer W510 chip
  digitalWrite(Ethernetshield_CS_W5100, LOW);
  digitalWrite(EthernetShield_CS_SDCard,HIGH);

  return r;
  }


boolean FileList(void)
  { 
  boolean x=false;
  File root;
  File entry;

  PrintTerminal(ProgmemString(Text_22));
  digitalWrite(Ethernetshield_CS_W5100, HIGH);
  digitalWrite(EthernetShield_CS_SDCard,LOW);

  if(root = SD.open("/"))
    {
    root.rewindDirectory();
    while(entry = root.openNextFile())
      {
      if(!entry.isDirectory())
        {
        digitalWrite(EthernetShield_CS_SDCard,HIGH);
        digitalWrite(Ethernetshield_CS_W5100, LOW);

        strcpy(TempString,entry.name());
        TempString[StringFind(TempString,".")]=0;
        PrintTerminal(TempString);

        digitalWrite(Ethernetshield_CS_W5100, HIGH);
        digitalWrite(EthernetShield_CS_SDCard,LOW);
        }
      }
    root.close();
    entry.close();
    x=true;
    }
  digitalWrite(EthernetShield_CS_SDCard,HIGH);
  digitalWrite(Ethernetshield_CS_W5100, LOW);
  PrintTerminal(ProgmemString(Text_22));
  
  return x;
  }
  
  
 /**********************************************************************************************\
 * Geeft een string terug met een cookie op basis van een random HEX-waarde van 32-bit.
 \*********************************************************************************************/
void RandomCookie(char* Ck)
  {
  byte  x,y;
  
  for(x=0;x<8;x++)
    {
    y=random(0x0, 0xf);
    Ck[x]=y<10?(y+'0'):((y-10)+'A');
    }
  Ck[8]=0; // afsluiten string
  }
      
      
 /*********************************************************************************************\
 * Op het Ethernetshield kunnen de W5100 chip en de SDCard niet gelijktijdig worden gebruikt
 * Deze funktie zorgt voor de juiste chipselect. Default wordt in de Nodo software uitgegaan dat
 * de ethernetchip W5100 is geselecteerd. Met deze routine kan de SDCard worden geselecteerd.
 * input: true = SD_Card geselecteerd.
 \*********************************************************************************************/
void SelectSD(byte sd)
  {
  digitalWrite(Ethernet_shield_CS_W5100, sd);
  digitalWrite(Ethernet_shield_CS_SDCard,!sd);
  }


/*********************************************************************************************\
* Funties op dit tabblad worden gebruikt voor het genereren van een MD5-hascode
* voor het veilig kunnen checken van een wachtwoord.
\*********************************************************************************************/
uint32_t md5_T[] PROGMEM = {
	0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf,
	0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af,
	0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e,
	0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
	0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6,
	0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
	0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122,
	0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
	0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039,
	0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97,
	0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d,
	0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
	0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };

  
  #define MD5_HASH_BITS  128
  #define MD5_HASH_BYTES (MD5_HASH_BITS/8)
  #define MD5_BLOCK_BITS 512
  #define MD5_BLOCK_BYTES (MD5_BLOCK_BITS/8)
  #define MD5_HASH_BITS  128

  struct md5_ctx_t 
    {
    uint32_t a[4];
    int32_t counter;
    };
  
  // typedef uint8_t md5_hash_t[MD5_HASH_BYTES];
  
  void md5_init(struct md5_ctx_t *s){
  	s->counter = 0;
  	s->a[0] = 0x67452301;
  	s->a[1] = 0xefcdab89;
  	s->a[2] = 0x98badcfe;
  	s->a[3] = 0x10325476;
  }
  
  static
  uint32_t md5_F(uint32_t x, uint32_t y, uint32_t z){
  	return ((x&y)|((~x)&z));
  }
  
  static
  uint32_t md5_G(uint32_t x, uint32_t y, uint32_t z){
  	return ((x&z)|((~z)&y));
  }
  
  static
  uint32_t md5_H(uint32_t x, uint32_t y, uint32_t z){
  	return (x^y^z);
  }
  
  static
  uint32_t md5_I(uint32_t x, uint32_t y, uint32_t z){
  	return (y ^ (x | (~z)));
  }
  
  typedef uint32_t md5_func_t(uint32_t, uint32_t, uint32_t);
  
  #define ROTL32(x,n) (((x)<<(n)) | ((x)>>(32-(n))))  
  
  static
  void md5_core(uint32_t* a, void* block, uint8_t as, uint8_t s, uint8_t i, uint8_t fi){
  	uint32_t t;
  	md5_func_t* funcs[]={md5_F, md5_G, md5_H, md5_I};
  	as &= 0x3;
  	/* a = b + ((a + F(b,c,d) + X[k] + T[i]) <<< s). */
  	t = a[as] + funcs[fi](a[(as+1)&3], a[(as+2)&3], a[(as+3)&3])
  	    + *((uint32_t*)block) + pgm_read_dword(md5_T+i) ;
  	a[as]=a[(as+1)&3] + ROTL32(t, s);
  }
  
  void md5_nextBlock(struct md5_ctx_t *state, const void* block){
  	uint32_t	a[4];
  	uint8_t		m,n,i=0;
  	/* this requires other mixed sboxes */
  	a[0]=state->a[0];
  	a[1]=state->a[1];
  	a[2]=state->a[2];
  	a[3]=state->a[3];
  
  	/* round 1 */
  	uint8_t s1t[]={7,12,17,22}; // 1,-1   1,4   2,-1   3,-2
  	for(m=0;m<4;++m){
  		for(n=0;n<4;++n){
  			md5_core(a, &(((uint32_t*)block)[m*4+n]), 4-n, s1t[n],i++,0);
  		}
  	}
  	/* round 2 */
  	uint8_t s2t[]={5,9,14,20}; // 1,-3   1,1   2,-2   2,4
  	for(m=0;m<4;++m){
  		for(n=0;n<4;++n){
  			md5_core(a, &(((uint32_t*)block)[(1+m*4+n*5)&0xf]), 4-n, s2t[n],i++,1);
  		}
  	}
  	/* round 3 */
  	uint8_t s3t[]={4,11,16,23}; // 0,4   1,3   2,0   3,-1
  	for(m=0;m<4;++m){
  		for(n=0;n<4;++n){
  			md5_core(a, &(((uint32_t*)block)[(5-m*4+n*3)&0xf]), 4-n, s3t[n],i++,2);
  		}
  	}
  	/* round 4 */
  	uint8_t s4t[]={6,10,15,21}; // 1,-2   1,2   2,-1   3,-3
  	for(m=0;m<4;++m){
  		for(n=0;n<4;++n){
  			md5_core(a, &(((uint32_t*)block)[(0-m*4+n*7)&0xf]), 4-n, s4t[n],i++,3);
  		}
  	}
  	state->a[0] += a[0];
  	state->a[1] += a[1];
  	state->a[2] += a[2];
  	state->a[3] += a[3];
  	state->counter++;
    }
  
  void md5_lastBlock(struct md5_ctx_t *state, const void* block, uint16_t length_b){
  	uint16_t l;
  	uint8_t b[64];
  	while (length_b >= 512){
  		md5_nextBlock(state, block);
  		length_b -= 512;
  		block = ((uint8_t*)block) + 512/8;
  	}
  	memset(b, 0, 64);
  	memcpy(b, block, length_b/8);
  	/* insert padding one */
  	l=length_b/8;
  	if(length_b%8){
  		uint8_t t;
  		t = ((uint8_t*)block)[l];
  		t |= (0x80>>(length_b%8));
  		b[l]=t;
  	}else{
  		b[l]=0x80;
  	}
  	/* insert length value */
  	if(l+sizeof(uint64_t) >= 512/8){
  		md5_nextBlock(state, b);
  		state->counter--;
  		memset(b, 0, 64-8);
  	}
  	*((uint64_t*)&b[64-sizeof(uint64_t)]) = (state->counter * 512) + length_b;
  	md5_nextBlock(state, b);
  }
  
  void md5_ctx2hash(struct md5_hash_t* dest, const struct md5_ctx_t* state){
  	memcpy(dest, state->a, MD5_HASH_BYTES);
  }

void md5(char* dest)
  {
  const void* src=dest;
  uint32_t length_b = strlen((char*)src) * 8;
  struct md5_ctx_t ctx;
  char *Str=(char*)malloc(20);

  md5_init(&ctx);
  while(length_b>=MD5_BLOCK_BITS)
    {
    md5_nextBlock(&ctx, src);
    src = (uint8_t*)src + MD5_BLOCK_BYTES;
    length_b -= MD5_BLOCK_BITS;
    }
  md5_lastBlock(&ctx, src, length_b);
  md5_ctx2hash((md5_hash_t*)&MD5HashCode, &ctx);
  
  strcpy(Str,PROGMEM2str(Text_05));              
  int y=0;
  for(int x=0; x<16; x++)
    {
    dest[y++]=Str[MD5HashCode[x]>>4  ];
    dest[y++]=Str[MD5HashCode[x]&0x0f];
    }
  dest[y]=0;

  free(Str);
  }
  
#endif
