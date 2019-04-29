#include <Arduino.h>
#include <U8x8lib.h>

#define LED_pin 25

#define PWM_channel 0 // channel (0-15)
#define PWM_frequency 5 // frequency in Hz
#define PWM_resolution 10 // nbr on bits

#define SDA_pin 21 // data
#define SCL_pin 22 // clock

//U8X8_SSD1306_128X64_NONAME_HW_I2C oledDisplay(U8X8_PIN_NONE);
U8X8_SSD1306_128X32_UNIVISION_HW_I2C oledDisplay(U8X8_PIN_NONE);

float hallValue = 0;
int pwmValue = 0;
void setup() {
  // put your setup code here, to run once:
  // configure serial monitoring
  Serial.begin(115200);
  Serial.println("Start program");
  // configure led pin as output
  pinMode(LED_pin, OUTPUT);
  // configure PWM
  ledcSetup(PWM_channel, PWM_frequency, PWM_resolution);
  ledcAttachPin(LED_pin, PWM_channel);
  Serial.println("PWM duty 25%");
  ledcWrite(PWM_channel, 256); // 1024/4 = 256
  // configure and initiate OLED display
  oledDisplay.begin();
  oledDisplay.setPowerSave(0);
  oledDisplay.setFont(u8x8_font_amstrad_cpc_extended_f);
  oledDisplay.clear();
  oledDisplay.setCursor(0,0);
  oledDisplay.println("Hello1");
  oledDisplay.println("Hello2");
  oledDisplay.println("Hello3");
  oledDisplay.println("Hello4");
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Blink led by on and off
  /*digitalWrite(LED_pin, HIGH);
  delay(500);
  digitalWrite(LED_pin, LOW);
  delay(500);*/
  // read internal temperature of esp32. return 53.33 C all the time.
  Serial.print(temperatureRead());
  Serial.println(" C");
  // read internal hall. hall sensor is used to measure the magnitude of a magnetic field.
  hallValue = hallRead();
  Serial.println(hallValue);
  if((hallValue>10) and (pwmValue<8)){
    pwmValue++;
  }
  if((hallValue<-10) and (pwmValue>0)){
    pwmValue--;
  }
  ledcWrite(PWM_channel, 128*pwmValue);
  delay(1000);
}