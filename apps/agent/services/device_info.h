#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

typedef struct {
    char *name;
    char *brand;
    char *model;
} DeviceProfile;

typedef struct {
    char *device_id;
    char *mac;
    char *name;
    char *brand;
    char *model;
    char *arch;
    char *public_ip;
    char *os_name;
    char *os_version;
    char *os_services_version;
    char *did_public_key;
} DeviceInfo;

DeviceInfo *init_device_info();
void clean_device_info(DeviceInfo *device_info);
char *get_os_version();
char *get_os_services_version();
char *get_os_name();
char *get_public_ip();
char *get_arch();

#endif // DEVICE_INFO_H
