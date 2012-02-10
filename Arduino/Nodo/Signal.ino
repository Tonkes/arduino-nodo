
// timings NODO signalen
#define NODO_PULSE_0                    500   // PWM: Tijdsduur van de puls bij verzenden van een '0' in uSec.
#define NODO_PULSE_MID                 1000   // PWM: Pulsen langer zijn '1'
#define NODO_PULSE_1                   1500   // PWM: Tijdsduur van de puls bij verzenden van een '1' in uSec. (3x NODO_PULSE_0)
#define NODO_SPACE                      500   // PWM: Tijdsduur van de space tussen de bitspuls bij verzenden van een '1' in uSec.   

unsigned long AnalyzeRawSignal(void)
  {
  unsigned long Code=0L;
  
  if(RawSignal.Number==RAW_BUFFER_SIZE)return 0L;     // Als het signaal een volle buffer beslaat is het zeer waarschijnlijk ruis.

  if(!(Code=RawSignal_2_Nodo()))
    if(!(Code=RawSignal_2_KAKU()))
      if(!(Code=RawSignal_2_NewKAKU()))
        Code=RawSignal_2_32bit();
      
  return Code;   // Geen Nodo, KAKU of NewKAKU code. Genereer uit het onbekende signaal een (vrijwel) unieke 32-bit waarde uit.  
  }

/*********************************************************************************************\
* Deze routine berekent de uit een RawSignal een NODO code
* Geeft een false retour als geen geldig NODO signaal
\*********************************************************************************************/
unsigned long RawSignal_2_Nodo(void)
  {
  unsigned long bitstream=0L;
  int x,y,z;
  
  // nieuwe NODO signaal bestaat altijd uit start bit + 32 bits. Ongelijk aan 66, dan geen Nodo signaal
  if (RawSignal.Number!=66)return 0L;

  x=3; // 0=aantal, 1=startpuls, 2=space na startpuls, 3=1e pulslengte
  do{
    if(RawSignal.Pulses[x]>NODO_PULSE_MID)
      bitstream|=(long)(1L<<z); //LSB in signaal wordt  als eerste verzonden
    x+=2;
    z++;
    }while(x<RawSignal.Number);

  if(((bitstream>>28)&0xf) == SIGNAL_TYPE_NODO)// is het type-nibble uit het signaal gevuld met de aanduiding NODO ?  
    {
    RawSignal.Type=SIGNAL_TYPE_NODO;
    return bitstream;
    }
  else
    return 0L;  
  }



 /**********************************************************************************************\
 * Deze functie genereert uit een willekeurig gevulde RawSignal afkomstig van de meeste 
 * afstandsbedieningen een (vrijwel) unieke bit code.
 * Zowel breedte van de pulsen als de afstand tussen de pulsen worden in de berekening
 * meegenomen zodat deze functie geschikt is voor PWM, PDM en Bi-Pase modulatie.
 * LET OP: Het betreft een unieke hash-waarde zonder betekenis van waarde.
 \*********************************************************************************************/
unsigned long RawSignal_2_32bit(void)
  {
  int x,y,z;
  int Counter_pulse=0,Counter_space=0;
  int MinPulse=0xffff;
  int MinSpace=0xffff;
  unsigned long CodeP=0L;
  unsigned long CodeS=0L;
  
  // zoek de kortste tijd (PULSE en SPACE)
  x=5; // 0=aantal, 1=startpuls, 2=space na startpuls, 3=1e puls
  while(x<=RawSignal.Number-4)
    {
    if(RawSignal.Pulses[x]<MinPulse)MinPulse=RawSignal.Pulses[x]; // Zoek naar de kortste pulstijd.
    x++;
    if(RawSignal.Pulses[x]<MinSpace)MinSpace=RawSignal.Pulses[x]; // Zoek naar de kortste spacetijd.
    x++;
    }
  MinPulse+=(MinPulse*S.AnalyseSharpness)/100;
  MinSpace+=(MinSpace*S.AnalyseSharpness)/100;
     
  x=3; // 0=aantal, 1=startpuls, 2=space na startpuls, 3=1e pulslengte
  z=0; // bit in de Code die geset moet worden 
  do{
    if(z>31)
      {
      CodeP=CodeP>>1;
      CodeS=CodeS>>1;
      }
      
    if(RawSignal.Pulses[x]>MinPulse)
      {
      if(z<=31)// de eerste 32 bits vullen in de 32-bit variabele
          CodeP|=(long)(1L<<z); //LSB in signaal wordt  als eerste verzonden
      else // daarna de resterende doorschuiven
        CodeP|=0x80000000L;
      Counter_pulse++;
      }
    x++;

    if(RawSignal.Pulses[x]>MinSpace)
      {
      if(z<=31)// de eerste 32 bits vullen in de 32-bit variabele
          CodeS|=(long)(1L<<z); //LSB in signaal wordt  als eerste verzonden
      else // daarna de resterende doorschuiven
        CodeS|=0x80000000L;
      Counter_space++;
      }
    x++;
    z++;
    }while(x<RawSignal.Number);

 if(Counter_pulse>=1 && Counter_space<=1)return CodeP; // data zat in de pulsbreedte
 if(Counter_pulse<=1 && Counter_space>=1)return CodeS; // data zat in de pulse afstand
 RawSignal.Type=SIGNAL_TYPE_UNKNOWN;
 return ((CodeS^CodeP)&(0x7fffffff))|((unsigned long)SIGNAL_TYPE_UNKNOWN<<28); // data zat in beide = bi-phase, maak er een leuke mix van.
 }


 /**********************************************************************************************\
 * Deze functie wacht totdat de 433 band vrij is of er een timeout heeft plaats gevonden 
 * Window en delay tijd in milliseconden
 \*********************************************************************************************/
# define WAITFREERF_TIMEOUT             30000 // tijd in ms. waarna het wachten wordt afgebroken als er geen ruimte in de vrije ether komt

void WaitFreeRF(int Delay, int Window)
  {
  unsigned long Timer, TimeOutTimer;  
  
  // eerst de 'dode' wachttijd
  Timer=millis()+Delay; // set de timer.
  while(Timer>millis())
    digitalWrite(PIN_LED_RGB_R,(millis()>>7)&0x01);
  
  // dan kijken of de ether vrij is.
  Timer=millis()+Window; // reset de timer.
  TimeOutTimer=millis()+WAITFREERF_TIMEOUT; // tijd waarna de routine wordt afgebroken in milliseconden

  while(Timer>millis() && TimeOutTimer>millis())
    {
    if((*portInputRegister(RFport)&RFbit)==RFbit)// Kijk if er iets op de RF poort binnenkomt. (Pin=HOOG als signaal in de ether). 
      {
      if(FetchSignal(PIN_RF_RX_DATA,HIGH,SIGNAL_TIMEOUT_RF))// Als het een duidelijk signaal was
        Timer=millis()+Window; // reset de timer weer.
      }
    digitalWrite(PIN_LED_RGB_R,(millis()>>7)&0x01);
    }
  }


 /**********************************************************************************************\
 * Wacht totdat de pin verandert naar status state. Geeft de tijd in uSec. terug. 
 * Als geen verandering, dan wordt na timeout teruggekeerd met de waarde 0L
 \*********************************************************************************************/
unsigned long WaitForChangeState(uint8_t pin, uint8_t state, unsigned long timeout)
	{
        uint8_t bit = digitalPinToBitMask(pin);
        uint8_t port = digitalPinToPort(pin);
	uint8_t stateMask = (state ? bit : 0);
	unsigned long numloops = 0; // keep initialization out of time critical area
	unsigned long maxloops = microsecondsToClockCycles(timeout) / 19;
	
	// wait for the pulse to stop. One loop takes 19 clock-cycles
	while((*portInputRegister(port) & bit) == stateMask)
		if (numloops++ == maxloops)
			return 0;//timeout opgetreden
	return clockCyclesToMicroseconds(numloops * 19 + 16); 
	}


 /*********************************************************************************************\
 * Deze routine zendt een RAW code via RF. 
 * De inhoud van de buffer RawSignal moet de pulstijden bevatten. 
 * RawSignal.Number het aantal pulsen*2
 \*********************************************************************************************/
void RawSendRF(void)
  {
  int x;
    
  digitalWrite(PIN_RF_RX_VCC,LOW);   // Spanning naar de RF ontvanger uit om interferentie met de zender te voorkomen.
  digitalWrite(PIN_RF_TX_VCC,HIGH); // zet de 433Mhz zender aan
  delay(5);// kleine pause om de zender de tijd te geven om stabiel te worden 
  
  for(byte y=0; y<S.TransmitRepeatRF; y++) // herhaal verzenden RF code
    {
    x=1;
    while(x<=RawSignal.Number)
      {
      digitalWrite(PIN_RF_TX_DATA,HIGH); // 1
      delayMicroseconds(RawSignal.Pulses[x++]); 
      digitalWrite(PIN_RF_TX_DATA,LOW); // 0
      delayMicroseconds(RawSignal.Pulses[x++]); 
      }
    }
  digitalWrite(PIN_RF_TX_VCC,LOW); // zet de 433Mhz zender weer uit
  digitalWrite(PIN_RF_RX_VCC,HIGH); // Spanning naar de RF ontvanger weer aan.
  }


 /*********************************************************************************************\
 * Deze routine zendt een 32-bits code via IR. 
 * De inhoud van de buffer RawSignal moet de pulstijden bevatten. 
 * RawSignal.Number het aantal pulsen*2
 * Pulsen worden verzonden op en draaggolf van 38Khz.
 *
 * LET OP: Deze routine is speciaal geschreven voor de Arduino Mega1280 of Mega2560 met een
 * klokfrequentie van 16Mhz. De IR pin is D17.
 \*********************************************************************************************/

void RawSendIR(void)
  {
  int pulse;  // pulse (bestaande uit een mark en een space) uit de RawSignal tabel die moet worden verzonden
  int mod;    // pulsenteller van het 38Khz modulatie signaal

  // kleine pause zodat verzenden event naar de USB poort gereed is, immers de IRQ's worden tijdelijk uitgezet
  delay(10);

  for(int repeat=0; repeat<S.TransmitRepeatIR; repeat++) // herhaal verzenden IR code
    {
    pulse=1;
    noInterrupts(); // interrupts tijdelijk uitschakelen om zo en zuiverder signaal te krijgen
    while(pulse<=RawSignal.Number)
      {
      // Mark verzenden. Bereken hoeveel pulsen van 26uSec er nodig zijn die samen de lenge van de mark zijn.
      mod=RawSignal.Pulses[pulse++]/26; // delen om aantal pulsen uit te rekenen

      do
        {
        // Hoog
        bitWrite(PORTH,0, (HIGH));
        delayMicroseconds(12);
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");// per nop 62.6 nano sec. @16Mhz
        bitWrite(PORTH,0, (LOW));
        delayMicroseconds(12);
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t");// per nop 62.6 nano sec. @16Mhz
        }while(--mod);
  
      // Laag
      delayMicroseconds(RawSignal.Pulses[pulse++]);
      }
    interrupts(); // interupts weer inschakelen.
    }
  }

 /*********************************************************************************************\
 * Deze routine berekend de RAW pulsen van een 32-bit Nodo-code en plaatst deze in de buffer RawSignal
 * RawSignal.Bits het aantal pulsen*2+startbit*2 ==> 66
 * 
 \*********************************************************************************************/
void Nodo_2_RawSignal(unsigned long Code)
  {
  byte BitCounter,y=1;

  // begin met een lange startbit. Veilige timing gekozen zodat deze niet gemist kan worden
  RawSignal.Pulses[y++]=NODO_PULSE_1*2; 
  RawSignal.Pulses[y++]=NODO_SPACE*2;

  // de rest van de bits LSB als eerste de lucht in
  for(BitCounter=0; BitCounter<=31; BitCounter++)
    {
    if(Code>>BitCounter&1)
      RawSignal.Pulses[y++]=NODO_PULSE_1; 
    else
      RawSignal.Pulses[y++]=NODO_PULSE_0;   
    RawSignal.Pulses[y++]=NODO_SPACE;   
    }
  RawSignal.Pulses[y-1]=NODO_PULSE_1*10; // pauze tussen de pulsreeksen
  RawSignal.Number=66; //  1 startbit bestaande uit een pulse/space + 32-bits is 64 pulse/space = totaal 66
  }

 
 /**********************************************************************************************\
 * Haal de pulsen en plaats in buffer. Op het moment hier aangekomen is de startbit actief.
 * bij de TSOP1738 is in rust is de uitgang hoog. StateSignal moet LOW zijn
 * bij de 433RX is in rust is de uitgang laag. StateSignal moet HIGH zijn
 * 
 \*********************************************************************************************/

boolean FetchSignal(byte DataPin, boolean StateSignal, int TimeOut)
  {
  int RawCodeLength=1;
  unsigned long PulseLength;
  
  do{// lees de pulsen in microseconden en plaats deze in een tijdelijke buffer
    PulseLength=WaitForChangeState(DataPin, StateSignal, TimeOut); // meet hoe lang signaal LOW (= PULSE van IR signaal)
    if(PulseLength<MIN_PULSE_LENGTH)return false;
    RawSignal.Pulses[RawCodeLength++]=PulseLength;
    PulseLength=WaitForChangeState(DataPin, !StateSignal, TimeOut); // meet hoe lang signaal HIGH (= SPACE van IR signaal)
    RawSignal.Pulses[RawCodeLength++]=PulseLength;
    }while(RawCodeLength<RAW_BUFFER_SIZE && PulseLength!=0);// Zolang nog niet alle bits ontvangen en er niet vroegtijdig een timeout plaats vindt

  if(RawCodeLength>=MIN_RAW_PULSES)
    {
    RawSignal.Number=RawCodeLength-1;
    return true;
    }
  RawSignal.Number=0;
  return false;
  }



/**********************************************************************************************\
* Deze functie zendt gedurende Window seconden de IR ontvangst direct door naar RF
* Window tijd in seconden.
\*********************************************************************************************/
void CopySignalIR2RF(byte Window)
  {
  unsigned long Timer=millis()+((unsigned long)Window)*1000; // reset de timer.  

  digitalWrite(PIN_RF_RX_VCC,LOW);   // Spanning naar de RF ontvanger uit om interferentie met de zender te voorkomen.
  digitalWrite(PIN_RF_TX_VCC,HIGH); // zet de 433Mhz zender aan   
  while(Timer>millis())
    {
    digitalWrite(PIN_RF_TX_DATA,(*portInputRegister(IRport)&IRbit)==0);// Kijk if er iets op de IR poort binnenkomt. (Pin=LAAG als signaal in de ether). 
    digitalWrite(PIN_LED_RGB_R,(millis()>>7)&0x01);
    }
  digitalWrite(PIN_RF_TX_VCC,LOW); // zet de 433Mhz zender weer uit
  digitalWrite(PIN_RF_RX_VCC,HIGH); // Spanning naar de RF ontvanger weer aan.
  }

/**********************************************************************************************\
* Deze functie zendt gedurende Window seconden de RF ontvangst direct door naar IR
* Window tijd in seconden.
\*********************************************************************************************/
#define MAXPULSETIME 50 // maximale zendtijd van de IR-LED in mSec. Ter voorkoming van overbelasting
void CopySignalRF2IR(byte Window)
  {
// ??? aanpassen aan nieuwe IR transmissie
    
//  unsigned long Timer=millis()+((unsigned long)Window)*1000; // reset de timer.  
//  unsigned long PulseTimer;
//  
//  while(Timer>millis())// voor de duur van het opgegeven tijdframe
//    {
//    while((*portInputRegister(RFport)&RFbit)==RFbit)// Zolang de RF-pulse duurt. (Pin=HOOG bij puls, laag bij SPACE). 
//      {      
//      if(PulseTimer>millis())// als de maximale zendtijd van IR nog niet verstreken
//        {
//        digitalWrite(PIN_LED_RGB_R,HIGH);
//        TCCR2A|=_BV(COM2A0);  // zet IR-modulatie AAN
//        }
//      else // zendtijd IR voorbij, zet IR uit.
//        {
//        digitalWrite(PIN_LED_RGB_R,LOW);
//        TCCR2A&=~_BV(COM2A0); // zet IR-modulatie UIT
//        }
//      }
//    PulseTimer=millis()+MAXPULSETIME;
//    }
//  digitalWrite(PIN_LED_RGB_R,LOW);
//  TCCR2A&=~_BV(COM2A0);
  }
  
/**********************************************************************************************\
* verzendt een event en geeft dit tevens weer op SERIAL
* als het Event gelijk is aan 0L dan wordt alleen de huidige inhoud van de buffer als RAW
* verzonden.
\**********************************************************************************************/

boolean TransmitCode(unsigned long Event, byte Dest)
  {  
  int x;
  byte SignalType=(Event>>28) & 0x0f;
  byte Command   =(Event>>16) & 0xff;
  
  if(SignalType!=SIGNAL_TYPE_UNKNOWN)
    if((S.WaitFreeRF_Window + S.WaitFreeRF_Delay)>=0)
        WaitFreeRF(S.WaitFreeRF_Delay*100, S.WaitFreeRF_Window*100); // alleen WaitFreeRF als type bekend is, anders gaat SendSignal niet goed a.g.v. overschrijven buffer

  if(Command==CMD_KAKU)
    KAKU_2_RawSignal(Event);

  else if(Command==CMD_KAKU_NEW)
    NewKAKU_2_RawSignal(Event);

  else if(SignalType==SIGNAL_TYPE_NODO)
    Nodo_2_RawSignal(Event);

  if(Dest==VALUE_SOURCE_RF || (S.TransmitRF==VALUE_ON && Dest==VALUE_ALL))
    {
    PrintEvent(Event,VALUE_SOURCE_RF,VALUE_DIRECTION_OUTPUT);
    RawSendRF();
    }

  if(Dest==VALUE_SOURCE_IR || (S.TransmitIR==VALUE_ON && Dest==VALUE_ALL))
    { 
    PrintEvent(Event,VALUE_SOURCE_IR,VALUE_DIRECTION_OUTPUT);
    RawSendIR();
    } 

  // Het event verzenden naar de EventGhostServer
  if((Dest==VALUE_SOURCE_EVENTGHOST) || (Dest==VALUE_ALL && S.TransmitIP==VALUE_SOURCE_EVENTGHOST))
    {
    if(!TemporyEventGhostError)
      {
      // IP adres waar event naar toe moet even in de globale IP variabele plaatsen om de regel te kunnen printen.
      ClientIPAddress[0]=S.EventGhostServer_IP[0];
      ClientIPAddress[1]=S.EventGhostServer_IP[1];
      ClientIPAddress[2]=S.EventGhostServer_IP[2];
      ClientIPAddress[3]=S.EventGhostServer_IP[3];
  
      PrintEvent(Event,VALUE_SOURCE_EVENTGHOST,VALUE_DIRECTION_OUTPUT);
      
      if(!SendEventGhost(Event2str(Event),S.EventGhostServer_IP))
        {
        Serial.println("*** debug: EventGhost communicatie tijdelijk opgeheven i.v.m. communicatiefout.");//???
        TemporyEventGhostError=true;
        }
      }
    }

  if(EthernetEnabled)
    {
    if(Dest==VALUE_SOURCE_HTTP || (Dest==VALUE_ALL && S.TransmitIP==VALUE_SOURCE_HTTP))
      {
      SendHTTPRequestEvent(Event);
      PrintEvent(Event,VALUE_SOURCE_HTTP,VALUE_DIRECTION_OUTPUT);
      }
    }
  }



 /*********************************************************************************************\
 * Kijk of voor de opgegeven Hex-event (Code) een rawsignal file op de SDCard bestaat.
 * Als deze bestaat, dan het Hex-event vervangen door het commando "RawSignal <key>"
 * Call by referece !
 \*********************************************************************************************/
void CheckRawSignalKey(unsigned long *Code)
  {
  int x,y;

  if(SDCardPresent && RawSignal.Type==SIGNAL_TYPE_UNKNOWN)
    {
    // kijk of de hex-code toevallig al eerder is opgeslagen als rawsignal op de SDCard
    // maak er dan een Nodo event van. 
  
    // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer SDCard chip
    digitalWrite(Ethernetshield_CS_W5100, HIGH);
    digitalWrite(EthernetShield_CS_SDCard,LOW);      
    sprintf(TempString,"%s/%s.key",ProgmemString(Text_28),int2str(*Code)+2); // +2 omdat dan de tekens '0x' niet worden meegenomen. anders groter dan acht posities in filenaam.
    //  Serial.print("*** debug: check binnengekomen hex-event in file ");Serial.println(TempString);//??? Debug
    
    File dataFile=SD.open(TempString);
    if(dataFile) 
      {
      y=0;       
      while(dataFile.available())
        {
        x=dataFile.read();
        if(isDigit(x) && y<INPUT_BUFFER_SIZE)
          {
          TempString[y++]=x;
          }
        else if(x=='\n' || isPunct(x))
          {
          TempString[y]=0;
          y=0;
          // Serial.print("*** debug: Key op SDCard=");Serial.println(TempString);//??? Debug  
          *Code=command2event(CMD_RAWSIGNAL,str2int(TempString),0);
          return;
          }
        }
      dataFile.close();
      }  
  
    // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer W5100 chip
    digitalWrite(Ethernetshield_CS_W5100, LOW);
    digitalWrite(EthernetShield_CS_SDCard,HIGH);
    }
  }

 /*********************************************************************************************\
 * Sla de pulsen in de buffer Rawsignal op op de SDCard
 \*********************************************************************************************/
boolean SaveRawSignal(byte Key)
  {
  boolean error=false;
  unsigned long Event;

  if(SDCardPresent)
    {
    Event=AnalyzeRawSignal();

    // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer SDCard chip
    digitalWrite(Ethernetshield_CS_W5100, HIGH);
    digitalWrite(EthernetShield_CS_SDCard,LOW);
  
    // maak een .raw file aan met als naam de key die door de gebruiker is gekozen  
    sprintf(TempString,"%s/%s.raw",ProgmemString(Text_27),int2str(Key));  
    // Serial.print("*** debug: File=");Serial.println(TempString);//??? Debug  
    SD.remove(TempString); // eventueel bestaande file wissen, anders wordt de data toegevoegd.    
    File KeyFile = SD.open(TempString, FILE_WRITE);
    if(KeyFile) 
      {
      for(int x=1;x<=RawSignal.Number;x++)
        {
        TempString[0]=0;
        if(x>1)
          strcat(TempString,",");
        strcat(TempString,int2str(RawSignal.Pulses[x]));
        KeyFile.write(TempString);
        }
      strcpy(TempString,"\n");
      KeyFile.write(TempString);
      KeyFile.close();

      // maak een .key file aan met als naam de key die door de gebruiker is gekozen  
      sprintf(TempString,"%s/%s.key",ProgmemString(Text_28),int2str(Event)+2); // +2 omdat dan de tekens '0x' niet worden meegenomen. anders groter dan acht posities in filenaam.

      // Serial.print("*** debug: File=");Serial.println(TempString);//??? Debug    
      SD.remove(TempString); // eventueel bestaande file wissen, anders wordt de data toegevoegd.      

      KeyFile = SD.open(TempString, FILE_WRITE);
      if(KeyFile) 
        {
        strcpy(TempString,int2str(Key));
        strcat(TempString,";\n");
        // Serial.print("*** debug: Schrijf naar *.key=");Serial.println(TempString);//??? Debug
        KeyFile.write(TempString);
        KeyFile.close();
        }
      else 
        error=true;
      }
    else 
      error=true;
   
    // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer W510 chip
    digitalWrite(Ethernetshield_CS_W5100, LOW);
    digitalWrite(EthernetShield_CS_SDCard,HIGH);
    }

  else
    error=true;
    
  if(error)
    {
    TransmitCode(command2event(CMD_ERROR,ERROR_03,0),VALUE_ALL);
    return false;
    }
  return true;
  }


 /*********************************************************************************************\
 * Haal de RawSignal pulsen op uit het bestand <key>.raw en sla de reeks op in de 
 * RawSignal buffer, zodat deze vervolgens weer kan wordn gebruikt om te verzenden.
 \*********************************************************************************************/
void RawSignalGet(int Key)
  {
  int x,y,z;

  if(SDCardPresent)
    {
    // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer SDCard chip
    digitalWrite(Ethernetshield_CS_W5100, HIGH);
    digitalWrite(EthernetShield_CS_SDCard,LOW);      
    sprintf(TempString,"%s/%s.raw",ProgmemString(Text_27),int2str(Key));

    File dataFile=SD.open(TempString);
    if(dataFile) 
      {
      y=0;
      z=1;// [0] van RawSignal.Pulses wordt niet gebruikt
      while(dataFile.available())
        {
        x=dataFile.read();
        if(isDigit(x) && y<INPUT_BUFFER_SIZE)
          {
          TempString[y++]=x;
          }
        else if(x=='\n' || isPunct(x))
          {
          TempString[y]=0;
          y=0;
          RawSignal.Pulses[z++]=str2int(TempString);
          }
        }
      dataFile.close();
      RawSignal.Number=z-1;
      }
    else
      {
      // ??? nog afvangen als het bestand niet geopend kan worden
      }  
  
    // SDCard en de W5100 kunnen niet gelijktijdig werken. Selecteer W5100 chip
    digitalWrite(Ethernetshield_CS_W5100, LOW);
    digitalWrite(EthernetShield_CS_SDCard,HIGH);
    }
  }


boolean SendStringRF(char* string)
  {
  int length=strlen(string);
  byte checksum=0;
  int x;
  byte state=0;
 
  // verzend de volgende bytes:
  // blokgrootte 0.255
  // string[0] .. string[laatste teken +1] ; dus de afsluitende nul van de string wordt meegezonden
  // checksum byte
   
  SendFrameRF(length&0xff);
  for(x=0;x<length;x++)
    {
    SendFrameRF(string[x]);
    checksum=(checksum + string[x])%256; // bereken checksum (crc-8)
    }
  SendFrameRF(checksum);
  }

void SendFrameRF(byte c)
  {
  byte x;
  Serial.print("*** SendFrame: ");
  Serial.write(c);
  Serial.print("(");
  Serial.print(c,DEC);
  Serial.print(") = ");
  
  for(x=0;x<=7;x++)
    {
    if(c>>x&1)
      Serial.print("1");
    else
      Serial.print("0");
    }  
  Serial.println();
  }

