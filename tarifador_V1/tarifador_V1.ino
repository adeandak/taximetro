/*
 * @AdeAndaK  173347
 * @SLGonzAb  170404
 * 
 * Codigo desarrollado para la implementacion del calculo de tarifa de un taxi con base en
 * las variables de distancia, tiempo, horario, dia de la semana, zona, personas, paradas y
 * tipo de vehiculo.
 */

//Librerias utilizadas
#include <SevSeg.h>
#include <LiquidCrystal.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>

//Declaracion de objetos de librerias
SevSeg sevseg; 
LiquidCrystal lcd(36, 37, 38, 39, 40, 41);
ThreeWire myWire(34,35,42); // DAT, CLK, RST
RtcDS1302<ThreeWire> Rtc(myWire);

//Pines de botones
int startStop = 44;
int zMas = 45;
int zMen = 46;
int OHPOR = 47;
int coche = 48;
int persMas = 49;
int paraMas = 50;
int diaMas = 53;
int horaMas = 52;
int horaMen = 51;

int sMag = 43;  //Pin sensor magnetico

//Constantes
#define diaSec 86400;   //segundos en un dia
#define horaSec 1200;   //segundos en 20 minutos
#define banderaAuto 18; //tarifa base auto
#define banderaCami 25; //tarifa base camioneta
#define kmAuto 3.57;    //tarifa por km en auto
#define kmCami 6.28;    //tarifa por km en camioneta
#define minAuto 1.8;    //tarifa por minuto en auto
#define minCami 3.15;   //tarifa por minuto en camioneta
#define consAuto 7;     //consumo promedio de combustible de auto
#define consCami 8.9;   //consumo promedio de combustible de camioneta
#define diamAuto 0.00065; //diametro de llantas de auto en km
#define diamCami 0.00065; //diametro de llantas de camioneta en km
#define multHPico 1.5;  //multiplicador de tarifa en hora pico
#define multFinDe 0.8;  //multiplicador de tarifa en fin de semana
#define precioPara 4; //costo por parada

//Declaracion de variables de estado
bool  active;
bool  isCoche;
int   zona;
bool  modoDios;
int   personas;
int   paradas;

unsigned long startTime;
unsigned long vueltas;

//Declaracion de variables auxiliares
unsigned long refreshTimer;
RtcDateTime dt;
String activeStr;
String aux;
unsigned long bufferStart = 0;
unsigned long seconds2000;
double tarifa[4];
double distancia;
int dispTar;
double tarifaPar;
double bandera;
bool sMagAnt;
bool vueltaAct; //bandera de registro de vuelta en el ciclo actual
double kmVuelta;
double precioKm;
double precioMin;
unsigned long parCounter;
double consumoP;
unsigned int paraAnt;
unsigned int contadorDios;

void setup() {
  //Configuracion de 7segmentos
  byte numDigits = 4;
  byte digitPins[] = {30, 32, 31, 33};
  byte segmentPins[] = {29, 27, 25, 23, 22, 28, 26, 24};
  bool resistorsOnSegments = true; 
  bool updateWithDelaysIn = true;
  byte hardwareConfig = COMMON_CATHODE; 
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(70);

  //Configuracion de LCD
  lcd.begin(16, 2);

  //Configuracion RTC
  //Serial.begin(9600);
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  //Rtc.SetDateTime(compiled);
  
  if (!Rtc.IsDateTimeValid()){
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      //Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
  }
  if (Rtc.GetIsWriteProtected()){
      //Serial.println("RTC was write protected, enabling writing now");
      Rtc.SetIsWriteProtected(false);
  }
  if (!Rtc.GetIsRunning()){
      //Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled){
      //Serial.println("RTC is older than compile time!  (Updating DateTime)");
      Rtc.SetDateTime(compiled);
  }
  else if (now > compiled){
      //Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled){
      //Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  //Configuracion de botones
  pinMode(startStop,INPUT);
  pinMode(zMas,INPUT);
  pinMode(zMen,INPUT);
  pinMode(OHPOR,INPUT);
  pinMode(coche,INPUT);
  pinMode(persMas,INPUT);
  pinMode(paraMas,INPUT);
  pinMode(diaMas,INPUT);
  pinMode(horaMas,INPUT);
  pinMode(horaMen,INPUT);

  //Asignacion inicial de variables de estado
  active  = false;
  isCoche = true;
  zona    = 1;
  modoDios= false;
  personas= 1;
  paradas = 0;
  
  distancia=0;
  tarifa[0]=0;
  tarifa[1]=0;
  tarifa[2]=0;
  tarifa[3]=0;
  
  //Asignacion inicial de variables auxiliares
  refreshTimer=millis();
  activeStr="Inactivo";
  sMagAnt=false;
}

void loop() {
  //Cambios de estado por botones
  if(digitalRead(OHPOR)==1 && !modoDios){
    modoDios=true;  //OH POR DIOS
    contadorDios=0;
  }
  if(digitalRead(startStop)==1 && millis()-bufferStart >= 400){
    active=!active;
    if(active){
      activeStr="";
      paradas=0;
      paraAnt=0;
      personas=1;
      startTime=millis();
      parCounter=millis();
      vueltas=0;
      distancia=0;
      tarifa[1]=0;
      tarifa[2]=0;
      tarifa[3]=0;
      if(isCoche){
        bandera=banderaAuto;
        kmVuelta=diamAuto;
        precioKm=kmAuto;
        precioMin=minAuto;
        consumoP=consAuto;
      }else{
        bandera=banderaCami;
        kmVuelta=diamCami;
        precioKm=kmCami;
        precioMin=minCami;
        consumoP=consCami;
      }
      tarifa[0]=bandera;
      kmVuelta*=PI;
      //Serial.println("Distancia por vuelta "+(String)(kmVuelta*1000));
    }else{
      activeStr="Inactivo";
      personas=1;
    }
    bufferStart=millis();
  }
  if(digitalRead(horaMas)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    if(active){
      lcd.print("Bloqueado       ");
    }else{
      lcd.print("20min+  ");
      RtcDateTime dtH=Rtc.GetDateTime();
      seconds2000=dtH.TotalSeconds() + horaSec;
      dtH=RtcDateTime(seconds2000);
      Rtc.SetDateTime(dtH);
    }
    bufferStart = millis();
    refreshTimer=millis()-100;
  } 
  if(digitalRead(horaMen)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    if(active){
      lcd.print("Bloqueado       ");
    }else{
      lcd.print("20min-  ");
      RtcDateTime dtH=Rtc.GetDateTime();
      seconds2000=dtH.TotalSeconds() - horaSec;
      dtH=RtcDateTime(seconds2000);
      Rtc.SetDateTime(dtH);
    }
    bufferStart = millis();
    refreshTimer=millis()-100;
  } 
  if(digitalRead(diaMas)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    if(active){
      lcd.print("Bloqueado       ");
    }else{
      lcd.print("Dia+    ");
      RtcDateTime dtD=Rtc.GetDateTime();
      seconds2000=dtD.TotalSeconds() + diaSec;
      dtD=RtcDateTime(seconds2000);
      Rtc.SetDateTime(dtD);
    }
    bufferStart = millis();
    refreshTimer=millis()-100;
  } 
  if(digitalRead(zMas)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    lcd.print("Zona+   ");
    zona+=1;
    if(zona==5){
      zona=1;
    }
    bufferStart = millis();
    refreshTimer=millis()-100;
  } 
  if(digitalRead(zMen)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    lcd.print("Zona-   ");
    zona-=1;
    if(zona==0){
      zona=4;
    }
    bufferStart = millis();
    refreshTimer=millis()-100;
  } 
  if(digitalRead(coche)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    if(active){
      lcd.print("Bloqueado       ");
    }else{
      isCoche=!isCoche;
      if(isCoche){
        lcd.print("SLGA    ");
      }else{
        lcd.print("ADAKU   ");
      }
    }
    bufferStart = millis();
    refreshTimer=millis();
  }
  if(digitalRead(persMas)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    lcd.print("Pers+           ");
    if(active && personas!=4){
      tarifa[personas]+=bandera;
    }
    if(personas==4 && !active){
      personas=1;
    }else if(personas!=4){
      personas+=1;
    }
    bufferStart = millis();
    refreshTimer=millis();
  }
  if(digitalRead(paraMas)==1 && millis()-bufferStart >= 400){
    lcd.setCursor(0,0);
    if(active){
      paradas+=1;
      lcd.print("Para+           ");
    }else{
      lcd.print("Bloqueado");
    }
    bufferStart = millis();
    refreshTimer=millis();
  }

  //Calculo de tarifa parcial
  if(active){
    vueltaAct=false;
    tarifaPar=0;
    if(digitalRead(sMag)==1 && !sMagAnt){
      //se registro una vuelta y en el ciclo anterior el sensor estaba en bajo
      vueltas+=1;
      vueltaAct=true;
      sMagAnt=true;
      //Serial.println("Vuelta");
    }else if(digitalRead(sMag)==0 && sMagAnt){
      //el sensor esta en bajo pero estuvo en alto el ciclo anterior
      sMagAnt=false;
    }
    if(vueltaAct){
      tarifaPar+=precioKm*kmVuelta;
    }
    tarifaPar+=(millis()-parCounter)*precioMin/60000;
    parCounter=millis();

    dt=Rtc.GetDateTime();
    tarifaPar*=multPico(dt);
    tarifaPar*=multDia(dt);
    tarifaPar*=multZona();
    distancia=vueltas*kmVuelta;
    if(distancia/((millis()-startTime)/1000<=10) && millis()-startTime/60000<=10){  //si la velocidad promedio es menor a 10km/h y el tiempo de viaje es mayor a 10 minutos
      if(distancia/((millis()-startTime)/1000<=5)){
        tarifaPar*=2;
      }else{
        tarifaPar*=100/(distancia/(((millis()-startTime)/1000)*consumoP));
      }
    }
    if(paradas>paraAnt){
      tarifaPar+=(paradas-paraAnt)*precioPara;
      paraAnt=paradas;
    }
    for (int i = 0; i < personas; i++){
      tarifa[i]+=tarifaPar*((7-personas)/6.0);  //suma a la tarifa por persona
    }
  }

  //Mostrar informacion en displays LCD y 7seg
  if(millis()-refreshTimer >= 500){
    lcd.clear();
    lcd.setCursor(0,0);
    if(!modoDios){
      lcd.print(activeStr);
      if(active){
        distancia=vueltas*kmVuelta;
        lcd.print((String)distancia+"km ");
        lcd.setCursor(10,0);
        lcd.print((String)((millis()-startTime)/60000)+"min");
        dispTar=tarifa[0]*10;
      }else{
        lcd.setCursor(11,0);
        dt=Rtc.GetDateTime();
        aux="";
        if(dt.Minute()<10){
          aux="0";
        }
        lcd.print((String)dt.Hour()+":"+aux+dt.Minute());
        lcd.setCursor(15,1);
        lcd.print(letraDia(dt));
        dispTar=tarifa[personas-1]*10;
      }
      sevseg.setNumber(dispTar,1);
      lcd.setCursor(0,1);
      lcd.print("Zn "+letraZona(zona)+"/"+"Per"+(String)personas+"/"+"Par"+(String)paradas);
      refreshTimer=millis();
    }else{
      lcd.print("OH POR DIOS!, NO");
      lcd.setCursor(0,1);
      lcd.print("HAY VUELTA ATRAS");
      sevseg.setNumber(666,contadorDios%4);
      contadorDios++;
      refreshTimer=millis()+300;
    }
  }
  sevseg.refreshDisplay();
}

String letraDia(RtcDateTime dt){
  int dia=dt.DayOfWeek();

  switch (dia){
    case 0:
      return "D";
      break;
    case 1:
      return "L";
      break;
    case 2:
      return "M";
      break;
    case 3:
      return "W";
      break;
    case 4:
      return "J";
      break;
    case 5:
      return "V";
      break;
    case 6:
      return "S";
      break;
  }
  return "I";
}

String letraZona(int zn){
  switch (zn){
    case 1:
      return "G";
      break;
    case 2:
      return "A";
      break;
    case 3:
      return "C";
      break;
    case 4:
      return "P";
      break;
  }
  return "X";
}

double multZona(){
  switch(zona){
    case 1:
      return 1;
      break;
    case 2:
      return 1.5;
      break;
    case 3:
      return 1.3;
      break;
    case 4:
      return 1.2;
      break;
  }
  return 1;
}

double multPico(RtcDateTime dtP){
  dtP=Rtc.GetDateTime();
  if((dt.Hour()>6 && dt.Hour()<10) || (dt.Hour()>17 && dt.Hour()<20)){
    return multHPico;
  }
  return 1;
}

double multDia(RtcDateTime dtF){
  dtF=Rtc.GetDateTime();
  if(dt.DayOfWeek()==6 || dt.DayOfWeek()==0){
    return multFinDe;
  }
  return 1;
}