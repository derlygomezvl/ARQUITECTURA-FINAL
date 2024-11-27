//Laura Benavides, Derly Gomez, Isabella Trochez

#include "StateMachineLib.h"
#include "AsyncTaskLib.h"
#include "DHT.h"
#include <LiquidCrystal.h>
#include <Keypad.h>

//LEDS
#define LED_VERDE 9
#define LED_ROJO 10
#define LED_AZUL 8

//BUZZER
#define ZUMBADOR 7

//LUZ
#define photocellPin A1

//DTH
#define DHTPIN 46     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);


//TECLADO-CONTRASEÑA
const byte FILAS = 4;
const byte COLUMNAS = 4;
char teclas[FILAS][COLUMNAS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte filaPines[FILAS] = {24, 26, 28, 30};
byte columnaPines[COLUMNAS] = {32, 34, 36, 38};
Keypad teclado = Keypad(makeKeymap(teclas), filaPines, columnaPines, FILAS, COLUMNAS);

// LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//CONTRASEÑA
#define TIEMPO_MAXIMO 3000
unsigned char bandera = 0;
unsigned char indice = 0;
unsigned char intentos = 0;
char clave[5];
char contrasenia[5] = "12345";
char enter = 'A';
unsigned long tiempoInicio; 

//TEMPERATURA
#define TEMP_DIGITAL
float tempC = 0.0;

#define DEBUG(a) Serial.print(millis()); Serial.print(": "); Serial.println(a);

#define analogPin A0 //the thermistor attach to
#define beta 4090 //the beta of the thermistor

//PROCEDIMIENTOS
void read_Temperatura(void);
void read_Humedad(void);
void read_Luz (void);
void funct_Timeout(void);
void read_Sensores(void);
void funct_Display(void);
void detenerAlarma(void);
void manejarTeclado(void);
void activarAlarma(void);
void bloqueo(void);
void manejarSistemaDeAutenticacion(void);
void manejarParpadeoLED(void);
void funct_Display2(void);

AsyncTask TaskHumedad(1000, true, read_Humedad);
AsyncTask TaskTemperatura(1000, true, read_Temperatura);
AsyncTask TaskLuz(1000, true, read_Luz);
AsyncTask TaskDisplay(500, true, funct_Display);
AsyncTask TaskDisplay2(500, true, funct_Display2);
AsyncTask TaskSensor(500, true, read_Sensores);
AsyncTask TaskSeguridad(0,true, manejarSistemaDeAutenticacion);
AsyncTask TaskActivarAlarma(0, false, activarAlarma);
AsyncTask TaskDetenemosAlarma(0, false, detenerAlarma);
AsyncTask TaskmanejarTeclado(0, true, manejarTeclado);
AsyncTask TaskBloqueo(0, false, bloqueo);
AsyncTask TaskTimeout(5000, false, funct_Timeout);
AsyncTask TaskTimeout1(3000, false, funct_Timeout);  

// State Alias
enum State
{
	INICIO = 0,
	BLOQUEADO = 1,
	MONITOREO = 2,
	EVENTOS = 3,
  ALARMA = 4
};

// Input Alias
enum Input
{
	Sign_T = 0,
	Sign_P = 1,
	Sign_S = 2,
	Unknown = 3,
};

//INICIALIZA LA MAQUINA
StateMachine stateMachine(5, 8);
Input input;

// Setup the State Machine
void setupStateMachine()
{
	// TRANCICIONES 
	stateMachine.AddTransition(INICIO, MONITOREO, []() { return input == Sign_P; });

	stateMachine.AddTransition(INICIO, BLOQUEADO, []() { return input == Sign_S; });
	stateMachine.AddTransition(BLOQUEADO, INICIO, []() { return input == Sign_T; });
	stateMachine.AddTransition(MONITOREO, EVENTOS, []() { return input == Sign_T; });

	stateMachine.AddTransition(EVENTOS, MONITOREO, []() { return input == Sign_T; });
	stateMachine.AddTransition(EVENTOS, ALARMA, []() { return input == Sign_S; });
	stateMachine.AddTransition(MONITOREO, ALARMA, []() { return input == Sign_P; });

	stateMachine.AddTransition(ALARMA, INICIO, []() { return input == Sign_T; });

	// ACCIONES ENTRADA 
	stateMachine.SetOnEntering(INICIO, funct_Inicio);
	stateMachine.SetOnEntering(BLOQUEADO, funct_Bloqueado);
	stateMachine.SetOnEntering(MONITOREO, funct_Monitoreo);
	stateMachine.SetOnEntering(EVENTOS, funct_Eventos);
  stateMachine.SetOnEntering(ALARMA, funct_Alarma);

  // ACCIONES SALIDA 
	stateMachine.SetOnLeaving(INICIO, funct_out_Inicio);
	stateMachine.SetOnLeaving(BLOQUEADO, funct_out_Bloqueado);
	stateMachine.SetOnLeaving(MONITOREO, funt_out_Monitoreo);
	stateMachine.SetOnLeaving(EVENTOS, funct_out_Eventos);
  stateMachine.SetOnLeaving(ALARMA, funct_out_Alarma);
}
//LEER ENTRADAS
int readInput()
{
	Input currentInput = Input::Unknown;
	if (Serial.available())
	{
		char incomingChar = Serial.read();

		switch (incomingChar)
		{
			case 'P': currentInput = Input::Sign_P; break;
			case 'T': currentInput = Input::Sign_T; break;
			case 'S': currentInput = Input::Sign_S; break;
			default: break;
		}
	}
	return currentInput;
}
//ENTRADA INICIO
void funct_Inicio(void){
	Serial.println("INICIO");
  lcd.clear();
  lcd.print("INICIO");
	TaskSeguridad.Start();
}
//ENTRADA BLOQUEADO
void funct_Bloqueado(void){
	Serial.println("BLOQUEADO");
  lcd.clear();
  lcd.print("BLOQUEO");
	TaskBloqueo.Start();
}
//ENTRADA MONITOREO
void funct_Monitoreo(void){
	Serial.println("MONITOREO");
  lcd.clear();
  lcd.print("MONITOREO");
	TaskTemperatura.Start();
	TaskHumedad.Start();
	TaskLuz.Start();
	TaskDisplay.Start();
	TaskTimeout.Start();
}
//ENTRADA EVENTOS 
void funct_Eventos(void){
	Serial.println("EVENTOS");
  lcd.clear();
  lcd.print("EVENTOS");
	TaskSensor.Start();
  TaskDisplay2.Start();
	TaskTimeout1.Start();
}
//ENTRADA ALARMA
void funct_Alarma(void){
	Serial.println("ALARMA");
	TaskDisplay.Stop();
	lcd.clear();
  // print the number of seconds since reset:
  lcd.setCursor(0, 0);
	lcd.print("ALARMA");
	TaskmanejarTeclado.Start();
	TaskActivarAlarma.Start();
	
}
//SALIDAS
//SALIDA INICIO
void funct_out_Inicio(void){
	Serial.println("Leaving INICIO");
  lcd.clear();
		TaskSeguridad.Stop();
}
//SALIDA BLOQUEADO
void funct_out_Bloqueado(void){
	Serial.println("Leaving BLOQUEADO");
  lcd.clear();
	TaskBloqueo.Stop();
}
//SALIDA MONITOREO
void funt_out_Monitoreo(void){
	Serial.println("Leaving MONITOREO");
	TaskTemperatura.Stop();
  TaskDisplay.Stop();
	TaskHumedad.Stop();
	TaskLuz.Stop();
	TaskTimeout.Stop();
}
//SALIDA EVENTOS 
void funct_out_Eventos(void){
	Serial.println("Leaving EVENTOS");
	TaskSensor.Stop();
  TaskDisplay2.Stop();
	TaskTimeout1.Stop();
}
//SALIDA ALARMA 
void funct_out_Alarma(void){
	Serial.println("Leaving ALARMA");
	TaskActivarAlarma.Stop();
	TaskmanejarTeclado.Stop();
}

//Laura Benavides, Derly Gomez, Isabella Trochez

// FUNCIONES AUXILIARES
//TEMPERATURA
void read_Temperatura(void){

#if defined TEMP_DIGITAL
	Serial.println("TEMP_DIGITAL");
	// Read temperature as Celsius (the default)
  tempC = dht.readTemperature();
#else
	Serial.println("TEMP_ANALOG");
	//read thermistor value
  long a =1023 - analogRead(analogPin);
  //the calculating formula of temperature
  tempC = beta /(log((1025.0 * 10 / a - 10) / 10) + beta / 298.0) - 273.0;
#endif

	Serial.print("TEMP: ");
	Serial.println(tempC);
	if(tempC >= 27){
    input = Input::Sign_P;
  }
}

//HUMEDAD
float Hum;
void read_Humedad(void){
  Hum = dht.readHumidity(); 
	Serial.print("HUM: ");
	Serial.println(Hum);
}

//LUZ
float ValorLuz;
const float GAMMA = 1.2;
const float RL10 = 20.0;

void read_Luz (void){
ValorLuz  = analogRead(photocellPin);
Serial.print("LUZ: ");
Serial.println(ValorLuz);
}

//TIEMPO DE CAMBIO
void funct_Timeout(void) {
	input = Input::Sign_T;  
}

//HALL E INFRARROJO
#define PIN_IR 48
#define PIN_HALL 50

unsigned char valIR = HIGH;
unsigned char valHall = LOW;

void read_Sensores(void){
  valIR = digitalRead(PIN_IR);
  valHall = digitalRead(PIN_HALL);

	if((valIR == LOW)){
		input = Input::Sign_S;
	}

	Serial.print("IR: ");
	Serial.println(valIR);
	Serial.print("HALL: ");
	Serial.println(valHall);
}
void funct_Display2(void){
	lcd.clear();
  
  lcd.setCursor(0, 0);
	lcd.print("IR: ");
	lcd.print(valIR);
	
	lcd.setCursor(0, 1);
	lcd.print("Hall: ");
	lcd.print(valHall);
}
///MOSTRAR EN LCD
void funct_Display(void){
	lcd.clear();
  
  lcd.setCursor(0, 0);
	lcd.print("T:");
	lcd.print(tempC);

	lcd.setCursor(7, 0);
	lcd.print(" H:");
	lcd.print(Hum);

	lcd.setCursor(0, 1);
	lcd.print("LZ:");
	lcd.print(ValorLuz);

}

//MANEJAR TECLADO PARA ALARMA 
void manejarTeclado(void) {
  char tecla = teclado.getKey();
  if (tecla) {
    lcd.setCursor(indice, 1);
    if (tecla == '#') {
      	lcd.print("tecla");
        detenerAlarma();  
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Alarma Detenida");
				input = Input::Sign_T;
				return;
    }
  }
}

unsigned long tiempoUltimoCambio = 0;
bool ledEncendido = false;             
bool alarmaEncendida = false; 

const int MEL_MELODIA[] = {262, 294, 330, 262, 330, 349, 392}; 
const int DURACION[] = {300, 300, 300, 300, 300, 300, 300};  

//ACTIVA LA ALARMA 
void activarAlarma(void) {
  alarmaEncendida = true; 

  while (alarmaEncendida) {
    for (int i = 0; i < sizeof(MEL_MELODIA) / sizeof(MEL_MELODIA[0]) && alarmaEncendida; i++) {
      tone(ZUMBADOR, MEL_MELODIA[i]); // Genera tono según la melodía
      unsigned long tiempoInicio = millis(); 

      while (millis() - tiempoInicio < DURACION[i] && alarmaEncendida) {
        manejarParpadeoLED(); 
        manejarTeclado();    
      }
    }
  }
}
/* TONO RUSTICO 
void activarAlarma(void) {
  alarmaEncendida = true; 

  while (alarmaEncendida) {
    // Parte 1: Incrementar tono
    for (int i = 200; i <= 800 && alarmaEncendida; i++) {
      tone(ZUMBADOR, i); // Genera tono creciente en el buzzer
      manejarParpadeoLED(); // Maneja el parpadeo del LED
      manejarTeclado();     // Revisa si se presiona la tecla #
    }

    // Parte 2: Decrementar tono
    for (int i = 800; i >= 200 && alarmaActiva; i--) {
      tone(ZUMBADOR, i); // Genera tono decreciente en el buzzer
      manejarParpadeoLED(); // Maneja el parpadeo del LED
      manejarTeclado();     // Revisa si se presiona la tecla #
    }
  }
}*/

//PARPADEO DEL LED ALARMA 
void manejarParpadeoLED(void) {
  unsigned long tiempoActual = millis(); 

  if (tiempoActual - tiempoUltimoCambio >= 150) {
    if (ledEncendido) {
      digitalWrite(LED_ROJO, LOW);
      ledEncendido = false;
    } else {
      digitalWrite(LED_ROJO, HIGH); 
      ledEncendido = true;
    }
    tiempoUltimoCambio = tiempoActual; 
  }
}

//DETENER ALARMA 
void detenerAlarma(void) {
  alarmaEncendida = false;        
  noTone(ZUMBADOR);           
  digitalWrite(LED_ROJO, LOW); 
}
//BLOQUEO 
void bloqueo(void) {
    lcd.clear(); 
    lcd.setCursor(0, 0);
    lcd.print("SISTEMA BLOQUEADO"); 

    unsigned long tiempoUltimoLed = 0;     
    unsigned long tiempoUltimaAlarma = 0; 
     

    while (millis() - tiempoInicio < 7000) { 
        unsigned long tiempoActual = millis();

        if (tiempoActual - tiempoUltimoLed >= 500) {
            tiempoUltimoLed = tiempoActual;

            if (ledEncendido && alarmaEncendida) {
                digitalWrite(LED_ROJO, LOW);  
                noTone(ZUMBADOR);             
                alarmaEncendida = false;
                ledEncendido = false;
            } else {
                digitalWrite(LED_ROJO, HIGH); 
                tone(ZUMBADOR, 400);         
                alarmaEncendida = true;
                ledEncendido = true;
            }
        }
    }

    digitalWrite(LED_ROJO, LOW);    
    noTone(ZUMBADOR);               
    intentos = 0;                   

    lcd.clear();                    
    TaskSeguridad.Start();          
    input = Input::Sign_T;          
}

//CONTRASEÑA
void manejarSistemaDeAutenticacion(void) {
  char tecla = teclado.getKey();  

  if (tecla) {
    lcd.setCursor(indice, 1);  
    lcd.print("*"); 

    if (tecla != enter) {
      clave[indice++] = tecla;
      tiempoInicio = millis(); // Reinicia el tiempo de inactividad al presionar una tecla
    }
  }

  // Verifica si el tiempo límite se excedió
  if (millis() - tiempoInicio > TIEMPO_MAXIMO && indice > 0) { 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Clave Incorrecta");
    intentos++; 
    delay(1000);  
    lcd.clear();
    digitalWrite(LED_AZUL, HIGH);  
    delay(1000);  
    digitalWrite(LED_AZUL, LOW);

    if (intentos > 2) {
      input = Input::Sign_S;
    }
    indice = 0; // Reinicia el índice para empezar de nuevo
    tiempoInicio = millis(); // Reinicia el temporizador
  }

  // Verifica si la clave tiene el tamaño correcto
  if (indice > 4) {
    if (strncmp(clave, contrasenia, 4) == 0) {  
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Clave Correcta");
      digitalWrite(LED_VERDE, HIGH);  
      delay(3000); 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BIENVENIDO :)");
      intentos = 0;  
      digitalWrite(LED_VERDE, LOW);  
      input = Input::Sign_P;

    } else { 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Clave Incorrecta");
      intentos++; 
      digitalWrite(LED_AZUL, HIGH);  
      delay(1000);  
      digitalWrite(LED_AZUL, LOW);  

      if (intentos > 2) {
        input = Input::Sign_S;
      }
    }

    delay(1000);  
    lcd.clear();
    digitalWrite(LED_ROJO, LOW); 
    digitalWrite(LED_AZUL, LOW);  
    digitalWrite(LED_VERDE, LOW);  
    indice = 0; 
    tiempoInicio = millis(); // Reinicia el temporizador
  }

  if (indice == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Digite la clave:");
    tiempoInicio = millis(); // Reinicia el temporizador si comienza desde cero
  }
}
//---------------------------------------SETUP-------------------------------
void setup() 
{
	// set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  tiempoInicio = millis(); 

	pinMode(PIN_IR, INPUT);
  pinMode(PIN_HALL, INPUT);
	pinMode(ZUMBADOR, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AZUL, OUTPUT);

	Serial.begin(9600);

	Serial.println("Starting State Machine...");
	setupStateMachine();	
	Serial.println("Start Machine Started");

	dht.begin();
	// Initial state
	stateMachine.SetState(INICIO, false, true);
}

//----------------------------LOOP---------------------------
void loop() 
{
	
	//LEER ENTRADA
	input = static_cast<Input>(readInput());

  //TAREAS
	TaskSeguridad.Update();
	TaskBloqueo.Update();
  TaskDetenemosAlarma.Update();
	TaskmanejarTeclado.Update();
	TaskTemperatura.Update();
	TaskHumedad.Update();
	TaskLuz.Update();
	TaskTimeout.Update();
	TaskTimeout1.Update();
	TaskDisplay.Update();
  TaskDisplay2.Update();
	TaskSensor.Update();
	TaskActivarAlarma.Update();

	// ACTUALIZAR MAQUINA 
	stateMachine.Update();
  input = Input::Unknown;
}

//Laura Benavides, Derly Gomez, Isabella Trochez


