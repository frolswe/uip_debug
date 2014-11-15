#define SERVER 0
#define CLIENT 1

#include <UIPEthernet.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <HTU21D.h>

static Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
static HTU21D htu;
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

  if(!bmp.begin()) {
    Serial.print("BMP085.begin() error.");
  }

  htu.begin();

  uint8_t mac[6] = {0x00,0x01,0x02,0x03,0x04,0x05};
  IPAddress myIP(10,0,0,6);
  Ethernet.begin(mac,myIP);
#if SERVER
  tcpconn.begin();
#endif
}

void loop(void)
{
  static int bmp_cnt = 0;
  static float bmp_pres_sum = 0, bmp_pres_min = 999999.0, bmp_pres_max = 0.0;
  {
    float bmp_pres;
    bmp.getPressure(&bmp_pres);
    if (bmp_pres > bmp_pres_max)
      bmp_pres_max = bmp_pres;
    if (bmp_pres < bmp_pres_min)
      bmp_pres_min = bmp_pres;
    bmp_pres_sum += bmp_pres;
    bmp_cnt++;
  }

  static unsigned long last_millis = 0;
  static int count = 0;
  if (millis() - last_millis > 1000) {
    last_millis = millis();
    count++;

    Serial.print("Time: ");
    Serial.print(last_millis);
    Serial.print(" ");
    Serial.println(count);

    float bmp_temp;
    bmp.getTemperature(&bmp_temp);

    float htu_humd = htu.readHumidity();
    float htu_temp = htu.readTemperature();

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

    tcpconn.print(" BMP: ");
    tcpconn.print(bmp_temp, 1);
    tcpconn.print("C ");
    tcpconn.print(bmp_pres_sum / (float)bmp_cnt / 100.0, 2);
    tcpconn.print("hPa ");
    tcpconn.print(bmp_pres_min / 100.0, 2);
    tcpconn.print("hPa ");
    tcpconn.print(bmp_pres_max / 100.0, 2);
    tcpconn.print("hPa ");
    tcpconn.print(bmp_cnt);
    tcpconn.print("cnt ");

    tcpconn.print(" HTU21: ");
    tcpconn.print(htu_humd, 1);
    tcpconn.print("%RH ");
    tcpconn.print(htu_temp, 1);
    tcpconn.print("C");

    tcpconn.println();

#if CLIENT
    tcpconn.stop();
#endif
    }

    bmp_pres_min = 999999.0;
    bmp_pres_max = 0.0;
    bmp_pres_sum = 0.0;
    bmp_cnt = 0;
  }

  {
    int rc = Ethernet.maintain();
    if (rc != 0) {
      Serial.print("::maintain() ");
      Serial.println(rc);
    }
  }
}
