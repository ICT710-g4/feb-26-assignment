#include <mbed.h>
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"
#include <MQTTClientMbedOs.h>
 
DigitalOut led(LED1);
InterruptIn button(USER_BUTTON);
Thread t;
EventQueue queue(5 * EVENTS_EVENT_SIZE);
Serial pc(USBTX, USBRX);
WiFiInterface *wifi;

int arrivedcount = 0;
void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    pc.printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    pc.printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}


const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}

int scan_wifi() {
    WiFiAccessPoint *ap;

    printf("Scan:\n");
    int count = wifi->scan(NULL,0);
    if (count <= 0) {
        printf("scan() failed with return value: %d\n", count);
        return 0;
    }

    /* Limit number of network arbitrary to 15 */
    count = count < 15 ? count : 15;
    ap = new WiFiAccessPoint[count];
    count = wifi->scan(ap, count);
    if (count <= 0) {
        printf("scan() failed with return value: %d\n", count);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\n", ap[i].get_ssid(),
               sec2str(ap[i].get_security()), ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
               ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5], ap[i].get_rssi(), ap[i].get_channel());
    }
    printf("%d networks available.\n", count);

    delete[] ap;   

    return count; 
}


void pressed_handler() {
    int count;

    count = scan_wifi();
    if (count == 0) {
        pc.printf("No WIFI APs found - can't continue further.\n");
        return;
    }

    pc.printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        pc.printf("\nConnection error: %d\n", ret);
        return;
    }

    pc.printf("Success\n\n");
    pc.printf("MAC: %s\n", wifi->get_mac_address());
    SocketAddress a;
    wifi->get_ip_address(&a);
    pc.printf("IP: %s\n", a.get_ip_address());
    wifi->get_netmask(&a);
    pc.printf("Netmask: %s\n", a.get_ip_address());
    wifi->get_gateway(&a);
    pc.printf("Gateway: %s\n", a.get_ip_address());
    pc.printf("RSSI: %d\n\n", wifi->get_rssi());
    pc.printf("\nDone\n");
	
	
	pc.printf("\nMQTT\n");
	float version = 0.6;
    char* topic = "@msg/ict710";
    pc.printf("HelloMQTT: version is %.2f\r\n", version);
    
	MQTTNetwork mqttNet(wifi);
	TCPSocket socket;
	
	MQTTClient client(&socket);
	socket.open(wifi);
	
	
	//MQTT::Client<MQTTNetwork, Countdown> client(mqttNet);
 
    const char* hostname = "mqtt.netpie.io";
    int port = 1883;

    pc.printf("Connecting to %s:%d\r\n", hostname, port);
    //int rc = mqttNetwork.connect(hostname, port);
	int rc = socket.connect(hostname, port);
	
    //if (rc != 0)
    //    pc.printf("rc from TCP connect is %d\r\n", rc);
    
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	
    data.MQTTVersion = 3;
	
    data.clientID.cstring = "1a3949ab-c9d2-4145-a979-9253be0c6b24";
    data.username.cstring = "4nDGdGfTawopHZrYmejKdsNc86NmNfih";
    //data.password.cstring = "x.QlkitGMRIamsdPIcDbLgkWfV)X$xTI";
    
	client.connect(data);
	
	//if ((rc = client.connect(data)) != 0)
    //    pc.printf("rc from MQTT connect is %d\r\n", rc);
	
	//while(!client.isConnected()){
	//	client.connect(data);
	//}
	
    //if ((rc = client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
    //    pc.printf("rc from MQTT subscribe is %d\r\n", rc);
    
	MQTT::Message message;
    
	// QoS 0
    char buf[100];
    sprintf(buf, "Hello World!  QoS 0 message from app version %f\r\n", version);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
    //while (arrivedcount < 1)
    //    client.yield(100);
	pc.printf("QoS 0\n");
 
    // QoS 1
    sprintf(buf, "Hello World!  QoS 1 message from app version %f\r\n", version);
    message.qos = MQTT::QOS1;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
    //while (arrivedcount < 2)
    //    client.yield(100);
	pc.printf("QoS 1\n");
	
    // QoS 2
    sprintf(buf, "Hello World!  QoS 2 message from app version %f\r\n", version);
    message.qos = MQTT::QOS2;
    message.payloadlen = strlen(buf)+1;
    rc = client.publish(topic, message);
    //while (arrivedcount < 3)
    //    client.yield(100);
	pc.printf("QoS 2\n");
	
    if ((rc = client.unsubscribe(topic)) != 0)
        pc.printf("rc from unsubscribe was %d\r\n", rc);
 
    if ((rc = client.disconnect()) != 0)
        pc.printf("rc from disconnect was %d\r\n", rc);
 
    mqttNet.disconnect();
    pc.printf("Version %.2f: finish %d msgs\r\n", version, arrivedcount);
	wifi->disconnect();
	
}

int main() {
    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }
	
	
    t.start(callback(&queue, &EventQueue::dispatch_forever));
    button.fall(queue.event(pressed_handler));
    pc.printf("Starting\n");
    while(1) {
        led = !led;
        ThisThread::sleep_for(500);
    }
}