/*
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hardware_legacy/wifi.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <android-base/logging.h>
#include <cutils/misc.h>
#include <cutils/properties.h>
#include "private/android_filesystem_config.h"
#include <sys/stat.h>
#include <sys/types.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#include "libusb/libusb.h"
#define finit_module(fd, opts, flags) syscall(SYS_finit_module, fd, opts, flags)
extern "C" int delete_module(const char *, unsigned int);

#ifndef WIFI_DRIVER_FW_PATH_STA
#define WIFI_DRIVER_FW_PATH_STA NULL
#endif
#ifndef WIFI_DRIVER_FW_PATH_AP
#define WIFI_DRIVER_FW_PATH_AP NULL
#endif
#ifndef WIFI_DRIVER_FW_PATH_P2P
#define WIFI_DRIVER_FW_PATH_P2P NULL
#endif

#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG ""
#endif
#ifndef MULTI_WIFI_SUPPORT
static const char DRIVER_PROP_NAME[] = "wlan.driver.status";
#ifdef WIFI_DRIVER_MODULE_PATH
static const char DRIVER_MODULE_NAME[] = WIFI_DRIVER_MODULE_NAME;
static const char DRIVER_MODULE_TAG[] = WIFI_DRIVER_MODULE_NAME " ";
static const char DRIVER_MODULE_PATH[] = WIFI_DRIVER_MODULE_PATH;
static const char DRIVER_MODULE_ARG[] = WIFI_DRIVER_MODULE_ARG;
static const char MODULE_FILE[] = "/proc/modules";
#endif
#endif
#ifdef MULTI_WIFI_SUPPORT
char file_name[100];
static int load_dongle_index = -1;
typedef struct load_info{
    const char *chip_id;
    const char *wifi_module_name;
    const char *wifi_module_path;
    const char *wifi_module_arg;
    const char *wifi_name;
    const int wifi_pid;
}dongle_info;

static const dongle_info dongle_registerd[]={\
    {"a962","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/40181/fw_bcm40181a2.bin nvram_path=/vendor/etc/wifi/40181/nvram.txt","bcm6210",0x0},\
    {"4335","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6335/fw_bcm4339a0_ag.bin nvram_path=/vendor/etc/wifi/6335/nvram.txt","bcm6335",0x0},\
    {"a94d","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6234/fw_bcm43341b0_ag.bin nvram_path=/vendor/etc/wifi/6234/nvram.txt","bcm6234",0x0},\
    {"a9bf","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6255/fw_bcm43455c0_ag.bin nvram_path=/vendor/etc/wifi/6255/nvram.txt","bcm6255",0x0},\
    {"a9a6","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/6212/fw_bcm43438a0.bin nvram_path=/vendor/etc/wifi/6212/nvram.txt","bcm6212",0x0},\
    {"4356","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/4356/fw_bcm4356a2_ag.bin nvram_path=/vendor/etc/wifi/4356/nvram_ap6356.txt","bcm6356",0x0},\
    {"aa31","dhd","/vendor/lib/modules/dhd.ko","firmware_path=/vendor/etc/wifi/4358/fw_bcm4358_ag.bin nvram_path=/vendor/etc/wifi/4358/nvram_4358.txt","bcm4358",0x0},\
    {"8179","8189es","/vendor/lib/modules/8189es.ko","ifname=wlan0 if2name=p2p0","rtl8189es",0x0},\
    {"b822","8822bs","/vendor/lib/modules/8822bs.ko","ifname=wlan0 if2name=p2p0","rtl8822bs",0x0},\
    {"0000","bcmdhd","/vendor/lib/modules/bcmdhd.ko","","bcm43569",0xbd27}, \
	{"0000","bcmdhd","/vendor/lib/modules/bcmdhd.ko","","bcm43569",0x0bdc}, \
    {"0000","8822bu","/vendor/lib/modules/8822bu.ko","ifname=wlan0 if2name=p2p0","rtl8822bu",0xb82c}, \
    {"3030","ssv6051","/vendor/lib/modules/ssv6051.ko","stacfgpath=/system/vendor/etc/wifi/ssv6051/ssv6051-wifi.cfg","ssv6051",0x0}};


static const char *bcm6330_fw_path[] = {"/vendor/etc/wifi/AP6330/Wi-Fi/fw_bcm40183b2.bin","/vendor/etc/wifi/AP6330/Wi-Fi/fw_bcm40183b2_apsta.bin","/vendor/etc/wifi/AP6330/Wi-Fi/fw_bcm40183b2_p2p.bin"};
static const char *bcm6210_fw_path[] = {"/vendor/etc/wifi/40181/fw_bcm40181a2.bin","/vendor/etc/wifi/40181/fw_bcm40181a2_apsta.bin","/vendor/etc/wifi/40181/fw_bcm40181a2_p2p.bin"};
static const char *bcm6335_fw_path[] = {"/vendor/etc/wifi/6335/fw_bcm4339a0_ag.bin","/vendor/etc/wifi/6335/fw_bcm4339a0_ag_apsta.bin","/vendor/etc/wifi/6335/fw_bcm4339a0_ag_p2p.bin"};
static const char *bcm6234_fw_path[] = {"/vendor/etc/wifi/6234/fw_bcm43341b0_ag.bin","/vendor/etc/wifi/6234/fw_bcm43341b0_ag_apsta.bin","/vendor/etc/wifi/6234/fw_bcm43341b0_ag_p2p.bin"};
static const char *bcm4354_fw_path[] = {"/vendor/etc/wifi/4354/fw_bcm4354a1_ag.bin","/vendor/etc/wifi/4354/fw_bcm4354a1_ag_apsta.bin","/vendor/etc/wifi/4354/fw_bcm4354a1_ag_p2p.bin"};
static const char *bcm62x2_fw_path[] = {"/vendor/etc/wifi/62x2/fw_bcm43241b4_ag.bin","/vendor/etc/wifi/62x2/fw_bcm43241b4_ag_apsta.bin","/vendor/etc/wifi/62x2/fw_bcm43241b4_ag_p2p.bin"};
static const char *bcm6255_fw_path[] = {"/vendor/etc/wifi/6255/fw_bcm43455c0_ag.bin","/vendor/etc/wifi/6255/fw_bcm43455c0_ag_apsta.bin","/vendor/etc/wifi/6255/fw_bcm43455c0_ag_p2p.bin"};
static const char *bcm6212_fw_path[] = {"/vendor/etc/wifi/6212/fw_bcm43438a0.bin","/vendor/etc/wifi/6212/fw_bcm43438a0_apsta.bin","/vendor/etc/wifi/6212/fw_bcm43438a0_p2p.bin"};
static const char *bcm4356_fw_path[] = {"/vendor/etc/wifi/4356/fw_bcm4356a2_ag.bin","/vendor/etc/wifi/4356/fw_bcm4356a2_ag_apsta.bin","/vendor/etc/wifi/4356/fw_bcm4356a2_ag_p2p.bin"};
static const char *bcm4358_fw_path[] = {"/vendor/etc/wifi/4358/fw_bcm4358_ag.bin","/vendor/etc/wifi/4358/fw_bcm4358_ag_apsta.bin","/vendor/etc/wifi/4358/fw_bcm4358_ag_p2p.bin"};
static const char *wifi_driver_fw_path[]={"STA","AP","P2P"};
#endif

static int insmod(const char *filename, const char *args) {
  int fd = 0;
  int ret;

  fd = open(filename, O_RDONLY);
  if (fd < 0 ) return -1;

  ret = finit_module(fd, args, 0);

  close(fd);

  return ret;
}

static int rmmod(const char *modname) {
  int ret = -1;
  int maxtry = 10;

  while (maxtry-- > 0) {
    ret = delete_module(modname, O_NONBLOCK | O_EXCL);
    if (ret < 0 && errno == EAGAIN)
      usleep(500000);
    else
      break;
  }

  if (ret != 0)
    PLOG(DEBUG) << "Unable to unload driver module '" << modname << "'";
  return ret;
}

#ifdef WIFI_DRIVER_STATE_CTRL_PARAM
int wifi_change_driver_state(const char *state) {
  int len;
  int fd;
  int ret = 0;

  if (!state) return -1;
  fd = TEMP_FAILURE_RETRY(open(WIFI_DRIVER_STATE_CTRL_PARAM, O_WRONLY));
  if (fd < 0) {
    PLOG(ERROR) << "Failed to open driver state control param";
    return -1;
  }
  len = strlen(state) + 1;
  if (TEMP_FAILURE_RETRY(write(fd, state, len)) != len) {
    PLOG(ERROR) << "Failed to write driver state control param";
    ret = -1;
  }
  close(fd);
  return ret;
}
#endif

#ifndef MULTI_WIFI_SUPPORT
int is_wifi_driver_loaded() {
  char driver_status[PROPERTY_VALUE_MAX];
#ifdef WIFI_DRIVER_MODULE_PATH
  FILE *proc;
  char line[sizeof(DRIVER_MODULE_TAG) + 10];
#endif
  if (!property_get(DRIVER_PROP_NAME, driver_status, NULL) ||
      strcmp(driver_status, "ok") != 0) {
    return 0; /* driver not loaded */
  }
#ifdef WIFI_DRIVER_MODULE_PATH
  /*
   * If the property says the driver is loaded, check to
   * make sure that the property setting isn't just left
   * over from a previous manual shutdown or a runtime
   * crash.
   */
  if ((proc = fopen(MODULE_FILE, "r")) == NULL) {
    PLOG(WARNING) << "Could not open " << MODULE_FILE;
    property_set(DRIVER_PROP_NAME, "unloaded");
    return 0;
  }
  while ((fgets(line, sizeof(line), proc)) != NULL) {
    if (strncmp(line, DRIVER_MODULE_TAG, strlen(DRIVER_MODULE_TAG)) == 0) {
      fclose(proc);
      return 1;
    }
  }
  fclose(proc);
  property_set(DRIVER_PROP_NAME, "unloaded");
  return 0;
#else
  return 1;
#endif
}
#endif
#ifdef MULTI_WIFI_SUPPORT
#include <asm/ioctl.h>

#define USB_POWER_UP    _IO('m',1)
#define USB_POWER_DOWN  _IO('m',2)
#define SDIO_POWER_UP    _IO('m',3)
#define SDIO_POWER_DOWN  _IO('m',4)
#define SDIO_GET_DEV_TYPE  _IO('m',5)
//0: power on
//!0: power off
void set_wifi_power(int on)
{
    int fd;
    fd = open("/dev/wifi_power", O_RDWR);
    if (fd !=  - 1) {
        if (on == USB_POWER_UP) {
            if (ioctl (fd,USB_POWER_UP) < 0) {
                printf("Set usb Wi-Fi power up error!!!\n");
        close(fd);
                return;
            }
        }else if(on== USB_POWER_DOWN) {
            if (ioctl (fd,USB_POWER_DOWN)<0) {
                printf("Set usb Wi-Fi power down error!!!\n");
        close(fd);
                return;
            }
        }else if(on== SDIO_POWER_UP) {
            if (ioctl (fd,SDIO_POWER_UP)<0) {
                printf("Set SDIO Wi-Fi power up error!!!\n");
        close(fd);
                return;
            }
        }else if (on== SDIO_POWER_DOWN) {
            if (ioctl (fd,SDIO_POWER_DOWN)<0) {
                printf("Set SDIO Wi-Fi power down error!!!\n");
        close(fd);
                return;
            }
        }
    }
    else {
        PLOG(ERROR) <<"Device open failed";
    }
    close(fd);
    return;
}

int get_wifi_dev_type(char *dev_type)
{
    int fd;
    fd = open("/dev/wifi_power", O_RDWR);
    if (fd < 0) {
       return -1;
    }

    if (ioctl (fd,SDIO_GET_DEV_TYPE, dev_type)<0) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

char dongle_id[] = "/data/misc/wifi/wid_fp";
int write_no(const char *wifi_type)
{
    int fd,len;
    fd = open(dongle_id,O_CREAT|O_RDWR, S_IRWXU);
    if (fd == -1) {
        PLOG(ERROR) << "write no Open file failed";
        return -1;
    }
    len = write(fd,wifi_type,strlen(wifi_type));
    if (len == -1) {

        close(fd);
        return -1;
    }
    close(fd);
    if (chmod(dongle_id, 0664) < 0 || chown(dongle_id, AID_SYSTEM, AID_WIFI) < 0) {
       return -1;
    }
    return 0;
}

int read_no(char *wifi_type)
{
    int fd,len;
    fd = open(dongle_id,O_RDONLY, S_IRWXU);
    if (fd == -1) {
        PLOG(ERROR) << "Open file failed";
        return -1;
    }
    len = read(fd,wifi_type,15);
    if (len == -1) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

const char *get_wifi_vendor_name()
{
    char wifi_type[10];
    read_no(wifi_type);
    if (strstr(wifi_type, "bcm6330") != NULL) {
        return "bcm6330";
    } else if(strstr(wifi_type, "bcm6210") != NULL) {
        return "bcm6210";
    } else if(strstr(wifi_type, "bcm6335") != NULL) {
        return "bcm6335";
    } else if(strstr(wifi_type, "bcm6234") != NULL) {
        return "bcm6234";
    } else if(strstr(wifi_type, "bcm4354") != NULL) {
        return "bcm4354";
    } else if(strstr(wifi_type, "bcm62x2") != NULL) {
        return "bcm62x2";
    } else if(strstr(wifi_type, "bcm6255") != NULL) {
        return "bcm6255";
    } else if(strstr(wifi_type, "bcm6212") != NULL) {
        return "bcm6212";
    } else if(strstr(wifi_type, "bcm6356") != NULL) {
        return "bcm6356";
    } else if(strstr(wifi_type, "bcm4358") != NULL) {
        return "bcm4358";
    } else if(strstr(wifi_type, "qca9377") != NULL) {
        return "qca9377";
    } else if(strstr(wifi_type, "qca6174") != NULL) {
        return "qca6174";
    } else if(strstr(wifi_type, "rtl8723bs") != NULL) {
        return "rtl8723bs";
    } else if(strstr(wifi_type, "rtl8189es") != NULL) {
        return "rtl8189es";
    } else if(strstr(wifi_type, "rtl8822bs") != NULL) {
        return "rtl8822bs";
    } else if(strstr(wifi_type, "rtl8822bu") != NULL) {
        return "rtl8822bu";
    } else if(strstr(wifi_type, "rtl8189ftv") != NULL) {
        return "rtl8189ftv";
    } else if(strstr(wifi_type, "rtl8192es") != NULL) {
        return "rtl8192es";
    } else if(strstr(wifi_type, "rtl8188eu") != NULL) {
        return "rtl8188eu";
    } else if(strstr(wifi_type, "rtl8723bu") != NULL) {
        return "rtl8723bu";
    } else if(strstr(wifi_type, "rtl8723du") != NULL) {
        return "rtl8723du";
    } else if(strstr(wifi_type, "rtl8723ds") != NULL) {
        return "rtl8723ds";
    } else if(strstr(wifi_type, "rtl8821au") != NULL) {
        return "rtl8821au";
    } else if(strstr(wifi_type, "rtl8812au") != NULL) {
        return "rtl8812au";
    } else if(strstr(wifi_type, "rtl8188ftv") != NULL) {
        return "rtl8188ftv";
    } else if(strstr(wifi_type, "rtl8192cu") != NULL) {
        return "rtl8192cu";
    } else if(strstr(wifi_type, "rtl8192du") != NULL) {
        return "rtl8192du";
    } else if(strstr(wifi_type, "rtl8192eu") != NULL) {
        return "rtl8192eu";
    } else if(strstr(wifi_type, "mtk7601") != NULL) {
        return "mtk7601";
    } else if(strstr(wifi_type, "mtk7662") != NULL) {
        return "mtk7662";
    } else if(strstr(wifi_type, "mtk7668") != NULL) {
        return "mtk7668";
    } else if(strstr(wifi_type, "mtk7603") != NULL) {
        return "mtk7603";
    } else if(strstr(wifi_type, "bcm43569") != NULL) {
        return "bcm43569";
    } else if(strstr(wifi_type, "ssv6051") != NULL) {
        return "ssv6051";
    } else {
        PLOG(ERROR) << "can not found manch wifi";
        return "bcm";
    }
}

static int print_devs(libusb_device **devs)
{
    libusb_device *dev;
    int i = 0,j;

    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            return -1;
        }
        for (j=0;j <(int)(sizeof(dongle_registerd)/sizeof(dongle_info)); j++) {
            if (dongle_registerd[j].wifi_pid == desc.idProduct) {
                load_dongle_index=j;
                write_no(dongle_registerd[j].wifi_name);
                if (strcmp(get_wifi_vendor_name(), "bcm43569") == 0) {/*the service is for bcm usb wifi*/
                    property_set("ctl.start", "bcmdl");
                    usleep(500000);
                }
                PLOG(WARNING) << "found the march usb wifi is: " << dongle_registerd[j].wifi_name;
                insmod(dongle_registerd[j].wifi_module_path,dongle_registerd[j].wifi_module_arg);
                return 0;
            }
        }
    }
    return -1;
}

int usb_wifi_load_driver()
{
    libusb_device **devs;
    int r;
    ssize_t cnt;

    r = libusb_init(NULL);
    if (r < 0)
        return r;

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
        return (int) cnt;

    r = print_devs(devs);
    if (r < 0)
        return r;

    libusb_free_device_list(devs, 1);

    libusb_exit(NULL);
    return 0;
}

int sdio_wifi_load_driver()
{
    int i;
    char sdio_buf[128];
    FILE *fp = fopen(file_name,"r");
    if (!fp) {
        PLOG(ERROR) << "Open sdio wifi file failed";
        return -1;
    }
    memset(sdio_buf,0,sizeof(sdio_buf));
    if (fread(sdio_buf, 1,128,fp) < 1) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    for (i=0;i <(int)(sizeof(dongle_registerd)/sizeof(dongle_info)); i++) {
        if (strstr(sdio_buf,dongle_registerd[i].chip_id)) {
            load_dongle_index = i;
            write_no(dongle_registerd[i].wifi_name);
            PLOG(WARNING) << "found the march sdio wifi is: " << dongle_registerd[i].wifi_name;
            insmod(dongle_registerd[i].wifi_module_path,dongle_registerd[i].wifi_module_arg);
            return 0;
        }
    }
    return -1;
}

int multi_wifi_load_driver()
{
    int wait_time=0,ret;
    char dev_type[10] = {'\0'};
    get_wifi_dev_type(dev_type);
    sprintf(file_name, "/sys/bus/mmc/devices/%s:0001/%s:0001:1/device", dev_type, dev_type);
    if (!sdio_wifi_load_driver()) {
        return 0;
    }
    do {
        ret = usb_wifi_load_driver();
        if (!ret)
            return 0;
        else if (ret> 0)
            break;
        else {
        wait_time++;
        usleep(50000);
        PLOG(ERROR) << "wait usb ok";
        }
    }while(wait_time<300);

    return -1;
}
#endif

int wifi_load_driver() {
#ifdef WIFI_DRIVER_MODULE_PATH
#ifdef MULTI_WIFI_SUPPORT
  set_wifi_power(SDIO_POWER_UP);
  if (multi_wifi_load_driver() < 0) {
        set_wifi_power(SDIO_POWER_DOWN);
        return -1;
  }
#else
#ifndef MULTI_WIFI_SUPPORT
  if (is_wifi_driver_loaded()) {
    return 0;
  }
#endif
  if (insmod(DRIVER_MODULE_PATH, DRIVER_MODULE_ARG) < 0) return -1;
#endif
#endif

#ifdef WIFI_DRIVER_STATE_CTRL_PARAM
#ifndef MULTI_WIFI_SUPPORT
  if (is_wifi_driver_loaded()) {
    return 0;
  }
#endif
  if (wifi_change_driver_state(WIFI_DRIVER_STATE_ON) < 0) return -1;
#endif
#ifndef MULTI_WIFI_SUPPORT
  property_set(DRIVER_PROP_NAME, "ok");
#endif
  return 0;
}

int wifi_unload_driver() {
#ifndef MULTI_WIFI_SUPPORT
  if (!is_wifi_driver_loaded()) {
    return 0;
  }
#endif
  usleep(200000); /* allow to finish interface down */
#ifdef MULTI_WIFI_SUPPORT
  rmmod(dongle_registerd[load_dongle_index].wifi_module_name);
  if (strcmp(get_wifi_vendor_name(), "bcm43569") == 0)/*the service is for bcm usb wifi*/
    property_set("ctl.stop", "bcmdl");

  usleep(500000);
  set_wifi_power(SDIO_POWER_DOWN);
  return 0;
#else
#ifdef WIFI_DRIVER_MODULE_PATH
  if (rmmod(DRIVER_MODULE_NAME) == 0) {
    int count = 20; /* wait at most 10 seconds for completion */
    while (count-- > 0) {
#ifndef MULTI_WIFI_SUPPORT
      if (!is_wifi_driver_loaded()) break;
#endif
      usleep(500000);
    }
    usleep(500000); /* allow card removal */
    if (count) {
      return 0;
    }
    return -1;
  } else
    return -1;
#else
#ifdef WIFI_DRIVER_STATE_CTRL_PARAM
#ifndef MULTI_WIFI_SUPPORT
  if (is_wifi_driver_loaded()) {
    if (wifi_change_driver_state(WIFI_DRIVER_STATE_OFF) < 0) return -1;
  }
#endif
#endif
  property_set(DRIVER_PROP_NAME, "unloaded");
  return 0;
#endif
#endif
}

const char *wifi_get_fw_path(int fw_type) {
    switch (fw_type) {
    case WIFI_GET_FW_PATH_STA:
#ifdef MULTI_WIFI_SUPPORT
        if (strncmp(get_wifi_vendor_name(), "bcm", 3) !=0 || strcmp(get_wifi_vendor_name(), "bcm43569") ==0) {
            return wifi_driver_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6330") ==0) {
            return bcm6330_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6210") ==0) {
            return bcm6210_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6335") ==0) {
            return bcm6335_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6234") ==0) {
            return bcm6234_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm4354") ==0) {
            return bcm4354_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm62x2") ==0) {
            return bcm62x2_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6255") ==0) {
            return bcm6255_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6212") ==0) {
            return bcm6212_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6356") ==0) {
            return bcm4356_fw_path[0];
        } else if (strcmp(get_wifi_vendor_name(), "bcm4358") ==0) {
            return bcm4358_fw_path[0];
        }
#else
        return WIFI_DRIVER_FW_PATH_STA;
#endif
    case WIFI_GET_FW_PATH_AP:
#ifdef MULTI_WIFI_SUPPORT
        if (strcmp(get_wifi_vendor_name(), "mtk7668") == 0) {
            rmmod("wlan_mt76x8_sdio");
            usleep(100000);
            insmod("/system/lib/wlan_mt76x8_sdio.ko", "sta=sta ap=wlan");
            return wifi_driver_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6330") ==0) {
            return bcm6330_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6210") ==0) {
            return bcm6210_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6335") ==0) {
            return bcm6335_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6234") ==0) {
            return bcm6234_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm4354") ==0) {
            return bcm4354_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm62x2") ==0) {
            return bcm62x2_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6255") ==0) {
            return bcm6255_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6212") ==0) {
            return bcm6212_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6356") ==0) {
            return bcm4356_fw_path[1];
        } else if (strcmp(get_wifi_vendor_name(), "bcm4358") ==0) {
            return bcm4358_fw_path[1];
        } else if (strncmp(get_wifi_vendor_name(), "bcm", 3) !=0 || strcmp(get_wifi_vendor_name(), "bcm43569") ==0) {
            return wifi_driver_fw_path[1];
        }
#else
        return WIFI_DRIVER_FW_PATH_AP;
#endif
    case WIFI_GET_FW_PATH_P2P:
#ifdef MULTI_WIFI_SUPPORT
        if (strncmp(get_wifi_vendor_name(), "bcm", 3) !=0 || strcmp(get_wifi_vendor_name(), "bcm43569") ==0) {
            return wifi_driver_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6330") ==0) {
            return bcm6330_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6210") ==0) {
            return bcm6210_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6335") ==0) {
            return bcm6335_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6234") ==0) {
            return bcm6234_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm4354") ==0) {
            return bcm4354_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm62x2") ==0) {
            return bcm62x2_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6255") ==0) {
            return bcm6255_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6212") ==0) {
            return bcm6212_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm6356") ==0) {
            return bcm4356_fw_path[2];
        } else if (strcmp(get_wifi_vendor_name(), "bcm4358") ==0) {
            return bcm4358_fw_path[2];
        }
#else
        return WIFI_DRIVER_FW_PATH_P2P;
#endif
    }
    return NULL;
}


int wifi_change_fw_path(const char *fwpath) {
  int len;
  int fd;
  int ret = 0;
#ifdef MULTI_WIFI_SUPPORT
    if (strncmp(get_wifi_vendor_name(), "bcm", 3) !=0 || strcmp(get_wifi_vendor_name(), "bcm43569") ==0)
        return ret;
#endif
  if (!fwpath) return ret;
  fd = TEMP_FAILURE_RETRY(open(WIFI_DRIVER_FW_PATH_PARAM, O_WRONLY));
  if (fd < 0) {
    PLOG(ERROR) << "Failed to open wlan fw path param";
    return -1;
  }
  len = strlen(fwpath) + 1;
  if (TEMP_FAILURE_RETRY(write(fd, fwpath, len)) != len) {
    PLOG(ERROR) << "Failed to write wlan fw path param";
    ret = -1;
  }
  close(fd);
  return ret;
}
