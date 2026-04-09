#include <WiFi.h>
#include <ModbusIP_ESP8266.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "esp_wifi.h"
// Configuración Matriz
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // Cambia a GENERIC_HW si se ve raro
#define MAX_DEVICES 8  // Número de módulos 8x8 que tienes
#define CS_PIN 5
TaskHandle_t TareaMatriz;
IPAddress local_IP(192, 168, 1, 150); // La IP que quieres para el ESP32
IPAddress gateway(192, 168, 1, 110);    // La IP de tu router
IPAddress subnet(255, 255, 255, 0);   // Máscara de subred típica
IPAddress primaryDNS(192, 168, 1, 110);

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Configuración Modbus y Red
ModbusIP mb;
IPAddress logoIP(192, 168, 1, 100); // LA IP DE TU LOGO!

void setup() {
  Serial.begin(115200);
  //delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname("Panel-Lavadero-Belen"); 
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("Error al configurar IP Estática");
  }
WiFi.setSleep(false);
//inicializar wifi
 WiFi.begin("PLC_lavaderoBelen", "Lag11Msu");
 // Conexión WiFi
  while (WiFi.status() != WL_CONNECTED){ 
    delay(500);    
    }  
  P.begin(2);
  P.setZone(0,0,3);
  P.setZone(1,4,7);
  P.displayClear(0);
  P.displayClear(1);

  P.displayReset(0);
  P.displayReset(1);
  P.setIntensity(5); // Brillo de 0 a 15
  //P.displayZoneText(0,"CONECTANDO...", PA_CENTER, 100, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  P.displayZoneText(1,"CREDITO", PA_CENTER, 100, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
   P.displayZoneText(0,"TIEMPO", PA_CENTER, 100, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  xTaskCreatePinnedToCore(
    LoopMatriz,    // Función de la tarea
    "TareaMatriz", // Nombre
    20000,         // Tamaño de la pila (Stack)
    NULL,          // Parámetros
    1,             // Prioridad (1 es alta)
    &TareaMatriz,  // Handshake
    1              // <--- PINNED TO CORE 1 (El núcleo de la App)
  );
  
  
  mb.client(); 
}

uint16_t valorAnterior[3] = {0,0,0};
uint16_t valorActual[3]={9,3,5};
char reloj[10]={0,0,0,0,0,0,0,0,0,0};
unsigned long ultimoChoque = 0;
unsigned int registroActual = 0;
int indiceRegistro = 0;
int registroAPedir=0;
void loop() {
  mb.task();
  
  if (mb.isConnected(logoIP)) {
    
    // Solo entramos aquí cada 300ms
    if (millis() - ultimoChoque > 100) { 
      ultimoChoque = millis();

      // Pedimos el registro según el turno: 1, luego 2, luego 3
      
      mb.readHreg(logoIP, indiceRegistro, &valorActual[indiceRegistro], 1);
      
      Serial.print("Pidiendo Registro Modbus: "); 
      Serial.println(registroAPedir);

      // Pasamos al siguiente para la próxima vuelta
      indiceRegistro++;
      if (indiceRegistro > 2) {
        indiceRegistro = 0; // Reinicia el ciclo
      }
    }
  if (valorActual[0] != valorAnterior[0]) {
      static char bufferCredito[10];
      itoa(valorActual[0], bufferCredito, 10); // Convierte número a texto
      P.displayClear(0); // Limpia solo esa zona
      P.displayZoneText(1, bufferCredito, PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
      valorAnterior[0] = valorActual[0]; // ¡IMPORTANTE! Para que no repita
    }
  for (int i=1;i<3;i++){
    if (valorActual[i] != valorAnterior[i]) {
       // Buffer para el texto "00:00"
      
      // Suponiendo que el LOGO entrega el valor en segundos:
      uint16_t minutos = valorActual[i] / 60;
      uint16_t segundos = valorActual[i] % 60;

      // sprintf con %02u asegura que siempre tenga 2 dígitos (ej: 05 en vez de 5)
      sprintf(reloj, "%02u:%02u", minutos, segundos);

      // Mostramos en la matriz sin scroll (estático) para que sea legible
      P.displayZoneText(0,reloj, PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
      
      valorAnterior[i] = valorActual[i];
      //delay(500);
  }
//else Serial.println("no cambio!!");
}
  }
else {
    mb.connect(logoIP);
    Serial.println("NADA QUE SE CONECTA!!!");
  }
  
  delay(10);
}
// Variable para la tarea


void LoopMatriz(void * parameter) {
  for (;;) {
    if (P.displayAnimate()) { // Esto anima TODAS las zonas registradas
      // Si usas zonas, es mejor resetear cada zona individualmente si es necesario
      for (uint8_t i = 0; i < 2; i++) {
        if (P.getZoneStatus(i)) {
          P.displayReset(i);
        }
      }
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); 
  }
}
