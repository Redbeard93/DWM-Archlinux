#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* See LICENSE file for copyright and license details. */

/* interval between updates (in ms) */
const unsigned int interval = 1000;

/* text to show if no value can be retrieved */
static const char unknown_str[] = "n/a";

/* maximum output string length */
#define MAXLEN 2048

/*
 * function            description                     argument (example)
 *
 * battery_perc        battery percentage              battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * battery_remaining   battery remaining HH:MM         battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * battery_state       battery charging state          battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * cat                 read arbitrary file             path
 * cpu_freq            cpu frequency in MHz            NULL
 * cpu_perc            cpu usage in percent            NULL
 * datetime            date and time                   format string (%F %T)
 * disk_free           free disk space in GB           mountpoint path (/)
 * disk_perc           disk usage in percent           mountpoint path (/)
 * disk_total          total disk space in GB          mountpoint path (/)
 * disk_used           used disk space in GB           mountpoint path (/)
 * entropy             available entropy               NULL
 * gid                 GID of current user             NULL
 * hostname            hostname                        NULL
 * ipv4                IPv4 address                    interface name (eth0)
 * ipv6                IPv6 address                    interface name (eth0)
 * kernel_release      `uname -r`                      NULL
 * keyboard_indicators caps/num lock indicators        format string (c?n?)
 *                                                     see keyboard_indicators.c
 * keymap              layout (variant) of current     NULL
 *                     keymap
 * load_avg            load average                    NULL
 * netspeed_rx         receive network speed           interface name (wlan0)
 * netspeed_tx         transfer network speed          interface name (wlan0)
 * num_files           number of files in a directory  path
 *                                                     (/home/foo/Inbox/cur)
 * ram_free            free memory in GB               NULL
 * ram_perc            memory usage in percent         NULL
 * ram_total           total memory size in GB         NULL
 * ram_used            used memory in GB               NULL
 * run_command         custom shell command            command (echo foo)
 * swap_free           free swap in GB                 NULL
 * swap_perc           swap usage in percent           NULL
 * swap_total          total swap size in GB           NULL
 * swap_used           used swap in GB                 NULL
 * temp                temperature in degree celsius   sensor file
 *                                                     (/sys/class/thermal/...)
 *                                                     NULL on OpenBSD
 *                                                     thermal zone on FreeBSD
 *                                                     (tz0, tz1, etc.)
 * uid                 UID of current user             NULL
 * uptime              system uptime                   NULL
 * username            username of current user        NULL
 * vol_perc            OSS/ALSA volume in percent      mixer file (/dev/mixer)
 *                                                     NULL on OpenBSD/FreeBSD
 * wifi_essid          WiFi ESSID                      interface name (wlan0)
 * wifi_perc           WiFi signal in percent          interface name (wlan0)
 */

// 获取无线网卡名称的函数
static char wireless_interface[32] = "n/a"; // 全局变量，存储无线网卡名称

const char *get_wireless_interface(void) {
  FILE *fp;
  char line[256];

  // 打开/proc/net/wireless文件
  fp = fopen("/proc/net/wireless", "r");
  if (fp == NULL) {
    perror("Error opening file");
    return unknown_str; // 返回未知值
  }

  // 跳过前两行
  fgets(line, sizeof(line), fp);
  fgets(line, sizeof(line), fp);
  // 读取第三行，获取无线网卡名称
  if (fgets(line, sizeof(line), fp) != NULL) {
    // 提取无线网卡名称，去除前面的空格
    if (sscanf(line, " %[^:]", wireless_interface) != 1) {
      fprintf(stderr, "Error parsing interface name from line: %s", line);
      strcpy(wireless_interface, unknown_str);
    } else {
      // 去除字符串末尾的空格
      size_t len = strlen(wireless_interface);
      while (len > 0 && (wireless_interface[len - 1] == ' ' ||
                         wireless_interface[len - 1] == '\n')) {
        wireless_interface[len - 1] = '\0';
        len--;
      }
    }
  } else {
    fprintf(stderr, "Error reading third line\n");
    strcpy(wireless_interface, unknown_str);
  }

  fclose(fp);
  return wireless_interface;
}

// 构造函数，确保在 main 函数执行前调用
static void __attribute__((constructor)) init_wireless_interface(void) {
  get_wireless_interface();
}

// use wifi_perc to show signal icons
static inline const char *get_wifi_icon_based_on_perc(const char *interface) {
  const char *perc_str = wifi_perc(interface);
  if (perc_str == NULL || perc_str[0] == '\0') {
    return "󰤭"; // WiFi 未连接
  }

  int perc = atoi(perc_str); // 将 wifi_perc 返回的百分比字符串转换为整数
  const char *icon = "";
  if (perc <= 0)
    icon = "󰤯";
  else if (perc <= 25)
    icon = "󰤟";
  else if (perc <= 50)
    icon = "󰤢";
  else if (perc <= 75)
    icon = "󰤥";
  else
    icon = "󰤨";
  return icon;
}

// show corresponding volume icons
static const char vol[] =
    "muted=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{print $3;}'); \
     volume=$(wpctl get-volume @DEFAULT_AUDIO_SINK@ | awk '{print $2;}' | awk '{print $1 * 100}' | cut -d. -f1); \
     if [ -z \"${muted}\" ]; then \
         if [ \"$volume\" -le 30 ]; then \
         printf \"%d \" \"$(echo \"$volume\" )\"; \
         elif [ \"$volume\" -le 60 ]; then \
         printf \"%d \" \"$(echo \"$volume\" )\"; \
         elif [ \"$volume\" -le 100 ]; then \
         printf \"%d \" \"$(echo \"$volume\" )\"; \
         fi; \
     else \
         printf \"\"; \
     fi";

// show mic icons
static const char mic[] =
    "muted=$(wpctl get-volume @DEFAULT_AUDIO_SOURCE@ | awk '{print $3;}'); \
     volume=$(wpctl get-volume @DEFAULT_AUDIO_SOURCE@ | awk '{print $2;}'); \
     if [ \"$muted\" ]; then \
         printf \" \"; \
     elif [ \"$volume\" = \"1.00\" ]; then \
         printf \" \"; \
     else \
         printf \"%.0f \" \"$volume\"; \
     fi";

static const char brightness[] =
    "brightnessctl g | awk '{print int($1 * 100 / 3000)}' | tr -d '\\n'";

static const struct arg args[] = {
    /* function format          argument */
    {cpu_perc, " 󰍛 %s ", NULL},
    {ram_perc, "  %s ", NULL},
    {run_command, " 󱍖%s ", brightness},
    {run_command, " %s", vol},
    {run_command, " %s", mic},
    {battery_state, " %s", "BAT0"},
    {battery_perc, "%s ", "BAT0"},
    {get_wifi_icon_based_on_perc, "%s", wireless_interface},
    //{wifi_perc, "%s", wireless_interface},
    {datetime, " %s", "%c"},
};
