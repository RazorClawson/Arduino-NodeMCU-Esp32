/*
  Programa: TesteMedidorGauge3.ino
  Funcao..: Módulo medidor de combustivel, com gauge de barra utilizando
            o display tft 24 pol de 320x240.
  Autor...: Rodrigo R P Lanzarini AKA Razor
  Data....: 25/Jan/2022
  Versao..: 3

  Para o RTC DS1302, as conexões usadas foram:

  DS1302 CLK/SCLK --> D1
  DS1302 DAT/IO --> D0
  DS1302 RST/CE --> D6
  DS1302 VCC --> 3.3v - 5v
  DS1302 GND --> GND
*/

#include <TJpg_Decoder.h>     // Include the jpeg decoder library
#define FS_NO_GLOBALS         // Include SPIFFS
#include <FS.h>
#ifdef ESP32
  #include "SPIFFS.h"         // ESP32 only
#endif
#include <TFT_eSPI.h>         // Hardware-specific library
#include <SPI.h>
#include "Free_Fonts.h"       // Include the header file attached to this sketch
#include <ThreeWire.h>  
#include <RtcDS1302.h>

ThreeWire myWire(D0,D1,D6);   // DAT/IO, SCLK, RST/CE
RtcDS1302<ThreeWire> Rtc(myWire);

TFT_eSPI tft = TFT_eSPI();    // Invoke custom library

#define TFT_GREY 0x5AEB
#define TFT_LIGHTRED ConvertRGB(140, 0, 0)
#define TFT_LIGHTBLUE ConvertRGB(77, 122, 255)

// Definição das variáveis para o calculo e medição da resistência da bóia
int aPinLeitura = A0;
float R1 = 0;   
float val = 0;
float Vin = 3.3;  
float Vout = 0.0; 
int R2 = 1000;
float RC;
float I;
float LT;

int total_tanque = 70;
int bar_width = 7;
int bar_height = 30;
int bar_spacement = 1; 
int bar_all = (bar_width+bar_spacement)+1;

int PosHxVBar [70][35];
int PosHorBar [70];

// #########################################################################
// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
// #########################################################################
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}

void setup() {

  Serial.begin(9600);  // For debug

  Rtc.Begin();  // Iniciar o RTC

  tft.init();
  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);

  // Initialise SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");

  tft.setSwapBytes(true); // We need to swap the colour bytes (endianess)

  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);

  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);

  // Get the width and height in pixels of the jpeg if you wish
  uint16_t w = 0, h = 0;
  TJpgDec.getFsJpgSize(&w, &h, "/CapaTempra320x240.jpg"); // Note name preceded with "/"
  Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);

  // Draw the image, top left at 0,0
  TJpgDec.drawFsJpg(0, 0, "/CapaTempra320x240.jpg");

  delay(5000);

  tft.fillScreen(TFT_BLACK);

  TJpgDec.getFsJpgSize(&w, &h, "/BombaGasAzul44x48.jpg"); // Note name preceded with "/"
  Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);

  TJpgDec.setJpgScale(1);

  // Draw the image, top left at 0,0
  TJpgDec.drawFsJpg(0, 0, "/BombaGasAzul44x48.jpg");

  //byte d = 40;

  drawWin("TANQUE", "70", 0, 60, 76, 61, TFT_GREY);
  drawWin("RESTANTE", "63", 90, 60, 76, 61, TFT_GREY);

  //SetPosHorBar(0, 35, 18, 160);
  
  // Faz o mesmo que a funcao acima só que sem encher o saco usando uma funcao 2x
  // e ja faz tudo aqui mesmo... simplificando a coisa por hora
  int posH = 0;
  for (int i = 0; i < 70; i += 1)
  {
    if (i == 35) posH=0;
    if (i == 17 || i == 52) posH=160;
    PosHorBar[i]=posH;
    posH=posH+bar_all;
  }

  drawHorBar(1, 132, 35, TFT_GREY);
  drawHorBar(1, 189,  7, TFT_LIGHTRED);
  drawHorBar(8, 189, 28, TFT_GREY);

}

void loop() {

  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  if (!now.IsValid())
  {
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
  }

  ImpHorBar();

  delay(500);

}

// #########################################################################
//  Preencher a barra com a quantidade de combustível
// #########################################################################
void ImpHorBar() {

  val = 1.0*analogRead (aPinLeitura);  // Aquisição analógica de valores pelo pino A0
  Serial.println(val);
  Vout = (val*Vin)/1090;               // Fórmula para calcular o Vout - 1024 É PRA 5V ARDUINO E 1090 FOI PRO NODE 3.3V
  R1 = (R2*(Vin-Vout))/Vout;           // Fòrmula do divisor de tensão

  LT = 70-(((R1-66)*70)/(570));        // Para o uso do trimpot simulando a boia, que vai de 64 a 636 ohms
  //LT = 70-(((R1-64)*70)/(249));        // Para o uso com a bóia, que vai de 0 a 317 ohms
  if (LT < 0) LT = 0;

  char buf1[20];
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("V Out.: ", 180, 64, 2);  dtostrf(Vout,5,2,buf1); tft.drawString(buf1, 250, 64, 2);
  tft.drawString("R1....: ", 180, 84, 2);  dtostrf(R1,5,2,buf1)  ; tft.drawString(buf1, 250, 84, 2);
  tft.drawString("LITROS: ", 180, 104, 2); dtostrf(LT,5,2,buf1)  ; tft.drawString(buf1, 250, 104, 2);
  
  tft.setTextSize(4); dtostrf(LT,2,0,buf1); tft.drawString(buf1, 99, 84, 1); tft.setTextSize(1); 
    
  int y=189;
  int LT2 = int(round(LT));

  word color  = TFT_GREEN;  // qq cor, só pra inicializar

  for (int i = 1; i <= 70; i += 1)
  {
    // 64 ~ 70 ; 650 ~ 0; cada unidade = 8,37    
    if (i<36) y=189; else y=132;
    if (i > LT2) {
      if (i<8) color = TFT_LIGHTRED; else color = TFT_GREY;
    }
    else {
      if (i<8) color = TFT_RED; else color = TFT_BLUE;
    }
    tft.fillRect(PosHorBar[i-1], y, bar_width, bar_height, color);
  }

}

// #########################################################################
// Converter a cor no formato RGB para o formato 565 (FFFF)
// #########################################################################
word ConvertRGB(byte R, byte G, byte B)
{
  return ( ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3) );
}

// #########################################################################
// Imprimir a data e a hora
// #########################################################################
#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime& dt)
{
  char datestring[20]; String datetime;
  snprintf_P(datestring, countof(datestring), PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Day(), dt.Month(), dt.Year(), dt.Hour(), dt.Minute(), dt.Second() );
  Serial.println(datestring);
  datetime = String(datestring);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  //tft.setFreeFont(FF4);
  tft.drawString(datetime.substring(11,19), 106, 0, 7);
  Serial.println("só a hora: "+datetime.substring(11));
}

// #########################################################################
// Desenhar a janelinha com os dados do tanque
// #########################################################################
void drawWin(String lb1, String lb2, int x, int y, int w, int h, word color)
{
  tft.fillRect(x, y , w, h, color);
  tft.fillRect(x+3, y+18 , w-6, h-21, TFT_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_GREY);
  tft.drawCentreString(lb1, x+(w/2), h, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(4);
  tft.drawString(lb2, x+9, y+24, 1);
  tft.setTextSize(1);
  tft.setFreeFont(FF4);
  tft.drawString("L", x+61, y+40, 2);
  tft.setFreeFont(NULL);
}

// #########################################################################
// Preencher o vetor (array monodimensional) da seguinte forma: 
// Calcular as posições Horizontais da barra, para facilitar o desenho da
// mesma na função DrawHorBar, bem como ajudar no posicionamento também na
// função ImpHorBar, que é onde se calcula o marcador correto do combustivel.
// x = posicao inicial HORIZONTAL da barra
// ndx = posicao a qual iniciara o preenchimento no VETOR
// qtde = quantidade de segmentos que deverao ser preenchidos na barra
// njump = numero do segmento que haverá um pulo na posição
// pjump = posicao "geografica" do pulo para o segmento indicado em njump
// #########################################################################
void SetPosHorBar(int x, int ndx, int qtde, int njump, int pjump)
{
  for (int i = 0; i < qtde; i += 1)
  {
    if (i == njump-1) x=pjump;
    PosHorBar[i]=x;
    x=x+bar_all;
  }
}

// #########################################################################
// Desenhar o gauge da barra
// pini = posicao inicial HORIZONTAL da barra
// y = posicao inicial VERTICAL da barra
// qtde = quantidade de segmentos a serem printados na tela
// color = a cor dos benditos segmentos
// #########################################################################
void drawHorBar(int pini, int y, int qtde, word color)
{
  if (pini > 1) qtde=qtde+(pini-1);
  for (int i = pini; i <= qtde; i += 1)
  {
    tft.fillRect(PosHorBar[i-1], y, bar_width, bar_height, color);
  }

  tft.drawCentreString("40", 40, 176, 1);
  tft.drawCentreString("45", 85, 176, 1);
  tft.drawCentreString("50", 130, 176, 1);
  tft.drawCentreString("55", 182, 176, 1);
  tft.drawCentreString("60", 227, 176, 1);
  tft.drawCentreString("65", 272, 176, 1);
  tft.drawCentreString("70", 313, 176, 1);

  tft.drawFastHLine(0, 168, 316, TFT_WHITE);
  tft.drawFastHLine(4, 225, 316, TFT_WHITE);
  tft.drawFastVLine(155, 128, 105, TFT_WHITE);

  tft.fillRect(153, 163 , 5, 14, TFT_WHITE);
  tft.drawFastVLine(39, 163, 11, TFT_WHITE);
  tft.drawFastVLine(84, 163, 11, TFT_WHITE);
  tft.drawFastVLine(129, 163, 11, TFT_WHITE);
  tft.drawFastVLine(181, 163, 11, TFT_WHITE);
  tft.drawFastVLine(226, 163, 11, TFT_WHITE);
  tft.drawFastVLine(271, 163, 11, TFT_WHITE);
  tft.drawFastVLine(316, 163, 5, TFT_WHITE);

  tft.drawString("1", 1, 233, 1);
  tft.drawCentreString("5", 40, 233, 1);
  tft.drawCentreString("10", 85, 233, 1);
  tft.drawCentreString("15", 130, 233, 1);
  tft.drawCentreString("20", 182, 233, 1);
  tft.drawCentreString("25", 227, 233, 1);
  tft.drawCentreString("30", 272, 233, 1);
  tft.drawCentreString("35", 314, 233, 1);

  tft.fillRect(153, 220 , 5, 14, TFT_WHITE);
  tft.drawFastVLine(3, 220, 5, TFT_WHITE);
  tft.drawFastVLine(39, 220, 11, TFT_WHITE);
  tft.drawFastVLine(84, 220, 11, TFT_WHITE);
  tft.drawFastVLine(129, 220, 11, TFT_WHITE);
  tft.drawFastVLine(181, 220, 11, TFT_WHITE);
  tft.drawFastVLine(226, 220, 11, TFT_WHITE);
  tft.drawFastVLine(271, 220, 11, TFT_WHITE);
  tft.drawFastVLine(316, 220, 11, TFT_WHITE);
}
