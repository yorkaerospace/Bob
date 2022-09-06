#![no_std]
#![no_main]

use defmt::*;
use defmt_rtt as _;
use panic_probe as _;
use rp2040_hal as hal;

use hal::gpio;

use embedded_hal_compat::{ForwardCompat, ReverseCompat};
use fugit::RateExtU32;
use hp203b::HP203B;
use ouroboros::self_referencing;
use shared_bus::{BusManagerSimple, I2cProxy};

type I2C = hal::I2C<hal::pac::I2C0, (gpio::bank0::Gpio16, gpio::bank0::Gpio17)>;
type I2CHolder<'a> = I2cProxy<'a, I2C>;

// TODO: implement
pub struct Buzzer(gpio::bank0::Gpio5);

// TODO: fix ouroboros?
// TODO:
// - flash
// - IMU
// - magnetometer
#[self_referencing]
pub struct Peripherals {
    _i2c: BusManagerSimple<I2C>,
    #[borrows(_i2c)]
    pub altimeter_thermometer: HP203B<I2CHolder<'this>, hp203b::csb::CSBHigh>,
    pub buzzer: Buzzer,
    pub gpio0: gpio::bank0::Gpio0,
    pub gpio1: gpio::bank0::Gpio1,
    pub gpio2: gpio::bank0::Gpio2,
    pub gpio3: gpio::bank0::Gpio3,
    pub gpio4: gpio::bank0::Gpio4,
    pub gpio26: gpio::bank0::Gpio26,
    pub gpio27: gpio::bank0::Gpio27,
    pub gpio28: gpio::bank0::Gpio28,
    pub gpio29: gpio::bank0::Gpio29,
}

impl Peripherals {
    pub fn take(alti_osr: hp203b::OSR, alti_ch: hp203b::Channel) -> Self {
        info!("Initialising Bob Peripherals");

        let mut perips = hal::pac::Peripherals::take().unwrap();
        let sio = hal::Sio::new(perips.SIO);
        let pins = gpio::Pins::new(
            perips.IO_BANK0,
            perips.PADS_BANK0,
            sio.gpio_bank0,
            &mut perips.RESETS,
        );

        let i2c = BusManagerSimple::new(hal::I2C::new_controller(
            perips.I2C0,
            pins.gpio16.into_mode(),
            pins.gpio17.into_mode(),
            400.kHz(),
            &mut perips.RESETS,
            125_000_000.kHz(),
        ));

        info!("Initialising altimeter...");
        let mut altimeter =
            HP203B::<_, hp203b::csb::CSBHigh>::new(i2c.acquire_i2c().forward(), alti_osr, alti_ch)
                .unwrap();

        Self {
            _i2c: i2c,
            altimeter_thermometer: altimeter,
            buzzer: Buzzer(pins.gpio5),
            gpio0: pins.gpio0,
            gpio1: pins.gpio1,
            gpio2: pins.gpio2,
            gpio3: pins.gpio3,
            gpio4: pins.gpio4,
            gpio26: pins.gpio26,
            gpio27: pins.gpio27,
            gpio28: pins.gpio28,
            gpio29: pins.gpio29,
        }
    }
}
