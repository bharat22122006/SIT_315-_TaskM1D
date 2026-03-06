// Define pin assignments for inputs and outputs
#define BUTTON_PIN   9      // Digital pin 9 connected to push button (PORTB - PCINT1 interrupt pin)
#define IR_PIN       8      // Digital pin 8 connected to IR obstacle sensor (PORTB - PCINT0 interrupt pin)
#define PIR_PIN      2      // Digital pin 2 connected to PIR motion sensor (External Interrupt INT0)
#define TRIG_PIN     6      // Digital pin 6 used as trigger for ultrasonic sensor
#define ECHO_PIN     5      // Digital pin 5 used to receive echo from ultrasonic sensor
#define LED_EVENT    13     // Built-in LED on the Arduino board used as an output indicator

// Flags updated inside interrupt routines
volatile bool buttonFlag = false;   // Flag set true when button interrupt occurs
volatile bool irFlag     = false;   // Flag set true when IR sensor detects obstacle
volatile bool pirFlag    = false;   // Flag set true when PIR sensor detects motion
volatile bool timerFlag  = false;   // Flag set true when timer interrupt occurs

// Variable used to store the previous state of PORTB pins
volatile uint8_t lastPortBState = 0;

// Variables used for periodic ultrasonic readings
unsigned long lastUltrasonicRead = 0;           // Stores last time ultrasonic sensor was read
const unsigned long ultrasonicInterval = 1000;  // Interval between readings (1000ms = 1 second)

void setup() {

  Serial.begin(9600); // Start serial communication at 9600 baud rate

  pinMode(BUTTON_PIN, INPUT_PULLUP); // Configure button as input with internal pull-up resistor
  pinMode(IR_PIN, INPUT);            // Configure IR sensor pin as input
  pinMode(PIR_PIN, INPUT);           // Configure PIR sensor pin as input
  pinMode(TRIG_PIN, OUTPUT);         // Set ultrasonic trigger pin as output
  pinMode(ECHO_PIN, INPUT);          // Set ultrasonic echo pin as input
  pinMode(LED_EVENT, OUTPUT);        // Set built-in LED pin as output

  setupExternalInterrupt(); // Configure external interrupt for PIR sensor
  setupPCI();               // Configure pin-change interrupts for button and IR sensor
  setupTimer1();            // Configure Timer1 interrupt

  Serial.println("System Started"); // Print startup message in Serial Monitor
}

// Function to configure external interrupt for PIR motion sensor
void setupExternalInterrupt() {

  // Attach interrupt to PIR_PIN and trigger on rising edge (when motion detected)
  attachInterrupt(digitalPinToInterrupt(PIR_PIN), pirISR, RISING); 

}

// Interrupt Service Routine for PIR sensor
void pirISR() {

  pirFlag = true;  // Set motion detection flag

}

// Configure Pin Change Interrupt for PORTB pins (D8 and D9)
void setupPCI() {

  cli();  // Disable global interrupts temporarily during configuration

  PCICR |= (1 << PCIE0);  // Enable Pin Change Interrupt for PORTB group

  PCMSK0 |= (1 << PCINT0);  // Enable interrupt for D8 (IR sensor)
  PCMSK0 |= (1 << PCINT1);  // Enable interrupt for D9 (push button)

  lastPortBState = PINB;    // Save the initial state of PORTB pins

  sei();  // Enable global interrupts again
}

// Interrupt Service Routine for Pin Change Interrupt on PORTB
ISR(PCINT0_vect) {

  uint8_t currentState = PINB;  // Read current state of PORTB pins
  uint8_t changed = currentState ^ lastPortBState; // Detect which pins changed

  // Check if IR sensor pin (D8 / PB0) changed
  if (changed & (1 << PB0)) {

    // Detect falling edge (signal goes from HIGH to LOW)
    if (!(currentState & (1 << PB0))) {

      irFlag = true;  // Set IR obstacle detection flag

    }
  }

  // Check if push button pin (D9 / PB1) changed
  if (changed & (1 << PB1)) {

    // Detect falling edge when button is pressed
    if (!(currentState & (1 << PB1))) {

      buttonFlag = true; // Set button press flag

    }
  }

  lastPortBState = currentState;  // Update stored port state for next interrupt

}

// Function to configure Timer1 for periodic interrupts
void setupTimer1() {

  cli();  // Disable interrupts during timer configuration

  TCCR1A = 0;  // Clear Timer1 control register A
  TCCR1B = 0;  // Clear Timer1 control register B
  TCNT1  = 0;  // Reset Timer1 counter value

  OCR1A = 15624;  // Set compare match value for 1 second interval (16MHz clock with prescaler 1024)

  TCCR1B |= (1 << WGM12);  // Set Timer1 to CTC mode (Clear Timer on Compare Match)

  TCCR1B |= (1 << CS12) | (1 << CS10); // Set prescaler to 1024

  TIMSK1 |= (1 << OCIE1A);  // Enable Timer1 compare interrupt

  sei();  // Enable global interrupts again
}

// Timer1 Compare Match Interrupt Service Routine
ISR(TIMER1_COMPA_vect) {

  timerFlag = true; // Set timer event flag

}

void loop() {

  // Check if button interrupt occurred
  if (buttonFlag) {

    buttonFlag = false;  // Reset button flag

    Serial.println("Button Pressed - Pin Change Interrupt Triggered");

    // Toggle LED state
    digitalWrite(LED_EVENT, !digitalRead(LED_EVENT)); 

  }

  // Check if IR sensor detected obstacle
  if (irFlag) {

    irFlag = false;  // Reset IR flag

    Serial.println("IR Obstacle Detected - Pin Change Interrupt Triggered");

    digitalWrite(LED_EVENT, HIGH);  // Turn LED ON

  }

  // Check if PIR sensor detected motion
  if (pirFlag) {

    pirFlag = false;  // Reset PIR flag

    Serial.println("Motion Detected - External Interrupt Triggered");

    digitalWrite(LED_EVENT, HIGH);  // Turn LED ON

  }

  // Check if timer interrupt occurred
  if (timerFlag) {

    timerFlag = false;  // Reset timer flag

    Serial.println("Timer Event - Timer1 Compare Match Occurred");

  }

  // Read ultrasonic sensor every 1 second using millis()
  if (millis() - lastUltrasonicRead >= ultrasonicInterval) {

    lastUltrasonicRead = millis();  // Update last reading time

    digitalWrite(TRIG_PIN, LOW);    // Ensure trigger pin is LOW
    delayMicroseconds(2);           // Wait for 2 microseconds

    digitalWrite(TRIG_PIN, HIGH);   // Send 10 microsecond trigger pulse
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);    // Stop trigger pulse

    // Measure echo pulse duration
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); 

    // Convert duration into distance in centimeters
    float distance = duration * 0.034 / 2;          

    Serial.print("Ultrasonic Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // If object is closer than 20 cm
    if (distance > 0 && distance < 20) {

      digitalWrite(LED_EVENT, HIGH);  // Turn LED ON

    }
  }
}