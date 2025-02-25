/* SPDX-License-Identifier: MIT */

#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define PERSIST_CUSTOM_ALERT_TONE "cust_alert_tone"
#define CONFIG_FILE_PATH "/persist/openqti.conf"
#define SCHEDULER_DATA_FILE_PATH "/persist/sched.raw"
#define PERSISTENT_LOGFILE_PATH "/persist/log"
#define MAX_NAME_SZ 32
struct config_prototype {
  uint8_t custom_alert_tone;
  uint8_t persistent_logging;
  char user_name[MAX_NAME_SZ];
  char modem_name[MAX_NAME_SZ];
  uint8_t signal_tracking;
  uint8_t sms_logging;
  uint8_t callwait_autohangup;
  bool first_boot;
};

/* Set mount as readwrite or readonly */
int set_persistent_partition_rw();
int set_persistent_partition_ro();
void do_sync_fs();

int set_initial_config();
int read_settings_from_file();

/* Signal tracking */
int is_signal_tracking_enabled();
void enable_signal_tracking(bool en);

/* Custom alert tone */
int use_custom_alert_tone();
void set_custom_alert_tone(bool en);

/* Modem Name */
int get_modem_name(char *buff);
void set_modem_name(char *name);
/* User name */
int get_user_name(char *buff);
void set_user_name(char *name);

/* Persistent logging */
int use_persistent_logging();
void set_persistent_logging(bool en);

/* Is first boot? */
bool is_first_boot();
void clear_ifrst_boot_flag();

/* SMS logging */
int is_sms_logging_enabled();
void set_sms_logging(bool en);

/* Automatically hang up call waiting */
int callwait_auto_hangup_operation_mode();
void enable_call_waiting_autohangup(uint8_t en);

#endif