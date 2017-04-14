#include "SdFat.h"
#include "ParticleFtpClient.h"
#include <time.h>

int pushButton = D1;
int lastButtonState = 0;
using namespace particleftpclient;
// Configuracion del servidor FTP
String hostname = "<IP-FTP-SERVER>";
String username = "<USER>";
String password = "<PASS>";
// Tiempo max de respuesta para servidor FTP.
int timeout = 5;
ParticleFtpClient ftp = ParticleFtpClient();

// Configuracion SPI con DMA para modulo SD Card
//Conexiones: SCK => A3, MISO => A4, MOSI => A5, SS => A2
SdFat sd;
const uint8_t chipSelect = SS;

File pFile;

void setup() {
  Serial.begin(9600);
  // Esperar conexion USB Serial (Solo para desarrollo)
  while (!Serial) {
    SysCall::yield();
  }

  Serial.println("Pulse una tecla para empezar");
  while (Serial.read() <= 0) {
    SysCall::yield();
  }
  pinMode(pushButton, INPUT_PULLUP);
  Particle.variable("btn_push", &lastButtonState, INT);
  initSD();
  //createFile();
  // TODO: Configurar camara.
  connectFTP();
}

void initSD(){

  // Inicializar SdFat o mostar errores y deneter
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
    Serial.println("Fallo al inicializar SD card");
  }

    Serial.println("1. Inicializando SD card ..");
    printFreeSpace();
    Serial.println("---------------------------------");

}

// Serial output stream
ArduinoOutStream cout(Serial);

void printFreeSpace() {
  uint32_t volFree = sd.vol()->freeClusterCount();
  float fs = 0.000512*volFree*sd.vol()->blocksPerCluster();
  cout << F("Espacio libre: ") << fs << F(" MB (MB = 1,000,000 bytes)\n\n");
}

String getFilename() {

   String file_namespace = "porterito-capture-";
   String timestamp = Time.format(Time.now(), "%Y-%m-%dT%H:%M:%SZ");
   String file_ext = ".txt";

   String filename = file_namespace + timestamp + file_ext;

   return filename;
 }

void createFile(){
  // Crear y abir el archivo para escribir contenido en el
  if (!pFile.open("test-2.txt", O_RDWR | O_CREAT | O_AT_END)) {
    sd.errorHalt("Fallo escritura");
  }
  // Escribir contenido en el archivo
  pFile.println("Contenido de archivo");
  pFile.close();
}

void connectFTP(){
  // Conectar al host y autenticarze, o detener sin falla.
  if (!ftp.open(hostname, timeout)) {
    Serial.println("No se pudo conectar al host FTP");
    //while (1);
  }
  if (!ftp.user(username)) {
    Serial.println("Usuario no existe");
    //while (1);
  }
  if (!ftp.pass(password)) {
    Serial.println("Contrase;a erronea");
    //while (1);
  }

  if (!ftp.simple_command("SYST", 200)) {
    Serial.println("No se pudo conectar al servidor FTP :(");
  } else {
    Serial.println("2. Conectando a servidor FTP ..");
    Serial.println("Prueba SYST");
    Serial.println(ftp.get_response());
    Serial.println("---------------------------------");

  }


}

void uploadFileFTP(){
  // Definir tipo de archivo a ASCII
  ftp.type("A");
  // Iniciar carga del alrchivo al servidor FTP
  Serial.println("4. Nombre de archivo generado:");
  Serial.println(getFilename());
  Serial.println("---------------------------------");
  ftp.stor(getFilename());

  if (!pFile.open("test-2.txt", O_READ)) {
    sd.errorHalt();
    Serial.println("Fallo lectura SD card");
  } else {
    Serial.println("5. Subiendo archivo a servidor FTP ..");
    Serial.println("---------------------------------");
    // Leer archivo
    int data;
    while ((data = pFile.read()) >= 0) {
        ftp.data.write(data);
    }
    delay(500);
    // Cerrar el archivo
    pFile.close();
    // Finalizar carga de archivo
    ftp.finish();
    Serial.println("-- FINAL :) --");
    Serial.println("---------------------------------");
  }

}

void loop() {
  int pushButtonState;

  pushButtonState = digitalRead(pushButton);

  if(pushButtonState != lastButtonState){
   if (pushButtonState == LOW) {
     Particle.publish("btn_push", "1");
     Serial.println("---------------------------------");
     Serial.println("3. Timbre Pulsado !");
     Serial.println("---------------------------------");
     uploadFileFTP();

   } else {
     Serial.println(".. Timbre escuchando ..");
     Particle.publish("btn_push", "0");
   }
   delay(200);
 }
   lastButtonState = pushButtonState;
 }
