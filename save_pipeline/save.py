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
            logging.info(f"Received `{msg.payload.decode()}` from topic `{msg.topic}` in MQTT")
            try:
                value = json.loads(str(msg.payload.decode("utf-8")))
            except json.JSONDecodeError:
                value = msg.payload.decode("utf-8")
            device = msg.topic.split("/")[-2]
            msg_processed = {"timestamp":datetime.datetime.now()}
            msg_processed["device"] = device
            if device == "card_reader":
                if msg.topic.split("/")[-3]=="response":
                    msg_processed['card_id'] = value["card_id"]
                    msg_processed['status']= value["status"]
                else:
                    msg_processed['card_id'] = value
                    msg_processed['status']= "requested"
                msg_processed["device_id"] = msg.topic.split("/")[-1]
                table = cards
                logging.info(f"Saving to database: {msg_processed}")
            else:
                msg_processed['value'] = value
                msg_processed["id"] = msg.topic.split("/")[-1]
                table = timeseries
                logging.info(f"Saving to database: {msg_processed}")
            with engine.connect() as conn:
                res = conn.execute(table.insert(), msg_processed)
                # commit the changes
                conn.commit()
            if res:
                logging.info("Data inserted successfully")
        except Exception as e:
            logging.error(f"Error processing message: {e}")

    for topic in topic_mqtt:
        mqtt_client.subscribe(topic)
        logging.info(f'Subscribed to topic {topic}')
    mqtt_client.on_message = on_message

def publish(client):
    msg_count = 1
    devices = ["door", "light", "heating", "AC", "noise"]
    while True:
        device = random.choice(devices)
        # topic = f"actuators/{device}/1"
        topic = f"actuators/1"
        time.sleep(1)
        msg = str(random.randint(0, 1))
        result = client.publish(topic, msg)
        # result: [0, 1]
        status = result[0]
        if status == 0:
            print(f"Send `{msg}` to topic `{topic}`")
        else:
            print(f"Failed to send message to topic {topic}")
        msg_count += 1
        time.sleep(1)
        if msg_count > 1:
            break

def run():
    global mqtt_client, timeseries, engine, cards
    time.sleep(20)
    logging.basicConfig(format='%(asctime)s - %(levelname)s: %(message)s', level=logging.INFO)
    
    mqtt_client = connect_mqtt()
    subscribe_and_publish(mqtt_client)

    engine = create_engine("postgresql://root:example@localhost:5432/HackUPC", future=True)

    metadata_obj = db.MetaData()
    timeseries = db.Table(
        'timeseries',                                        
        metadata_obj,                                    
        db.Column('id', db.String(20)), 
        db.Column('timestamp', db.TIMESTAMP),                    
        db.Column('device', db.String(20)),
        db.Column('value', db.Integer),         
    )

    cards = db.Table(
        'cards',                                        
        metadata_obj,                                    
        db.Column('timestamp', db.TIMESTAMP),                    
        db.Column('device_id', db.String(20)), 
        db.Column('device', db.String(20)),
        db.Column('card_id', db.String(20)),  
        db.Column('status', db.String(20)),    
    )
 
    # use connect function to establish the connection

    mqtt_client.loop_forever()
    

if __name__ == '__main__':
    run()