#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial gpsSerial(0, 1); // RX, TX
TinyGPS gps;

// НАСТРОЙКИ
// Интервал отображения данных
const int updateDataInterval = 3000;
// Интервал, по прошествии которого фиксируется ошибка,
// Если данные с GPS-модуля не получены
const int noDataInverval = 5000;
// Скорость передачи данных GPS-модуля
const int gpsBaudrate = 9600;
// Вывод для кнопки изменения режима
const int changeStateButtonPin = 2;

// Флаг успешного получения данных от GPS-модуля
bool isGpsDataReady = false;
// Флаг ошибки получения данных от GPS-модуля
bool gpsDataFail = false;

// Флаг необходимости обновить данные на дисплее
bool updateDisplay = false;

// Данные с GPS-модуля кэшируются для того,
// чтобы можно было выполнять их получение и 
// отображение параллельно
float latitude, longitude;
int year;
byte month, day, hours, minutes, seconds, hundredths;
    
// Перечисление возможных режимов работы программы
// Инициализация (начальная загрузка), отображение координат, отображение времени
enum ProgramState {
  Initialization, Coordinates, Time
};

// Текущий режим работы программы
ProgramState programMode = Initialization;

unsigned long updateDataTimer;

unsigned long changeModeBtnPressedTimer;
bool wasChangeModeBtnPressed = false;

void setup()  
{
  // Инициализируем экран
  lcd.init();
  //включаем подсветку
  lcd.backlight();
  //Устанавливаем позицию, начиная с которой выводится текст.
  lcd.setCursor(0, 0);

  lcd.print("Please, wait...");

  // Инициализизруем выводы для работы c GPS-модуля
  gpsSerial.begin(gpsBaudrate);

  // Инициализируем цифровой вход для кнопки изменения режима
  pinMode(changeStateButtonPin, INPUT);
  
  updateDataTimer = millis();
  changeModeBtnPressedTimer = millis();
}

// Обновляет данные с GPS-модуля
void updateGpsData(TinyGPS &gps) {
  isGpsDataReady = false;
  
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    if (gps.encode(c)) {
      unsigned long age;
      
      gps.f_get_position(&latitude, &longitude, &age);
      gps.crack_datetime(&year, &month, &day, &hours, &minutes, &seconds, &hundredths, &age);
      
      isGpsDataReady = true;

      if (programMode == Initialization)
        programMode = Coordinates;
    }
  }
}

// Отображает координаты на LCD
void displayGpsData() {
  unsigned long age;
  
  lcd.clear();

  if (programMode == Coordinates) {
    
    
    lcd.print("N: ");
    lcd.print(latitude, 6);
    lcd.setCursor(0, 1);
    lcd.print("E: ");
    lcd.print(longitude, 6);
  }
  else if (programMode == Time) {
    lcd.print(year);
    lcd.print("/");
    lcd.print(month);
    lcd.print("/");
    lcd.print(day);

    lcd.setCursor(0, 1);

    lcd.print(hours);
    lcd.print(":");
    lcd.print(minutes);
    lcd.print(":");
    lcd.print(seconds);
  }
}

// Отображает сообщение об отсутствии данных на LCD
void displayNoData() {
    lcd.clear();
    lcd.print("No data");
    lcd.setCursor(0, 1);
    lcd.print("Check GPS module");
}

void loop()
{
  // Если была нажата кнопка, то обновляем режим работы программы
  int buttonState = digitalRead(changeStateButtonPin);
  bool isButtonPressed = (buttonState == HIGH);

  // Если кнопка была нажата, но сейчас отпущена
  if (wasChangeModeBtnPressed && !isButtonPressed) {
    // Делаем задержку, чтобы исключить дребезг контактов
    delay(10); 

    isButtonPressed = (digitalRead(changeStateButtonPin) == HIGH);
    
    // Если всё равно отпущена, то поймали событие клика
    if (!isButtonPressed) {
      // Переключаем режим работы программы
      if (programMode == Coordinates)
        programMode = Time;
      else if (programMode == Time)
        programMode = Coordinates;

      // Требуем обновления данных на дисплее
      updateDisplay = true;

      // Помечаем кнопку как отпущенную
      wasChangeModeBtnPressed = false;
    }
  }

  // Если кнопка нажата, то запоминаем факт нажатия
  if (!wasChangeModeBtnPressed && isButtonPressed)
    wasChangeModeBtnPressed = true;

  // Если данные с GPS-модуля помечены как устаревшие, то пытаемся обновить их
  if (millis() - updateDataTimer > updateDataInterval) {
    updateGpsData(gps);

    // Если не получается обновить данные долгое время, то поднимаем флаг ошибки
    if (!isGpsDataReady && millis() - updateDataTimer > noDataInverval) {
      if (gpsDataFail == false)
        updateDisplay = true;
        
      gpsDataFail = true;  
    }

    // Если получилось считать новые данные, обновляем таймер
    // и помечаем данные на дисплее устаревшими
    if (isGpsDataReady) {
      updateDisplay = true;
      
      updateDataTimer = millis();
      gpsDataFail = false;
    }
  }

  // Если требуется обновить данные на дисплее
  if (updateDisplay) {
    if (gpsDataFail) {
      // В случае ошибки получения данных отображаем сообщение об этом
      displayNoData();
      updateDisplay = false;
    } 
    else if (isGpsDataReady) {
      // Если данные готовы, показывыем их
      displayGpsData();
      updateDisplay = false;
    }
  }
}
