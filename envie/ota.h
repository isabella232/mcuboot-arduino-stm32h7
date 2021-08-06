#ifndef __OTA_H
#define __OTA_H

#define INTERNAL_FLASH_FLAG         (1 << 1)
#define QSPI_FLASH_FLAG             (1 << 2)
#define SDCARD_FLAG                 (1 << 3)
#define RAW_FLAG                    (1 << 4)
#define FATFS_FLAG                  (1 << 5)
#define LITTLEFS_FLAG               (1 << 6)
#define MBR_FLAG                    (1 << 7)

enum storageType {
    INTERNAL_FLASH_OFFSET = INTERNAL_FLASH_FLAG | RAW_FLAG,
    INTERNAL_FLASH_FATFS = INTERNAL_FLASH_FLAG | FATFS_FLAG,
    INTERNAL_FLASH_LITTLEFS = INTERNAL_FLASH_FLAG | LITTLEFS_FLAG,
    QSPI_FLASH_OFFSET = QSPI_FLASH_FLAG | RAW_FLAG,
    QSPI_FLASH_FATFS = QSPI_FLASH_FLAG | FATFS_FLAG,
    QSPI_FLASH_LITTLEFS = QSPI_FLASH_FLAG | LITTLEFS_FLAG,
    QSPI_FLASH_FATFS_MBR = QSPI_FLASH_FLAG | FATFS_FLAG | MBR_FLAG,
    QSPI_FLASH_LITTLEFS_MBR = QSPI_FLASH_FLAG | LITTLEFS_FLAG | MBR_FLAG,
    SD_OFFSET = SDCARD_FLAG | RAW_FLAG,
    SD_FATFS = SDCARD_FLAG | FATFS_FLAG,
    SD_LITTLEFS = SDCARD_FLAG | LITTLEFS_FLAG,
    SD_FATFS_MBR = SDCARD_FLAG | FATFS_FLAG | MBR_FLAG,
    SD_LITTLEFS_MBR = SDCARD_FLAG | LITTLEFS_FLAG | MBR_FLAG,
};

#define WRONG_OTA_BINARY     (-1)
#define MOUNT_FAILED         (-2)
#define NO_OTA_FILE          (-3)
#define INIT_FAILED          (-4)

//int tryOTA(enum storageType storage_type, uint32_t data_offset, uint32_t update_size);
int setOTAData(enum storageType storage_type, uint32_t data_offset, uint32_t update_size);
int getOTAData(enum storageType* storage_type, uint32_t* data_offset, uint32_t* update_size);

#endif //__OTA_H