# Bob Firmware
Rewrite of Bob's firmware, using platformIO. The main goal here is to make something simpler, which will hopefully be more reliable and easier to extend.

## Known issues with the old firmware:
 - Overuse of single use function calls.
 - Firmware was split into too many modules, which, combined with the single use functions, made it hard to keep track of program flow.
 - Design was generally bad. It included a lot of functions and states for features that would be implemented "eventually".
 - Used both cores. While in theory this allowed it to sample and log concurrently, in practice you can only write to flash while the other core sleeps.
 - The flash write code is pretty grim.

## Design of the new firmware:
 - Write individual samples to flash; flash writes are *way* faster than initially anticipated, so it may be possible to get away with this. It'd nicely sidestep all the issues we've been having with the flash buffer. The problem is that to do this nicely we effectively need to write 2 pages at once. Not sure what the speed hit from this will be, but I'm willing to take a lower sample rate if we actually get data.
 - Use one core; I am clearly not a good enough programmer to effectively use both. This'll also simplify any write buffer code, if we need it.
 - State machine has been reworked:
   - LOG: Logs data while unplugged.
   - DEBUG_LOG: Logs data while plugged in.
   - DEBUG_PRINT: Prints live data to console. Looks cool, but I'm a bit iffy on this one.
   - DATA_OUT: Prints recorded data as CSV.
   - PLUGGED_IN: Listens to commands over USB.
 - More states may be added once we actually get some flight data.
 - State machine has been moved into main. This makes main() a bit big but I'm not sure how else to do it.
