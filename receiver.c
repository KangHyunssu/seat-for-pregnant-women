#include <stdio.h> //입출력 라이브러리
#include <wiringPi.h> //WiringPi 라이브러리
#include <softTone.h> //softTone 라이브러리

#undef millis //WiringPi와 RF24에 겹치는 millis 충돌을 방지하기 위해 millis 비활성화
#include <RF24/RF24.h> // RF24 무선 통신 라이브러리

#define CE_PIN 22 //NRF모듈 CE 핀
#define CSN_PIN 0 //NRF모듈 CSN 핀
#define TP 23 //초음파 센서 HC-SR04 모듈 트리거 핀 
#define EP 24 //초음파 센서 HC-SR04 모듈 에코 핀
#define LED_PIN 13 //LED 핀
#define MOTOR_PIN 5 //진동모터 핀
#define BUZZER_PIN 17 //버저 핀
#define BUZZER_FREQ 1000 //버저 주파수 
#define PERSON_DISTANCE 15.0 //사람과 초음파 센서 사이의 거리 임계값

RF24 radio(CE_PIN, CSN_PIN); //nRF24L01 모듈을 제어할때 사용할 수 있도록 CE,CSN 값을 RF24에 전달


const uint8_t address[6] = "1Node";  //송/수신에 사용할 주소

int currentState = 0; //현재상태 초기화
int receivedSignal = 0; //수신 신호 초기화
int signalState = 1; //신호 상태 초기화

int ledBlinkState = 0; //LED 깜빡임 상태 초기화
unsigned long lastBlinkTime = 0; //LED가 마지막으로 깜빡인 시간 초기화
const unsigned long blinkInterval = 500; //LED가 깜빡이는 간격 설정

void DeviceControl(int Cmd) {
    unsigned long currentTime = millis(); //프로그램 시작 이후 경과된 시간을 밀리초 단위로 설정
    if (Cmd) { 
        if (currentTime - lastBlinkTime >= blinkInterval) { // LED 깜빡임 간격 계산
            ledBlinkState = !ledBlinkState; //LED 상태 전환
            digitalWrite(LED_PIN, ledBlinkState ? HIGH : LOW); //LED 켜거나 끄기
            lastBlinkTime = currentTime; //마지막으로 깜빡일 시간 업데이트
        }
    } else {
        digitalWrite(LED_PIN, LOW); //LED 끔
    }
    digitalWrite(MOTOR_PIN, (Cmd) ? HIGH : LOW); //진동 모터 제어
    softToneWrite(BUZZER_PIN, (Cmd) ? BUZZER_FREQ : 0); //부저 소리 제어
}

float getDistance(void) { 
    float fDistance; // 계산된 거리를 저장할 변수 선언
    int nStartTime, nEndTime;  //초음파가 발사되고 에코 핀의 HIGH 신호가 시작된 시간 저장

    digitalWrite(TP, LOW); //초음파 센서의 안정화를 위해 트리거 핀을 LOW로 설정
    delayMicroseconds(2); //안정화를 위해 2마이크로초 동안 대기

    digitalWrite(TP, HIGH); //초음파 신호를 발사하기 위해 트리거 핀을 HIGH로 설정
    delayMicroseconds(10); //초음파 신호 발사하기 위해 트리거 핀을 10마이크로 초 동안 유지

    digitalWrite(TP, LOW); //초음파 신호 발사를 종료하기 위해 트리거 핀을 다시 LOW로 설정

    while (digitalRead(EP) == LOW); //에코 핀이 HIGH로 전환될 때까지 대기해서 초음파 신호를 기다림
    nStartTime = micros(); //에코 핀이  HIGH로 전환된 시간을 마이크로 초로 기록

    while (digitalRead(EP) == HIGH); //에코 핀이 LOW로 전환될 때까지 대기하고 초음파 신호가 돌아오는 시간을 기다림
    nEndTime = micros(); //에코 핀이 LOW로 전환된 시간을 마이크로초 단위로 기록함

    fDistance = (nEndTime - nStartTime) * 0.034 / 2; //초음파의 이동 시간과 속도를 이용해 거리를 계산

    return fDistance; //계산된 거리를 반환
}

void setup() {
    if (wiringPiSetupGpio() == -1) { //GPIO 핀 초기화
        return;
    }

    pinMode(LED_PIN, OUTPUT); //LED 핀 출력 모드로 설정
    pinMode(MOTOR_PIN, OUTPUT); //모터 핀 출력 모드로 설정
    pinMode(TP, OUTPUT); //초음파 센서 트리거 핀 출력 모드로 설정
    pinMode(EP, INPUT); /초음파 센서 에코 핀 입력 모드로 설정
    digitalWrite(LED_PIN, LOW); //LED를 초기 상태로 끔
    digitalWrite(MOTOR_PIN, LOW); //모터를 초기 상태로 끔

    if (softToneCreate(BUZZER_PIN) != 0) { //부저 핀을 톤 제어 모드로 설정
        return;
    }
    softToneWrite(BUZZER_PIN, 0); //부저를 초기 상태로 끔

    if (!radio.begin()) { //nrf24L01 모듈 초기화
        return;
    }

    radio.openReadingPipe(1, address); //RF24 모듈의 수신 파이프를 설정해서 주소 지정
    radio.setPALevel(RF24_PA_HIGH); //RF24 모듈의 출력 전력을 HIGH로 설정
    radio.startListening(); //RF24 모듈을 수신 모드로 설정
}

void signal() { //신호 수신 처리 함수
    if (radio.available()) { //RF24 모듈에 수신된 데이터가 있는지 확인
        radio.read(&receivedSignal, sizeof(receivedSignal)); // 있으면 sizeof(receivedSignal)만큼 receivedSignal에 저장
    }
}

int main() {
    setup(); //GPIO 핀, 부저, RF24 모듈 초기화

    while (1) { //무한루프 
        if (signalState == 1) signal(); //signalState가 1이면 RF24 모듈로부터 신호 수신
        signal(); // RF24 모듈로부터 신호를 수신
        float distance = getDistance(); //초음파 센서로 현재 물체와 거리 측정
        switch (currentState) { //현재 시스템 상태에 따른 case문 
        case 0: 
            DeviceControl(0); //모든 장치 비활성화
            if (distance < PERSON_DISTANCE) { //측정된 거리가 임계값보다 작으면 상태를 1로 저장
                currentState = 1;
            }
            break;
        case 1: 
            if (receivedSignal == 1) { //RF24 모듈로부터 신호가 1로 수신되면 상태를 2로 전환
                currentState = 2;
            }
            break;
        case 2:
            DeviceControl(1); //모든 장치(LED, 모터, 부저)를 활성화
	        if (distance > PERSON_DISTANCE) { // 측정된 거리가 임계값보다 크면 상태를 3으로 전환
                currentState = 3;
            }
            break;
        case 3:
            DeviceControl(1); //모든 장치 활성화 
            if (distance < PERSON_DISTANCE) {
                signalState = 0; //signalState를 0으로 설정
                currentState = 4; //측정된 거리가 임계값보다 작으면 상태를 4로 전환
            }
            break;
        case 4: 
            if (distance > 400) distance = 10;
            DeviceControl(0); //모든 장치 비활성화
            digitalWrite(LED_PIN, HIGH); // LED 활성화
            if (distance > PERSON_DISTANCE) { //거리가 임계값보다 크면  
                currentState = 0; //상태를 0으로 전환
                receivedSignal = 0; //수신 신호 초기화
                signalState = 1; //신호 상태 초기화
            }
            break;
        default: //모든 상태 초기화
            currentState = 0;
            receivedSignal = 0;
            signalState = 1;
            break;
        }
        delay(100);
    }

    return 0;
}
