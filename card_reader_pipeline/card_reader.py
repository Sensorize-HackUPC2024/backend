from paho.mqtt import client as mqtt_client
import logging
import time
import json
import Config
import os
import datetime
from sqlalchemy import create_engine
import sqlalchemy as db
import random


# Connection settings
FIRST_RECONNECT_DELAY = 1
RECONNECT_RATE = 2
MAX_RECONNECT_COUNT = 12
MAX_RECONNECT_DELAY = 60
FLAG_EXIT = False

# Configuración de MQTT
mqtt_broker = os.environ.get('MQTT_BROKER', 'localhost')
mqtt_port = int(os.environ.get('MQTT_PORT', 1888))
topic_mqtt = Config.MQTT_TOPIC_RECEIVE
MQTT_CLIENT_ID = Config.MQTT_CLIENT_ID
MQTT_USERNAME = os.environ.get('MQTT_USERNAME', 'public')
MQTT_PASSWORD = os.environ.get('MQTT_PASSWORD', 'public')

def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            logging.info("Connected to MQTT Broker!")
        else:
            logging.error("Failed to connect, return code %d\n", rc)
    # Set Connecting Client ID
    logging.info("Connecting to {0}:{1}".format(mqtt_broker, mqtt_port))
    client = mqtt_client.Client(MQTT_CLIENT_ID)
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

    # Security settings, not working
    # client.tls_set(certfile=None,
    #            keyfile=None,
    #            cert_reqs=ssl.CERT_REQUIRED)

    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.connect(mqtt_broker, mqtt_port)
    return client

def on_disconnect(client, userdata, rc):
    logging.info("Disconnected with result code: %s", rc)
    reconnect_count, reconnect_delay = 0, FIRST_RECONNECT_DELAY
    while reconnect_count < MAX_RECONNECT_COUNT:
        logging.info("Reconnecting in %d seconds...", reconnect_delay)
        time.sleep(reconnect_delay)

        try:
            client.reconnect()
            logging.info("Reconnected successfully!")
            return
        except Exception as err:
            logging.error("%s. Reconnect failed. Retrying...", err)

        reconnect_delay *= RECONNECT_RATE
        reconnect_delay = min(reconnect_delay, MAX_RECONNECT_DELAY)
        reconnect_count += 1
    logging.eerror("Reconnect failed after %s attempts. Exiting...", reconnect_count)
    global FLAG_EXIT
    FLAG_EXIT = True

def subscribe_and_publish(mqtt_client):
    def on_message(client, userdata, msg):
        try:
            value = msg.payload.decode()
            logging.info(f"Received `{value}` from topic `{msg.topic}` in MQTT")
            logging.info(f"Processing message: {value}")
            publish(client, value)
        except Exception as e:
            logging.error(f"Error processing message: {e}")

    for topic in topic_mqtt:
        mqtt_client.subscribe(topic)
        logging.info(f'Subscribed to topic {topic}')
    mqtt_client.on_message = on_message

def publish(client, value):
    topic = Config.MQTT_TOPICS[Config.MQTT_TOPIC_RECEIVE[0].split('/')[-2]]
    msg_processed= {"card_id": value}
    if value in Config.ACCEPTED_CARDS:
        msg = "1"
        msg_processed['status']="accepted"
        logging.info(f"Card accepted")
    else:    
        msg = "0"
        msg_processed['status']="rejected"
        logging.info(f"Card rejected")
    result = client.publish(topic, msg)
    result = client.publish("sensors/response/card_reader/1", json.dumps(msg_processed))
    # result: [0, 1]
    status = result[0]
    if status == 0:
        logging.info(f"Send `{msg}` to topic `{topic}`")
        logging.info(f"Send `{msg_processed}` to topic `sensors/response/card_reader/1`")
    else:
        print(f"Failed to send message to topic {topic}")


def run():
    global mqtt_client, timeseries, engine
    time.sleep(20)
    logging.basicConfig(format='%(asctime)s - %(levelname)s: %(message)s', level=logging.INFO)
    
    mqtt_client = connect_mqtt()
    subscribe_and_publish(mqtt_client)
    # use connect function to establish the connection

    mqtt_client.loop_forever()
    

if __name__ == '__main__':
    run()