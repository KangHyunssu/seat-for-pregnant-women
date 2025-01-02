#include <stdio.h> //입출력 라이브러리
#include <wiringPi.h> //WiringPi 라이브러리

#undef millis // WiringPi와 RF24에 겹치는 millis 충돌을 방지하기 위해 millis 비활성화
#include <RF24/RF24.h> //RF 라이브러리

#define CE_PIN 22 //nrf24L01 모듈 CE 핀
#define CSN_PIN 0 //nrf24L01 모듈 CSN 핀

#define BUTTON_PIN 4 //버튼 핀
#define LED_PIN 24 //LED 핀

RF24 radio(CE_PIN, CSN_PIN); //nRF24L01 모듈을 제어할때 사용할 수 있도록 CE,CSN 값을 RF24에 전달

const uint8_t address[6] = "1Node"; //송/수신에 사용할 주소

int KeypadRead(void) {
    return !digitalRead(BUTTON_PIN); //버튼의 상태를 읽고 LOW를 1로 반환
}

void LedControl(int Cmd) {
    digitalWrite(LED_PIN, (Cmd) ? HIGH : LOW); //Cmd 값에 따라 LED를 켜거나 끔
}

void setup() {
    if (wiringPiSetupGpio() == -1) { //GPIO핀 초기화
        return;
    }

    pinMode(BUTTON_PIN, INPUT); //버튼 핀을 입력으로 설정
    pullUpDnControl(BUTTON_PIN, PUD_UP);// GPIO 핀 상태를 HIGH로 설정
    pinMode(LED_PIN, OUTPUT); //LED 핀을 출력으로 설정
    digitalWrite(LED_PIN, LOW); //LED 초기상태 OFF로 설정

    if (!radio.begin()) { //nrf24L01 초기화
        return;
    }

    radio.openWritingPipe(address); //송신 파이프에 주소 설정 
    radio.setPALevel(RF24_PA_HIGH); //nrf24L01의 출력 전력 HIGH로 설정
    radio.stopListening(); //NRF모듈을 송신 모드로 설정
}

void loop() {
    static int buttonPressed = 0; //버튼상태 초기화
    static int currentSignal = 1; //현재신호상태 초기화 

    int buttonState = KeypadRead(); //현재 버튼상태

    if (buttonState == 1 && buttonPressed == 0) { //버튼이 눌리고 이전 상태에 눌리지 않을때 실행
        LedControl(currentSignal); //현재 신호에 따라 LED 끄거나 킴

        bool success = radio.write(&currentSignal, sizeof(currentSignal)); //RF24로 현재신호 값을 송신

        currentSignal = !currentSignal; //송신할 신호를 반대 상태로 바꿈
        buttonPressed = 1; //버튼이 눌린 상태를 저장
    } else if (buttonState == 0) { //버튼이 떼지면 상태 초기화
        buttonPressed = 0; 
    }

    delay(100);
}

int main() { 
    setup(); //초기 설정

    while (1) { //무한루프로 LED 제어와 신호처리 반복
        loop();
    }

    return 0;
}
