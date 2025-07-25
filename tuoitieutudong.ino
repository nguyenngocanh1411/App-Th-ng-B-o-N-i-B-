#define BLYNK_TEMPLATE_ID "TMPL6lwsU-fwv"        // ID template của Blynk
#define BLYNK_TEMPLATE_NAME "Tuoi tieu tu dong"   // Tên template của Blynk
#define BLYNK_AUTH_TOKEN "U7_fxrshTbdyzcwQqoUJ_WJAUxq2rGcy" // Token xác thực Blynk

#include <ESP8266WiFi.h>      // Thư viện kết nối WiFi cho ESP8266
#include <BlynkSimpleEsp8266.h> // Thư viện Blynk cho ESP8266
#include <DHT.h>              // Thư viện cho cảm biến DHT11
#include <LiquidCrystal_I2C.h> // Thư viện cho LCD I2C

// Thông tin WiFi
char ssid[] = "HoaiThuong";   // Tên WiFi
char pass[] = "vit11052007";  // Mật khẩu WiFi

// Định nghĩa chân kết nối
#define DHTPIN D4          // Chân kết nối DHT11 (GPIO2)
#define DHTTYPE DHT11      // Loại cảm biến DHT11
#define RAIN_ANALOG A0     // Chân A0 cho cảm biến mưa (giá trị analog)
#define RAIN_DIGITAL D5    // Chân D5 cho cảm biến mưa (GPIO14, 0 = mưa, 1 = không mưa)
#define SOIL_ANALOG A0     // Chân A0 cho cảm biến độ ẩm đất (giá trị analog, dùng chung nếu có)
#define SOIL_DIGITAL D6    // Chân D6 cho cảm biến độ ẩm đất (GPIO12, 0 = ướt, 1 = khô)
#define RELAY_PIN D7       // Chân D7 cho relay (GPIO13, Low Level Trigger)
#define BUZZER_PIN D8      // Chân D8 cho buzzer (GPIO15)
#define BUTTON_MODE D0     // Chân D0 cho nút chế độ (GPIO16)
#define BUTTON_FUNCTION D3 // Chân D3 cho nút chức năng (GPIO0)

// Khởi tạo đối tượng
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Khởi tạo LCD I2C (thay 0x27 bằng địa chỉ thực tế)

// Biến trạng thái
int mode = 0;         // 0 = Tự động, 1 = Thủ công
int manualPump = -1;  // -1 = Không thay đổi, 0 = Tắt, 1 = Bật
int alarmActive = 0;  // Cờ cảnh báo (0 = Tắt, 1 = Bật)
int lastModeState = HIGH;   // Lưu trạng thái trước của nút chế độ
int lastFuncState = HIGH;   // Lưu trạng thái trước của nút chức năng

// Hàm xử lý nút chế độ qua Blynk (V5)
BLYNK_WRITE(V5) {     // Nút chuyển chế độ (Tự động/Thủ công) trên Blynk
  int newMode = param.asInt(); // Đọc giá trị từ Blynk (0 = Tự động, 1 = Thủ công)
  if (newMode != mode) {      // Chỉ cập nhật nếu có thay đổi
    mode = newMode;
    if (mode == 1) {
      lcd.setCursor(15, 1); lcd.print("M"); // Hiển thị "M" cho Thủ công
    } else {
      lcd.setCursor(15, 1); lcd.print("A"); // Hiển thị "A" cho Tự động
    }
  }
}

// Hàm xử lý nút chức năng qua Blynk (V6)
BLYNK_WRITE(V6) {     // Nút điều khiển chức năng (Bật/Tắt máy bơm) trên Blynk
  manualPump = param.asInt(); // Đọc giá trị từ Blynk (0 = Tắt, 1 = Bật)
  if (manualPump == 1) {
    digitalWrite(RELAY_PIN, LOW);   // Bật relay
  } else {
    digitalWrite(RELAY_PIN, HIGH);  // Tắt relay
  }
  alarmActive = 0;                  // Ngưng cảnh báo khi nhấn nút
}

void setup() {
  Serial.begin(115200);                 // Khởi động Serial để debug
  Serial.println("System Start - " + String(__DATE__) + " " + String(__TIME__));

  dht.begin();                          // Khởi động cảm biến DHT11
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); // Kết nối WiFi và Blynk

  pinMode(RAIN_DIGITAL, INPUT);         // Cấu hình chân cảm biến mưa
  pinMode(SOIL_DIGITAL, INPUT);         // Cấu hình chân cảm biến đất
  pinMode(RELAY_PIN, OUTPUT);           // Cấu hình chân relay
  digitalWrite(RELAY_PIN, HIGH);        // Tắt relay lúc khởi động
  pinMode(BUZZER_PIN, OUTPUT);          // Cấu hình chân buzzer
  digitalWrite(BUZZER_PIN, LOW);        // Tắt buzzer lúc khởi động
  pinMode(BUTTON_MODE, INPUT);          // Cấu hình chân nút chế độ
  pinMode(BUTTON_FUNCTION, INPUT);      // Cấu hình chân nút chức năng

  lcd.init();                           // Khởi động LCD
  lcd.backlight();                      // Bật đèn nền LCD
  lcd.setCursor(0, 0);                  // Đặt con trỏ dòng 1
  lcd.print("System Init");             // Hiển thị thông báo khởi động
  delay(2000);                          // Đợi 2 giây
  lcd.clear();                          // Xóa màn hình
}

void loop() {
  Blynk.run();                          // Chạy Blynk

  delay(2000);                          // Đợi 2 giây (thích hợp cho DHT11)

  // Đọc dữ liệu từ DHT11
  float humidity = dht.readHumidity();   // Đọc độ ẩm không khí (%)
  float temperature = dht.readTemperature(); // Đọc nhiệt độ không khí (°C)

  // Đọc dữ liệu từ cảm biến mưa
  int rainAnalog = analogRead(RAIN_ANALOG); // Giá trị analog từ 0-1023
  int rainDigital = digitalRead(RAIN_DIGITAL); // 0 = Có mưa, 1 = Không mưa
  int rainPercent = map(rainAnalog, 1023, 0, 0, 100); // Chuyển thành %

  // Đọc dữ liệu từ cảm biến độ ẩm đất
  int soilAnalog = analogRead(RAIN_ANALOG); // Dùng chung A0, điều chỉnh nếu có chân riêng
  int soilDigital = digitalRead(SOIL_DIGITAL); // 0 = Ướt, 1 = Khô
  int soilPercent = map(soilAnalog, 1023, 0, 0, 100); // Chuyển thành %

  // Đọc trạng thái nút bấm (debouncing cơ bản)
  int modeState = digitalRead(BUTTON_MODE);
  int funcState = digitalRead(BUTTON_FUNCTION);
  if (modeState == LOW && lastModeState == HIGH) {
    mode = 1 - mode;                    // Chuyển đổi chế độ
    if (mode == 1) {
      lcd.setCursor(15, 1); lcd.print("M"); // Hiển thị "M" cho Thủ công
    } else {
      lcd.setCursor(15, 1); lcd.print("A"); // Hiển thị "A" cho Tự động
    }
    delay(50);                          // Debouncing
  }
  if (funcState == LOW && lastFuncState == HIGH) {
    manualPump = 1 - manualPump;        // Chuyển đổi bật/tắt
    if (manualPump == 1) digitalWrite(RELAY_PIN, LOW);
    else digitalWrite(RELAY_PIN, HIGH);
    alarmActive = 0;                    // Ngưng cảnh báo
    delay(50);                          // Debouncing
  }
  lastModeState = modeState;
  lastFuncState = funcState;

  // Điều khiển relay
  int pumpState = HIGH;                 // Mặc định tắt relay
  if (mode == 0) {                      // Chế độ Tự động
    if (soilPercent > 50 && rainPercent > 50) { // Đất ướt và có mưa
      pumpState = HIGH;                 // Tắt relay (hiển thị OFF)
    } else if (soilPercent <= 50 && rainPercent <= 50) { // Đất khô và không mưa
      pumpState = LOW;                  // Bật relay (hiển thị ON)
    }
  } else if (manualPump != -1) {        // Chế độ Thủ công
    pumpState = (manualPump == 1) ? LOW : HIGH; // Theo nút chức năng
  }
  digitalWrite(RELAY_PIN, pumpState);   // Điều khiển relay

  // Gửi dữ liệu lên Blynk
  Blynk.virtualWrite(V0, temperature);  // Gửi nhiệt độ
  Blynk.virtualWrite(V1, humidity);     // Gửi độ ẩm
  Blynk.virtualWrite(V2, rainPercent);  // Gửi mức mưa
  Blynk.virtualWrite(V3, soilPercent);  // Gửi độ ẩm đất
  Blynk.virtualWrite(V4, (pumpState == LOW) ? 1 : 0); // Gửi trạng thái relay

  // Kiểm tra lỗi DHT11
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT11 Error");
    lcd.setCursor(0, 0);
    lcd.print("DHT11 Error  ");
  } else {
    // Hiển thị lên Serial
    Serial.print("Nhiet do: "); Serial.print(temperature); Serial.print(" *C\t");
    Serial.print("Do am: "); Serial.print(humidity); Serial.println(" %");

    // Hiển thị lên LCD (dòng 1): Nhiệt độ và độ ẩm
    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(temperature, 1); lcd.print("C H:"); lcd.print(humidity, 1); lcd.print("%        ");

    // Hiển thị lên LCD (dòng 2): Mưa, Đất, Relay, Chế độ
    lcd.setCursor(0, 1);
    lcd.print("R:"); lcd.print(rainPercent); lcd.print("% ");
    lcd.print(rainPercent > 50 ? "CM" : " KM"); // >50% Có mưa, <=50% Trời không mưa
    lcd.print(" S:"); lcd.print(soilPercent); lcd.print("% ");
    lcd.print(soilPercent > 50 ? "DK" : "DU"); // >50% Đất khô, <=50% Đất ướt
    lcd.print(" R:"); lcd.print(pumpState == LOW ? "ON" : "OFF"); // ON = Bật, OFF = Tắt
    lcd.print(" "); lcd.print(mode == 0 ? "A" : "M"); // A = Tự động, M = Thủ công
  }

  // Kiểm tra và kích hoạt cảnh báo
  if (!alarmActive) {
    if (soilDigital == 1 && rainDigital == 1 && temperature > 30 && pumpState == HIGH) {
      // Cảnh báo: Đất khô, không mưa, nhiệt độ cao (>30°C) mà không bơm
      tone(BUZZER_PIN, 1000); // Bật buzzer với tần số 1000Hz
      alarmActive = 1;
    } else if (rainDigital == 0 && soilPercent > 50 && pumpState == LOW) {
      // Cảnh báo: Có mưa, đất ẩm (>50%) mà máy bơm vẫn chạy
      tone(BUZZER_PIN, 1000);
      alarmActive = 1;
    }
  }

  // In dữ liệu lên Serial
  Serial.print("Mua: "); Serial.print(rainPercent); Serial.print("% "); Serial.print(rainPercent > 50 ? "M" : "KM");
  Serial.print("\tDat: "); Serial.print(soilPercent); Serial.print("% "); Serial.print(soilPercent > 50 ? "DK" : "DU");
  Serial.print("\tRelay: "); Serial.println(pumpState == LOW ? "ON" : "OFF");

  Serial.println("-------------------");
}