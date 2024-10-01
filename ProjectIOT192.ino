/* Đấu nối:
 + RFID:
  -D13: nối chân SCK của RFID      
  -D12: nối chân MISO của RFID
  -D11: nối chân MOSI của RFID      
  -D10: nối chân SS(DSA)của RFID  
  -D9: nối chân RST của RFID 
 + LCD:
  -D3: nối chân RS của LCD  
  -D4: nối chân ENABLE của LCD
  -D5: nối chân D4 của LCD     
  -D6: nối chân D5 của LCD 
  -D7: nối chân D6 của LCD 
  -D8: nối chân D7 của LCD   
 + Loa BUZZER:
  -A0: nối chân Anot của loa   
 + Động cơ Servo:
  -D2: nối chân tín hiệu của động cơ Servo
 */

//Khai báo thư viện

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <LiquidCrystal.h>

//Khai báo các chân kết nối của Arduino với các linh kiện

#define SS_PIN 10
#define RST_PIN 9
#define SERVO_PIN 2
#define BUZZER_PIN A0
#define LCD_RS 3
#define LCD_ENABLE 4
#define LCD_D4 5
#define LCD_D5 6
#define LCD_D6 7
#define LCD_D7 8

//Khởi tạo các đối tượng (instances) cho các module

MFRC522 mfrc522(SS_PIN, RST_PIN);                                       // Tạo đối tượng cho đọc thẻ RFID
Servo servo;                                                            // Tạo đối tượng cho Servo
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);  // Tạo đối tượng cho màn hình LCD

int availableSlots = 3;                                                 // Số lượng chỗ đỗ trống ban đầu
String parkedRFIDs[3];                                                  // Mảng để lưu trữ thông tin về các thẻ RFID đã đỗ

// Mảng chứa các mã thẻ RFID hợp lệ
String validRFIDs[] = {"53412914", "12345678", "87654321"};             // Thay đổi các mã thẻ tùy ý

// Thêm biến để theo dõi thời gian còi kêu
unsigned long buzzerStartTime = 0;
const unsigned long buzzerDuration = 5000;                              // Thời gian kêu còi: 5 giây

void setup() 
{
  Serial.begin(9600);
  SPI.begin();                                                          // Khởi tạo bus SPI
  mfrc522.PCD_Init();                                                   // Khởi tạo đọc thẻ RFID

  servo.attach(SERVO_PIN);
  servo.write(0);                                                       // Đóng Servo ban đầu
  
  pinMode(BUZZER_PIN, OUTPUT);
  
  lcd.begin(16, 2);                                                     // Khởi tạo màn hình LCD 16x2
  lcd.print("Smart Parking");
  Serial.println("Smart Parking");                                      // In ra Serial Monitor
  lcd.setCursor(0, 1);
  lcd.print("Available: " + String(availableSlots));
  Serial.println("Available: " + String(availableSlots));               // In ra Serial Monitor
}

void loop() 
{
  // Kiểm tra nếu có thẻ RFID mới được đưa vào
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
  {
    String rfid = readRFID();  // Đọc mã RFID từ thẻ

    if (isValidRFID(rfid))      // Kiểm tra xem mã thẻ có hợp lệ không
    {
      lcd.clear();
      lcd.print("RFID: " + rfid);
      Serial.println("RFID: " + rfid); // In ra Serial Monitor

      bool isParkedCar = false;
      for (int i = 0; i < 3; i++) 
      {
        if (rfid == parkedRFIDs[i]) 
        {
          isParkedCar = true;
          break;
        }
      }

      if (isParkedCar) 
      {
        servo.write(90); // Open the servo
        lcd.setCursor(1, 1);
        lcd.print("See You Again!");
        Serial.println("See You Again!"); // In ra Serial Monitor
        buzzBuzzer(1000);  
        delay(2000);
        lcd.clear();
        lcd.print("Available: " + String(++availableSlots));
        Serial.println("Available: " + String(availableSlots)); // In ra Serial Monitor
        delay(1000);

        servo.write(0); // Close the servo
        removeParkedRFID(rfid);
      } 
      else if (availableSlots > 0) 
      {
        servo.write(90); // Open the servo
        lcd.setCursor(4, 1);
        lcd.print("Welcome!");
        Serial.println("Welcome!"); // In ra Serial Monitor
        buzzBuzzer(1000);  
        delay(2000);
        lcd.clear();
        lcd.print("Available: " + String(--availableSlots));
        Serial.println("Available: " + String(availableSlots)); // In ra Serial Monitor
        delay(1000);

        servo.write(0); 
        addParkedRFID(rfid);
      } 
      else 
      {
        lcd.setCursor(3, 1);
        lcd.print("Full Slot");
        Serial.println("Full Slot"); // In ra Serial Monitor
        buzzBuzzer(5000);  
        delay(2000);
      }

      lcd.clear();
      lcd.print("Smart Parking");
      Serial.println("Smart Parking"); // In ra Serial Monitor
      lcd.setCursor(0, 1);
      lcd.print("Available: " + String(availableSlots));
      Serial.println("Available: " + String(availableSlots)); // In ra Serial Monitor
    }
    else 
    {
      lcd.clear();
      lcd.print("Invalid RFID!");
      Serial.println("Invalid RFID!"); // In ra Serial Monitor
      buzzBuzzer(2000);  
      delay(2000);
    }

    if (isBuzzerOn()) 
    {
      if (millis() - buzzerStartTime >= buzzerDuration) 
      {
        stopBuzzer();
      }
    }
    mfrc522.PICC_HaltA();                                             // Dừng truyền thẻ RFID
  }
}

String readRFID() 
{
  String rfid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
    rfid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    rfid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  mfrc522.PICC_HaltA();
  return rfid;
}

bool isValidRFID(String rfid)   // Hàm kiểm tra xem mã thẻ có hợp lệ không
{
  for (int i = 0; i < sizeof(validRFIDs) / sizeof(validRFIDs[0]); i++) 
  {
    if (rfid == validRFIDs[i]) 
    {
      return true;
    }
  }
  return false;
}

void addParkedRFID(String rfid)                                       
{
  for (int i = 0; i < 3; i++) 
  {
    if (parkedRFIDs[i] == "") 
    {
      parkedRFIDs[i] = rfid;
      break;
    }
  }
}

void removeParkedRFID(String rfid)                                     
{
  for (int i = 0; i < 3; i++) 
  {
    if (parkedRFIDs[i] == rfid) 
    {
      parkedRFIDs[i] = "";
      break;
    }
  }
}

void buzzBuzzer(unsigned int duration)                                 
{
  digitalWrite(BUZZER_PIN, HIGH);                                      
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void startBuzzer()                                                     
{
  digitalWrite(BUZZER_PIN, HIGH);                                       
  buzzerStartTime = millis();                                           
}

void stopBuzzer()                                                       
{
  digitalWrite(BUZZER_PIN, LOW);                                        
  buzzerStartTime = 0;                                                  
}

bool isBuzzerOn()                                                       
{
  return buzzerStartTime > 0;                                           
}
