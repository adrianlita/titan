# embedded-TITAN - ToDO (sa nu uit)

- ninab312
    - gatt client



- la drivers: facem cu interrupts parametrizabili ca sa fie mai ok
- la drivers/* vedem pe unde nu returnam uint32_t ca ne-a fost lene, si chiar trebuie

- internal_flash
- UUID:
    - din CPU
    - din secure element
    - din flash, setat dinainte, in memoria bootloaderului :D ?
- bootloader
- watchdog
- softwatchdog
- buzzer - eventual cand primesti clickboards

timer:
- uitat pe xtimer vedem ce si cum ne luam de-acolo
- attach/deattach la un hardware timer
- start_timeout(period)
- start_periodic(period)
- stop

stopwatch:
- start
- stop
- pause

Top todo:
- label perihperal functions as ISR safe/unsafe
- Ecran de la Bostan
- ESP8266
- ATWINC
- afe4400
- ublox m8n
    - test pinii aia ce fac cand comunica asta
    - vazut in codul gasit daca merge facut driver frumos la ublox prin care putem controla mult mai mult
- test aprs ca merge cu gpio dac si timer

- LSM9DS1 (pmod nav sensors)
- LPS25HB (pmod nav sensors)
- PAC1944
- PAC1954
- st25dv04k (nfc)

Refactor ca sa nu para copiat:
- stepper
- base64
- adaugat info de sursele de aes/etc

lwip nu e atat de rau?? ne uitam cum face RIOT, si daca RIOT are 

- explorer - facem codul mai clar pe embedded
- explorer - facem codul mai clar pe C#
- explorer - versiune de explorer /pe firmware
- explorer - add add some soft of software flow control
- explorer - transfer larger file part by part
- explorer - ceva e handicapat cu uint16 pe-acolo.. poate vrem uint32 peste tot si devine mai clar si codul


- semaphore_multilock ?
- task id si primitive id nu sunt deocamdata folosite
- K_CSECT_MUST_CHECK se poate face mai bine, ca sa mearga ok si pe intreruperi ?
- la context switcher, se verifica starea si daca e running, o face ready. se poate scapa de ea cu 2 functii scheduler_trigger si scheduler_trigger_current_ready, dar deocamdata nu merita
- deocamdata taskurile pot schimba cum vor prioritati, inclusiv pe ale lor. sys_define daca un task are voie sa creeze taskuri de orice prio, sau doar mai mica sau egala cu el, si nu numai
- WFI merge si cu interrupts disabled ???
- use CYCLECOUNTER
    

- kernel aware/unaware interrupts
    - cu BASEPRI
    - trebuiesc review-uite si prioritatile si modul in care trebuiesc implementate
- softimers (cum implementam pe priority-ul taskului curent?)


futureAL:
[0]     - mutex priority inheritance / ceilingprio / prioceiling
[1]     - ne uitam daca taskul are obiecte lock-uite, si le dam release
[0]     - kernel_services: time
[0]     - move kernel functions on SVC
            - probabil o sa dispara si chestii de K_ISR_FORBID, etc
[3]     - SysTick stops and LPTimer starts, or we modify SysTIck period to accomodate what we need to implement true tickless
        - above only happens when sleep() is called, otherwise we wait, and enable tick/timer ISR with a very large period to update timer overflow
[4]     - dynamic tasks. daca e task dinamic, trebuie sa malloc stack la adresa buna
[5]     - dynamic tasks. daca e task dinamic, trebuie sa free(stack)
[7]     - use MPU
[8]     - use TrustZone
[9]     - signal()


- poate in primitive putem sa punem un state (created/destroyed) si asa putem refolosi id-uri daca le reincarca pe aceeasi structura



 MULTA SIINTA IN
    - R:\mandrel\doc\RIOT-master\cpu\cortexm_common\task_arch.c
    - .../qpc/ports/arm-cm/qk/iar/qk_port.c
    - .../qpc/ports/arm-cm/qxk/iar/qxk_port.c


 https://www.gnu.org/software/libc/manual/html_node/POSIX-Safety-Concepts.html#POSIX-Safety-Concepts
 

- [always] verifica pentru checkAL (probleme)
- [cand e gata] scoate tot ce e cu debugAL


Examples
=====================
- rendevous synchronization
- systest - comprehensive check

Peripherals:
- simple pwm generation
- servo motor control
- stepper motor control
- 

Peripheral insight
=====================

spi_slave
qspi_master
i2c_slave

USB
------
usb_device_cdc
usb_device_msd
usb_device_mouse
usb_device_keyboard
usb_device_mouse_keyboard
usb_device_uvc
usb_device_midi (usb_audio ?)

ce punem pe usb_master, sau cum se face?
- keyboard/mouse input
- msd


Click-Boards:
--- comandat, reverificam
[want] https://www.mikroe.com/ble-4-click [5.0 ublox nina-b312]
[want] https://www.mikroe.com/ble-8-click [5.0 ublox anna-b112 care e defapt nrf52832 + antena]
[want] https://www.mikroe.com/ble-7-click [5.0 chip de la silabs]
[want] https://www.mikroe.com/wifi-nina-click [ublox nina-w132, are security intrinsec]
[want] https://www.mikroe.com/gnss-7-click [ublox m9n]
[want] https://www.mikroe.com/ccrf-3-click [cc1120 433MHz -- x2 bucati]
[want] https://www.mikroe.com/ccrf-click [cc2500 2.4GHz -- x2 bucati]
[want] https://www.mikroe.com/buzz-click [simplu si eficient, ia mai multe bucati]
[want] https://www.mikroe.com/stepper-2-click [A4988 IC]
[want] https://www.mikroe.com/stepper-5-click [TM2208]
[want] https://www.mikroe.com/lte-iot-click
[want] https://www.mikroe.com/ir-click [2 bucati minim, ca sa mearga, foarte foarte ieftin si simplu]
[want] https://www.mikroe.com/heart-rate-3-click [scump - afe4404+sfh7050]
[want] https://www.mikroe.com/nrf-c-click [nrf24 - 2 bucati]
[want] https://www.mikroe.com/nfc-tag-click [pe i2c]
[want] https://www.mikroe.com/servo-click [PCA9685 + LTC2497 --- 16channel cu tot cu ADC]
[want] https://www.mikroe.com/motion-click [pir normal.. fara protocol]
[want] https://www.mikroe.com/fan-click [microchip emc2301 - 4 wire coolers, i2c control]
[want] https://www.mikroe.com/thumbstick-click [joystick analogic + MCP3204]
[want] https://www.mikroe.com/rotary-g-click [prea ieftin sa nu merite]
[want] https://www.mikroe.com/temphum-13-click [ieftin, de la te connectivity]
[want] https://www.mikroe.com/thermo-9-click [tsys01 de la TE connectivity]
[want] https://www.mikroe.com/6dof-imu-7-click [chipul nou de la invensense. ieftinut]
[want] https://www.mikroe.com/touchkey-click [foarte simplu]
[want] https://www.mikroe.com/rtc-10-click [ds3231m i2c]
[nice] https://www.mikroe.com/dc-motor-2-click [1.2A]
[nice] https://www.mikroe.com/dc-motor-3-click [3.5A]
[want] https://www.mikroe.com/capsense-click [pe i2c, cu un chip de la cypress]
[want] https://www.mikroe.com/proximity-3-click    [i2c. vcnl4200 - multi app: lux meter, plus ca are LED]




[poate gasim la beton][nice] https://www.mikroe.com/mcp1664-click [practic doar PWM]
[want] https://www.mikroe.com/6dof-imu-11-click [kmx63 - foarte low power]
[want] https://www.mikroe.com/h-bridge-click [dual h-bridge -- works for stepper as well ... no measurements]
[want] https://www.mikroe.com/sqi-flash-click [microchip SST26VF064B]
[want] https://www.mikroe.com/expand-4-click [open drain cu 50V 350 mA output]
[want] https://www.mikroe.com/ir-sense-3-click [ak9754 - ir sensor 1m range human]
[want] https://www.mikroe.com/e-paper-bundle-3  [ecran mare, dar sunt identice aman2 ca chip-uri]
[want] https://www.mikroe.com/e-paper-bundle-1  [ecran mic]
[want] https://www.mikroe.com/mp3-click [pe SPI, decodeaza multe, encodeaza ogg]
[want] https://www.mikroe.com/thunder-click [scump, pe SPI, dar foarte interesant - as3935 + ma5532 -- 40KM detection, 1km heading]
[want] https://www.mikroe.com/fiber-opt-33v-click [foarte misto, scumpicel, si trebuiesc 2 bucati]

[want] https://www.mikroe.com/13dof-click [scump. bme680 + bmm150(geomagnetic) + bmi088]
[want] https://www.mikroe.com/ook-rx-click [microchip. wiring ciudat la mufa]
[want] https://www.mikroe.com/ook-tx-click [microchip. wiring ciudat la mufa]
[want] https://www.mikroe.com/emg-click [analog based, full microchip]
[want] https://www.mikroe.com/ecg-4-click [uart based, cu un chip de la neurosky. scumpicel]

[nice] https://www.mikroe.com/altitude-click   [mpl3115a2]
[nice] https://www.mikroe.com/weather-click [bme280]
[nice] https://www.mikroe.com/10dof-click [bosch powered, 9axis+baro bno055 + bmp180 --- cam scump]
[nice] https://www.mikroe.com/sigfox-click [sigfox de europe, vine cu 1 year subscription]
[nice] https://www.mikroe.com/amr-current-click [senzor high voltage high current + mcp3221]
[nice] https://www.mikroe.com/no2-2-click [scump. mics2714+MCP3201 detecteaza NO2 si H2]
[nice] https://www.mikroe.com/ambient-3-click [lux meter, chromatic meter, cam scump, si are si o memorie pe langa]
[nice] https://www.mikroe.com/lsm303agr-click-click [lsm303agr nu cred ca mai conteaza, plus ca cred ca gasim altceva]
[nice] https://www.mikroe.com/compass-click [lsm303dlhc]
[nice] https://www.mikroe.com/barometer-2-click [lps35hw]
[nice] https://www.mikroe.com/fm-click [pe i2c, are si RDS, antena e in cablul de casti]
[nice] https://www.mikroe.com/fram-3-click [un fel de ATECC608 dar mai slab?]
[nice] https://www.mikroe.com/rtc-3-click [bq3200]
[nice] https://www.mikroe.com/irda2-click [microchip.. dar nu stiu daca mai e de actualitate, si trebie 2]
[nice] https://www.mikroe.com/ir-distance-click        [an only, cu sharp]
[nice] https://www.mikroe.com/alcohol-click    [AN-only senzor MQ-3, cu potentiometru]
[nice] https://www.mikroe.com/air-quality-click [mq-135, an-only]

[no] https://www.mikroe.com/oled-switch-click [mult prea scump]
[no] https://www.mikroe.com/compass-2-click [e inclus in unul din MPU9x50]
[no] https://www.mikroe.com/vibro-motor-click
[no] https://www.mikroe.com/vibro-motor-2-click
[no] https://www.mikroe.com/proximity-click  [vcnl4010 - 20 cm]
[no] https://www.mikroe.com/nfc-click [34$ scump rau]
