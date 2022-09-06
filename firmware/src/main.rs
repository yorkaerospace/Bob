#![no_std]
#![no_main]

use defmt::*;
use defmt_rtt as _;
use panic_probe as _;
use rp2040_hal as hal;

use hal::entry;
use hal::gpio;

use embedded_hal_compat::{ForwardCompat, ReverseCompat};
use fugit::RateExtU32;
use hp203b::HP203B;
use shared_bus::BusManagerSimple;

#[entry]
fn main() -> ! {
    info!("Program start");

    let mut perips = crate::Peripherals::take();
    loop {}
}
