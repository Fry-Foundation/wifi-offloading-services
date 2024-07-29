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
    char *public_ip;
    char *os_name;
    char *os_version;
    char *os_services_version;
    char *did_public_key;
} DeviceInfo;

extern DeviceInfo device_info;

void init_device_info();

void clean_device_info_service();

#endif // DEVICE_INFO_H
