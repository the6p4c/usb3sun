#![no_std]
#![no_main]

use defmt_rtt as _;
use panic_probe as _;

use defmt::info;
use embedded_graphics::prelude::*;
use embedded_hal::digital::{OutputPin, PinState};
use rp_pico::hal::clocks::init_clocks_and_plls;
use rp_pico::hal::fugit::RateExtU32;
use rp_pico::hal::gpio::{FunctionI2C, Pin};
use rp_pico::hal::{Clock, Sio, Watchdog};
use rp_pico::{entry, pac, Pins};
use ssd1306::prelude::*;
use ssd1306::{I2CDisplayInterface, Ssd1306};

mod display;

use display::Display;

#[entry]
fn main() -> ! {
    let mut pac = pac::Peripherals::take().unwrap();
    let core = pac::CorePeripherals::take().unwrap();
    let mut watchdog = Watchdog::new(pac.WATCHDOG);
    let sio = Sio::new(pac.SIO);

    let external_xtal_freq_hz = 12_000_000u32;
    let clocks = init_clocks_and_plls(
        external_xtal_freq_hz,
        pac.XOSC,
        pac.CLOCKS,
        pac.PLL_SYS,
        pac.PLL_USB,
        &mut pac.RESETS,
        &mut watchdog,
    )
    .unwrap();

    let mut delay = cortex_m::delay::Delay::new(core.SYST, clocks.system_clock.freq().to_Hz());

    /******
     * io *
     ******/
    let pins = Pins::new(
        pac.IO_BANK0,
        pac.PADS_BANK0,
        sio.gpio_bank0,
        &mut pac.RESETS,
    );

    let mut led_pin = pins.led.into_push_pull_output_in_state(PinState::Low);
    let mut display_enable_pin = pins.gpio12.into_push_pull_output_in_state(PinState::Low);
    let display_sda_pin: Pin<_, FunctionI2C, _> = pins.gpio16.reconfigure();
    let display_scl_pin: Pin<_, FunctionI2C, _> = pins.gpio17.reconfigure();

    /***********
     * display *
     ***********/
    let display_i2c = rp_pico::hal::I2C::i2c0(
        pac.I2C0,
        display_sda_pin,
        display_scl_pin,
        400.kHz(),
        &mut pac.RESETS,
        &clocks.system_clock,
    );
    let mut display = Ssd1306::new(
        I2CDisplayInterface::new(display_i2c),
        DisplaySize128x32,
        DisplayRotation::Rotate0,
    )
    .into_buffered_graphics_mode();

    // see src/pinout.cc:74
    delay.delay_ms(30);
    display_enable_pin.set_high().unwrap();
    delay.delay_ms(30);

    display.init().unwrap();

    /********
     * main *
     ********/
    Display {
        clk: false,
        bel: false,
        cap: true,
        cmp: false,
        scr: false,
        num: false,
        buzzer: true,
    }
    .draw(&mut display)
    .unwrap();

    display.flush().unwrap();

    loop {
        info!("on!");
        led_pin.set_high().unwrap();
        delay.delay_ms(500);

        info!("off!");
        led_pin.set_low().unwrap();
        delay.delay_ms(500);
    }
}
