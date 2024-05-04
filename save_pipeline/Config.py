import random

MQTT_TOPIC_RECEIVE = ['actuators/light_bulb/state', 
                      'actuators/heat_pump/state',
                      'sensors/#',
                      'test',]

# MQTT_TOPIC_RECEIVE = ['sensors/response/#',
#                       'sensors/card_reader/1',]
MQTT_CLIENT_ID = f'python-mqtt-{random.randint(0, 1000)}'

MQTT_TOPICS = {"heat_pump": "actuators/heat_pump/action",
               "light_bulb": "actuators/light_bulb/action",}