#define SERVER 0
#define CLIENT 1

#include <UIPEthernet.h>

#if SERVER
static EthernetServer tcpconn = EthernetServer(1000);
#endif
#if CLIENT
static EthernetClient tcpconn;
#endif

static int readVcc(void)
{
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
    ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

void setup(void)
{
  Serial.begin(57600);
  Serial.println("setup()");

  uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
  IPAddress myIP(10,0,0,6);
  Ethernet.begin(mac,myIP);
#if SERVER
  tcpconn.begin();
#endif
}

void loop(void)
{
  static unsigned long last_millis = 0;
  static int count = 0;
  if (millis() - last_millis > 1000) {
    last_millis = millis();
    count++;

    Serial.print("Time: ");
    Serial.print(last_millis);
    Serial.print(" ");
    Serial.println(count);

#if CLIENT
    if (tcpconn.connect(IPAddress(10,0,0,1), 1234))
#endif
    {

    tcpconn.print("Time: ");
    tcpconn.print(last_millis);
    tcpconn.print("ms ");
    tcpconn.print(count);
    tcpconn.print(" ");
    tcpconn.print(readVcc(), DEC);
    tcpconn.print("mV ");
    tcpconn.println();

#if CLIENT
    tcpconn.stop();
#endif
    }
  }

  {
    int rc = Ethernet.maintain();
    if (rc != 0) {
      Serial.print("::maintain() ");
      Serial.println(rc);
    }
  }
}
