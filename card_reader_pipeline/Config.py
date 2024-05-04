import random

MQTT_TOPIC_RECEIVE = ['sensors/card_reader/1',]
MQTT_CLIENT_ID = f'python-mqtt-{random.randint(0, 1000)}'

ACCEPTED_CARDS = ["c4abc422"]

MQTT_TOPICS = {"card_reader": "actuators/doorlock/1",}