/*
  Redes Industriais - FIPMOC

  Sistema de controle para misturador de tintas personalizadas
  utilizando arduino e o software livre ScadaBR

  Created 08/06/2020
  By Caio Tomich (caiotomich@gmail.com)
*/

#include <SimpleModbusSlave.h>


const int BOMBA_1 = 8; // led azul
const int BOMBA_2 = 12; // led azul

const int MISTURADOR = 10; // led amarelo
const int VAPOR = 9; // led vermelho

const int DRENO = 11; // led verde

long previous = 0;

enum 
{
  LED1, // BOMBA_1 (azul)
  LED2, // VAPOR (vermelho)
  LED3, // MISTURADOR (amarelo)
  LED4, // DRENO (verde)
  LED5, // BOMBA_2 (azul)
  INIT_PROCESS, // INICIA O PROCESSO
  HOLDING_REGS_SIZE
};

unsigned int holdingRegs[HOLDING_REGS_SIZE];

void setup()
{
  modbus_configure(&Serial, 9600, SERIAL_8N1, 1, 2, HOLDING_REGS_SIZE, holdingRegs);
  modbus_update_comms(9600, SERIAL_8N1, 1);

  pinMode(BOMBA_1, OUTPUT);
  pinMode(BOMBA_2, OUTPUT);
  
  pinMode(MISTURADOR, OUTPUT);
  pinMode(VAPOR, OUTPUT);
  pinMode(DRENO, OUTPUT);
  
  pinMode(DRENO, OUTPUT);
}

int led_dreno = -1;

int PROCESS_STATUS = -1;
int PROCESS_STATUS_AUX = 0;

long INTERVAL = 10000;

void loop()
{
  modbus_update();

  // Atualiza lista de registradores
  holdingRegs[LED1] = digitalRead(3); // BOMBA_1 (azul)
  holdingRegs[LED2] = digitalRead(4); // VAPOR (vermelho)
  holdingRegs[LED3] = digitalRead(5); // MISTURADOR (amarelo)
  holdingRegs[LED4] = digitalRead(6); // DRENO (verde)
  holdingRegs[LED5] = digitalRead(7); // BOMBA_2 (azul)

  // Inicia processo a partir de comando enviado através do ScadaBR
  if (holdingRegs[INIT_PROCESS] == 1 && PROCESS_STATUS == 0) {
    holdingRegs[INIT_PROCESS] = 0;
    PROCESS_STATUS = 0;
  }
  
  // PROC=0       > PROC=1       > PROC=2                   > PROC=3
  // BOMBA_1(10s) > BOMBA_2(10s) > (MISTURADOR, VAPOR)(10s) > DRENO(até reiniciar o processo)

  // Liga BOMBA_1 e aguarda 10s
  if (PROCESS_STATUS == 0) {
      digitalWrite(BOMBA_1, HIGH);
      digitalWrite(DRENO, LOW);
      
      PROCESS_STATUS = -1;
      PROCESS_STATUS_AUX = 1;
  }
  // Desliga BOMBA_1, Liga BOMBA_2 e aguarda 10s
  else if (PROCESS_STATUS == 1) {
      digitalWrite(BOMBA_1, LOW);
      digitalWrite(BOMBA_2, HIGH);
      
      PROCESS_STATUS = -1;
      PROCESS_STATUS_AUX = 2;
  }
  // Desliga BOMBA_2, Aciona o MISTURADOR, Aciona VAPOR e aguarda 10s
  else if (PROCESS_STATUS == 2) {
      digitalWrite(BOMBA_2, LOW);
      
      digitalWrite(VAPOR, HIGH);
      digitalWrite(MISTURADOR, HIGH);
      
      PROCESS_STATUS = -1;
      PROCESS_STATUS_AUX = 3;
  }
  // Desliga o MISTURADOR, Desliga VAPOR e Aciona DRENO
  // Aguardar com led de dreno ligado até reiniciar o processo
  else if (PROCESS_STATUS == 3) {
      digitalWrite(VAPOR, LOW);
      digitalWrite(MISTURADOR, LOW);
      
      digitalWrite(DRENO, HIGH);
      
      PROCESS_STATUS = -1;
      PROCESS_STATUS_AUX = -1;
  }

  // efetua espera programada
  if (timeDelay(INTERVAL) && PROCESS_STATUS_AUX > 0) {
    PROCESS_STATUS = PROCESS_STATUS_AUX;
  }

  // aciona led de dreno intermitente
  if (PROCESS_STATUS_AUX == -1 && timeDelay(500)) {
    if (led_dreno == -1) {
      digitalWrite(DRENO, LOW);
      led_dreno = 0;
    }
    else {
      digitalWrite(DRENO, HIGH);
      led_dreno = -1;
    }
  }

// Ligar o sistema
  //- Supervisório -> Arduino
  // inicia o sistema a partir do supervisório (scadabr)

// Acionar a bomba 1 para encher o tanque com ingrediente 1 (10 segundos)
  //- Arduino aciona LED1
  //- Arduino -> Supervisório Bomba 1

// Desligar bomba 1 e acionar a bomba 2 para encher o tanque com ingrediente 2 (10 segundos)
  //- Arduino aciona LED2
  //- Arduino -> Supervisório Bomba 2

// Desligar bomba 2 e acionar o misturador e a válvula de vapor (período de 10 segundos)
  //- Arduino aciona LED3 e LED4
  //- Arduino -> Supervisório Misturador/Válvula Vapor
  
// Desligar misturador e válvula de vapor e drenar a mistura do tanque através da válvula de dreno
  //- Arduino aciona LED4
  //- Arduino -> Supervisório Válvula Dreno

// Reiniciar o processo de forma manual.
}

bool timeDelay(long interval) {
  unsigned long current = millis();
  if ((current - previous) > interval) {
    previous = current;
    return true;
  }
  return false;
}
