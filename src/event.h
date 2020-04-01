#ifndef EVENT_H
#define EVENT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* All types of events. */
typedef enum
{
  DO_EVENT = 0,
  MODBUS_EVENT,
  DI_EVENT,
  SW_EVENT,
  AI1_EVENT,
  AI2_EVENT,
  AI3_EVENT,
  AI4_EVENT,
  BAT_EVENT,
  AO1_EVENT,
  AO2_EVENT,
  COUNTER1_EVENT, /* On Counter increment event */
  COUNTER2_EVENT,
  COUNTER3_EVENT,
  COUNTER4_EVENT,
  COUNTER5_EVENT,
  COUNTER6_EVENT,
  COUNTER7_EVENT,
  COUNTER8_EVENT,
  EXT0_EVENT,    /* On Change of the frame */
  EXT1_EVENT,
  EXT2_EVENT,
  EXT3_EVENT,
  EXT4_EVENT,
  EXT5_EVENT,
  EXT6_EVENT,
  EXT7_EVENT,
  EXT8_EVENT,
  EXT9_EVENT,
  EXT10_EVENT,
  EXT11_EVENT,
  EXT12_EVENT,
  EXT13_EVENT,
  EXT14_EVENT,
  EXT15_EVENT,
  EVENT_TYPE_NBR, /* Should always be last to define the number of event type */
}event_type_t;

/* Event mode */
typedef enum
{
  ON_CHANGE,
  ON_TIME,
}event_mode_t;

enum
{
  EVENT_OK,
  ERR_EVENT_TYPE_INVALID = -1,
};

void event_init(void);

int event_attach (event_type_t evtype, event_mode_t evmode, uint32_t evperiod);

int event_remove(event_type_t evtype);

void event_loop(void);

int event_getstatus(event_type_t evtype);

int event_getmode(event_type_t evtype);

int event_geteventnbr(event_type_t evtype);

#endif // EVENT_H
