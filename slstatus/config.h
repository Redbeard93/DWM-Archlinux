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
 * battery_state       battery charging state          battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * battery_remaining   battery remaining HH:MM         battery name (BAT0)
 *                                                     NULL on OpenBSD/FreeBSD
 * cpu_perc            cpu usage in percent            NULL
 * cpu_freq            cpu frequency in MHz            NULL
 * datetime            date and time                   format string (%F %T)
 * disk_free           free disk space in GB           mountpoint path (/)
 * disk_perc           disk usage in percent           mountpoint path (/)
 * disk_total          total disk space in GB          mountpoint path (/")
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
 * separator           string to echo                  NULL
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
 *                                                     NULL on OpenBSD
 * wifi_perc           WiFi signal in percent          interface name (wlan0)
 * wifi_essid          WiFi ESSID                      interface name (wlan0)
 */

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
    "brightness=$(light -G | sed 's/\\(.*\\)\\.00/\\1/' | grep -v 100);" \
    "printf \"%s\" \"$brightness\";";

static const struct arg args[] = {
	/* function format          argument */
    { wifi_essid, " ﯟ %s ",  "wlan0"},
    { netspeed_rx, " ﯲ %sB/s ",  "wlan0"},
    { netspeed_tx, " ﯴ %sB/s ",  "wlan0"},
    { cpu_perc, " 󰍛%s ",     NULL },
    { ram_perc, " %s ",     NULL },
    { run_command, " 󱍖%s ",   brightness },
    { run_command,          " %s",        vol },
    { run_command,          " %s",        mic },
    { battery_state, " %s",  "BAT0"},
    { battery_perc, "%s ",  "BAT0"},
/*	{ battery_remaining, "  ",  "BAT0"},*/
    {get_wifi_icon_based_on_perc, "%s", "wlp3s0"},
    {wifi_perc, "%s%%", "wlp3s0"},
    {wifi_essid, "%s", "wlp3s0"},
    { datetime, " 󰃰 %s",        "%c"},
/*    { run_command, "  ",       "%2s |" },*/
};
