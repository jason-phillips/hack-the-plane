#include <PowerFunctions.h>
#include <Wire.h>
#include <CircularBuffer.h>

/**
 *  Project: Hack The "Airplane" - dds.mil
 *  Title: Engine Control Unit
 *  
 *  Purpose: To expose people to common low level protocols that mimic aviation protocols, specifically 
 *  using I2C as a correlation to 1553.
 *    
 *    
 *  @author Dan Allen
 *  @version 1.0 7/25/19
 *  
 *  Contributers:
 *  Jason Phillips
 *    
 *  Credits:
 *    https://github.com/jurriaan/Arduino-PowerFunctions *    
 */

/*
 * General Config Definitions
 */
#define ENGINE_I2C_ADDRESS 0x54
#define LEGO_IR_CHANNEL 0 //0=ch1 1=ch2 etc.
#define LEGO_MOTOR_OUTPUT_BLOCK BLUE
#define LEGO_SMOKE_OUTPUT_BLOCK RED
#define SERIAL_BAUD 9600
#define I2C_RX_BUFFER_SIZE 50
#define I2C_TX_BUFFER_SIZE 50
#define SMOKE_LENGTH_MS   2000

/*
 * Pin Definitions
 */
#define GREEN_LED 3
#define YELLOW_LED 5
#define RED_LED 6
#define LEGO_PF_PIN 10

/*
 * State Machine Definitions
 */
#define OFF 0x00
#define ON  0x01
#define DC  0x10

#define SMOKE_OFF     0x00
#define SMOKE_START   0x01
#define SMOKE_RUNNING 0x02
#define SMOKE_STOP    0x03


/*
 * I2C Comms Definitions
 */
//Commands
#define GET_ENGINE_STATUS 0x22
#define SET_ENGINE_SPEED  0x23
#define MARCO             0x24
#define QUERY_COMMANDS    0x63
#define SMOKEN            0x42

//Response
#define UNKNOWN_COMMAND   0x33
#define NO_DATA           0xFF

/*
 * Library Instantiations
 */
PowerFunctions pf(LEGO_PF_PIN, LEGO_IR_CHANNEL);   //Setup Lego Power functions pin


/*
 * Globals
 */
short volatile g_engine_speed = 0;
short volatile g_smoke_state = 0;
boolean g_mode_change = false;
//Timers
unsigned long volatile g_last_time_ms = 0;
unsigned long volatile g_current_time_ms =0;
unsigned long volatile g_smoke_timer_ms = 0;


CircularBuffer<short,I2C_RX_BUFFER_SIZE> g_i2c_rx_buffer;
CircularBuffer<short,I2C_TX_BUFFER_SIZE> g_i2c_tx_buffer;
String g_command_list = "0x22 0x23 0x24 0x63 0x42";

/*
 * Setup method to handle I2C Wire setup, LED Pins and Serial output
 */
void setup() {
    
  Wire.begin(ENGINE_I2C_ADDRESS);
  Wire.onReceive(receiveEvent); // register event handler for recieve
  Wire.onRequest(requestEvent); // register event handler for request

  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  
  Serial.begin(SERIAL_BAUD);           // start serial for output debugging
  Serial.println("Main Engine Control Unit is online, ready for tasking");
  set_led(ON, OFF, OFF);

  g_last_time_ms = millis();
  g_current_time_ms = millis();
  
  
}

/*
 * The main loop of execution for the Engine Control Unit
 */
void loop() {
  service_ir_comms();
  service_timers();
}

/*
 * Allows for non-blocking timers
 * Note, the timer accuraccy is no better than 1ms and only as good as
 * how often the service_timers function get's called. A better way to
 * do this is run a decrementer in a timer inturrupt but Arduino framework
 * makes that difficult.
 */
 void service_timers() {
  unsigned long delta = 0;
  g_current_time_ms = millis();
  delta = g_current_time_ms - g_last_time_ms;
  g_last_time_ms = g_current_time_ms;
  
  //decrement individual timers
  decrement_timer(&g_smoke_timer_ms, &delta);
  
  
  
 }

/*
 * Helper function to decrement timers
 */
 void decrement_timer(unsigned long volatile *timer, unsigned long *delta) {
  if(*timer > *delta) {
    *timer = *timer - *delta;
  }
  else {
    *timer = 0;
  }
 }


/*
 * Manages IR comms interface
 */
void service_ir_comms() {
  if(g_mode_change == true){   //process new mode of operation
    Serial.print("Mode change request to: 0x");
    Serial.println(g_engine_speed, HEX);
    g_mode_change = false;

    //Handle the desired mode change
    switch(g_engine_speed){
      case 0:
        Serial.println("Engine off");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_BRK);
        break;
      case 1:
        Serial.println("Slow Speed 1");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_FWD1);
        break;
      case 2:
        Serial.println("Slow Speed 2");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_FWD2);
        break;
      case 3:
        Serial.println("Medium Speed 1");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_FWD3);
        break;
      case 4:
        Serial.println("Medium Speed 2");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_FWD4);
        break;
      case 5:
        Serial.println("Fast Speed 1");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_FWD5);
        break;
      case 6:
        Serial.println("Fast Speed 2");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_FWD6);
        break;
      case 7:
        Serial.println("Fast Speed 2");
        pf.single_pwm(LEGO_MOTOR_OUTPUT_BLOCK, PWM_FWD7);
        break;
      default:   
        Serial.println("Unknown Motor Speed");         
        break;
    }
  }

  switch(g_smoke_state){
    case SMOKE_OFF:
      //Just chill here and wait
      break;
      
    case SMOKE_START:
      g_smoke_timer_ms = SMOKE_LENGTH_MS;
      Serial.println("SMOKEN!!!");
      pf.single_pwm(LEGO_SMOKE_OUTPUT_BLOCK, PWM_FWD7);
      g_smoke_state = SMOKE_RUNNING;
      break;

    case SMOKE_RUNNING:
      if(g_smoke_timer_ms == 0) {
          g_smoke_state = SMOKE_STOP;
      }
      break;
      
    case SMOKE_STOP:
      Serial.println("Extinguish!!!");
      pf.single_pwm(LEGO_SMOKE_OUTPUT_BLOCK, PWM_BRK);
      pf.single_pwm(LEGO_SMOKE_OUTPUT_BLOCK, PWM_FLT);
      g_smoke_state = SMOKE_OFF;
      break;
  }
  //Limit loop speed
  //delay(100);
}

/*
 * Parse a string into the I2C buffer to be spit out
 */
void string_to_i2c_buffer(String data) {
  int i;
  for(i=0; i < data.length(); i++) {
    g_i2c_tx_buffer.push(data.charAt(i));
  }
}

/*
 * I2c State machine
 * Needs to be non-blocking and as quick as possible
 */
void process_i2c_request(void) {
  short command_temp;
  if(g_i2c_rx_buffer.isEmpty() != true) {
    //clear any unsent responses
    g_i2c_tx_buffer.clear();
    //read command and pull it out of the buffer
    command_temp = g_i2c_rx_buffer.shift();
    switch(command_temp){
      case GET_ENGINE_STATUS:
        Serial.println("Command Received, GET_ENGINE_STATUS");
        g_i2c_tx_buffer.push(g_engine_speed);
        //string_to_i2c_buffer("hello");
        break;
  
      case SET_ENGINE_SPEED:
        Serial.print("Command Received, SET_ENGINE_SPEED : ");
        g_engine_speed = g_i2c_rx_buffer.shift();
        g_mode_change = true;
        Serial.println(g_engine_speed, HEX);
        //Note, there should be some sanitization here, but maybe not for hacking comp?
        break;

      case SMOKEN:
        g_smoke_state = SMOKE_START;
        g_mode_change = true;
        break;

      case MARCO:
        Serial.println("Command Received, MARCO");
        string_to_i2c_buffer("Polo");
        break;
  
      default:
        Serial.print("Received unknown command: ");
        Serial.println(command_temp);
        g_i2c_tx_buffer.push(UNKNOWN_COMMAND);
    }
    g_i2c_rx_buffer.clear(); //flush buffer after processing  
  }
}


/*
 * Event Handler for processing I2C commands sent to this device
 * NOTE: I don't like accessing the response in this inturrupt,
 * but I2C needs an imeediate response.
 */
void receiveEvent(int numofbytes)
{  
  while(Wire.available()){
    g_i2c_rx_buffer.push((short) Wire.read());
  }
  process_i2c_request();
}

/*
 * Event Handler for processing an I2C request for data
 */
void requestEvent() {
  if(g_i2c_tx_buffer.isEmpty() != true) {
    while(g_i2c_tx_buffer.isEmpty() != true) {
      Wire.write(g_i2c_tx_buffer.shift());
    }
  }
  else {
    Wire.write(NO_DATA); // Out of data, respond with NO DATA
  }
}


/*
 * Set's LED State
 * 0x00 = OFF = LED off
 * 0x01 = ON = LED on
 * 0x10 = DC = Don't change the state
 */
void set_led(short g, short y, short r) {
  switch(g) {
    case 0x00:
      digitalWrite(GREEN_LED, LOW);
      break;

    case 0x01:
      digitalWrite(GREEN_LED, HIGH);
      break;
  }

  switch(y){
    case 0x00:
      digitalWrite(YELLOW_LED, LOW);
      break;

    case 0x01:
      digitalWrite(YELLOW_LED, HIGH);
      break;
  }

  switch(r) {
    case 0x00:
      digitalWrite(RED_LED, LOW);
      break;

    case 0x01:
      digitalWrite(RED_LED, HIGH);
      break;
  }
}


/*
 * DEBUG ONLY
 * print rx receive buffer
 */
void dbg_print_rx_buffer(void) {
  int i;
  if(g_i2c_rx_buffer.isEmpty() != true) {
    for(i=0; i< g_i2c_rx_buffer.size() - 1; i++) {
      Serial.print(g_i2c_rx_buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println("Done");
  }
  else {
    Serial.println("Buffer Empty");
  }
}
