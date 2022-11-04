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
// Sets text to be rendered neither bold nor faint
#define NORM  "\x1b[22m"

// Colours
#define WHITE "\x1b[0;1m"
#define RED   "\x1b[31;1m"
#define GREEN "\x1b[32;1m"
#define BLUE  "\x1b[34;1m"

// Save and restore cursor positions
#define SCP "\x1b[s"
#define RCP "\x1b[u"

#endif
