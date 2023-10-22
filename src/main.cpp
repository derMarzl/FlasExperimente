#include <Arduino.h>
#include <FSTools.h>
#include <LittleFS.h>
#include <flash_hal.h>
#include <flash_quirks.h>
#include <flash_utils.h>
#include <FlashMap.h>

//extern "C" {
#include "spi_flash.h"
//}

void FlashInfo();
void FlashCheckCRC();
void SelfDestruct();
void MemStress();
void Test();
void leseSektor(uint32);
void löscheSektor(uint32, bool);
bool prüfeSektor(uint32, uint32, bool);
void schreibeSektor(uint32, uint32, bool);
void LEDan();
void LEDaus();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  LEDan();
  Serial.begin(1152000);
  wdt_disable();
}

void loop() {
  Serial.printf("\r\n\r\nStart\r\nFlashExpreimente\r\n");
  delay(3000);
  Serial.printf("\r\n\r\nund los\r\n");
  LEDaus();
  FlashInfo();
  //FlashCheckCRC();
  MemStress();

  //Test();

  while(true){ delay(100);wdt_reset();}
  delay(4000);
  //SelfDestruct();
  wdt_reset();
}

void FlashInfo()
{
  Serial.printf("\r\n\r\n");
  Serial.printf("FlashInfo\r\n");
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

  Serial.printf("Flash real id:   %08X\r\n", ESP.getFlashChipId());
  Serial.printf("Flash real size: %u bytes\r\n", realSize);

  Serial.printf("Flash ide  size: %u bytes\r\n", ideSize);
  Serial.printf("Flash ide speed: %u Hz\r\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash ide mode:  %s\r\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT"
                                                                    : ideMode == FM_DIO  ? "DIO"
                                                                    : ideMode == FM_DOUT ? "DOUT"
                                                                                         : "UNKNOWN"));

  if (ideSize != realSize) {
    Serial.println("Flash Chip configuration wrong!\r\n");
  } else {
    Serial.println("Flash Chip configuration ok.\r\n");
  }

  Serial.printf("Page size            \t%u\r\n",SPI_FLASH_SEC_SIZE);
  Serial.printf("FS_start             \t%u\r\n",FS_start);
  Serial.printf("FS_end               \t%u\r\n",FS_end);
  Serial.printf("FS_end-FS_start      \t%u\r\n",FS_end-FS_start);
  Serial.printf("FS_block             \t%u\r\n",FS_block);
  Serial.printf("FS_page              \t%u\r\n",FS_page);
  Serial.printf("FS_PHYS_PAGE         \t%u\r\n",FS_PHYS_PAGE);
  Serial.printf("FS_PHYS_SIZE         \t%u\r\n",FS_PHYS_SIZE);
  Serial.printf("(uint32_t)&_FS_start \t%u\r\n",(uint32_t)&_FS_start);
  Serial.printf("ESP.getSketchSize()  \t%u\r\n",ESP.getSketchSize());
}


//extern "C" {
//#include "spi_flash.h"
//}
// Artificially create a space in PROGMEM that fills multiple sectors so
// we can corrupt one without crashing the system
const int corruptme[SPI_FLASH_SEC_SIZE * 4] PROGMEM = { 0 };

void FlashCheckCRC()
{
  Serial.printf("\r\n\r\n");
  Serial.printf("Starting  FlashCheckCRC\r\n");
  Serial.printf("CRC check: %s\r\n", ESP.checkFlashCRC() ? "OK" : "ERROR");
  Serial.printf("...Corrupting a portion of flash in the array...\r\n");

  uint32_t ptr = (uint32_t)corruptme;
  // Find a page aligned spot inside the array
  ptr += 2 * SPI_FLASH_SEC_SIZE;
  ptr &= ~(SPI_FLASH_SEC_SIZE - 1);  // Sectoralign
  uint32_t sector = ((((uint32_t)ptr - 0x40200000) / SPI_FLASH_SEC_SIZE));

  // Create a sector with 1 bit set (i.e. fake corruption)
  uint32_t *space = (uint32_t *)calloc(SPI_FLASH_SEC_SIZE, 1);
  space[42] = 64;

  // Write it into flash at the spot in question
  spi_flash_erase_sector(sector);
  spi_flash_write(sector * SPI_FLASH_SEC_SIZE, (uint32_t *)space, SPI_FLASH_SEC_SIZE);
  Serial.printf("CRC check: %s\r\n", ESP.checkFlashCRC() ? "OK" : "ERROR");

  Serial.printf("...Correcting the flash...\r\n");
  memset(space, 0, SPI_FLASH_SEC_SIZE);
  spi_flash_erase_sector(sector);
  spi_flash_write(sector * SPI_FLASH_SEC_SIZE, (uint32_t *)space, SPI_FLASH_SEC_SIZE);
  Serial.printf("CRC check: %s\r\n", ESP.checkFlashCRC() ? "OK" : "ERROR");
}

void SelfDestruct()
{
  Serial.printf("\r\n\r\n");
  Serial.printf("Selbszerstörung aktiviert\r\n");

  for(int i =20; i>0; --i)
  {
    Serial.printf("%u\r\n",i);
    delay(1000);
  }
  Serial.printf("\r\n\r\nzu spät\r\n\r\n");

  wdt_disable();
  uint32_t AnzahlSektoren = (1024*1024)/SPI_FLASH_SEC_SIZE;
  noInterrupts();
  for(uint32_t i=AnzahlSektoren;i>0;--i)
  {
    Serial.printf("lösche Sektor %u\r\n",i);
    //spi_flash_erase_sector(i);
    //delay(200);
    wdt_reset();
  }
  Serial.printf("\r\nFertig\r\n");
  interrupts();

  delay(4000);
}


void MemStress()
{
  Serial.printf("\r\n\r\n");
  Serial.printf("MemStress\r\n");
  bool verbose=false;
  uint32_t letzterSektor = (FS_end / SPI_FLASH_SEC_SIZE);
  uint32_t vorletzterSektor= letzterSektor-1;
  uint32_t Sektor = vorletzterSektor;
  //uint32_t Sektor = 0;

  //leseSektor(Sektor);
  //leseSektor(998);
  Sektor=998;

  for(uint32 i=0;i<1000000000;++i)
  {
    LEDaus();
    Serial.printf("\r\nDurchgang Nr. %u\r\n",i);
    löscheSektor(Sektor, verbose);
    bool löschen = prüfeSektor(Sektor, 0xffffffff, verbose);
    if (!löschen) {delay(10000); verbose=true;}
    LEDan();
    schreibeSektor(Sektor,0, verbose);
    bool schreiben = prüfeSektor(Sektor, 0, verbose);
    if (!schreiben) {delay(10000); verbose=true;}
  }
}

bool prüfeSektor(uint32 Sektor, uint32 Wert,bool verbose)
{
  uint32_t Werte[SPI_FLASH_SEC_SIZE/4]{0}; // muss initialisert werden also mitndestens eine 0 rein schreiben
  uint32_t Start = Sektor*SPI_FLASH_SEC_SIZE;
  uint32_t Ende = ((Sektor+1)*SPI_FLASH_SEC_SIZE)-1;
  if(verbose) {Serial.printf("prüfe Sektor %u  also Adresse von %u  (0x%08x) bis %u  (0x%08x) auf den Wert 0x%08x\r\n",Sektor,Start,Start,Ende,Ende,Wert);}
  //SpiFlashOpResult Egenbnis =
  SpiFlashOpResult Ergebnis = spi_flash_read(Sektor * SPI_FLASH_SEC_SIZE,(uint32_t *)Werte,SPI_FLASH_SEC_SIZE);
  if(verbose|(Ergebnis!=0))
  {
    Serial.printf("spi_flash_read: ");
    Serial.print(Ergebnis);
    Serial.printf("\r\n");
  }
  bool OK=true;
  for(int i=0; i<(SPI_FLASH_SEC_SIZE/4);++i)
  {
    //wdt_reset();
    if(Werte[i]!=Wert)
    {
      OK=false;
      Serial.printf("0x%08X ist 0x%08X  anstatt 0x%08X\r\n",Start + i,Werte[i],Wert);
    }
  }
  if(OK)
  {
    Serial.printf("OK\r\n");
  }
  return OK;
}

void schreibeSektor(uint32 Sektor, uint32 Wert, bool verbose)
{
  uint32_t Start = Sektor*SPI_FLASH_SEC_SIZE;
  uint32_t Ende = ((Sektor+1)*SPI_FLASH_SEC_SIZE)-1;
  if(verbose) {Serial.printf("scheibe Sektor %u  also Adresse von %u  (0x%08x) bis %u  (0x%08x)  mit dem Wert 0x%08x\r\n",Sektor,Start,Start,Ende,Ende,Wert);}

  uint32_t Werte[SPI_FLASH_SEC_SIZE/4]{0};
  for(int i=0; i<(SPI_FLASH_SEC_SIZE/4);++i)
  {
    Werte[i]=Wert;
  }
  SpiFlashOpResult Ergebnis = spi_flash_write(Sektor * SPI_FLASH_SEC_SIZE,(uint32_t *)Werte,SPI_FLASH_SEC_SIZE);
  if(verbose|(Ergebnis!=0))
  {
    Serial.printf("spi_flash_write: ");
    Serial.print(Ergebnis);
    Serial.printf("\r\n");
  }
}

void löscheSektor(uint32 Sektor, bool verbose)
{
  uint32_t Start = Sektor*SPI_FLASH_SEC_SIZE;
  uint32_t Ende = ((Sektor+1)*SPI_FLASH_SEC_SIZE)-1;
  if(verbose) { Serial.printf("lösche Sektor %u  also Adresse von %u  (0x%08x) bis %u  (0x%08x)\r\n",Sektor,Start,Start,Ende,Ende);}
  SpiFlashOpResult Ergebnis = spi_flash_erase_sector(Sektor);
  if(verbose|(Ergebnis!=0))
  {
    Serial.printf("spi_flash_erase_sector: ");
    Serial.print(Ergebnis);
    Serial.printf("\r\n");
  }
}

void leseSektor(uint32 Sektor)
{
  //byte Bytes[SPI_FLASH_SEC_SIZE]{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,'a','b','c','\r','\n',' '};
  uint32_t Werte[SPI_FLASH_SEC_SIZE/4]{0}; // muss initialisert werden also mitndestens eine 0 rein schreiben

  //spi_flash_read(0,(uint32_t *)Bytes,SPI_FLASH_SEC_SIZE);  // mit HexEditor geprüft OK
  //ESP.flashRead(0,(uint32_t *)Bytes,4096);  // mit HexEditor geprüft OK

  uint32_t Start = Sektor*SPI_FLASH_SEC_SIZE;
  uint32_t Ende = ((Sektor+1)*SPI_FLASH_SEC_SIZE)-1;
  Serial.printf("lese Sektor %u  also Adresse von %u  (0x%08x) bis %u  (0x%08x)\r\n",Sektor,Start,Start,Ende,Ende);
  //SpiFlashOpResult Ergebnis = 
  SpiFlashOpResult Ergebnis = spi_flash_read(Sektor * SPI_FLASH_SEC_SIZE,(uint32_t *)Werte,SPI_FLASH_SEC_SIZE);
  Serial.printf("spi_flash_read: ");
  Serial.print(Ergebnis);
  Serial.printf("\r\n");
  for(int i=0; i<(SPI_FLASH_SEC_SIZE/4);++i)
  {
    //wdt_reset();
    Serial.printf("0x%04X = 0x%08X  \r\n",i,Werte[i]);
  }
}

void Test()
{
  Serial.printf("\r\n\r\n");
  Serial.printf("TestFunktion\r\n");

  byte Bytes[32]{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,'a','b','c','\r','\n',' '};
  for(int i=0; i<32;++i) // tesweise auch mal bis 64
  {
    //wdt_reset();
    Serial.printf("0x%02X = 0x%02X  \r\n",i,Bytes[i]);
  }
}

void LEDan() {
  digitalWrite(LED_BUILTIN, LOW);
  //digitalWrite(LED_PIN, LOW);
}

void LEDaus() {
  digitalWrite(LED_BUILTIN, HIGH);
  //digitalWrite(LED_PIN, HIGH);
}