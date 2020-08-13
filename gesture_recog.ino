#include <WiFi.h>
#include <mpu6050_esp32.h>
#include<math.h>
#include<string.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
uint8_t state = 0;
int steps = 0;
float old_acc_mag, older_acc_mag;
const uint8_t PIN_1 = 16; //button 1
const uint8_t PIN_2 = 5; //button 2
uint8_t button_state;
uint8_t old_val;
const uint8_t LOOP_PERIOD = 1000; //milliseconds
uint32_t primary_timer = 0;
float x,y,z; //variables for grabbing x,y,and z values
int old_button_state;
MPU6050 imu; //imu object called, appropriately, imu
char list1[12000];
char network[] = "ATT66TpaI2";  //SSID CHANGE!!
char password[] = "y8ux=xu=82f7"; //Password for WiFi CHANGE!!!
char host[] = "608dev-2.net";
const uint16_t IN_BUFFER_SIZE = 1700; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 2000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
const int RESPONSE_TIMEOUT = 6000;
 
void setup() {
  Serial.begin(115200); //for debugging if needed.
  delay(50); //pause to make sure comms get set up
  Wire.begin();
  delay(50); //pause to make sure comms get set up
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  if (imu.setupIMU(1)){
    Serial.println("IMU Connected!");
  }else{
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  pinMode(PIN_1, INPUT_PULLUP);
  pinMode(PIN_2, INPUT_PULLUP);
  tft.init(); //initialize the screen
  tft.setRotation(2); //set rotation for our layout
  primary_timer = millis();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  steps = 0; //initialize steps to zero!
  old_val = digitalRead(PIN_1);
}
 
 
 
void loop() {
  button_state = digitalRead(PIN_1);
  if ( digitalRead(PIN_1) != 1 )  {
    Serial.println("Recording");
    record();
    Serial.println("Done");
    Serial.println(list1);
    char body[1700]; //for body;
    sprintf(body, "x=%s", list1); //generate body, posting to User, 1 step
    int body_len = strlen(body); //calculate body length (for header reporting)
    sprintf(request_buffer, "POST http://608dev-2.net/sandbox/sc/hienle/gesture_recog/gesture_recog.py HTTP/1.1\r\n");
    strcat(request_buffer, "Host: 608dev-2.net\r\n");
    strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
    strcat(request_buffer, "\r\n"); //new line from header to body
    strcat(request_buffer, body); //body
    strcat(request_buffer, "\r\n"); //header
    Serial.println(request_buffer);
    do_http_request("608dev-2.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
    Serial.println(response_buffer);
    memset(list1, 0, sizeof(list1));
   
  }
  old_button_state = button_state;
}
   
void record(){
  while ( digitalRead(PIN_1) != 1 ) {
  if(millis()%50==0){
  imu.readAccelData(imu.accelCount);
  x = imu.accelCount[0]*imu.aRes;
  char output[80];
  sprintf(output,"%4.2f",x); //render numbers with %4.2 float formatting
  while (millis()-primary_timer<LOOP_PERIOD) {; //wait for primary timer to increment
  primary_timer =millis();
  }
  strcat(list1,output);
  strcat(list1,"_");
  }
}
 
}
 
 
 
/*----------------------------------
 * char_append Function:
 * Arguments:
 *    char* buff: pointer to character array which we will append a
 *    char c:
 *    uint16_t buff_size: size of buffer buff
 *    
 * Return value:
 *    boolean: True if character appended, False if not appended (indicating buffer full)
 */
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}
 
 
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (serial) Serial.println(response);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}
