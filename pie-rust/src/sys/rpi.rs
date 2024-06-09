use volatile_register::{RW, RO};
use core::arch::asm;

type GpioSetting = u32;
type GpioResistor = u32;

const GPIO_SETTING_INPUT : GpioSetting = 0x00;
const GPIO_SETTING_OUTPUT : GpioSetting = 0x01;
const GPIO_SETTING_ALTFN0 : GpioSetting = 0x04;
const GPIO_SETTING_ALTFN1 : GpioSetting = 0x05;
const GPIO_SETTING_ALTFN2 : GpioSetting = 0x06;
const GPIO_SETTING_ALTFN3 : GpioSetting = 0x07;
const GPIO_SETTING_ALTFN4 : GpioSetting = 0x03;
const GPIO_SETTING_ALTFN5 : GpioSetting = 0x02;

const GPIO_RESISTOR_NONE : GpioResistor = 0x00;
const GPIO_RESISTOR_PUP : GpioResistor = 0x01;
const GPIO_RESISTOR_PDP : GpioResistor = 0x02;

#[link(name = "base")]
extern "C" {
    // void uart_init(uint32_t id, uint32_t baudrate);
    fn uart_init(id: u32, baudrate: u32);
}

// fn setup_gpio(pin: u32, setting: GpioSetting, resistor: GpioResistor) {
//     let reg: u32 = pin / 10;
//     let shift: u32 = (pin % 10) * 3;
//     uint32_t status = gpio->GPFSEL[reg]; // read status
//     status &= ~(7u << shift);            // clear bits
//     status |= (setting << shift);        // set bits
//     gpio->GPFSEL[reg] = status;          // write back
  
//     reg = pin / 16;
//     shift = (pin % 16) * 2;
//     status = gpio->PUP_PDN_CNTRL_REG[reg]; // read status
//     status &= ~(3u << shift);              // clear bits
//     status |= (resistor << shift);         // set bits
//     gpio->PUP_PDN_CNTRL_REG[reg] = status; // write back
// }

#[repr(C)]
pub struct RpiUart {
    pub DR: RW<u8>,
    DR_unused: [u8; 3],
    pub RSRECR: u32,
    RSRECR_unused: [u8; 16],
    pub FR: RW<u32>,
    FR_unused: [u8; 4],
    pub ILPR: u32,
    pub IBRD: u32,
    pub FBRD: u32,
    pub LCRH: u32,
    pub CR: u32,
    pub IFLS: u32,
    pub IMSC: u32,
    pub RIS: u32,
    pub MIS: u32,
    pub ICR: u32,
    pub DMACR: u32,
    DMACR_unused: [u8; 52],
    pub ITCR: u32,
    pub ITIP: u32,
    pub ITOP: u32,
    pub TDR: u32,
}

impl RpiUart {
    pub fn new(uart_id: u32, baudrate: u32) -> &'static mut RpiUart {
        unsafe {
            uart_init(uart_id, baudrate);
            &mut *((0xfe201000 + 0x200 * uart_id) as *mut RpiUart)
        }
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
}