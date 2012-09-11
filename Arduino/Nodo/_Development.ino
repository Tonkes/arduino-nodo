
/**************************************************************************************************************************\

Aanpassingen:
- Issue 510: compileer opties
- Issue 509: BreakOnDaylight waarde
- WiredCalibrate settings worden nu weer opgeslagen
- herhaaldelijk uitvoeren van een Filelist leidt tot vastloper: workaround op bug in SD library.

Known Errors / ToDo:
- SendBusy van de Master zet de slave tijdelijk op onbereikbaar.
- Resetten:  verbinden in/out poorten, ????
- Status All uitvraag bij een mini levert: [Input=File, Unit=15, Event=Debug] Geen geldig event
- WaitFreeRF: Fijnafstemming komt later nog o.b.v. Hans zijn praktijk situatie
- Vreemde weergave events: Internal=System
- In Executeline afgevangen commando worden niet gelogt op SDCard.
- Event 0,0 wordt getoond na ongeldig commando


Aanpassingen t.o.v. Nodo Due:
- All:WaitFreeRF aangepast. Nu uitsluitend nog On/Off instellen.
- All: Aanpassing: Commando [WaitBusy] heeft als parameters [All, Off, <leeg> ] Parameter [On] wordt geaccepteerd, maar is heeft zelfde werking als leeg. Default staat de WaitBusy op [All]. Timeout=60Sec.
- Mega: Prompt in Terminal venster wordt nu weergegeven NADAT verwerking plaats heeft gevonden
- Mega: Datum / Tijd wordt in Terminalvenster niet meer weergegeven. Nog wel in LOG.DAT
- Mega: nieuw commando Echo <on/off> voor terugsturen ingevoerde tekens terminalprogramma
- All: Commando [SendStatus] is komen te vervallen.
- All: WildCard uitgebreid met Message en Boot events.
- All: Status en SendStatus geintegreerd. Resultaat wordt altijs alleen naar de bron verstuurd.
- Mini: aantal variabelen terug gebracht naar 8
- Mini: Nesting diepte teruggebracht naar 4. Dit i.v.m. besparing ram geheugen.
- Mini: queue voor opvangen events 8 events.
- mega: Queue op SDCard. Geen limiet voor aantal events dat kan worden opgevangen in de queue tijdens wachten
- Commando lock voorzien van code o.b.v. ingestelde wachtwoord. (Na veranderen wachtwoord zal de lock verdwijnen)
- Status uitgebreid met optie EventlistCount. Geeft aantal bezette regels
- WaitBusy staat default op <On>
- WildCard uitgebreid met Message en Boot events.
- mega: Queue op SDCard. Geen limiet voor aantal events dat kan worden opgevangen in de queue tijdens wachten
- Status en SendStatus geintegreerd. Resultaat wordt altijs alleen naar de bron verstuurd.
- nieuw commando Echo <on/off> voor terugsturen ingevoerde tekens terminalprogramma
- Status uitgebreid met optie "EventlistCount". Geeft aantal bezette regels
- Nieuw commando "ClientIP" toegevoegd. Aanvaard alleen HTTP-Requests en EventGhst events van opgegeven IP adres.
- Commando "Divert" vervallen. Nieuw commando "SendTo" voor versturen van een commandoregel via RF naar een andere Nodo
- Nieuw commando voor in eventlist "BreakOnDaylight". 
- Weergave status nu niet meer volledig zoals een event weergegeven
- commando "DnsServer" toegevoegd voor instellen van een DNS server.
- Als IP adres an de Nodo is toegekend door DHCP dan wordt dit in de status weergegeven
- EventListshow, eventListWrite, eventListErase hebben nu als parameter-1 de regel waar het betrekking op heeft (regel, ALL, 0=All)
- Nodo ontvangt events via http. Voorbeeld: http://192.168.1.110/?event=sound&passwoord=Nodo&id=00000000
- Ethernet intergratie. Events van EventGhost (PC, Android) ontvangen en verzenden over IP;
- Toevoeging commando "URL <line>", hiermee kan de URL van de server worden ingesteld waar de events (via HTTP-Poort 80) naar toegezonden moeten worden. (max. 40 tekens)
- Nieuw commando "OutputIP <HTTP|EventGhost|Off> , <SaveIP Save|On|Off>"
- Bij opstarten de melding "Booting..." omdat wachten op IP adres van de router de eerste keer even tijd in beslag kan nemen.
- Indien SDCard geplaatst, dan logging naar Log.txt.
- UserPlugin maakt mogelijk om gebruiker zelf code aan de Nodo code toe te voegen.
- 8 digitale wired poorten i.p.v. 4
- 8 analoge wired poorten i.p.v. 4
- Aanpassing "WiredSmittTrigger": invoer analoge decimale waarde. 
- Aanpassing "WiredTreshold": invoer analoge decimale waarde.
- Eventlist uitgebreid van 120 posities naar 256
- Welkomsttekst uitgebreid met de IP-settings
- Welkomsttekst uitgebreid met melding logging naar SDCard.
- Toevoeging commando "Reboot"
- Ccommando "Status" geeft in als resultaat in de tag "output=Status"
- Commando "Display" Vervallen.
- Commando hoeft niet meer te worden afgesloten met een puntkomma. Puntkomma wordt alleen gebruikt om meerdere commandos per regel te scheiden.
- Toevoeging commando "VariableSave <variabele>", slaat variabelen op zodat deze na een herstart weer worden geladen. 0=alle, Opslaan gebeurt NIET meer automatisch.
- Logging naar SDCard naar bestand log.dat. 
- Commando "RawSignalGet" en "RawSignlPut" vervallen;
- Toevoeging commando "RawSignalSave <key>". Slaat pulsenreeks van het eerstvolgende ontvangen signaal op op SDCard onder opgegeven nummer
- Toevoeging commando "RawSignalSend <key>". Verzend een eerder onder <key> opgeslagen pulsenreeks. Als <key> = 0, dan wordt huidige inhoud verzonden
- "SendVarUserEvent" renamed naar "VariableSendUserEvent"
- nieuw commando: "VariableUserEvent" genereert een userevent op basis van de inhoud van twee variabelen.
- Commando "TransmitSettings" vervallen. Vervangen door "OutputIR", "OutputRF"
- Commando "Simulate" vervallen. Kan worden opgelost met de nieuwe commandos "Output...".
- Aanpassing weergave van datum/tijd. De dag wordt na NA de datum weergegeven ipv VOOR (ivm kunnen sorteren logfile).
- Errors worden nu weergeven met een tekstuele toelichting wat de fout veroorzaakte: "Error=<tekst>". 
- "Timestamp=" wordt nu iets anders weergegeven als "Date=yyyy-mm-dd, Time=hh:mm".
- Variabelen worden na wijzigen niet meer automatisch opgeslagen in het EEPROM geheugen. Opslaan kan nu met commando "VariableSave"
- Tag 'Direction' vervangen door Input, Output, Internal
- Commando "SendEvent <poort>" toegevoegd. Vervangt oude SendSignal. Stuurt laatst ontvangen event door. Par1 bevat de poort( EventGhost, IR, RF, HTTP, All)
- Onbekende hex-events worden mogelijk door andere waarde weergegeven a.g.v. filtering aan Nodo gelijke events. ??? wenselijk/noodzakelijk?
- Een EventlistWrite commando met bijhehorende event en actie moeten zich binnen 1 regel bevinden die wordt afgesloten met een \n
- Verzenden naar Serial vindt pas plaats als er door ontvangst van een teken gecontroleerd is dat seriele verbinding nodig is;
- Commando "VariableSetWiredAnalog" vervallen. Past niet meer bij 10-bit berwerking en calibratie/ijking
- Commando "WiredRange" vervallen. Overbodig geworden n.a.v. calibratie/ijking funktionaliteit.
- Event aangepast "WiredAnalog". Geeft nu gecalibreerde waarde weer metdecimalen achter de komma
- Verzenden van IR staat default op Off na een reset.
- Sound: als Par1 groter dan 8, dan tijdsduur=Par2*100 milliseconde, toonhoogte=Par2*100 Hz
- Aanpassing weergave van EventList zodat deze direct weer gebruikt kan worden om weg te schrijven
- Commando's/Events worden niet meer tussen "(" en ")" haken weergegeven.
- Commando "EventGhostServer" toegevoegd. Hiermee kun je het IP adres van een EG-server opgeven waar events naar toe moeten worden verstuurd. 'Auto,On' slaat laatste client op als een server.
- Commando "Debug" toegevoegd. Geeft extra informatie over de verwerking van events door de Nodo. Was vroeger de parameter "Trace" van het vervallen commando "Display"
- Commando "VariableUserEvent" vervallen. Niet meer compatibel met analoge waarden in variabelen,
- Commando "VariableSendUserEvent" vervallen. Niet meer compatibel met analoge waarden in variabelen,
- VariableSave nu ook per individuele variabele mogelijk. 0=alle;
- Alle commando's die werken met gebruikersvariabelen zijn geschikt gemaakt voor bereik van +100..-100
- Toevoeging commando "WiredAnalogVariable <wiredpoort>,<variabele>". Kent de meetwaarde van een WiredIn toe aan een variabele
- (New)KAKU signaal geeft een unit-0. (NIET KUNNEN TESTEN!)
- LET OP: MAC adres even naar je egen vaste IP adres wijzigen
- Nieuw commando "EventlistFile <filename>". Slaat de eventlist op in een bestand op SDCard. Naam max. acht posities ZONDER EXTENTIE.
- Nieuw commando "FileErase <filename>". Verwijderd een file van SDCard.
- Nieuw commando "FileExecute <filename>". Voert alle regels in een file uit.
- Nieuw commando "FileWrite <filename>". Schrijft alle via TelNet of Serial binnenkomende regels naar bestand <filename> totdat FileWrite weerals tekst voorbij komt of er 60sec. voorbij is
- HTTP-request dat de Nodo ontvangt kan worden gebruikt om een file terug te laten sturen. ?password=<password>&id=<id>&file=<filename>
- Commando "Divert" verwijderd. wordt mogelijk vervangen door "Send <unit>,<poort>" TIJDELIJK T.B.V. TEST OP BRUIKBAARHEID, NOG IN ONTWIKKELING.???
- Veranderen van een variabele genereerd GEEN automatisch event meer
- Nieuw commando: "VariableEvent <variable>". Genereert een event met betreffende variabele;
- Nieuw commando: "PulseCalibrate <high | low>, <pulse_in_mSec>, <value>". Mapt een waarde en de tijd tussen twee pulsen. T.b.v. energiemeting
- Nieuw commando: "VariablePulse <variable>, <'Count'>". Variabele vullen met gemeten pulstijd of aantal pulsen. 
- LET OP: weergave naar Serial pas nadat er tekens via serial zijn ontvangen. Voorkomt zinloze tijdsbesteding als er geen serial aangesloten is!
- Nieuw commando: BreakOnLater en BreakOnEarlier


*** TRACE: 2
