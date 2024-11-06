#include <CAN.h>
#include "HX711.h"

// Definição dos pinos para comunicação CAN
#define TX_GPIO_NUM   5     // Pino de transmissão (TX) do barramento CAN
#define RX_GPIO_NUM   4     // Pino de recepção (RX) do barramento CAN

// Definição dos pinos para o sensor de pressão
const int SENSOR_SCK = 18;  // Pino SCK do sensor de pressão
const int DOUT = 19;        // Pino DOUT do sensor de pressão

// Instanciação do objeto HX711 para leitura do sensor de pressão
HX711 scale;

// Definição da faixa de medição do sensor de pressão (em kPa)
const float maxPressaoKPa = 40.0;  // Faixa máxima de pressão em kPa
const float fatorConversaoKPa = maxPressaoKPa / 8388607.0; // Conversão de leitura raw para kPa
const float fatorConversaoPsi = fatorConversaoKPa * 0.145038; // Conversão de kPa para psi

// Definição de constantes
const uint32_t CAN_ID_PRESSURE = 0x12;  // ID do pacote CAN para envio de dados de pressão (11 bits)

void setup() {
  // Inicializa a comunicação serial
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  Serial.println("Inicializando o transmissor CAN e o sensor de pressão");

  // Configuração do barramento CAN
  CAN.setPins(RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(500E3)) {  // Inicializa o CAN a 500 kbps
    Serial.println("Erro ao iniciar o barramento CAN!");
    while (1);
  }

  // Inicialização do sensor de pressão
  scale.begin(DOUT, SENSOR_SCK);
}

void loop() {
  // Declaração de variáveis para armazenar as leituras do sensor
  long leituraRaw = 0;   // Variável para a leitura raw do sensor
  float leituraKPa = 0;  // Variável para a leitura em kPa
  float leituraPsi = 0;  // Variável para a leitura em psi

  // Verifica se o sensor está pronto para leitura
  if (scale.is_ready()) {
    leituraRaw = scale.read();  // Realiza a leitura do sensor
    // Converte a leitura raw para kPa e psi
    leituraKPa = leituraRaw * fatorConversaoKPa;
    leituraPsi = leituraRaw * fatorConversaoPsi;

    // Exibe a leitura no monitor serial
    Serial.print("Leitura do sensor de pressão: ");
    Serial.print(leituraKPa);
    Serial.print(" kPa, ");
    Serial.print(leituraPsi);
    Serial.println(" psi");
  } else {
    Serial.println("Sensor não está pronto.");
    delay(500);
    return;  // Retorna caso o sensor não esteja pronto
  }

  // Envio da leitura do sensor via barramento CAN
  Serial.print("Enviando leitura via CAN... ");

  // Verifica se o ID do pacote está dentro do intervalo válido
  if (CAN_ID_PRESSURE >= 0x000 && CAN_ID_PRESSURE <= 0x7FF) {
    CAN.beginPacket(CAN_ID_PRESSURE);  // Define o ID do pacote CAN
  } else {
    Serial.println("Erro: ID do pacote CAN fora do intervalo válido.");
    return;  // Retorna se o ID estiver fora do intervalo
  }

  // Formata as leituras como strings e as envia
  String leituraParaEnviar = String(leituraKPa, 2) + " kPa, " + String(leituraPsi, 2) + " psi";
  CAN.write((const uint8_t*)leituraParaEnviar.c_str(), leituraParaEnviar.length());

  CAN.endPacket(); // Finaliza o envio do pacote

  Serial.println("Enviado!");

  delay(1000);  // Aguarda antes de nova leitura e envio
}
