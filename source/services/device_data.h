#ifndef DEVICE_DATA_H
#define DEVICE_DATA_H

typedef struct {
    char *name;
    char *brand;
    char *model;
} DeviceInfo;

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
} DeviceData;

extern DeviceData device_data;

void init_device_data();

void clean_device_data();

#endif // DEVICE_DATA_H
