from serial          import Serial
from argparse        import ArgumentParser
from pathlib         import Path
from yaspin          import yaspin
from yaspin.spinners import Spinners
import os

descString = '''Tool for managing the internal flash of YARs bob series of boards.
Default behaviour is to download data from ttyACM0 and save it into ./data.
This may be customised with the arguments below.'''

# Funny spinny thing :P
def printAlive(i):
    spinny = "\\|/-"
    print("\b" + spinny[i % len(spinny)])


parser = ArgumentParser(prog='bobTool',
                        description=descString,
                        epilog='Claire Hinton, 2023')
parser.add_argument('tty', default='/dev/ttyACM0', help="The serial port that bob is connected to")
parser.add_argument('outDir', default='./data', help="The directory to save data to.")
parser.add_argument('-c', '--clear', action='store_true', help="Clear the flash. Will prompt for confirmation unless -f is set")
parser.add_argument('-f', '--force', action='store_true', help="Skips asking for confirmation.")
args = parser.parse_args()

tty = Serial(args.tty, 9600, timeout=1);

if not args.clear:
    tty.write(b'r')

    line = tty.readline()
    split = dict()
    records = 0

    # Iterate until we run out of data
    with yaspin(Spinners.bouncingBar) as sp:
        while b"\n" in line:
            # Housekeeping
            records = records + 1
            sp.text = str(records) + " records read."

            key  = line.split(b',', 1)[0]
            data = line.split(b',', 1)[1]

            if key in split:
                split[key] = split[key] + data
            else:
                split[key] = data

            line = tty.readline()

    print("\n"+ str(records) + " records read! Outputting to " + str(args.outDir))

    if not os.path.exists(args.outDir):
        os.makedirs(args.outDir)

    for key in split.keys():
        with open(str(args.outDir) + "/" + key.decode('ascii') + ".csv", "a") as text_file:
            text_file.write(split[key].decode('ascii'))
