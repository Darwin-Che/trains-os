use volatile_register::{RW, RO};
use crate::api::clock::*;

type GpioSetting = u32;
type GpioResistor = u32;

pub const GPIO_SETTING_INPUT : GpioSetting = 0x00;
pub const GPIO_SETTING_OUTPUT : GpioSetting = 0x01;
pub const GPIO_SETTING_ALTFN0 : GpioSetting = 0x04;
pub const GPIO_SETTING_ALTFN1 : GpioSetting = 0x05;
pub const GPIO_SETTING_ALTFN2 : GpioSetting = 0x06;
pub const GPIO_SETTING_ALTFN3 : GpioSetting = 0x07;
pub const GPIO_SETTING_ALTFN4 : GpioSetting = 0x03;
pub const GPIO_SETTING_ALTFN5 : GpioSetting = 0x02;

pub const GPIO_RESISTOR_NONE : GpioResistor = 0x00;
pub const GPIO_RESISTOR_PUP : GpioResistor = 0x01;
pub const GPIO_RESISTOR_PDP : GpioResistor = 0x02;

#[link(name = "base")]
extern "C" {
    // void uart_init(uint32_t id, uint32_t baudrate);
    fn uart_init(id: u32, baudrate: u32);
    // void setup_gpio(uint32_t pin, uint32_t setting, uint32_t resistor)
    pub fn setup_gpio(pin: u32, setting: u32, resistor: u32);
    // void set_outpin_gpio(uint32_t pin)
    pub fn set_outpin_gpio(pin: u32);
    // void clear_outpin_gpio(uint32_t pin)
    pub fn clear_outpin_gpio(pin: u32);
}

const UART_CLK: u64 = 48000000;

const UART_CR_UARTEN: u32 = 0x01;
const UART_CR_LBE: u32 = 0x80;
const UART_CR_TXE: u32 = 0x100;
const UART_CR_RXE: u32 = 0x200;
const UART_CR_RTS: u32 = 0x800;
const UART_CR_RTSEN: u32 = 0x4000;
const UART_CR_CTSEN: u32 = 0x8000;

const UART_LCRH_PEN: u32 = 0x2;
const UART_LCRH_EPS: u32 = 0x4;
const UART_LCRH_STP2: u32 = 0x8;
const UART_LCRH_FEN: u32 = 0x10;
const UART_LCRH_WLEN_LOW: u32 = 0x20;
const UART_LCRH_WLEN_HIGH: u32 = 0x40;

const UART_IMSC_CTSMIM: u32 = 0x2;
const UART_IMSC_RXIM: u32 = 0x10;
const UART_IMSC_TXIM: u32 = 0x20;
const UART_IMSC_RTIM: u32 = 0x40;
const UART_IMSC_FEIM: u32 = 0x80;
const UART_IMSC_PEIM: u32 = 0x100;
const UART_IMSC_BEIM: u32 = 0x200;
const UART_IMSC_OEIM: u32 = 0x400;
const UART_IMSC_FULL: u32 = 0x7F2;

#[repr(C)]
pub struct RpiUart {
    pub DR: RW<u8>,
    DR_unused: [u8; 3],
    pub RSRECR: u32,
    RSRECR_unused: [u8; 16],
    pub FR: RW<u32>,
    FR_unused: [u8; 4],
    pub ILPR: u32,
    pub IBRD: RW<u32>,
    pub FBRD: RW<u32>,
    pub LCRH: RW<u32>,
    pub CR: RW<u32>,
    pub IFLS: u32,
    pub IMSC: RW<u32>,
    pub RIS: u32,
    pub MIS: u32,
    pub ICR: RW<u32>,
    pub DMACR: u32,
    DMACR_unused: [u8; 52],
    pub ITCR: u32,
    pub ITIP: u32,
    pub ITOP: u32,
    pub TDR: u32,
}

#[derive(Default)]
struct RpiUartGpioPins {
    pub tx: u32,
    pub rx: u32,
    pub cts: u32,
    pub rts: u32,
    pub setting: u32,
    pub resistor: u32,
}

static RPI_UART_GPIO_PINS: &'static [RpiUartGpioPins] = &[
    RpiUartGpioPins{tx: 32, rx: 33, cts: 30, rts: 31, setting: GPIO_SETTING_ALTFN3, resistor: GPIO_RESISTOR_NONE},
    RpiUartGpioPins{tx: 14, rx: 15, cts: 16, rts: 17, setting: GPIO_SETTING_ALTFN5, resistor: GPIO_RESISTOR_NONE},
    RpiUartGpioPins{tx: 0, rx: 1, cts: 2, rts: 3, setting: GPIO_SETTING_ALTFN4, resistor: GPIO_RESISTOR_NONE},
    RpiUartGpioPins{tx: 4, rx: 5, cts: 6, rts: 7, setting: GPIO_SETTING_ALTFN4, resistor: GPIO_RESISTOR_NONE},
    RpiUartGpioPins{tx: 8, rx: 9, cts: 10, rts: 11, setting: GPIO_SETTING_ALTFN4, resistor: GPIO_RESISTOR_NONE},
    RpiUartGpioPins{tx: 12, rx: 13, cts: 14, rts: 15, setting: GPIO_SETTING_ALTFN4, resistor: GPIO_RESISTOR_NONE},
];

impl RpiUart {
    pub fn new(uart_id: u32, baudrate: u64, flow_control: bool) -> &'static mut RpiUart {
        assert!(uart_id != 1);

        // Setup GPIO
        let pins = &RPI_UART_GPIO_PINS[uart_id as usize];

        unsafe {
            setup_gpio(pins.tx, pins.setting, pins.resistor);
            setup_gpio(pins.rx, pins.setting, pins.resistor);
            if flow_control {
                setup_gpio(pins.cts, pins.setting, pins.resistor);
                setup_gpio(pins.rts, pins.setting, pins.resistor);
            }
        }

        let uart = unsafe {
            &mut *((0xfe201000 + 0x200 * uart_id) as *mut RpiUart)
        };

        let baud_divisor = ((UART_CLK * 4) / baudrate) as u32;

        unsafe {
            uart.CR.write(0x0);
            uart.ICR.write(0x0);
            uart.LCRH.write(UART_LCRH_WLEN_HIGH | UART_LCRH_WLEN_LOW | UART_LCRH_FEN);
            uart.IBRD.write(baud_divisor >> 6);
            uart.FBRD.write(baud_divisor & 0x3f);
            uart.CR.write(UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE | UART_CR_RTS);
        }

        uart
    }

    pub fn drain(&self) {
        while self.FR.read() & 0x10 == 0 {
            self.DR.read();
        }
    }

    pub fn getc(&self) -> Option<u8> {
        if self.FR.read() & 0x10 != 0 {
            None
        } else {
            Some(self.DR.read())
        }
    }

    pub fn putc(&self, c: u8) -> Option<u8> {
        if self.FR.read() & 0x20 != 0 {
            None
        } else {
            unsafe {
                self.DR.write(c);
            }
            Some(c)
        }
    }

    pub fn arm_rx(&self) {
        let mut status = self.IMSC.read();
        status = status | (UART_IMSC_RXIM | UART_IMSC_RTIM);
        unsafe {
            self.IMSC.write(status);
        }
    }

    pub fn arm_tx(&self) {
        let mut status = self.IMSC.read();
        status = status | UART_IMSC_TXIM;
        unsafe {
            self.IMSC.write(status);
        }
    }
}

#[repr(C)]
pub struct RpiClock {
    pub CS: RW<u32>,
    pub CLO: RW<u32>,
    pub CHI: RW<u32>,
    pub C0: RW<u32>,
    pub C1: RW<u32>,
    pub C2: RW<u32>,
    pub C3: RW<u32>,
}

impl RpiClock {
    pub fn new() -> &'static mut RpiClock {
        unsafe {
            &mut *(0xFE003000 as *mut RpiClock)
        }
    }

    pub fn cur_u64(&self) -> u64 {
        let clo = self.CLO.read();
        let chi = self.CHI.read();
        ((chi as u64) << 32) | (clo as u64)
    }

    pub fn intr_arm(&self, future: u64) {
        unsafe {
            self.C1.write(future as u32);
        }
    }

    pub fn intr_clear(&self) {
        unsafe {
            self.CS.write(self.CS.read() | (0x01 << 1));
            self.C1.write(0);
        }
    }
}

#[repr(C)]
pub struct RpiPwmReg {
    pub CTL: RW<u32>,
    pub STA: RW<u32>,
    pub DMAC: RW<u64>,
    pub RNG1: RW<u32>,
    pub DAT1: RW<u32>,
    pub FIF1: RW<u64>,
    pub RNG2: RW<u32>,
    pub DAT2: RW<u32>,
}

#[derive(Default)]
struct RpiPwmConfig {
    pub pin: u32,
    pub setting: u32,
    pub id: u32, // 0 or 1
    pub channel: u32, // 0 or 1
}

static RPI_PWM_CONFIG: &'static [RpiPwmConfig] = &[
    RpiPwmConfig{pin: 12, setting: GPIO_SETTING_ALTFN0, id: 0, channel: 0},
    RpiPwmConfig{pin: 13, setting: GPIO_SETTING_ALTFN0, id: 0, channel: 1},
    RpiPwmConfig{pin: 18, setting: GPIO_SETTING_ALTFN5, id: 0, channel: 0},
    RpiPwmConfig{pin: 19, setting: GPIO_SETTING_ALTFN5, id: 0, channel: 1},
];

pub struct RpiPwm {
    pub id: u32,
    pub channel: u32,
    pub reg: &'static mut RpiPwmReg,
}

pub struct RpiGpClock {
    pub ctl: RW<u32>,
    pub div: RW<u32>,
}

const CLOCK_PASSWORD: u32 = 0x5A000000;
const CLOCK_CTL_ENAB: u32 = 1 << 4;
const CLOCK_CTL_KILL: u32 = 1 << 5;
const CLOCK_CTL_BUSY: u32 = 1 << 7;

const CLOCK_SRC_OSCI: u32 = 1 << 0;

impl RpiPwm {
    pub fn global_setup() {
        unsafe {
            let clock = &mut *(0xFE1010A0 as *mut RpiGpClock);
            clock.ctl.write(CLOCK_PASSWORD | CLOCK_CTL_KILL);
            while clock.ctl.read() & CLOCK_CTL_BUSY != 0 {
                wait_ticks(1);
            }
            clock.div.write(CLOCK_PASSWORD | (1 << 12));
            clock.ctl.write(CLOCK_PASSWORD | CLOCK_CTL_ENAB | CLOCK_SRC_OSCI);
            while clock.ctl.read() & CLOCK_CTL_BUSY == 0 {
                wait_ticks(1);
            }

            let reg = &mut *(0xFE20C000 as *mut RpiPwmReg);
            reg.CTL.write((0x01 << 0) | (0x01 << 8));
        }
    }

    pub fn new(pin: u32, M: u32) -> Self {
        let mut config_find = None;
        for c in RPI_PWM_CONFIG {
            if c.pin == pin {
                config_find = Some(c);
            }
        }

        let config = config_find.unwrap();

        unsafe {
            setup_gpio(config.pin, config.setting, GPIO_RESISTOR_NONE);
        }

        let mut s = Self {
            id: config.id,
            channel: config.channel,
            reg: unsafe {
                &mut *(0xFE20C000 as *mut RpiPwmReg)
            },
        };

        if s.channel == 0 {
            unsafe {
                s.reg.RNG1.write(M);
            }
        } else if s.channel == 1 {
            unsafe {
                s.reg.RNG2.write(M);
            }
        }

        s
    }

    pub fn set(&mut self, N: u32) {
        if self.channel == 0 {
            unsafe {
                self.reg.DAT1.write(N);
            }
        } else if self.channel == 1 {
            unsafe {
                self.reg.DAT2.write(N);
            }
        }
    }
}