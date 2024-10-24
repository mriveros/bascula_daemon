/* Autor: Marcos A. Riveros
 * mriveros.py@gmail.com
 * Demonio con conexión Wi-Fi para enviar incomedate al móvil
 */
#include <SoftwareSerial.h>

SoftwareSerial espSerial(3, 2); // RX, TX para la comunicación serial con ESP8266
unsigned char ReadMulti[10] = {0XAA,0X00,0X27,0X00,0X03,0X22,0XFF,0XFF,0X4A,0XDD};
unsigned int timeSec = 0;
unsigned int timemin = 0;
unsigned int dataAdd = 0;
unsigned int incomedate = 0;
unsigned int parState = 0;
unsigned int codeState = 0;

// Declaraciones de funciones
void leerPuertoSerial();
void enviarValorIncomedate();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200); // Comunicación con el puerto serie principal
  espSerial.begin(115200); // Comunicación con el ESP8266

  Serial.println("Iniciando demonio con Wi-Fi...");

  // Configuración del ESP8266 como AP (Punto de acceso)
  espSerial.println("AT+RST"); // Reiniciar ESP8266
  delay(5000);  
  espSerial.println("AT+CWMODE=2"); // Configurar en modo AP
  delay(1000);

  // Configuración del punto de acceso Wi-Fi
  espSerial.println("AT+CWSAP=\"Bascula\",\"\",1,0"); // Crear AP con SSID "Bascula" sin contraseña
  delay(2000);

  // Habilitar múltiples conexiones
  espSerial.println("AT+CIPMUX=1"); 
  delay(1000);

  // Iniciar un servidor TCP en el puerto 80
  espSerial.println("AT+CIPSERVER=1,80");
  delay(1000);

  Serial.println("Configuración del servidor TCP completada.");
}

void loop() {
  timeSec++;
  if (timeSec >= 50000) {
    timemin++;
    timeSec = 0;
    if (timemin >= 20) {
      timemin = 0;
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.write(ReadMulti, 10);  // Enviar datos periódicos
      digitalWrite(LED_BUILTIN, LOW); 
    }
  }

  // Leer datos del puerto serial
  leerPuertoSerial();

  // Verificar si el ESP8266 tiene alguna solicitud pendiente
  if (espSerial.available()) {
    String response = espSerial.readString();
    Serial.println("Solicitud recibida: " + response);

    // Verificar si la solicitud es de una conexión GET (de la aplicación móvil)
    if (response.indexOf("GET") != -1) {
      enviarValorIncomedate();
    }
  }
}

void leerPuertoSerial() {
  if (Serial.available() > 0) {
    incomedate = Serial.read();
    if ((incomedate == 0x02) & (parState == 0)) {
      parState = 1;
    } else if ((parState == 1) & (incomedate == 0x22) & (codeState == 0)) {
      codeState = 1;
      dataAdd = 3;
    } else if (codeState == 1) {
      dataAdd++;
      if (dataAdd == 6) {
        Serial.print("RSSI:"); 
        Serial.println(incomedate, HEX); 
      } else if ((dataAdd == 7) | (dataAdd == 8)) {
        if (dataAdd == 7) {
          Serial.print("PC:"); 
          Serial.print(incomedate, HEX);
        } else {
          Serial.println(incomedate, HEX);
        }
      } else if ((dataAdd >= 9) & (dataAdd <= 20)) {
        if (dataAdd == 9) {
          Serial.print("EPC:"); 
        }
        Serial.print(incomedate, HEX);
      } else if (dataAdd >= 21) {
        Serial.println(""); 
        dataAdd = 0;
        parState = 0;
        codeState = 0;
      }
    } else {
      dataAdd = 0;
      parState = 0;
      codeState = 0;
    }
  }
}

void enviarValorIncomedate() {
  // Preparamos la respuesta TCP que contiene el valor de incomedate
  String response = "Valor de incomedate: " + String(incomedate) + "\r\n";
  
  // Enviar la longitud de la respuesta
  espSerial.print("AT+CIPSEND=0," + String(response.length()) + "\r\n");
  delay(100);
  
  // Enviar el valor de incomedate al cliente conectado (móvil)
  espSerial.print(response);
  delay(1000);

  // Cerrar la conexión TCP
  espSerial.println("AT+CIPCLOSE=0");
  delay(100);
}
