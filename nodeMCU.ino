#define BLYNK_PRINT Serial 
// Khai báo ID 
#define BLYNK_TEMPLATE_ID "TMPL-a75PCCo"
#define BLYNK_DEVICE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "HbkDrAvBEOHfdFhRziycOgy_VoHjJOpn"

#include <ESP8266WiFi.h>    
#include <BlynkSimpleEsp8266.h>
#include <Ticker.h>
Ticker myTic;
char auth[] = BLYNK_AUTH_TOKEN;           // dia chi auuth cua Blynk
const char* ssid      = "VIETTEL";        // ten cua wifi
const char* pass      = "sonnguyen";      // mat khau cua wifi

//define cac chan của ESP vd chan D0 là GPIO16
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;

// define cac kenh va chan phan cung
#define Ts 0.1 
#define res 495
#define IN1 D1 
#define IN2 D2 
#define ENCA D5 
#define ENCB D6 
#define ENA D3

// Khai bao cac bien 
bool START=0,Mode=0;
// Gia tri cuoi cung sau khi tool PID
float SPEED_KP = 1.74; 		//Kgh=3.5
float SPEED_KI = 0.52; 		//~0.5
float SPEED_KD = 0.12;

float POSITION_KP = 0.382; 	//Kgh~0.8
float POSITION_KI = 0.11; 	//0.1 
float POSITION_KD = 0.023;	//0.02


int encoderPos = 0; 
int _speed, _speed1 ,_position, _position1; 
int enable; 
int cur_mode, pre_mode; 
float error, pre_error=0, error_sum, d_error; 
float control_signal; 
int flag_speed =1, flag_position=0; 
int setpoint; 
int reset_position_signal ,reset_speed_signal; 
int cnt = 0;
float my_duty_cycle;

BLYNK_WRITE(V0) // Nhân setpoint (Vận tốc và vị trí mong muốn) từ điện thoại
{
    setpoint = param.asInt(); 
    
}
BLYNK_WRITE(V2) // Nhận tín hiệu khởi động từ blynk
{ 
    START = param.asInt(); 
    
}
void Clear_all(); 
BLYNK_WRITE(V3) //Nhan tin hieu chon Mode (1: chay mode Speed 0: chay mode  Position)
{
  if(START) {
    Mode = param.asInt();
    if(Mode){
   flag_speed=1;
   flag_position=0;
   Serial.println("Speed mode ");
  }
 else {
   flag_speed=0;
   flag_position=1;
   Serial.println("Position mode ");
 }
 delay(100); 
 Clear_all(); 
  }
  
}
float myPID(float KP,float KI,float KD,float current,int setpoint) ;
void Speed_motor();
void Position_motor();
void send_Serial();
// Hàm setup khởi tạo ban đầu cho chương trình.
void setup() {
  Serial.begin(115200); 
  while (!Serial) {
    Serial.println(" COM is connected !! ");  //chờ cho cổng COM được kết nối
  }        
  Blynk.begin(auth, ssid, pass);              // kết nối blynk điện thoại thông qua wifi
  pinMode(IN1, OUTPUT);     
  pinMode(IN2, OUTPUT);       
  pinMode(ENA, OUTPUT);
  pinMode(ENCA, INPUT);
  pinMode(ENCB, INPUT); 
  attachInterrupt(14, ISR_encoder, RISING  ); // ngắt chân ENCA để update encoder position 
  myTic.attach(Ts, handle_interrupt);
 
}
// Hàm ngắt đọc encoder
ICACHE_RAM_ATTR void ISR_encoder() { 
 int b=digitalRead(ENCB);
 if (b>0) encoderPos++;  // cạnh lên 
 else encoderPos--; 
}
// Hàm ngắt điều khiển theo tín hiệu từ blynk gửi về
void handle_interrupt() {  
  if(START){
  if (flag_speed ) { 
   Speed_motor();
  } 
  else if(flag_position ) { 
    Position_motor();
  }
  }
  else{
    Serial.println("Waiting for Start");
    Clear_all();

  } 
}
// hàm main cho blynk chạy và kiểm tra serial port nhận dữ liệu gửi đến
void loop() {
  Blynk.run();
  if(Serial.available()){
    String teststr=Serial.readString();
    setpoint=teststr.toFloat();
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

 }
// Hàm dieu khiển tốc độ động cơ
void Speed_motor()
{
    if(setpoint>130) setpoint=130; // tốc độ tối đa của động cơ 130 vòng trên phút
    
    my_duty_cycle = myPID(SPEED_KP,SPEED_KI,SPEED_KD,_speed,setpoint);
    control_signal = my_duty_cycle*255/100.0;
    if (setpoint>0){
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    } 
    else if (setpoint==0){
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    }
    else {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    }
    if(control_signal<0) control_signal=-control_signal;
    analogWrite(ENA, control_signal);
    
 
  _speed = (float)(encoderPos/(float)Ts/(float)res*60); 
  encoderPos = 0;// reset coun
  send_Serial();
  Blynk.virtualWrite(V1,_speed1); 
}
// hàm thực hiện chức năng
void Position_motor(){
    my_duty_cycle = myPID(POSITION_KP,POSITION_KI,POSITION_KD,_position,setpoint);
    control_signal = my_duty_cycle*255/100; 
    if (my_duty_cycle >= 0) { 
      digitalWrite(IN1, HIGH); 
      digitalWrite(IN2, LOW);
      analogWrite(ENA, control_signal);
    } 
    else {
      digitalWrite(IN1, LOW); 
      digitalWrite(IN2, HIGH);
      analogWrite(ENA, -control_signal);
    }

  _position = (float)(encoderPos)*360/(float)res; 
  send_Serial();
  Blynk.virtualWrite(V1,_position1); 


}
// Hàm gửi lên serial port 
void send_Serial(){
  Serial.print(" Mode ");
  if (Mode) Serial.print(" Speed ");
  else Serial.print(" Position");
  Serial.print(" Setpoint: "); 
  Serial.print(setpoint); 
  if(Mode) {
    Serial.print(" (rpm) "); 
  Serial.print(" Speed: "); 
  Serial.print(_speed);
  Serial.print(" (rpm) "); 
    Serial.print(" Kp ");
  Serial.print(SPEED_KP);
  Serial.print(" Ki ");
  Serial.print(SPEED_KI);
  Serial.print(" Kd ");
  Serial.print(SPEED_KD);
  }
  else {
  Serial.print(" (degree) "); 
  Serial.print(" Position: "); 
  Serial.print(_position); 
  Serial.print(" (degree) ");
   Serial.print(" Kp ");
  Serial.print(POSITION_KP);
  Serial.print(" Ki ");
  Serial.print(POSITION_KI);
  Serial.print(" Kd ");
  Serial.print(POSITION_KD);
  } 
  Serial.print(" Duty cycle ");
  Serial.print(my_duty_cycle);
  Serial.print(" Xung");
  Serial.println(control_signal);
 
}
// Hàm clear tất cả dữ liệu đưa về trạng thái ban đầu
void Clear_all(){
  _position=0;
  _speed=0;
  encoderPos=0;
  error_sum =0; 
   setpoint = 0; 
   digitalWrite(IN1, LOW); 
   digitalWrite(IN2, LOW); 
   Blynk.virtualWrite(V1,0); 
}
// Hàm PID
float myPID(float KP,float KI,float KD,float current,int setpoint) 
{ 
  error = setpoint - current; 
  error_sum += error; 
  d_error = (error - pre_error); 
  float duty_cycle;  
  duty_cycle = KP*error + KI*Ts*error_sum + KD*d_error/Ts ; 
  pre_error = error ; 
  if (duty_cycle > 100) 
  duty_cycle = 99; 
  else if(duty_cycle<-100) 
  duty_cycle = -99; 
  return(duty_cycle); 
}
  