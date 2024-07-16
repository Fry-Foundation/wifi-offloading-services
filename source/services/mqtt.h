#ifndef MQTT_H
#define MQTT_H

struct mosquitto * init_mqtt();
void publish_mqtt(struct mosquitto * mosq, char *topic, char *message);
void subscribe_mqtt(struct mosquitto *mosq, char *topic, int qos);
void clean_up_mosquitto(struct mosquitto **mosq);

#endif /* MQTT_H */