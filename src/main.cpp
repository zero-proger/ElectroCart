#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Определение пинов для подключения GPS-модуля
#define RX_PIN 16
#define TX_PIN 17
#define ssid "SSIP_AP"
#define password "TestPass"
// Пины для подключения педали газа и контроллера моторов
#define analogInPin 36
#define pwmPin 13
// Пин для подключения аналогового датчика и коэффициенты для расчета напряжения
#define sensorPin 33
#define voltageDividerFactor 12.73
#define maxVoltage 42.01
#define minVoltage 30.01

// Создание объектов для работы с GPS-модулем
SoftwareSerial gpsSerial(RX_PIN, TX_PIN);
TinyGPSPlus gps;

// Настройки WiFi-точки доступа
float speed = 0;

//Функция loop, выполняемая в бесконечном цикле на ядре 1
void loop1(void *);

// Создание объекта веб-сервера
WebServer server(80);

// Функция для отображения веб-интерфейса
void WebInterface()
{
    String html = R"=====(<!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>WebInterface</title>
        <style>
          .container {
              width: 400px;
              height: 400px;
              margin: 0 auto;
          }
            body {
                display: grid;
                justify-content: center;
                align-items: center;
                height: 50vh;
                margin: 0;
                background-color: #eed3ec;
            }

            .battery {
                width: 225px;
                height: 122.5px;
                background-color: #2b2b2b;
                border: 1px solid #000000;
                border-radius: 35px;
                position: relative;
                top: 300%;
                left: -2%;
                overflow: hidden;
            }

            .battery-level {
                height: 100%;
                position: relative;
                top: 0;
                left: 0;
            }

            .battery-percentage {
                position: absolute;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                font-size: 35px;
                color: #000000;
            }

            .speedometer {
                width: 200px;
                height: 200px;
                border: 10px solid #000000;
                border-radius: 50%;
                position: relative;
                background-color: #ffffff;
            }

            .needle {
                width: 2px;
                height: 80px;
                background-color: red;
                position: relative;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -100%) rotate(-180deg);
                transform-origin: bottom center;
                transition: transform 0.5s ease-in-out;
            }

            .speed {
                position: absolute;
                bottom: 10px;
                left: 50%;
                transform: translateX(-50%);
                font-size: 24px;
            }

            .division {
                position: absolute;
                width: 2px;
                height: 10px;
                background-color: black;
                transform-origin: bottom center;
                left: 50%;
            }

            .footer {
                position: absolute;
                bottom: 10px;
                left: 50%;
                transform: translateX(-50%);
                font-size: 14px;
                color: #666;
                text-align: center;
            }
        </style>
    </head>
    <body>
        <div id="sputniks-element">Подключено спутников:0</div>
        <div class="battery" id="battery">
            <div class="battery-level" id="battery-level"></div>
            <span class="battery-percentage" id="battery-percentage"></span>
        </div>
        <div class="speedometer">
            <div class="needle"></div>
            <div class="speed">0<br>
            км/ч</div>
        </div>
        <div class="footer">
            Web-Interface разработан<br>
            студентом группы 9ММР-46-20<br>
            Нехорошевым Антоном Андреевичем<br>
            <br>
            2024
        </div>
        <script>
            function setSputniks(sputniks) {
                const element = document.getElementById("sputniks-element"); // replace with the ID of the HTML element you want to update
                element.textContent = "Подключено спутников: " + sputniks;
            }
        </script>

        <script>
            // Функция для установки уровня заряда аккумулятора
            function setBatteryLevel(charge) {
                var batteryLevel = document.getElementById("battery-level");
                var batteryPercentage = document.getElementById("battery-percentage");
                var color;
                if (charge >= 75) {
                    color = "#00FF00"; // Green 100-75%
                } else if (charge >= 50) {
                    color = "#00FFFF"; // Cyan 75-50%
                } else if (charge >= 25) {
                    color = "#FFFF00"; // Yellow 50-25%
                } else {
                    color = "#FF0000"; // Red 25-0%
                }
                batteryLevel.style.width = charge + "%";
                batteryLevel.style.backgroundColor = color;
                batteryPercentage.textContent = charge.toFixed(0) + "%";
            }

            // Получение элемента спидометра
            const speedometer = document.querySelector('.speedometer');

            // Функция для создания делений на спидометре
            function createDivisions() {
                const divisions = 13;
                const startAngle = 138.5;
                const endAngle = 498.5;
                const radius = 90;
                const centerX = 100;
                const centerY = 100;

                // Создание делений
                for (let i = 3; i <= divisions; i++) {
                    let value = 0;
                    const angle = startAngle + (i / divisions) * (endAngle - startAngle);
                    value = (i * 10) - 30;
                    if (value < 0) value = 360 + value;

                    const radians = (angle - 90) * (Math.PI / 180);
                    const x = centerX + radius * Math.cos(radians);
                    const y = centerY + radius * Math.sin(radians);

                    const division = document.createElement('div');
                    division.classList.add('division');
                    division.style.transform = `translate(-50%, -100%) rotate(${angle}deg)`;
                    division.style.left = `${x}px`;
                    division.style.top = `${y}px`;
                    division.textContent = " " + value;
                    speedometer.appendChild(division);

                    if (value === 100) break;
                }
            }
            
            // Создание делений на спидометре при загрузке страницы
            createDivisions();

            // Функция для получения данных через AJAX
            function getData() {
                fetch('/data')
                    .then(response => response.json())
                    .then(data => {
                        setBatteryLevel(data.charge);
                        setSpeedometer(data.speed);
                        setSputniks(data.sputniks);
                    })
                    .catch(error => console.error('Error:', error));
            }

            // Функция для установки скорости на спидометре
            function setSpeedometer(speed) {
                const needle = document.querySelector('.needle');
                const speedText = document.querySelector('.speed');

                // Поворот стрелки спидометра
                const angle = ((speed / 1.2820512821 / 100) * (360) - 138.5);
                needle.style.transform = `translate(-50%, -100%) rotate(${angle}deg)`;

                // Обновление отображаемого значения скорости
                speedText.textContent = `${speed.toFixed(0)} км/ч`;
            }

            // Периодическое получение данных
            setInterval(getData, 500);
        </script>
    </body>
    </html>
  )=====";

    server.send(200, "text/html", html);
}

// Обработчик запроса за данными
void handleData()
{
    // Получение данных о скорости и заряде аккумулятора
    int batteryValue = analogRead(sensorPin);
    float voltage = batteryValue / 4095.0 * voltageDividerFactor * 3.3;
    float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0;
    if (percentage < 0) {
        percentage = 0;
    }
    speed = gps.speed.kmph();

    // Получение данных о количестве спутников
    int sputniks = 0;
    if (gps.satellites.isValid())
    {
        sputniks = gps.satellites.value();
    }

    // Формирование и отправка JSON-ответа
    String json = "{\"charge\": " + String(percentage, 1) + ", \"speed\":" + String(speed, 1) + ", \"sputniks\":" + String(sputniks) + "}";
    server.send(200, "application/json", json);
}

// Функция setup, выполняемая при запуске
void setup()
{
    // Инициализация последовательного порта и WiFi-точки доступа

    Serial.begin(9600);
    Serial.println();
    gpsSerial.begin(9600);

    pinMode(pwmPin, OUTPUT);
    pinMode(sensorPin, INPUT);
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(IP);

    // Регистрация обработчиков запросов
    server.on("/", HTTP_GET, WebInterface);
    server.on("/data", HTTP_GET, handleData);
    server.begin();
    Serial.println("HTTP server started");

    xTaskCreatePinnedToCore(
        loop1,   // функция
        "loop2", // имя
        2048,    // размер стека
        NULL,    // параметры для передачи в функцию
        1,       // приоритет
        NULL,    // дескриптор задачи
        1        // ядро 1
    );
}

float percentage = 0;

// Функция loop, выполняемая в бесконечном цикле на ядре 0
void loop()
{

    int batteryValue = analogRead(sensorPin);
    float voltage = batteryValue / 4095.0 * voltageDividerFactor * 3.3;
    float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0;
    if (percentage < 0) {
        percentage = 0;
    }

    // Чтение значения с педали газа и управление контроллером моторов
    int gas = analogRead(analogInPin);
    int dutyCycle = map(gas, 0, 4095, 0, 255);
    analogWrite(pwmPin, dutyCycle);

    server.handleClient(); // Обработка запросов к веб-серверу

}

// Функция loop, выполняемая в бесконечном цикле на ядре 1
void loop1(void *)
{
    for (;;)
    {
        while (gpsSerial.available() > 0)
        {
            if (gps.encode(gpsSerial.read()))
            {
                // Данные от GPS-модуля успешно обработаны
                speed = gps.speed.kmph();
                int sputniks = gps.satellites.value();
                Serial.println(sputniks);
                // Формирование и отправка JSON-ответа
                String json = "{\"charge\": " + String(percentage, 1) + ", \"speed\":" + String(speed, 1) + ", \"sputniks\":" + String(sputniks) + "}";
                server.send(200, "application/json", json);
            }
        }
        delay(1);
    }
}
