#ifndef DATA
#define DATA

#define LAUNCH_FLAG 0x1
#define APOGEE_FLAG 0x2

typedef struct {
	int16_t accel[3];
	int16_t gyro[3];
	int16_t temp[2]; 	// Temperature in 1/10ths of a degree
	uint32_t press;		// Pressure in pascals
	uint32_t time;		// Timestamp in milliseconds. 
	uint8_t flags;		// Random bits of metadata. Flags are defined above.
} data_t;

/* Set a flag on a packet */
void setFlag(data_t *data, uint8_t flag);

/* Unset a flag on a packet */
void unsetFlag(data_t *data, uint8_t flag);

#endif
