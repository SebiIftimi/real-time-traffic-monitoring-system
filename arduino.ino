#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// The LCD
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Pins for car LEDs
const int LED_MAXSINA[] = {2, 3, 4, 5, 6};

// Pin for the LED that indicates the maximum number of cars has been reached
#define LED_NR_MAX  7; 

// Maximum number of cars
const int nr_max = 5;

// Panic button pin
#define PIN_BUTON_PANICA  8;

// Pin for the panic button LED
#define LED_BUTON_PANICA  9; 

// GAS SENSOR pin
#define PIN_SENZOR_GAZ  10;

// SMOKE SENSOR pin
#define PIN_SENZOR_FUM 11;

// States of the LEDs, entrance, exit,
// and panic button
bool StareLeduri[5] = {false};
bool BlocareIesire = false; // Exit block
bool BlocareIntrare = false; // Entrance block
bool Panica = false; // Panic mode

// Timers for LEDs
unsigned long ledTimers[5];

// Semaphores 
SemaphoreHandle_t sem_intrare, sem_iesire;
SemaphoreHandle_t sem_monitorizare;
SemaphoreHandle_t sem_turnOnLED;

// MUTEX
SemaphoreHandle_t mutex_panica;

// How long an LED stays on/
// How long a car stays in the tunnel
long interval = 3000;

// Function prototypes
void Monitorizare(void *pvParameters); // Monitoring
void Intrare_blocare(void *pvParameters); // Entry block
void Iesire(void *pvParameters); // Exit
void Buton_Panica(void *pvParameters); // Panic Button
bool turnOnLEDS();
int NrMasini(); // Number of Cars


void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  // Creating binary semaphores for different functionalities
  sem_intrare = xSemaphoreCreateBinary(); // Entrance semaphore
  sem_iesire = xSemaphoreCreateBinary(); // Exit semaphore
  sem_monitorizare = xSemaphoreCreateBinary(); // Monitoring semaphore
  sem_serial_monitor = xSemaphoreCreateBinary(); // Serial monitor semaphore
  sem_turnOnLED = xSemaphoreCreateBinary(); // LED control semaphore
  mutex_panica = xSemaphoreCreateMutex(); // Panic mutex

  // Set pin modes for LEDs
  for (int i = 0; i < nr_max; i++) {
    pinMode(LED_MAXSINA[i], OUTPUT);
  }
  pinMode(LED_NR_MAX, OUTPUT);
  pinMode(PIN_BUTON_PANICA, INPUT_PULLUP); 
  pinMode(LED_BUTON_PANICA, OUTPUT); 

  // Create tasks for different functionalities
  xTaskCreate(Monitorizare, "Monitorizare", 128, NULL, 1, NULL); // Monitoring task
  xTaskCreate(Intrare, "Intrare", 128, NULL, 1, NULL); // Entrance task
  xTaskCreate(Iesire, "Iesire", 128, NULL, 1, NULL); // Exit task
  xTaskCreate(Buton_Panica, "ButonPanica", 100, NULL, 2, NULL); // Panic button task
  xTaskCreate(Task_SenzorGaz, "SenzorGaz", 128, NULL, 1, NULL); // Gas sensor task
}

void loop() {}

void Intrare(void *pvParameters) {
  (void) pvParameters;
  while(1) {
    if (Serial.available() > 0) {
      char command = Serial.read();
      if (command == '1') {
        if (xSemaphoreTake(sem_intrare, (TickType_t)10) == pdTRUE) {
          if (Nrmasini() >= nr_max && Panica) {
            // The tunnel is full
            // Entry blocked
            Serial.println("Entry blocked");
            // Light up the LED for NR_MAX
            digitalWrite(LED_NR_MAX, HIGH);
          } else if (turnOnLEDS()) {
            Serial.print("A car entered the tunnel. Total cars: ");
            Serial.println(Nrmasini());
          }
          xSemaphoreGive(sem_intrare);

          // Signal turnOnLEDS
          xSemaphoreGive(sem_turnOnLED);
          // Signal the monitoring task
          xSemaphoreGive(sem_monitorizare);
        }
      }
      if (command == '2') {
        if (sem_intrareTake(sem_intrare, (TickType_t)10) == pdTRUE) {
          BlocareIesire = !BlocareIesire; // Toggle the block state
          digitalWrite(LED_NR_MAX, BlocareIesire ? HIGH : LOW);
          Serial.print("The system is ");
          Serial.println(BlocareIesire ? "blocking exit" : "unblocking exit");
          sem_intrareGive(sem_intrare);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
      }
      if (command == '3') {
        if (sem_intrareTake(sem_intrare, (TickType_t)10) == pdTRUE) {
          // Change the state of entrance blocking
          BlocareIntrare = !BlocareIntrare;
          // If blocking the entrance and the exit is already blocked,
          // unlock the exit
          if (BlocareIntrare && BlocareIesire) {
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Unblock the exit
            BlocareIesire = false;
            // Turn off the LED number 7
            digitalWrite(LED_NR_MAX, LOW);
            xSemaphoreGive(mutex_panica);
          }
          Serial.print("The entrance is ");
          Serial.println(BlocareIntrare ? "blocking exit" : "unblocking exit");
          sem_intrareGive(sem_intrare);
        }
      }
    }
  }
}

// Display the number of cars on the LCD
void Monitorizare(void *pvParameters) {
  (void) pvParameters;
  // Set cursor at the beginning of the first line
  lcd.setCursor(0, 0);
  lcd.print("Cars: ");

  while(1) {
    if (sem_intrareTake(sem_monitorizare, (TickType_t)10) == pdTRUE) {
      // Set the cursor at the beginning of the second line
      lcd.setCursor(0, 1);
      lcd.print("                "); // Clear the line
      // Reset cursor position
      lcd.setCursor(0, 1);
      lcd.print(Nrmasini()); // Print the number of cars
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

bool turnOnLEDS() {
    if (xSemaphoreTake(sem_monitorizare, portMAX_DELAY) == pdTRUE){
        // Check if the entry is blocked
        if (BlocareIesire || BlocareIntrare) {
            return false;
        }
        if (xSemaphoreTake(sem_turnOnLED, portMAX_DELAY) == pdTRUE){
            if (BlocareIesire || Panica) {
                // If the entry is blocked,
                // do not allow the LEDs to be turned on
                return false;
            } else {
                // Traverse the LEDs and
                // turn on the first LED that is in the "off" state
                for (int i = 0; i < nr_max; i++) {
                    if (!StareLeduri[i]) {
                        digitalWrite(LED_MAXSINA[i], HIGH);
                        ledTimers[i] = millis();
                        StareLeduri[i] = true;
                        return true;
                    }
                }
                return false;
            }
        }
    }
}

void Iesire(void *pvParameters) {
  (void) pvParameters;
  while(1) {
    if (xSemaphoreTake(sem_iesire, (TickType_t)10) == pdTRUE) {
       if (!BlocareIesire || Panica){
            for (int i = 0; i < nr_max; i++) {
                if (StareLeduri[i] && (millis() - ledTimers[i] >= interval)) {
                    digitalWrite(LED_MAXSINA[i], LOW);
                    StareLeduri[i] = false;
                    Serial.println("A car has exited the tunnel.");
                    Serial.println(Nrmasini());

                    if (Nrmasini() < nr_max) {
                        digitalWrite(LED_NR_MAX, LOW);
                    }
                }
            }
        }
        // Signal the exit
        xSemaphoreGive(sem_iesire);
        // Signal turnOnLEDS
        xSemaphoreGive(sem_turnOnLED);
        // Signal the monitoring task
        xSemaphoreGive(sem_monitorizare);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}


// Definition of the panic button task
void Buton_Panica(void *pvParameters) {
  (void) pvParameters;
  while(1) {
    if (digitalRead(PIN_BUTON_PANICA) == LOW) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      if (sem_intrareTake(sem_intrare, (TickType_t)10) == pdTRUE) {
        xSemaphoreTake(mutex_panica, portMAX_DELAY);
        // Toggle the panic mode
        Panica = !Panica;
        
        if (Panica) {
          // Activating panic mode
          BlocareIntrare = true;
          BlocareIesire = false;
          digitalWrite(LED_BUTON_PANICA, HIGH); 
          // Light up the panic LED
          Serial.println("Panic button pressed");
        } else {
          // Deactivating panic mode
          BlocareIntrare = false;
          digitalWrite(LED_BUTON_PANICA, LOW); 
          // Turn off the panic LED
          Serial.println("Entry and exit unblocked");
        }
        xSemaphoreGive(mutex_panica);
        sem_intrareGive(sem_intrare);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Task for the gas sensor
void Task_SenzorGaz(void *pvParameters) {
    while(1) {
        int sensorStatus = digitalRead(PIN_SENSOR_GAZ);

        if (sensorStatus == HIGH) {
            // Gas detected
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Activate panic mode
            Panica = true;
            xSemaphoreGive(mutex_panica);
            Serial.println("Gas detected! Tunnel entry blocked.");
        } else{
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Deactivate panic mode
            Panica = false;
            xSemaphoreGive(mutex_panica);
        }
         xSemaphoreGive(sem_monitorizare);
         xSemaphoreGive(sem_turnOnLED);

        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}

// Task for the smoke sensor
void Task_SenzorFum(void *pvParameters) {
    while(1) {
        int sensorStatus = digitalRead(PIN_SENSOR_FUM);

        if (sensorStatus == HIGH) {
            // Smoke detected
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Activate panic mode
            Panica = true;
            xSemaphoreTake(mutex_panica);
            Serial.println("Smoke detected! Tunnel entry blocked.");
        } else{
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Deactivate panic mode
            Panica = false;
            xSemaphoreGive(mutex_panica);
        }
        xSemaphoreGive(sem_monitorizare);
        xSemaphoreGive(sem_turnOnLED);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Function to count cars
int Nrmasini() {
    // Car counter
    int numCars = 0;
    for (int i = 0; i < nr_max; i++) {
        if (StareLeduri[i]) {
            numCars++;
        }
    }
    return numCars;
}
