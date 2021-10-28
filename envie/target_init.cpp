#if MCUBOOT_AS_ENVIE

#include "mbed.h"
#include "target_init.h"
#include "ota.h"
#include "rtc.h"
#include "bootutil/bootutil_log.h"

// clock source is selected with CLOCK_SOURCE in json config
#define USE_PLL_HSE_EXTC     0x8  // Use external clock (ST Link MCO)
#define USE_PLL_HSE_XTAL     0x4  // Use external xtal (X3 on board - not provided by default)
#define USE_PLL_HSI          0x2  // Use HSI internal clock

volatile const uint8_t bootloader_data[] __attribute__ ((section (".bootloader_version"), used)) = {
  BOOTLOADER_CONFIG_MAGIC,
  BOOTLOADER_VERSION,
  CLOCK_SOURCE,
  PORTENTA_USB_SPEED,
  PORTENTA_HAS_ETHERNET,
  PORTENTA_HAS_WIFI,
  PORTENTA_RAM_SIZE,
  PORTENTA_QSPI_SIZE,
  PORTENTA_HAS_VIDEO,
  PORTENTA_HAS_CRYPTO,
  PORTENTA_EXTCLOCK,
};

static bool double_tap_flag = true;
volatile uint8_t ledKeepValue = 0;
volatile uint8_t ledTargetValue = 20;
volatile int8_t ledDirection = 1;
volatile int divisor = 0;

DigitalOut led(PK_6);

static inline void LED_pulse(DigitalOut* led)
{
  if (divisor++ % 40) {
    return;
  }

  if (HAL_GetTick() > 500 && double_tap_flag && RTCGetBKPRegister(RTC_BKP_DR0) == 0xDF59) {
    RTCSetBKPRegister(RTC_BKP_DR0, 0);
    double_tap_flag = false;
  }

  if (ledKeepValue == 0) {
    ledTargetValue += ledDirection;
    *led = !*led;
  }
  ledKeepValue ++;

  if (ledTargetValue > 250 || ledTargetValue < 10) {
    ledDirection = -ledDirection;
    ledTargetValue += ledDirection;
  }

  if (ledKeepValue == ledTargetValue) {
    *led = !*led;
  }
}


int target_init(void) {
  DigitalIn boot_sel(PI_8,PullDown);

  int magic = RTCGetBKPRegister(RTC_BKP_DR0);
  BOOT_LOG_DBG("Envie magic 0x%x", magic);

  // in case we have been reset let's wait 500 ms to see if user is trying to stay in bootloader
  if (ResetReason::get() == RESET_REASON_PIN_RESET) {
    // now that we've been reset let's set magic. resetting with this set will
    // flag we need to stay in bootloader.
    RTCSetBKPRegister(RTC_BKP_DR0, 0xDF59);
    HAL_Delay(500);
    BOOT_LOG_DBG("Envie magic set 0x%x", RTCGetBKPRegister( RTC_BKP_DR0));
  }

  DigitalOut usb_reset(PJ_4, 0);
  DigitalOut video_enable(PJ_2, 0);
  DigitalOut video_reset(PJ_3, 0);

  //Ticker pulse;
  //DigitalOut eth_rst(PJ_15, 1);

  HAL_FLASH_Unlock();

  I2C i2c(PB_7, PB_6);

  char data[2];

  // LDO2 to 1.8V
  data[0]=0x4F;
  data[1]=0x0;
  i2c.write(8 << 1, data, sizeof(data));
  data[0]=0x50;
  data[1]=0xF;
  i2c.write(8 << 1, data, sizeof(data));

  // LDO1 to 1.0V
  data[0]=0x4c;
  data[1]=0x5;
  i2c.write(8 << 1, data, sizeof(data));
  data[0]=0x4d;
  data[1]=0x3;
  i2c.write(8 << 1, data, sizeof(data));

  // LDO3 to 1.2V
  data[0]=0x52;
  data[1]=0x9;
  i2c.write(8 << 1, data, sizeof(data));
  data[0]=0x53;
  data[1]=0xF;
  i2c.write(8 << 1, data, sizeof(data));

  HAL_Delay(10);

  data[0]=0x9C;
  data[1]=(1 << 7);
  i2c.write(8 << 1, data, sizeof(data));

  // Disable charger led
  data[0]=0x9E;
  data[1]=(1 << 5);
  i2c.write(8 << 1, data, sizeof(data));

  HAL_Delay(10);

  // SW3: set 2A as current limit
  // Helps keeping the rail up at wifi startup
  data[0]=0x42;
  data[1]=(2);
  i2c.write(8 << 1, data, sizeof(data));

  HAL_Delay(10);

  // Change VBUS INPUT CURRENT LIMIT to 1.5A
  data[0]=0x94;
  data[1]=(20 << 3);
  i2c.write(8 << 1, data, sizeof(data));

#if 1
  // SW2 to 3.3V (SW2_VOLT)
  data[0]=0x3B;
  data[1]=0xF;
  i2c.write(8 << 1, data, sizeof(data));

  // SW1 to 3.0V (SW1_VOLT)
  data[0]=0x35;
  data[1]=0xF;
  i2c.write(8 << 1, data, sizeof(data));

  //data[0]=0x36;
  //data[1]=(2);
  //i2c.write(8 << 1, data, sizeof(data));
#endif

  HAL_Delay(10);

  usb_reset = 1;
  HAL_Delay(10);
  usb_reset = 0;
  HAL_Delay(10);
  usb_reset = 1;

  HAL_Delay(10);

  /* Test if user code is programmed starting from USBD_DFU_APP_DEFAULT_ADD
   * address. TODO check MCUBoot header instead.
   */
  //int app_valid = (((*(__IO uint32_t *) 0x08040000) & 0xFF000000) == 0x20000000)
  //             || (((*(__IO uint32_t *) 0x08040000) & 0xFF000000) == 0x24000000)
  //             || (((*(__IO uint32_t *) 0x08040000) & 0xFF000000) == 0x30000000)
  //             || (((*(__IO uint32_t *) 0x08040000) & 0xFF000000) == 0x38000000);

  if (/*app_valid &&*/ magic != 0xDF59 /*&& magic != 0x07AA*/ && boot_sel==0) {
    RTCSetBKPRegister(RTC_BKP_DR0, 0);
    HAL_FLASH_Lock();
    BOOT_LOG_DBG("Envie app magic 0x%x", RTCGetBKPRegister(RTC_BKP_DR0));
    return 0;

  } else {
    BOOT_LOG_DBG("Envie loop magic 0x%x", RTCGetBKPRegister(RTC_BKP_DR0));
    if(boot_sel) {
      return 1;
    } else  {
      return 2;
    }
  }
}

#if MCUBOOT_ENVIE_DFU
USBD_HandleTypeDef USBD_Device;
extern PCD_HandleTypeDef hpcd;
extern void init_Memories(void);
#endif

extern "C" {
  uint8_t SetSysClock_PLL_HSE(uint8_t bypass, bool lowspeed);
}

void envie_loop(void) {
  RTCSetBKPRegister(RTC_BKP_DR0, 0);

  SetSysClock_PLL_HSE(1, false);
  SystemCoreClockUpdate();

  //turnDownEthernet();

#if MCUBOOT_ENVIE_DFU
  init_Memories();

  /* Otherwise enters DFU mode to allow user programming his application */
  /* Init Device Library */
  USBD_Init(&USBD_Device, &DFU_Desc, 0);

  /* Add Supported Class */
  USBD_RegisterClass(&USBD_Device, USBD_DFU_CLASS);

  /* Add DFU Media interface */
  USBD_DFU_RegisterMedia(&USBD_Device, &USBD_DFU_Flash_fops);

  /* Start Device Process */
  USBD_Start(&USBD_Device);

  /* Set USBHS Interrupt to the lowest priority */
  // HAL_NVIC_SetPriority(OTG_HS_IRQn, 1, 0);

  /* Enable USBHS Interrupt */
  HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
  HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
#endif

  while(1) {
#if MCUBOOT_ENVIE_DFU
#ifdef USE_USB_HS
    if (USB_OTG_HS->GINTSTS & USB_OTG_HS->GINTMSK) {
#else // USE_USB_FS
    if (USB_OTG_FS->GINTSTS & USB_OTG_FS->GINTMSK) {
#endif
      HAL_PCD_IRQHandler(&hpcd);
    }
#endif
    LED_pulse(&led);
  }
}

#endif