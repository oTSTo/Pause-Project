// Created by Thomas with the help of Alesssandro,Jey,Noah
#include "Arduino.h"
#include "U8g2lib.h"
#include "Adafruit_NeoPixel.h"
#include <Wire.h>
#include <SPI.h>
#include "WiFi.h"
#include "NTPClient.h"
#include "TimeLib.h"
#include "SPIFFS.h"
#include "iostream"
#include "stdio.h"
// SCREEN
#define PIN 12
#define NUM_LEDS 24
// ENCODER
#define ENCODER_A 26
#define ENCODER_B 25
#define ENCODER_BUTTON 27
// ENCODER
volatile long pos = 0;
volatile int lastEncoded = 2;
unsigned long t1;
int bt, pvbt;
int UP = 0;
int DN = 0;
int CLIC = 0;
long t_btn = 0;
long debounce_delay = 500;
bool uscitaMENU = 0;
// WIFI
const char *ssid = "Omega2000";
const char *password = "zanyplum204";
const char *ntpServer = "europe.pool.ntp.org";
long gmtOffset_sec = 7200;        // 3600 - 3600
const int daylightOffset_sec = 0; // 3600
// BUZZER
const long durataCiclo = 5000; // 5 secondi in millisecondi
unsigned long tempoInizioCiclo = 0;
unsigned long tempoInizioAccensione = 0;
unsigned long tempoInizioSpegnimento = 0;

bool Pausa = false;
unsigned long tempoInizioPausa = 0;
unsigned long tempoPausa = 20000;
bool acceso = 1;
int cambiogmt = 0;
// ORARIO
int oreMattino = 0;
int minutiMattino = 0;
int orePomeriggio = 13;
int minutiPomeriggio = 0;
int pausa = 0;
int posizione = 16;
// PAUSA POMERIGGIO
int oreInizioMezzogiorno = 0;
int minutiInizioMezzogiorno = 0;
int oreFineMezzogiorno = 13;
int minutiFineMezzogiorno = 0;
int livelloLuminosita = 100;

String oreMatt = "/oreMattino.txt";
String minutiMatt = "/minutiMattino.txt";
String orePome = "/orePomeriggio.txt";
String minutiPome = "/minutiPomeriggio.txt";
String minutiPausa = "/pausa.txt";
String cambioGmt = "/cambioGMT.txt";
String cambioora = "/cambioora.txt";
String oreInizioMattMezz = "/oreInizioMattMezz.txt";
String minInizoPomeMezz = "/minInizoPomeMezz.txt";
String oreFineMattMezz = "/oreFineMattMezz.txt";
String minFinePomeMezz = "/minFinePomeMezz.txt";
String spiffLuminosita = "luminosita.txt";

time_t epoch;
U8G2_ST7567_OS12864_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/18, /* data=*/23, /* cs=*/15, /* dc=*/2, /* reset=*/4);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
const char *menuItems[] = {"Mattino", "Pomeriggio", "Durata Pausa", "Pausa mezzogiorno", "Cambio ora", "Luminosita", "Exit"};
const char *menuItems2[] = {"Ora legale", "Ora solare", "Exit"};
const char *menuItems3[] = {"Pausa ON/OFF", "Inizio pausa", "Fine pausa", "Exit"};
const char *menuItems4[] = {"On", "Off"};
int totaleItem = 7;
int totaleItem2 = 3;
int totaleItem3 = 4;
int totaleItem4 = 2;

int currentMenuItem = 0;
int currentMenuItem2 = 0;
int currentMenuItem3 = 0;
int currentMenuItem4 = 0;

int onOff = 0;

void contrasto(uint8_t contrasto)
{
  static char cstr[6];
  u8g2.setContrast(contrasto);
  strcpy(cstr, u8x8_u8toa(contrasto, 3));
}

void backlight()
{
  strip.begin();
  strip.setPixelColor(0, 255, 255, 255);
  strip.show();
}

void wifi()
{
  WiFi.begin(ssid, password);
  while (!Serial)
    ;
  Serial.println("Connessione al WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connesso!");
  Serial.println("Indirizzo IP: ");
  Serial.println(WiFi.localIP().toString());

  setenv("TZ", "GMT+00", 1);
  tzset();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm *timeinfo;
  if (!gettimeofday(&tv, NULL))
  {
    epoch = tv.tv_sec;
    struct tm *timeinfo = gmtime(&epoch);
    int hour = timeinfo->tm_hour;
    int min = timeinfo->tm_min;
    int sec = timeinfo->tm_sec;
  }
}
// ENCODER
void updateEncoder()
{
  int MSB = digitalRead(ENCODER_A); // MSB = most significant bit
  int LSB = digitalRead(ENCODER_B); // LSB = least significant bit

  int encoded = (MSB << 1) | LSB;         // converting the 2 pin value to single number
  int sum = (lastEncoded << 2) | encoded; // adding it to the previous encoded value

  // if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  // if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
  if (sum == 0b1000)
    pos++;
  if (sum == 0b0010)
    pos--;

  lastEncoded = encoded; // store this value for next time
}

void whileEncoder()
{
  if (UP)
  {
    currentMenuItem++;
    UP = 0;
    if (currentMenuItem >= totaleItem)
    { // Corrected comparison for wrapping
      currentMenuItem = 0;
    }
  }

  if (DN)
  {
    currentMenuItem--;
    DN = 0;
    if (currentMenuItem < 0)
    {
      currentMenuItem = totaleItem - 1; // Wrap around to the end
    }
  }
}

void whileEncoder2()
{
  if (UP)
  {
    currentMenuItem2++;
    UP = 0;
    if (currentMenuItem2 >= totaleItem2)
    { // Corrected comparison for wrapping
      currentMenuItem2 = 0;
    }
  }
  if (DN)
  {
    currentMenuItem2--;
    DN = 0;
    if (currentMenuItem2 < 0)
    {
      currentMenuItem2 = totaleItem2 - 1; // Wrap around to the end
    }
  }
}

void whileEncoder3()
{
  if (UP)
  {
    currentMenuItem3++;
    UP = 0;
    if (currentMenuItem3 >= totaleItem3)
    { // Corrected comparison for wrapping
      currentMenuItem2 = 0;
    }
  }
  if (DN)
  {
    currentMenuItem3--;
    DN = 0;
    if (currentMenuItem3 < 0)
    {
      currentMenuItem3 = totaleItem3 - 1; // Wrap around to the end
    }
  }
}

void whileEncoder4()
{
  if (UP)
  {
    currentMenuItem4++;
    UP = 0;
    if (currentMenuItem4 >= totaleItem4)
    { // Corrected comparison for wrapping
      currentMenuItem4 = 0;
    }
  }
  if (DN)
  {
    currentMenuItem4--;
    DN = 0;
    if (currentMenuItem4 < 0)
    {
      currentMenuItem4 = totaleItem4 - 1; // Wrap around to the end
    }
  }
}

void update()
{
  CLIC = 0;
  if ((millis() - t1) > 10)
  { //  UP  o   DN
    if (pos > 0)
      UP = 1;
    else if (pos < 0)
      DN = 1;
    pos = 0;
    t1 = millis();
  }

  if (!digitalRead(ENCODER_BUTTON) == HIGH)
  { //  CLIC
    if ((millis() - t_btn) > debounce_delay)
    {
      CLIC = 1;
      t_btn = millis();
    }
  }
}
// DELAY
void delayMillis(unsigned long durata)
{
  unsigned long tempoInizio = millis();
  while (millis() - tempoInizio < durata)
  {
    // Aspetta
  }
}
// CATEGORIE PAUSA MEZZOGIORNO
void pausaMezzogiornoOnOff()
{
  currentMenuItem3 = 0;
  while (true)
  {
    u8g2.firstPage();
    do
    {
      whileEncoder4();
      update();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 10, "PAUSA ON / OFF");
      int startItem4 = currentMenuItem3 - 1;
      if (startItem4 < 0)
        startItem4 = 0;
      int endItem4 = startItem4 + 3;
      if (endItem4 > totaleItem4 - 1)
        endItem4 = totaleItem4 - 1;
      for (int a = startItem4; a <= endItem4; a++)
      {
        if (a == currentMenuItem4)
        {
          u8g2.drawBox(0, 24 + a * 10 - (startItem4 * 10), 128, 10);
          u8g2.setDrawColor(0);
          u8g2.drawStr(2, 30 + a * 10 - (startItem4 * 10), menuItems4[a]);
          u8g2.setDrawColor(1);
        }
        else
        {
          u8g2.drawStr(2, 30 + a * 10 - (startItem4 * 10), menuItems4[a]);
        }
      }
    } while (u8g2.nextPage());

    if (CLIC)
    {
      switch (currentMenuItem4)
      {
      case 0: // On
        onOff = 1;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.drawStr(8, 34, "SUONO PAUSA ACCESO");
        u8g2.sendBuffer();
        delay(500);
        return;
        break;

      case 1: // Off
        onOff = 0;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.drawStr(8, 34, "SUONO PAUSA SPENTO");
        u8g2.sendBuffer();
        delay(500);
        return;
        break;
      }
      CLIC = 0;
    }

    if (currentMenuItem4 < 0)
    {
      currentMenuItem4 = totaleItem4 - 1;
    }
    else if (currentMenuItem4 >= totaleItem4)
    {
      currentMenuItem4 = 0;
    }
  }
}

void inizoPausaMezz()
{
  while (true)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(30, 50);
    u8g2.print("^");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(70, 28);
    u8g2.print(minutiInizioMezzogiorno);
    u8g2.drawStr(55, 28, ":");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(0, 28);
    update();
    if (UP)
    {
      oreInizioMezzogiorno++;
      UP = 0;
      // Serial.println(oreInizioMezzogiorno);
      if (oreInizioMezzogiorno > 12)
      {
        oreInizioMezzogiorno = 0;
      }
    }
    if (DN)
    {
      oreInizioMezzogiorno--;
      DN = 0;
      // Serial.println(oreMattino);
      if (oreInizioMezzogiorno < 0)
      {
        oreInizioMezzogiorno = 12;
      }
    }
    u8g2.print(oreInizioMezzogiorno);
    u8g2.sendBuffer();

    if (CLIC)
    {
      while (true)
      {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(95, 50);
        u8g2.print("^");
        u8g2.setFont(u8g2_font_fub35_tf);
        u8g2.drawStr(55, 28, ":");
        u8g2.setCursor(0, 28);
        u8g2.print(oreInizioMezzogiorno);
        u8g2.setCursor(70, 28);
        update();

        if (UP)
        {
          minutiInizioMezzogiorno++;
          UP = 0;
          // Serial.println(minutiInizioMezzogiorno);
          if (minutiInizioMezzogiorno > 59)
          {
            minutiInizioMezzogiorno = 0;
          }
        }
        if (DN)
        {
          minutiInizioMezzogiorno--;
          DN = 0;
          // Serial.println(minutiInizioMezzogiorno);
          if (minutiInizioMezzogiorno < 0)
          {
            minutiInizioMezzogiorno = 59;
          }
        }
        u8g2.println(minutiInizioMezzogiorno);

        if (CLIC)
        {
          File file = SPIFFS.open(oreInizioMattMezz, "w");
          file.print(oreInizioMezzogiorno);
          file.close();
          File fil = SPIFFS.open(minInizoPomeMezz, "w");
          fil.print(minutiInizioMezzogiorno);
          fil.close();
          return;
        }

        u8g2.sendBuffer();
      }
    }
  }
}

void finePausaMezz()
{
  while (true)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(30, 50);
    u8g2.print("^");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(70, 28);
    u8g2.print(minutiFineMezzogiorno);
    u8g2.drawStr(55, 28, ":");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(0, 28);
    update();
    if (UP)
    {
      oreFineMezzogiorno++;
      UP = 0;
      // Serial.println(oreFineMezzogiorno);
      if (oreFineMezzogiorno > 23)
      {
        oreFineMezzogiorno = 13;
      }
    }
    if (DN)
    {
      oreFineMezzogiorno--;
      DN = 0;
      // Serial.println(oreMattino);
      if (oreFineMezzogiorno < 13)
      {
        oreFineMezzogiorno = 23;
      }
    }
    u8g2.print(oreFineMezzogiorno);
    u8g2.sendBuffer();

    if (CLIC)
    {
      while (true)
      {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(95, 50);
        u8g2.print("^");
        u8g2.setFont(u8g2_font_fub35_tf);
        u8g2.drawStr(55, 28, ":");
        u8g2.setCursor(0, 28);
        u8g2.print(oreFineMezzogiorno);
        u8g2.setCursor(70, 28);
        update();

        if (UP)
        {
          minutiFineMezzogiorno++;
          UP = 0;
          // Serial.println(minutiFineMezzogiorno);
          if (minutiFineMezzogiorno > 59)
          {
            minutiFineMezzogiorno = 0;
          }
        }
        if (DN)
        {
          minutiFineMezzogiorno--;
          DN = 0;
          // Serial.println(minutiFineMezzogiorno);
          if (minutiFineMezzogiorno < 0)
          {
            minutiFineMezzogiorno = 59;
          }
        }
        u8g2.println(minutiFineMezzogiorno);

        if (CLIC)
        {
          File file = SPIFFS.open(oreFineMattMezz, "w");
          file.print(oreFineMezzogiorno);
          file.close();
          File fil = SPIFFS.open(minFinePomeMezz, "w");
          fil.print(minutiFineMezzogiorno);
          fil.close();
          return;
        }

        u8g2.sendBuffer();
      }
    }
  }
}
// MENU
void drawMenu()
{
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "IMPOSTAZIONI");

    // Calculate which items should be visible based on the current position
    int startItem = currentMenuItem - 1; // Adjust for viewing at least one item above
    if (startItem < 0)
    {
      startItem = 0;
    }
    int endItem = startItem + 3; // Show 4 items at a time
    if (endItem > totaleItem - 1)
    {
      endItem = totaleItem - 1;
    }

    // Redraw only the necessary items
    for (int i = startItem; i <= endItem; i++)
    {
      if (i == currentMenuItem)
      {
        u8g2.drawBox(0, 24 + i * 10 - (startItem * 10), 128, 10);
        u8g2.setDrawColor(0);
        u8g2.drawStr(2, 30 + i * 10 - (startItem * 10), menuItems[i]);
        u8g2.setDrawColor(1);
      }
      else
      {
        u8g2.drawStr(2, 30 + i * 10 - (startItem * 10), menuItems[i]);
      }
    }
  } while (u8g2.nextPage());
}

void azioneMattino()
{
  while (true)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(30, 50);
    u8g2.print("^");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(70, 28);
    u8g2.print(minutiMattino);
    u8g2.drawStr(55, 28, ":");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(0, 28);
    update();
    if (UP)
    {
      oreMattino++;
      UP = 0;
      // Serial.println(oreMattino);
      if (oreMattino > 12)
      {
        oreMattino = 0;
      }
    }
    if (DN)
    {
      oreMattino--;
      DN = 0;
      // Serial.println(oreMattino);
      if (oreMattino < 0)
      {
        oreMattino = 12;
      }
    }
    u8g2.print(oreMattino);
    u8g2.sendBuffer();

    if (CLIC)
    {
      while (true)
      {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(95, 50);
        u8g2.print("^");
        u8g2.setFont(u8g2_font_fub35_tf);
        u8g2.drawStr(55, 28, ":");
        u8g2.setCursor(0, 28);
        u8g2.print(oreMattino);
        u8g2.setCursor(70, 28);
        update();

        if (UP)
        {
          minutiMattino++;
          UP = 0;
          // Serial.println(minutiMattino);
          if (minutiMattino > 59)
          {
            minutiMattino = 0;
          }
        }
        if (DN)
        {
          minutiMattino--;
          DN = 0;
          // Serial.println(minutiMattino);
          if (minutiMattino < 0)
          {
            minutiMattino = 59;
          }
        }
        u8g2.println(minutiMattino);

        if (CLIC)
        {
          File file = SPIFFS.open(oreMatt, "w");
          file.print(oreMattino);
          file.close();
          File fil = SPIFFS.open(minutiMatt, "w");
          fil.print(minutiMattino);
          fil.close();
          return;
        }

        u8g2.sendBuffer();
      }
    }
  }
}

void azionePomeriggio()
{
  while (true)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(30, 50);
    u8g2.print("^");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(70, 28);
    u8g2.print(minutiPomeriggio);
    u8g2.drawStr(55, 28, ":");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(0, 28);
    update();
    if (UP)
    {
      orePomeriggio++;
      UP = 0;
      // Serial.println(orePomeriggio);
      if (orePomeriggio > 23)
      {
        orePomeriggio = 13;
      }
    }
    if (DN)
    {
      orePomeriggio--;
      DN = 0;
      // Serial.println(orePomeriggio);
      if (orePomeriggio < 13)
      {
        orePomeriggio = 23;
      }
    }
    u8g2.print(orePomeriggio);
    u8g2.sendBuffer();

    if (CLIC)
    {
      while (true)
      {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.setCursor(95, 50);
        u8g2.print("^");
        u8g2.setFont(u8g2_font_fub35_tf);
        u8g2.drawStr(55, 28, ":");
        u8g2.setCursor(0, 28);
        u8g2.print(orePomeriggio);
        u8g2.setCursor(70, 28);
        update();

        if (UP)
        {
          minutiPomeriggio++;
          UP = 0;
          if (minutiPomeriggio > 59)
          {
            minutiPomeriggio = 0;
          }
        }
        if (DN)
        {
          minutiPomeriggio--;
          DN = 0;
          // Serial.println(minutiPomeriggio);
          if (minutiPomeriggio < 0)
          {
            minutiPomeriggio = 59;
          }
        }
        u8g2.println(minutiPomeriggio);

        if (CLIC)
        {
          File fi = SPIFFS.open(orePome, "w");
          fi.print(orePomeriggio);
          fi.close();
          File f = SPIFFS.open(minutiPome, "w");
          f.print(minutiPomeriggio);
          f.close();
          return;
        }

        u8g2.sendBuffer();
      }
    }
  }
}

void durataPausa()
{
  while (true)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(70, 20, "MINUTI");
    u8g2.drawStr(70, 30, "  DI");
    u8g2.drawStr(70, 40, "PAUSA");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(20, 34);
    update();
    if (UP)
    {
      pausa++;
      UP = 0;
      if (pausa > 59)
      {
        pausa = 0;
      }
    }
    if (DN)
    {
      pausa--;
      DN = 0;
      if (pausa < 0)
      {
        pausa = 59;
      }
    }
    if (pausa < 10)
    {
      u8g2.setCursor(30, 34);
      u8g2.print(pausa);
    }
    else
    {
      u8g2.setCursor(13, 34);
      u8g2.print(pausa);
    }
    u8g2.sendBuffer();

    if (CLIC)
    {
      File fi = SPIFFS.open(minutiPausa, "w");
      fi.print(pausa);
      fi.close();
      File f = SPIFFS.open(minutiPausa, "w");
      f.print(pausa);
      f.close();
      return;
    }
    u8g2.sendBuffer();
  }
}

void pausaMezzogiorno()
{
  currentMenuItem3 = 0;
  while (true)
  {
    u8g2.firstPage();
    do
    {
      whileEncoder3();
      update();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 10, "PAUSA MEZZOGIORNO");
      int startItem3 = currentMenuItem3 - 1;
      if (startItem3 < 0)
        startItem3 = 0;
      int endItem3 = startItem3 + 3;
      if (endItem3 > totaleItem3 - 1)
        endItem3 = totaleItem3 - 1;
      for (int a = startItem3; a <= endItem3; a++)
      {
        if (a == currentMenuItem3)
        {
          u8g2.drawBox(0, 24 + a * 10 - (startItem3 * 10), 128, 10);
          u8g2.setDrawColor(0);
          u8g2.drawStr(2, 30 + a * 10 - (startItem3 * 10), menuItems3[a]);
          u8g2.setDrawColor(1);
        }
        else
        {
          u8g2.drawStr(2, 30 + a * 10 - (startItem3 * 10), menuItems3[a]);
        }
      }
    } while (u8g2.nextPage());

    if (CLIC)
    {
      switch (currentMenuItem3)
      {
      case 0: // Pausa ON/OFF
        pausaMezzogiornoOnOff();
        break;

      case 1: // Inizio pausa
        inizoPausaMezz();
        break;

      case 2: // Fine pausa
        finePausaMezz();
        break;

      case 3: // Exit
        return;
        break;
      }
      CLIC = 0;
    }

    if (currentMenuItem3 < 0)
    {
      currentMenuItem3 = totaleItem3 - 1;
    }
    else if (currentMenuItem3 >= totaleItem3)
    {
      currentMenuItem3 = 0;
    }
  }
}

void cambioOra()
{
  while (true)
  {
    u8g2.firstPage();
    do
    {
      whileEncoder2();
      update();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(0, 10, "CAMBIO ORA");

      // Calculate which items should be visible
      int startItem2 = currentMenuItem2 - 1;
      if (startItem2 < 0)
        startItem2 = 0;
      int endItem2 = startItem2 + 3; // Show 4 items at a time
      if (endItem2 > totaleItem2 - 1)
        endItem2 = totaleItem2 - 1;

      // Redraw only the necessary items
      for (int a = startItem2; a <= endItem2; a++)
      {
        if (a == currentMenuItem2)
        {
          u8g2.drawBox(0, 24 + a * 10 - (startItem2 * 10), 128, 10);
          u8g2.setDrawColor(0);
          u8g2.drawStr(2, 30 + a * 10 - (startItem2 * 10), menuItems2[a]);
          u8g2.setDrawColor(1);
        }
        else
        {
          u8g2.drawStr(2, 30 + a * 10 - (startItem2 * 10), menuItems2[a]);
        }
      }
    } while (u8g2.nextPage());

    // Handle selection (CLIC)
    if (CLIC)
    {
      if (currentMenuItem2 == 0)
      {                                                           // Ora legale
        gmtOffset_sec = 7200;                                     // Imposta l'offset per l'ora legale
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Aggiorna l'orario
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.drawStr(8, 34, "ORA LEGALE SELEZIONATA");
        File fi = SPIFFS.open(cambioora, "w");
        fi.print(gmtOffset_sec);
        fi.close();
        u8g2.sendBuffer();
        delay(500);
        break;
      }
      else if (currentMenuItem2 == 1)
      {                                                           // Ora solare
        gmtOffset_sec = 3600;                                     // Imposta l'offset per l'ora solare
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); // Aggiorna l'orario
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_5x8_tr);
        u8g2.drawStr(8, 34, "ORA SOLARE SELEZIONATA");
        File fi = SPIFFS.open(cambioora, "w");
        fi.print(gmtOffset_sec);
        fi.close();
        u8g2.sendBuffer();
        delay(500);
        break;
      }
      else if (currentMenuItem2 == 2)
      { // Exit
        break;
      }
      CLIC = 0;
    }
  }
}

void luminosita()
{
  while (true)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "LUMINOSITA");
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(20, 34);
    update();
    if (UP)
    {
      livelloLuminosita++;
      UP = 0;
      if (livelloLuminosita > 255)
      {
        livelloLuminosita = 0;
      }
    }
    if (DN)
    {
      livelloLuminosita--;
      DN = 0;
      if (livelloLuminosita < 0)
      {
        livelloLuminosita = 255;
      }
    }
    if (livelloLuminosita < 10)
    {
      u8g2.setCursor(30, 34);
      u8g2.print(livelloLuminosita);
    }
    else
    {
      u8g2.setCursor(13, 34);
      u8g2.print(livelloLuminosita);
    }
    u8g2.sendBuffer();
    if (CLIC)
    {
      File fi = SPIFFS.open(spiffLuminosita, "w");
      fi.print(livelloLuminosita);
      fi.close();
      return;
    }
    u8g2.sendBuffer();
  }
}

// SALVATAGGIO SPIFFS
void salvataggio()
{
  // ORE MATTINO
  File fileOreMatt = SPIFFS.open(oreMatt, "r");
  if (fileOreMatt.available())
  {
    String strOreMattino = fileOreMatt.readString();
    oreMattino = strOreMattino.toInt();
    // Serial.println(oreMattino);
  }
  fileOreMatt.close();
  // MINUTI MATTINO
  File fileMinutiMatt = SPIFFS.open(minutiMatt, "r");
  if (fileMinutiMatt.available())
  {
    String strMinutiMattino = fileMinutiMatt.readString();
    minutiMattino = strMinutiMattino.toInt();
    // Serial.println(minutiMattino);
  }
  fileMinutiMatt.close();
  // ORE POMERIGGIO
  File fileOrePome = SPIFFS.open(orePome, "r");
  if (fileOrePome.available())
  {
    String strOrePomeriggio = fileOrePome.readString();
    orePomeriggio = strOrePomeriggio.toInt();
    // Serial.println(orePomeriggio);
  }
  fileOrePome.close();
  // MINUTI POMERIGGIO
  File fileMinutiPome = SPIFFS.open(minutiPome, "r");
  if (fileMinutiPome.available())
  {
    String strMinutiPomeriggio = fileMinutiPome.readString();
    minutiPomeriggio = strMinutiPomeriggio.toInt();
    // Serial.println(minutiPomeriggio);
  }
  fileMinutiPome.close();
  // PAUSA
  File fileMinutiPausa = SPIFFS.open(minutiPausa, "r");
  if (fileMinutiPausa.available())
  {
    String strMinutiPausa = fileMinutiPausa.readString();
    pausa = strMinutiPausa.toInt();
  }
  fileMinutiPome.close();
  // CAMBIO ORA
  File fileCambioOra = SPIFFS.open(cambioora, "r");
  if (fileCambioOra.available())
  {
    String strCambioOra = fileCambioOra.readString();
    gmtOffset_sec = strCambioOra.toInt();
  }
  fileCambioOra.close();
  // ORE INIZO MEZZOGIORNO
  File fileOreInizioMezzogiorno = SPIFFS.open(oreInizioMattMezz, "r");
  if (fileOreInizioMezzogiorno.available())
  {
    String strOreInizioMezzogiorno = fileOreInizioMezzogiorno.readString();
    oreInizioMezzogiorno = strOreInizioMezzogiorno.toInt();
  }
  fileOreInizioMezzogiorno.close();

  // MINUTI INIZO MEZZOGIORNO
  File fileMinutiInizioMezzogiorno = SPIFFS.open(minInizoPomeMezz, "r");
  if (fileMinutiInizioMezzogiorno.available())
  {
    String strMinInizioMezzogiorno = fileMinutiInizioMezzogiorno.readString();
    minutiInizioMezzogiorno = strMinInizioMezzogiorno.toInt();
  }
  fileMinutiInizioMezzogiorno.close();

  // ORE FINE MEZZOGIORNO
  File fileOreFineMezzogiorno = SPIFFS.open(oreFineMattMezz, "r");
  if (fileOreFineMezzogiorno.available())
  {
    String strOreFineMezzogiorno = fileOreFineMezzogiorno.readString();
    oreFineMezzogiorno = strOreFineMezzogiorno.toInt();
  }
  fileOreFineMezzogiorno.close();

  // MINUTI FINE MEZZOGIORNO
  File fileMinutiFineMezzogiorno = SPIFFS.open(minFinePomeMezz, "r");
  if (fileMinutiFineMezzogiorno.available())
  {
    String strMinFineMezzogiorno = fileMinutiFineMezzogiorno.readString();
    minutiFineMezzogiorno = strMinFineMezzogiorno.toInt();
  }
  fileMinutiFineMezzogiorno.close();
  // LUMINOSITA
  File fileLuminosita = SPIFFS.open(spiffLuminosita, "r");
  if (fileLuminosita.available())
  {
    String strLuminosita = fileLuminosita.readString();
    livelloLuminosita = strLuminosita.toInt();
  }
  fileMinutiPome.close();
}
// SCRITTURA ORARIO
void orario()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  // Serial.println(&timeinfo, "%H:%M"); /*"%A, %B %d %Y %H:%M:%S"*/
  u8g2.firstPage();
  do
  {
    // u8g2.setFont(u8g2_font_ncenB14_tr);
    Serial.println(&timeinfo, "%H:%M");

    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_fub35_tf);
    u8g2.setCursor(0, 28);
    u8g2.print(&timeinfo, "%H:%M");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(62, 50);
    u8g2.print(&timeinfo, "%S");
    // DATA
    //  u8g2.print(&timeinfo, "%a %d %y");

  } while (u8g2.nextPage());

  if (((timeinfo.tm_hour == oreMattino) && (timeinfo.tm_min == minutiMattino)) ||
      ((timeinfo.tm_hour == orePomeriggio) && (timeinfo.tm_min == minutiPomeriggio)) ||
      ((timeinfo.tm_hour == oreMattino) && (timeinfo.tm_min == minutiMattino + pausa)) ||
      ((timeinfo.tm_hour == orePomeriggio) && (timeinfo.tm_min == minutiPomeriggio + pausa)) ||
      ((timeinfo.tm_hour == oreInizioMezzogiorno) && (timeinfo.tm_min == minutiInizioMezzogiorno) && (onOff == 1)) ||
      ((timeinfo.tm_hour == oreFineMezzogiorno) && (timeinfo.tm_min == minutiFineMezzogiorno) && (onOff == 1)) ||
      ((timeinfo.tm_hour == 18) && (timeinfo.tm_min == 00)))
  {
    Serial.println("ORA GIUSTA");
    if (acceso == 1)
    {
      tempoInizioCiclo = millis();
      while ((millis() - tempoInizioCiclo) <= durataCiclo)
      {
        digitalWrite(13, HIGH);
        strip.setPixelColor(0, 0, 255, 0); // ROSSO
        strip.show();
        Serial.println(tempoInizioPausa);
        delayMillis(200);

        digitalWrite(13, LOW);
        delayMillis(200);

        digitalWrite(13, HIGH);
        strip.setPixelColor(0, 255, 255, 0); // GIALLO
        strip.show();
        Serial.println(tempoInizioPausa);
        delayMillis(200);

        digitalWrite(13, LOW);
        delayMillis(200);

        digitalWrite(13, HIGH);
        strip.setPixelColor(0, 255, 255, 255); // BIANCO
        strip.show();
        Serial.println(tempoInizioPausa);
        delayMillis(200);

        digitalWrite(13, LOW);
        delayMillis(200);

        digitalWrite(13, HIGH);
        strip.setPixelColor(0, 0, 255, 255); // VIOLA
        strip.show();
        Serial.println(tempoInizioPausa);
        delayMillis(200);

        digitalWrite(13, LOW);
        delayMillis(200);

        digitalWrite(13, HIGH);
        strip.setPixelColor(0, 255, 255, 255); // BIANCO
        strip.show();
        Serial.println(tempoInizioPausa);
        delayMillis(200);

        digitalWrite(13, LOW);
        delayMillis(200);
        Serial.println(tempoInizioPausa);

        acceso = 0;
      }
      Pausa = 1;
    }
  }
  else
  {
    acceso = 1;
  }
}

void setup()
{
  Serial.begin(115200);
  u8g2.begin();
  u8g2.setFlipMode(1);
  SPIFFS.begin();
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B), updateEncoder, CHANGE);

  salvataggio();
  contrasto(1);
  backlight();
  wifi();
}

void loop()
{
  orario();
  Serial.println(gmtOffset_sec);
  // Serial.println(orePomeriggio);
  update();
  // MENU
  if (CLIC)
  {
    currentMenuItem = 0;
    while (uscitaMENU == 0)
    {
      drawMenu();
      update();
      whileEncoder();

      // EXIT
      if (CLIC && currentMenuItem == 6)
      {
        currentMenuItem = 0;
        CLIC = 0;
        uscitaMENU = 1;
      }

      if (uscitaMENU == 0)
      {
        switch (currentMenuItem)
        {
        case 0: // Mattino
          if (CLIC)
          {
            azioneMattino();
          }
          break;
        case 1: // Pomerigio
          if (CLIC)
          {
            azionePomeriggio();
          }
          break;
        case 2: // Durata Pausa
          if (CLIC)
          {
            durataPausa();
          }
          break;
        case 3: // Pausa mezzogiorno
          if (CLIC)
          {
            pausaMezzogiorno();
          }
          break;
        case 4: // Cambio ora
          if (CLIC)
          {
            cambioOra();
          }
          break;
        case 5: // Luminosita
          if (CLIC)
          {
            luminosita();
          }
          break;
        }
      }
      CLIC = 0;
    }

    CLIC = 0;
    UP = 0;
    DN = 0;
    uscitaMENU = 0;
  }
}