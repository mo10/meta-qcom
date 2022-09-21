// SPDX-License-Identifier: MIT

#include "../inc/helpers.h"
#include "../inc/adspfw.h"
#include "../inc/atfwd.h"
#include "../inc/audio.h"
#include "../inc/devices.h"
#include "../inc/ipc.h"
#include "../inc/logger.h"
#include "../inc/md5sum.h"
#include "../inc/openqti.h"
#include "../inc/sms.h"
#include "../inc/tracking.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/reboot.h>
#include <pthread.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <syscall.h>
#include <unistd.h>

int write_to(const char *path, const char *val, int flags) {
  int ret;
  int fd = open(path, flags);
  if (fd < 0) {
    return -ENOENT;
  }
  ret = write(fd, val, strlen(val) * sizeof(char));
  close(fd);
  return ret;
}

uint32_t get_curr_timestamp() {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  uint32_t milliseconds =
      te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
  return milliseconds;
}

int is_adb_enabled() {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return 1;
  }
  lseek(fd, 64, SEEK_SET);
  if (read(fd, buff, sizeof(PERSIST_ADB_ON_MAGIC)) <= 0) {
    logger(MSG_ERROR, "%s: Error reading ADB state \n", __func__);
  }
  close(fd);
  if (strcmp(buff, PERSIST_ADB_ON_MAGIC) == 0) {
    logger(MSG_DEBUG, "%s: Persistent ADB is enabled\n", __func__);
    return 1;
  }

  logger(MSG_DEBUG, "%s: Persistent ADB is disabled \n", __func__);
  return 0;
}

void store_adb_setting(bool en) {
  char buff[32];
  memset(buff, 0, 32);

  int fd;
  if (en) { // Store the magic string in the second block of the misc partition
    logger(MSG_WARN, "Enabling persistent ADB\n");
    strncpy(buff, PERSIST_ADB_ON_MAGIC, 32);
  } else {
    logger(MSG_WARN, "Disabling persistent ADB\n");
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening misc partition to set adb flag \n",
           __func__);
    return;
  }
  lseek(fd, 64, SEEK_SET);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing the ADB flag \n", __func__);
  }
  close(fd);
}

int get_audio_mode() {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return AUDIO_MODE_I2S;
  }
  lseek(fd, 96, SEEK_SET);
  if (read(fd, buff, strlen(PERSIST_USB_AUD_MAGIC)) <= 0) {
    logger(MSG_ERROR, "%s: Error reading USB audio state \n", __func__);
  }
  close(fd);
  if (strcmp(buff, PERSIST_USB_AUD_MAGIC) == 0) {
    logger(MSG_INFO, "%s: Persistent USB audio is enabled\n", __func__);
    return AUDIO_MODE_USB;
  }

  logger(MSG_INFO, "%s: Persistent USB audio is disabled \n", __func__);
  return AUDIO_MODE_I2S;
}

void store_audio_output_mode(uint8_t mode) {
  char buff[32];
  memset(buff, 0, 32);
  int fd;
  if (mode == AUDIO_MODE_USB) { // Store the magic string in the second block of
                                // the misc partition
    logger(MSG_WARN, "Enabling USB Audio\n");
    strncpy(buff, PERSIST_USB_AUD_MAGIC, 32);
  } else {
    logger(MSG_WARN, "Disabling USB audio\n");
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR,
           "%s: Error opening misc partition to set audio output flag \n",
           __func__);
    return;
  }
  lseek(fd, 96, SEEK_SET);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing USB audio flag \n", __func__);
  }
  close(fd);
}

int is_ncm_enabled() {
  int fd;
  char buff[32];
  fd = open("/dev/mtdblock12", O_RDONLY);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening the misc partition \n", __func__);
    return 1;
  }
  lseek(fd, 128, SEEK_SET);
  if (read(fd, buff, sizeof(PERSIST_NCM_ON_MAGIC)) <= 0) {
    logger(MSG_ERROR, "%s: Error reading NCM state \n", __func__);
  }
  close(fd);
  if (strcmp(buff, PERSIST_NCM_ON_MAGIC) == 0) {
    logger(MSG_DEBUG, "%s: Persistent NCM is enabled\n", __func__);
    return 1;
  }

  logger(MSG_DEBUG, "%s: Persistent NCM is disabled \n", __func__);
  return 0;
}

void store_ncm_setting(bool en) {
  char buff[32];
  memset(buff, 0, 32);

  int fd;
  if (en) { // Store the magic string in the second block of the misc partition
    logger(MSG_WARN, "Enabling persistent NCM\n");
    strncpy(buff, PERSIST_NCM_ON_MAGIC, 32);
  } else {
    logger(MSG_WARN, "Disabling persistent NCM\n");
  }
  fd = open("/dev/mtdblock12", O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Error opening misc partition to set NCM flag \n",
           __func__);
    return;
  }
  lseek(fd, 128, SEEK_SET);
  if (write(fd, &buff, sizeof(buff)) < 0) {
    logger(MSG_ERROR, "%s: Error writing the NCM flag \n", __func__);
  }
  close(fd);
}

void reset_usb_port() {
  if (write_to(USB_EN_PATH, "0", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error disabling USB \n", __func__);
  }
  sleep(1);
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }
}

void restart_usb_stack() {
  char functions[64] = "diag,serial,rmnet";
  if (is_adb_enabled()) {
    strcat(functions, ",ffs");
  }

  if (get_audio_mode()) {
    strcat(functions, ",audio");
  }

  if (is_ncm_enabled()){
    strcat(functions, ",ncm");
  }

  if (write_to(USB_EN_PATH, "0", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error disabling USB \n", __func__);
  }

  if (write_to(USB_FUNC_PATH, functions, O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error setting USB functions \n", __func__);
  }

  sleep(1);
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }

  // Switch between I2S and usb audio depending on the misc partition setting
  set_output_device(get_audio_mode());
  // Enable or disable ADB depending on the misc partition setting
  set_adb_runtime(is_adb_enabled());

  // ADB should start when usb is available
  if (is_adb_enabled()) {
    if (system("/etc/init.d/adbd start") < 0) {
      logger(MSG_WARN, "%s: Failed to start ADB \n", __func__);
    }
  }
}

void enable_usb_port() {
  if (write_to(USB_EN_PATH, "1", O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error enabling USB \n", __func__);
  }
}

void set_suspend_inhibit(bool mode) {
  if (mode == true) {
    logger(MSG_INFO, "%s: Blocking USB suspend\n", __func__);
    if (write_to(SUSPEND_INHIBIT_PATH, "1", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting USB suspend \n", __func__);
    }
  } else {
    logger(MSG_INFO, "%s: USB suspend is allowed\n", __func__);
    if (write_to(SUSPEND_INHIBIT_PATH, "0", O_RDWR) < 0) {
      logger(MSG_ERROR, "%s: Error setting USB suspend \n", __func__);
    }
  }
}

char *get_gpio_dirpath(char *gpio) {
  char *path;
  path = calloc(256, sizeof(char));
  snprintf(path, 256, "%s%s/%s", GPIO_SYSFS_BASE, gpio, GPIO_SYSFS_DIRECTION);

  return path;
}

char *get_gpio_value_path(char *gpio) {
  char *path;
  path = calloc(256, sizeof(char));
  snprintf(path, 256, "%s%s/%s", GPIO_SYSFS_BASE, gpio, GPIO_SYSFS_VALUE);
  return path;
}

/* We're not using this: */

/*
void prepare_dtr_gpio() {
  logger(MSG_INFO, "%s: Getting GPIO ready\n", __func__);
  if (write_to(GPIO_EXPORT_PATH, GPIO_DTR, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error exporting GPIO_DTR pin\n", __func__);
  }
}

uint8_t get_dtr_state() {
  int dtr, ret;
  char dtrval;
  dtr = open(get_gpio_value_path(GPIO_DTR), O_RDONLY | O_NONBLOCK);
  if (dtr < 0) {
    logger(MSG_WARN, "%s: DTR not available: %s \n", __func__,
           get_gpio_value_path(GPIO_DTR));
    return 0;
  }

  lseek(dtr, 0, SEEK_SET);
  if (read(dtr, &dtrval, 1) < 0) {
    logger(MSG_WARN, "%s: Error reading DTR value \n", __func__);
  }
  if ((int)(dtrval - '0') == 1) {
    ret = 1;
  } else {
    ret = 0;
  }

  close(dtr);
  return ret;
} */

uint8_t pulse_ring_in() {
  int i;
  logger(MSG_INFO, "[[[RING IN]]]\n");

  if (write_to(GPIO_EXPORT_PATH, GPIO_RING_IN, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error exporting GPIO_RING_IN pin\n", __func__);
  }
  if (write_to(get_gpio_dirpath(GPIO_RING_IN), GPIO_MODE_OUTPUT, O_RDWR) < 0) {
    logger(MSG_ERROR, "%s: Error set dir: GPIO_RING_IN pin\n", __func__);
  }
  usleep(300);
  for (i = 0; i < 10; i++) {
    if (i % 2 == 0) {
      if (write_to(get_gpio_value_path(GPIO_RING_IN), "1", O_RDWR) < 0) {
        logger(MSG_ERROR, "%s: Error Writing to Ring in\n", __func__);
      }
    } else {
      if (write_to(get_gpio_value_path(GPIO_RING_IN), "0", O_RDWR) < 0) {
        logger(MSG_ERROR, "%s: Error Writing to Ring in\n", __func__);
      }
    }
    usleep(300);
  }

  usleep(300);
  if (write_to(GPIO_UNEXPORT_PATH, GPIO_RING_IN, O_WRONLY) < 0) {
    logger(MSG_ERROR, "%s: Error unexporting GPIO_RING_IN pin\n", __func__);
  }
  return 0;
}

int elapsed_time(struct timeval *prev, struct timeval *cur) {
  if (cur->tv_usec > prev->tv_usec) {
    cur->tv_usec += 1000000;
    cur->tv_sec--;
  }
  return (int)(cur->tv_sec - prev->tv_sec) * 1000000 + cur->tv_usec -
         prev->tv_usec;
}

int get_int_from_str(char *str, int offset) {
  int val = 0;
  char tmp[2];
  tmp[0] = str[offset];
  tmp[1] = str[offset + 1];

  if (tmp[1] < '0' || tmp[1] > '9') {
    tmp[1] = tmp[0];
    tmp[0] = '0';
  }
  val = strtol(tmp, NULL, 10);
  return val;
}

void *power_key_event() {
  int fd = 0;
  struct input_event ev;
  struct timeval prev;
  struct timeval cur;
  int ret = 0;
  memset(&prev, 0, sizeof(struct timeval));
  memset(&cur, 0, sizeof(struct timeval));

  fd = open(INPUT_DEV, O_RDONLY);
  if (fd == -1) {
    logger(MSG_ERROR, "%s: Error opening %s\n", __func__, INPUT_DEV);
    return NULL;
  }

  while ((ret = read(fd, &ev, sizeof(struct input_event))) > 0) {
    if (ret < sizeof(struct input_event)) {
      logger(MSG_ERROR, "%s: cannot read whole input event\n", __func__);
    } else {

      if (ev.type == EV_KEY && ev.code == KEY_POWER && ev.value == 1) {
        memcpy(&prev, &ev.time, sizeof(struct timeval));
      } else if (ev.type == EV_KEY && ev.code == KEY_POWER && ev.value == 0) {
        memcpy(&cur, &ev.time, sizeof(struct timeval));
        if (elapsed_time(&prev, &cur) > 1000000) {
          logger(MSG_ERROR, "%s: Power GPIO long press detected\n", __func__);
        } else {
          logger(MSG_ERROR, "%s: Poweroff requested!\n", __func__);
          syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                  LINUX_REBOOT_CMD_RESTART, NULL);
        }
      }
    }
  }

  return NULL;
}

int read_adsp_version() {
  char *md5_result;
  char *hex_md5_res;
  int i, j;
  int element = 0;
  int offset = 0;
  md5_result = calloc(64, sizeof(char));
  hex_md5_res = calloc(64, sizeof(char));
  bool matched = false;

  md5_file(ADSPFW_GEN_FILE, md5_result);
  if (strlen(md5_result) < 16) {
    logger(MSG_ERROR, "%s: Error calculating the MD5 for your firmware (%s) \n",
           __func__, md5_result);
  } else {
    for (j = 0; j < strlen(md5_result); j++) {
      offset += sprintf(hex_md5_res + offset, "%02x", md5_result[j]);
    }
    for (i = 0; i < (sizeof(known_adsp_fw) / sizeof(known_adsp_fw[0])); i++) {
      if (strcmp(hex_md5_res, known_adsp_fw[i].md5sum) == 0) {
        logger(MSG_INFO, "%s: Found your ADSP firmware: (%s) \n", __func__,
               known_adsp_fw[i].fwstring);
        element = i;
        matched = true;
        break;
      }
    }
  }
  if (!matched) {
    logger(MSG_WARN, "%s: Could not detect your ADSP firmware! \n", __func__);
  }
  free(md5_result);
  free(hex_md5_res);
  return element;
}

int wipe_message_storage() {
  int fd, sz, i;
  char command[128];

  logger(MSG_INFO, "%s: Wiping message storage\n", __func__);
  fd = open(SMD_SEC_AT, O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open SMD10 entry\n", __func__);
    return -EINVAL;
  }

  for (i = 0; i <= 100; i++) {
    sz = snprintf(command, 128, "%s%i\r\n", MSG_DELETE_PARTIAL_CMD, i);
    if (write(fd, command, sz) < 0)
      logger(MSG_ERROR, "%s: Error writing wipe cmd %i\n", __func__, i);
    usleep(100000);
  }
  close(fd);
  logger(MSG_INFO, "%s: Message storage cleared\n", __func__);

  return 0;
}

void add_message_to_queue(uint8_t *message, size_t len) {
  if (len <= 0) {
    logger(MSG_ERROR, "%s: Can't parse message, size is %i\n", __func__, len);
    return;
  }

  if (get_call_simulation_mode()) {
    add_voice_message_to_queue(message, len);
  } else {
    add_sms_to_queue(message, len);
  }
}

bool at_if_in_use = false;
int send_at_command(char *at_command, size_t cmdlen, char *response,
                    size_t response_sz) {
  int fd, ret;
  fd_set readfds;
  struct timeval tv;

  if (at_if_in_use) {
    logger(MSG_ERROR, "%s: AT Interface is already in use\n", __func__);
    return -EAGAIN;
  }

  at_if_in_use = true;
  fd = open(SMD_SEC_AT, O_RDWR);
  if (fd < 0) {
    logger(MSG_ERROR, "%s: Cannot open %s\n", __func__, SMD_SEC_AT);
    at_if_in_use = false;
    return -EINVAL;
  }

  logger(MSG_DEBUG, "%s: Sending %s\n", __func__, at_command);
  ret = write(fd, at_command, cmdlen);
  if (ret < 0) {
    logger(MSG_ERROR, "%s: Failed to write to %s\n", __func__, SMD_SEC_AT);
    at_if_in_use = false;
    return -EIO;
  }
  tv.tv_sec = 1;
  tv.tv_usec = 500000;
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  ret = select(MAX_FD, &readfds, NULL, NULL, &tv);
  if (FD_ISSET(fd, &readfds)) {
    ret = read(fd, response, response_sz);
    if (ret < 0) {
      logger(MSG_ERROR, "%s: Failed to read from %s\n", __func__, SMD_SEC_AT);
      at_if_in_use = false;
      close(fd);
      return -EIO;
    }
  } else {
    logger(MSG_ERROR, "%s: No response in time from %s\n", __func__,
           SMD_SEC_AT);
  }
  at_if_in_use = false;
  logger(MSG_DEBUG, "%s: Received %s\n", __func__, response);
  close(fd);
  return 0;
}
