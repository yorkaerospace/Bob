#ifndef ANSI_H
#define ANSI_H
/* A selection of macros for generating ANSI escape sequences */

// Moves the cursor n spaces from the end and clears whatever is in its way
#define MOV_EC(n) "\x1b[0K\x1b[999C\x1b[" #n "D"
// Moves the cursor to row, column
#define MOV(r, c) "\x1b[" #r ":" #c "H"
// Moves the cursor to column c
#define CHA(c) "\x1b[" #c "G"
// Clears from the cursor to the end of the line. Inserts a newline.
#define CLRLN "\x1b[0k\n"
// Clears screen
#define CLRSC "\x1b[2J"
// Sets text to be rendered neither bold nor faint
#define NORM  "\x1b[22m"
// Shows the cursor
#define SHOWC "\x1b[25h"
// Hides the cursor
#define HIDEC "\x1b[25l"


// Colours
#define WHITE "\x1b[0;1m"
#define RED   "\x1b[31;1m"
#define GREEN "\x1b[32;1m"
#define BLUE  "\x1b[34;1m"

// Save and restore cursor positions
#define SCP "\x1b[s"
#define RCP "\x1b[u"

#define MISSILE "The missile knows where it is at all times. It knows this because it knows where it isn't. By subtracting where it is from where it isn't, or where it isn't from where it is - whichever is greater - it obtains a difference or deviation. The guidance subsystem uses deviation to generate corrective commands to drive the missile from a position where it is to a position where it isn't, and arriving at a position that it wasn't, it now is. Consequently, the position where it is is now the position that it wasn't, and if follows that the position that it was is now the position that it isn't. In the event that the position that the position that it is in is not the position that it wasn't, the system has acquired a variation. The variation being the difference between where the missile is and where it wasn't. If variation is considered to be a significant factor, it too may be corrected by the GEA. However, the missile must also know where it was. The missile guidance computer scenario works as follows: Because a variation has modified some of the information that the missile has obtained, it is not sure just where it is. However, it is sure where it isn't, within reason, and it know where it was. It now subtracts where it should be from where it wasn't, or vice versa. And by differentiating this from the algebraic sum of where it shouldn't be and where it was, it is able to obtain the deviation and its variation, which is called error."

#endif
