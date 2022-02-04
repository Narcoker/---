
// LCD display를 사용하기 위한 라이브러리 정의
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);

// 각 모듈에 따른 핀 번호 정의
#define DUST_PIN A0
#define IRED_PIN 12
#define SW_POWER 4
#define SW_MOTOR 5
#define BUZZER 6
#define MOTOR 13
#define LED 8
#define RGB_R 3
#define RGB_G A1
#define RGB_B A2

// 미세먼지 센서를 위한 define
#define DUST_DENSITY_RATIO  0.167  
#define VOC 400
#define VCC 5000

#define SAMPLING_TIME 280
#define WAITING_TIME  40
#define STOP_TIME 9680    // 센서 미 구동 시간  

#define FILTER_MAX  3

float arrFilter[FILTER_MAX] = {0,};

// 각종 상태를 나타내기 위한 bool 변수
bool Pstate = false;
bool Mstate = false;
bool Pflag = false;
bool Mflag = false;
bool Mchk = false;


void setup() {
 // 시리얼 통신
  Serial.begin(9600);
 // lcd 동작 초기화
  lcd.init();
  lcd.clear();
  lcd.noBacklight();

// 미세먼지 측정 핀 설정
  pinMode(IRED_PIN, OUTPUT); 
  digitalWrite(IRED_PIN, HIGH);
  
// 배열 값 초기화
  for(int i=0; i<FILTER_MAX; i++){
    arrFilter[i] = 0;
  }
  
// 디지털 핀모드 설정
  pinMode(SW_POWER, INPUT_PULLUP);
  pinMode(SW_MOTOR, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  pinMode(BUZZER, OUTPUT);

// 시리얼 모니터에 START 출력
  Serial.println("START");
}


void loop() {
  // 전원 스위치 토글 동작
  if(digitalRead(SW_POWER) == LOW){
    if(Pflag == false){
      Pflag = true;
      Pstate = !Pstate;
    }
  }
  else
    if(Pflag == true)
      Pflag = false;

  // 전원 ON -------------------------------------------
  if(Pstate){
    // 모터 스위치 토글 동작
    if(digitalRead(SW_MOTOR) == LOW){
      if(Mflag == false){
        Mflag = true;
        Mstate = !Mstate;
      }
    }
    else 
      if(Mflag == true)
       Mflag = false;
          
    if(Mstate == true){ // 모터 스위치 ON
      if(Mchk == false) {
        MOTOR_RUN(true); // 모터 동작
        Mchk = true;
      }
    }
    else{ // 전원 ON and 모터 스위치 OFF
      //미세먼지 측정 함수
      float dustDensity = filtering(probe_dust_density()); 
      Serial.print("Dust Density = "); // 미세먼지 값 시리얼 모니터에 출력
      Serial.println(dustDensity);
      delay(300);

      // 전원 LED
      digitalWrite(LED, HIGH); // 전원 ON이므로 전원 LED ON

      // 측정한 미세먼지 LCD 출력
      lcd.backlight();
      lcd.setCursor(2,0);
      lcd.print("*DUST VALUE*");
      lcd.setCursor(0,1);
      lcd.print(dustDensity);

      if(dustDensity < 20){ // 미세먼지 좋음
        rgbOutput(0,0,255);  // RGB 파랑 출력
        delay(10);
        if(Mchk == true){
          MOTOR_RUN(false); // 모터 중지
          Mchk = false;
        }
      }
      else if (dustDensity > 80){ // 미세먼지 나쁨
        rgbOutput(255,0,0); // RGB 빨강 출력
        delay(10);
        if(Mchk == false){
          MOTOR_RUN(true); // 모터 동작 
          Mchk = true;
        }
      }
       else{  // 미세먼지 보통
        rgbOutput(255,255,0); // RGB 노랑 출력
        delay(10);
        if(Mchk == false){
          MOTOR_RUN(true); // 모터 동작
          Mchk = true;
        }
      }
    }
  }
  else{ // 전원이 꺼질 경우
    digitalWrite(LED,LOW); // 전원 LED 출력 중지
    if(Mchk == true) {
      MOTOR_RUN(false); // 모터 중지
      Mchk = false;
    }
    Mstate = 0;
    rgbOutput(0,0,0); // RGB 출력 중지
    lcd.clear(); // lcd 초기화
    lcd.noBacklight(); // lcd backlight 끔
  }
}


//- 미세먼지 측정 후 보정한 값 구하기 ---------------------------------------------
float probe_dust_density(){
  float dustValue = 0.0;
  float dustVo = 0.0;

  digitalWrite(IRED_PIN, LOW);
  delayMicroseconds(SAMPLING_TIME);

  dustValue = analogRead(DUST_PIN);
  delayMicroseconds(WAITING_TIME);

  digitalWrite(IRED_PIN, HIGH);
  delayMicroseconds(STOP_TIME);

  dustVo = (dustValue * (VCC / 1024.0));

  if(dustVo >= VOC)
  {
     float deltaV = dustVo - VOC;
     return deltaV * DUST_DENSITY_RATIO;
  }else{
     return 0;
  }
}

//- 미세먼지 측정 값 필터링 ---------------------------------------------------
float filtering(float val){
    int   sample_cnt = 0;
    float density = 0;
    
    for(int i=0; i<FILTER_MAX; i++)
      arrFilter[i-1] = arrFilter[i];

     arrFilter[FILTER_MAX-1]  = val;

    for(int i=0; i<FILTER_MAX; i++)
    {
        if( arrFilter[i] )
        {
            sample_cnt++;
            density += arrFilter[i];
        }
    }

    if(sample_cnt >0)
        return density / sample_cnt;

    return density;
}

// 모터 동작과 소리 출력---------------------------------------------
void MOTOR_RUN(bool chk) {
  if(chk){
    digitalWrite(MOTOR, HIGH);
    tone(BUZZER, 256, 100);
    delay(300);
    tone(BUZZER, 286, 100);
    delay(300);
    tone(BUZZER, 320,100);
    delay(300);
    noTone(BUZZER);
  }
  else{
    digitalWrite(MOTOR, LOW);
       tone(BUZZER, 320,100);
    delay(300);
 
    tone(BUZZER, 286, 100);
    delay(300);
    tone(BUZZER, 256, 100);
    delay(300);
    noTone(BUZZER);
  }
}

// RGB 출력----------------------------------------------------------
void rgbOutput(int Red, int Green, int Blue){
  analogWrite(RGB_R,Red);
  analogWrite(RGB_G,Green);
  analogWrite(RGB_B,Blue); 
}
