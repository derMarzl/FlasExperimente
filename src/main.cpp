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

void setup() {
    Serial.begin(115200);
    wdt_disable();
}

void loop() {
  Serial.printf("\r\n\r\nStart\r\nFlashExpreimente\r\n");
  delay(3000);
  Serial.printf("\r\n\r\nund los\r\n");

  FlashInfo();
  //FlashCheckCRC();
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