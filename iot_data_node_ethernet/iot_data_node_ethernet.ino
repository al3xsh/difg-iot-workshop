/*
 * iot_data_node.ino
 * 
 * arduino sketch to send temperature and humidity data to a public mqtt broker
 * in this example we are using the arduino ethernet shields for communication
 *
 * author:  alex shenfield
 * date:    21/02/2022
 */

// INCLUDES

// we are using the Ethernet library (which supports the WizNet w5500 Ethernet
// controller)
#include <SPI.h>
#include <Ethernet.h>

/// include the pubsub client
#include <PubSubClient.h>

// include the sensor library
#include "dht.h"

// ETHERNET CONNECTION DECLARATIONS

// mac address
byte mac[] = {0x90, 0xA2, 0xDA, 0x11, 0x44, 0xC7};

// MQTT DECLARATIONS

// mqtt server details
const char server[] = "test.mosquitto.org";
const long port     = 1883;

// get an Ethernet client object and pass it to the mqtt client
EthernetClient ethernet_client;
PubSubClient client(ethernet_client);

// timing variables - send data approximately every 10 seconds
long previous_time = 0;
long connection_interval = 10000;

// SENSOR DECLARATIONS

// dht sensor pin
#define dhtpin  2

// CODE

// set up code
void setup()
{
  // set up serial comms for debugging and display of DHCP allocated IP 
  // address
  Serial.begin(115200);
  Serial.println("starting mqtt client on arduino ...");

  // mqtt server and subscription callback
  client.setServer(server, port);
  client.setCallback(callback);

  // start the ethernet shield communication - initially try to get a DHCP ip
  // address
  if (Ethernet.begin(mac) == 0)
  {
    // if DHCP fails, print a message and wait forever
    Serial.println("failed to configure ethernet using DHCP");
    while (1)
    {
      ;
    }
  }

  // print the IP address to the serial monitor
  IPAddress myIP = Ethernet.localIP();
  Serial.print("my ip address is: "); 
  Serial.println(myIP);
}

// main loop
void loop()
{  
  // if the client isn't connected then try to reconnect
  if (!client.connected())
  {
    reconnect();
  }
  else
  {
    // handle subscriptions to topics (i.e. incoming messages)
    client.loop();

    // periodically publish a message to a feed (note, this uses the 
    // same non blocking timing mechanism as in blink_without_delay.ino
    // from lab 1)
    unsigned long current_time = millis();
    if(current_time - previous_time > connection_interval)
    {
      previous_time = current_time;
      send_sensors();
    }
  }
}

// MQTT FUNCTIONS

// mqtt message received callback
void callback(char* topic, byte* payload, unsigned int length) 
{
  // print the message
  Serial.print("message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) 
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// reconnect to server
void reconnect()
{
  // loop until we're reconnected
  while (!client.connected())
  {
    Serial.println("attempting mqtt connection...");

    // try to connect to the mqtt broker
    if (client.connect("axs_arduino_190310"))
    {
      Serial.println("... connected");
      
      // once connected, subscribe to the appropriate feeds
      client.subscribe("alexshenfield/shu-iot-demonstrator/#");      

      // ... and publish an announcement
      client.publish("alexshenfield/shu-iot-demonstrator/status-messages", "we are alive!");
    }
    else
    {
      // print some error status
      Serial.print("connection failed, rc = ");
      Serial.print(client.state());
      Serial.println();
      Serial.println("we will try again in 5 seconds");

      // wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// APPLICATION FUNCTIONS

// function to send the sensor values
void send_sensors()
{
  // if the client is already connected then send data
  if (client.connected())
  {
    // dht sensor
    
    // create a dht sensor instance and read it
    dht DHT;
    int chk = DHT.read11(dhtpin);

    // if the return code indicates success we get the data, turn it into a string, and 
    // send it out via mqtt
    if(chk == DHTLIB_OK)
    {
      // get the data from the sensor object
      float h = DHT.humidity;
      float t = DHT.temperature;

      // convert the sensor readings to character arrays
      char humidity_level[6];
      char temperature_level[6];
      dtostrf(h, 5, 2, humidity_level);
      dtostrf(t, 5, 2, temperature_level);
  
      // send via mqtt
      client.publish("alexshenfield/shu-iot-demonstrator/humidity", humidity_level);
      client.publish("alexshenfield/shu-iot-demonstrator/temperature", temperature_level);
    }        
  }
}
