#ifndef MQTT_H
#define MQTT_H

struct mosquitto * init_mqtt();
void clean_up_mosquitto(struct mosquitto **mosq);

#endif