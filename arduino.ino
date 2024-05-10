#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD-ul
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Pini pentru leduri masina
const int LED_MAXSINA[] = {2, 3, 4, 5, 6};

// Pin pentru LED-UL care indica ca a fost atins
// nr maxim de masini
#define LED_NR_MAX  7; 

// NR_MAX_MASINI
const int nr_max = 5;

//Pin buton panica
#define PIN_BUTON_PANICA  8;

//Pin pentru ledul butonului de panica
#define LED_BUTON_PANICA  9; 

//Pin SENZOR GAZ
#define PIN_SENZOR_GAZ  10;

//Pin SENZOR GAZ
#define PIN_SENZOR_FUM 11;

// Starile led-urilor, intrarii, iesirii,
//butonul de panica
bool StareLeduri[5] = {false};
bool BlocareIesire = false;
bool BlocareIntrare = false;
bool Panica = false;

//Timere pentru led-uri
unsigned long ledTimers[5];

//Semafoare 
SemaphoreHandle_t sem_intrare, sem_iesire;
SemaphoreHandle_t sem_monitorizare;
SemaphoreHandle_t sem_turnOnLED;

// MUTEX
SemaphoreHandle_t mutex_panica;

// Cat timp ramane un led aprins/
// Cat timp sta masina in tunel
long interval = 3000;

// Prototipuri functii
void Monitorizare(void *pvParameters);
void Intrare_blocare(void *pvParameters);
void Iesire(void *pvParameters);
void Buton_Panica(void *pvParameters);
bool turnOnLEDS();
int NrMasini();

void setup() {
  //Initializare comunicatia seriala
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();


  sem_intrare = xSemaphoreCreateBinary();
  sem_iesire = xSemaphoreCreateBinary();
  sem_monitorizare = xSemaphoreCreateBinary();
  sem_serial_monitor = xSemaphoreCreateBinary();
  sem_turnOnLED = xSemaphoreCreateBinary();
  mutex_panica = xSemaphoreCreateMutex();


  for (int i = 0; i < nr_max; i++) {
    pinMode(LED_MAXSINA[i], OUTPUT);
  }
  pinMode(LED_NR_MAX, OUTPUT); 
  pinMode(PIN_BUTON_PANICA, INPUT_PULLUP); 
  pinMode(LED_BUTON_PANICA, OUTPUT); 

  xTaskCreate(Monitorizare, "Monitorizare", 128, NULL, 1, NULL);
  xTaskCreate(Intrare, "Intrare", 128, NULL, 1, NULL);
  xTaskCreate(Iesire, "Iesire", 128, NULL, 1, NULL);
  xTaskCreate(Buton_Panica, "ButonPanica", 100, NULL, 2, NULL);
  xTaskCreate(Task_SenzorGaz, "SenzorGaz", 128, NULL, 1, NULL);
}

void loop() {}

void Intrare(void *pvParameters) {
  (void) pvParameters;
  while(1) {
    if (Serial.available() > 0 ) {
      char command = Serial.read();
      if (command == '1') {
        if (xSemaphoreTake(sem_intrare, (TickType_t)10) == pdTRUE) {
          if (Nrmasini() >= nr_max && Panica) {
            // Tunelu e plin
            // Intrare blocata
            Serial.println("Intrare blocata");
            // Aprinde LED-ul pentru NR_MAX
            digitalWrite(LED_NR_MAX, HIGH);
          } else if (turnOnLEDS()) {
            Serial.print("A intrat o masina in tunel. Total masini: ");
            Serial.println(Nrmasini());
          }
          xSemaphoreGive(sem_intrare);

          // Semnalizează turnOnLEDS
          xSemaphoreGive(sem_turnOnLED);
          // Semnalizează task-ul de monitorizare
          xSemaphoreGive(sem_monitorizare);
}
        }
      }
      if (command == '2') {
        if (sem_intrareTake(sem_intrare, (TickType_t)10) == pdTRUE) {
          BlocareIesire = !BlocareIesire; // Comuta la  starea de blocare
          digitalWrite(LED_NR_MAX, BlocareIesire ? HIGH : LOW);
          Serial.print("Sistemul este ");
          Serial.println(BlocareIesire ? "BlocareIesire" : "deBlocareIesire");
          sem_intrareGive(sem_intrare);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
  if (command == '3') {
        if (sem_intrareTake(sem_intrare, (TickType_t)10) == pdTRUE) {
            // Modifica starea de blocare a intrării
            BlocareIntrare = !BlocareIntrare;
            // Dacă blocăm intrarea și ieșirea este deja blocata,
            // deblochează ieșirea
            if (BlocareIntrare && BlocareIesire) {
                xSemaphoreTake(mutex_panica, portMAX_DELAY);
                // Deblochează ieșirea
                BlocareIesire = false;
                // Stinge LED-ul nr 7
                digitalWrite(LED_NR_MAX, LOW);
                xSemaphoreGive(mutex_panica);
            }
          Serial.print("Intrarea este ");
          Serial.println(BlocareIntrare ? "BlocareIesirea" : "deBlocareIesirea");
          sem_intrareGive(sem_intrare);
        }
      }
  }
}



//Afiseaza nr de masini pe LCD
void Monitorizare(void *pvParameters) {
  (void) pvParameters;
  //Setare cursor la inceputul primei linii
  lcd.setCursor(0, 0); 
  lcd.print("Masini: ");

  while(1) {
    if (sem_intrareTake(sem_monitorizare, (TickType_t)10) == pdTRUE) {
        // Setează cursorul pe linia a doua, la început
      lcd.setCursor(0, 1);
      lcd.print("                ");
      // Resetare poziție cursorului
      lcd.setCursor(0, 1);
      lcd.print(Nrmasini());
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}



bool turnOnLEDS() {
    if (xSemaphoreTake(sem_monitorizare, portMAX_DELAY) == pdTRUE){
        // Verifică dacă intrarea este Blocata
        if (BlocareIesire || BlocareIntrare) {
            return false;
        }
    if (xSemaphoreTake(sem_turnOnLED, portMAX_DELAY) == pdTRUE){
    if (BlocareIesire || Panica) {
        // Dacă intrarea este blocata,
        // nu permite aprinderea LED-urilor
        return false;
    } else {
        // Parcurge LED-urile și 
        //aprinde primul LED care este în starea "off"
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
          Serial.println("A iesit o masina din tunel.");
          Serial.println(Nrmasini());

            if (Nrmasini() < nr_max) {
              digitalWrite(LED_NR_MAX, LOW);
            }
        }
        }
       }
       // Semnalizează iesirea
      xSemaphoreGive(sem_iesire);
      // Semnalizează turnOnLEDS
      xSemaphoreGive(sem_turnOnLED);
      // Semnalizează task-ul de monitorizare
      xSemaphoreGive(sem_monitorizare);

    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Definiția task-ului buton de panică
void Buton_Panica(void *pvParameters) {
  (void) pvParameters;
  while(1) {
    if (digitalRead(PIN_BUTON_PANICA) == LOW) {
      vTaskDelay(50 / portTICK_PERIOD_MS);
      if (sem_intrareTake(sem_intrare, (TickType_t)10) == pdTRUE) {
        xSemaphoreTake(mutex_panica, portMAX_DELAY);
        // Modifica modul de panică
        Panica = !Panica;
        
        if (Panica) {
          // Acctivarea modului de panică
          BlocareIntrare = true;
          BlocareIesire = false;
          digitalWrite(LED_BUTON_PANICA, HIGH); 
          // Aprinde LED-ul de panică
          Serial.println("Butonul de panica a fost apasat");
        } else {
          // Dezactivarea modului de panică
          BlocareIntrare = false;
          digitalWrite(LED_BUTON_PANICA, LOW); 
          // Stinge LED-ul de panică
          Serial.println("Intrare si iesire deBlocareIesiree");
        }
        xSemaphoreGive(mutex_panica);
        sem_intrareGive(sem_intrare);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}  

void Task_SenzorGaz(void *pvParameters) {
    while(1) {
        int stareSenzor = digitalRead(PIN_SENSOR_GAZ);

        if (stareSenzor == HIGH) {
            // Detectare gaz
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Activarea modului de panică
            Panica = true;
            xSemaphoreGive(mutex_panica);
            Serial.println("Gaz detectat! Intrarea in tunel blocata.");
        } else{
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Dezactiveaza modul de panica
            Panica = false;
            xSemaphoreGive(mutex_panica);
        }
         xSemaphoreGive(sem_monitorizare);
         xSemaphoreGive(sem_turnOnLED);

        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}

void Task_SenzorFum(void *pvParameters) {
    while(1) {
        int stareSenzor = digitalRead(PIN_SENSOR_FUM);

        if (stareSenzor == HIGH) {
            // Detectare fum
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Activarea modului de panică
            Panica = true;
            xSemaphoreTake(mutex_panica);
            Serial.println("Fum detectat! Intrarea in tunel blocata.");
        } else{
            xSemaphoreTake(mutex_panica, portMAX_DELAY);
            // Dezactiveaza modul de panica
            Panica = false; 
            xSemaphoreGive(mutex_panica);
        }
        xSemaphoreGive(sem_monitorizare);
        xSemaphoreGive(sem_turnOnLED);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


int Nrmasini() {
    // Contor masini
    int numCars = 0;
  for (int i = 0; i < nr_max; i++) {
    if (StareLeduri[i]) {
      numCars++;
    }
  }
  return numCars;
}