//#############################################################################
//
// FILE:   main.c
//
// TITLE:  SPI-A Master driverlib example, CPUTimer0 1kHz interrupt
//         (전부 driverlib API로 통일, F28x_Project.h / InitSysCtrl() 등
//          bitfield 지원 함수는 사용하지 않음)
//
//#############################################################################
//
//
// $Copyright:
// Copyright (C) 2013-2026 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
// F28x_Project.h(bitfield 지원 헤더)는 쓰지 않음.
// EALLOW/EDIS/EINT/DINT/ERTM/NOP 등은 driverlib.h -> cpu.h 에 이미 정의돼 있음
#include "driverlib.h"
#include "device.h"

//
// SPI-A: GPIO58=SIMO(MOSI), GPIO59=SOMI(MISO), GPIO60=CLK, GPIO61=CS(수동 GPIO)
// GPIO16~19(SPI-A 기본 핀)은 LAUNCHXL-F28379D에서 GPIO17이 CAN-B RX와 겹쳐서
// 대체 핀 세트(GPIO58~61)로 옮김. Master 모드에서는 하드웨어 SPISTE보다
// 일반 GPIO로 CS를 직접 제어하는 게 흔하고 간단함
//
#define SPI_BAUDRATE    1000000UL   // 1Mbps

// CS를 내린 직후 곧바로 클록이 시작되면 슬레이브가 앞부분 비트를 놓칠 수 있어서
// (CS setup time 부족) 클록 시작 전에 짧게 지연을 준다
#define SPI_CS_LOW()    do { GPIO_writePin(61, 0); DEVICE_DELAY_US(1); } while(0)
#define SPI_CS_HIGH()   GPIO_writePin(61, 1)

// CPUTimer0 1kHz 주기 (1ms)
#define TIMER0_PERIOD_US   1000.0f   // 1ms = 1kHz

//
// Function Prototypes
//
void spia_init(void);
uint16_t spia_xfer(uint16_t data);
void timer0_init(void);
__interrupt void timer0_isr(void);

//
// Globals
//
volatile uint16_t g_rxData;     // 디버거 Expressions에서 확인용
volatile uint32_t g_sampleCnt;  // 1kHz로 몇 번 샘플링했는지 카운트 (동작 확인용)

//
// Main
//
void main(void)
{
    // ① 시스템 클록/플래시/워치독 등 디바이스 초기화 (driverlib 전용 초기화 함수)
    //    bitfield용 InitSysCtrl() 대신 device.c의 Device_init()을 사용.
    //    PLL 설정(DEVICE_SETCLOCK_CFG), 워치독 disable, 전체 페리퍼럴 클록
    //    인에이블(SPI_A 포함)까지 여기서 다 처리됨
    Device_init();

    // ② GPIO 설정 잠금 해제 + 미사용 핀 풀업 (driverlib 표준 초기화 루틴)
    Device_initGPIO();

    // ③ 인터럽트 모듈/벡터 테이블 초기화 (InitPieCtrl()/InitPieVectTable()의
    //    driverlib 버전. 내부적으로 전역 인터럽트 disable과 IER/IFR 클리어까지 함)
    Interrupt_initModule();
    Interrupt_initVectorTable();

    // ④ GPIO mux, SPI-A 통신 설정
    spia_init();

    // ⑤ CPUTimer0을 1kHz(1ms)로 설정
    timer0_init();

    // ⑥ ISR 등록 및 인터럽트 enable
    Interrupt_register(INT_TIMER0, &timer0_isr);
    Interrupt_enable(INT_TIMER0);

    Interrupt_enableGlobal();   // 전역 인터럽트 enable (EINT와 동일)
    ERTM;                        // 실시간 디버그 인터럽트 enable

    // ⑦ CPUTimer0 시작 -> 이 시점부터 1ms마다 timer0_isr() 호출됨
    CPUTimer_startTimer(CPUTIMER0_BASE);

    while(1)
    {
        // 실제 SPI 송수신은 timer0_isr() 안에서 1kHz로 일어남
        NOP;
    }
}

//
// spia_init - GPIO mux, SPI-A 클록, 마스터 모드 통신 설정 (driverlib)
//
void spia_init(void)
{
    // GPIO58 : SPISIMOA (MOSI, 출력)
    GPIO_setMasterCore(58, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_58_SPISIMOA);
    GPIO_setPadConfig(58, GPIO_PIN_TYPE_STD);   // 출력이므로 풀업 disable

    // GPIO59 : SPISOMIA (MISO, 입력)
    GPIO_setMasterCore(59, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_59_SPISOMIA);
    GPIO_setPadConfig(59, GPIO_PIN_TYPE_PULLUP);     // 입력 풀업 enable (floating 방지)
    GPIO_setQualificationMode(59, GPIO_QUAL_ASYNC);  // 비동기 입력

    // GPIO60 : SPICLKA (SCLK, 출력)
    GPIO_setMasterCore(60, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_60_SPICLKA);
    GPIO_setPadConfig(60, GPIO_PIN_TYPE_STD);

    // GPIO61 : CS (일반 GPIO 출력으로 수동 제어, SPISTEA 안 씀)
    GPIO_setMasterCore(61, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_61_GPIO61);
    GPIO_setPadConfig(61, GPIO_PIN_TYPE_STD);
    GPIO_setDirectionMode(61, GPIO_DIR_MODE_OUT);
    GPIO_writePin(61, 1);   // idle 상태 = High (active-low CS)

    // SPI-A 모듈 클록은 Device_init()의 Device_enableAllPeripherals()에서 이미
    // 켜졌지만(SYSCTL_PERIPH_CLK_SPIA), 이 함수만 따로 재사용할 수도 있으므로
    // 명시적으로 한 번 더 호출해 둠 (중복 호출해도 안전함)
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_SPIA);

    // SPI 통신 설정 (SPI_setConfig()는 모듈이 reset 상태(SWRESET=0)일 때 호출해야 함)
    SPI_disableModule(SPIA_BASE);

    SPI_setConfig(SPIA_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA0,
                  SPI_MODE_CONTROLLER, SPI_BAUDRATE, 8);   // Mode 0, 8비트, 마스터

    SPI_disableLoopback(SPIA_BASE);
    SPI_disableFIFO(SPIA_BASE);   // FIFO 미사용, 폴링 방식으로 동작

    SPI_enableModule(SPIA_BASE);  // 리셋 해제 -> SPI 동작 시작
}

//
// spia_xfer - 1바이트 풀-듀플렉스 송수신 (전송 완료까지 대기)
//
uint16_t spia_xfer(uint16_t data)
{
    // 8비트 데이터는 레지스터 상위 비트로 좌측 정렬해서 써야 함 (TX)
    SPI_writeDataBlockingNonFIFO(SPIA_BASE, data << 8);

    // 수신 데이터는 우측 정렬로 들어옴. SPI_readDataBlockingNonFIFO()가
    // INT_FLAG 폴링 대기까지 내부에서 처리함
    return SPI_readDataBlockingNonFIFO(SPIA_BASE);
}

//
// timer0_init - CPUTimer0을 TIMER0_PERIOD_US(1ms, 1kHz)로 설정
//
void timer0_init(void)
{
    uint32_t periodCount;

    // period count = SYSCLK(Hz) / 1,000,000 * 주기(us)
    periodCount = (uint32_t)((float)DEVICE_SYSCLK_FREQ / 1000000.0f * TIMER0_PERIOD_US);

    CPUTimer_setPeriod(CPUTIMER0_BASE, periodCount);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0);   // SYSCLKOUT 그대로 사용 (분주 없음)

    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE,
                               CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);

    CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    // CPUTimer_startTimer()는 main()에서 인터럽트 enable 이후에 호출함
}

//
// timer0_isr - 1ms(1kHz)마다 호출, SPI로 IMU(슬레이브) 1바이트 송수신
//
__interrupt void timer0_isr(void)
{
    SPI_CS_LOW();
    g_rxData = spia_xfer(0xE);   // 예시로 0xE 송신, 슬레이브 응답을 g_rxData로 수신
    SPI_CS_HIGH();

    g_sampleCnt++;

    // PIE 그룹1 인터럽트 처리 완료를 알림 (다음 TIMER0 인터럽트를 받기 위해 필수)
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}

//
// End of File
//
