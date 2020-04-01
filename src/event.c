#include "event.h"
#include "debug.h"

#define CONFIG_EXTMNG_MAXEXTS 16
#define CONFIG_EXTMNG_LEN     21

#define EXT_NBR             CONFIG_EXTMNG_MAXEXTS
#define NO_EVENT_LEN        0
#define DIGITAL_EVENT_LEN   4
#define ANALOG_EVENT_LEN    4
#define MODBUS_EVENT_LEN    256
#define POWER_EVENT_LEN     0
#define EXT_EVENT_LEN       CONFIG_EXTMNG_LEN
#define COUNTER1_EVENT_LEN  8

#define getindex(x, y) if ((x>=y)) return ((x- y));


/* This Creates a type event_handler_ft */
typedef void (*event_handler_ft)(void *, bool);

/* an event */
typedef struct
{
  event_handler_ft handler;
  event_type_t type;
  event_mode_t mode;
  uint32_t period;    /* In seconds. */
  uint32_t number;    /* number of event stored. */
  uint32_t lastvalue; /* last value for data events */
  uint8_t *buffer;    /* last read buffer for long data */
  bool attached;
}event_t;

static void event_dig_input(void *, bool store);
static void event_dig_output(void *, bool store);
static void event_ana_input(void *, bool store);
static void event_ana_output(void *, bool store);
static void event_counter_input(void *, bool store);
static void event_extension(void *, bool store);
static void event_modbus(void *, bool store);

/* Event list */
static event_t gevent_list[EVENT_TYPE_NBR];

static event_handler_ft gevhandler_list[] = 
{
  event_dig_output,    /* DO_EVENT */
  event_modbus,        /* MODBUS_EVENT */
  event_dig_input,     /* DI_EVENT */
  event_dig_input,     /* SW_EVENT */
  event_ana_input,     /* BAT_EVENT */
  event_ana_input,     /* AI1_EVENT */
  event_ana_input,     /* AI2_EVENT */
  event_ana_input,     /* AI3_EVENT */
  event_ana_input,     /* AI4_EVENT */
  event_ana_output,    /* AO1_EVENT */
  event_ana_output,    /* AO2_EVENT */
  event_counter_input, /* COUNTER1_EVENT */
  event_counter_input, /* COUNTER2_EVENT */
  event_counter_input, /* COUNTER3_EVENT */
  event_counter_input, /* COUNTER4_EVENT */
  event_counter_input, /* COUNTER5_EVENT */
  event_counter_input, /* COUNTER6_EVENT */
  event_counter_input, /* COUNTER7_EVENT */
  event_counter_input, /* COUNTER8_EVENT */
  event_extension,     /* EXT0_EVENT */
  event_extension,     /* EXT1_EVENT */
  event_extension,     /* EXT2_EVENT */
  event_extension,     /* EXT3_EVENT */
  event_extension,     /* EXT4_EVENT */
  event_extension,     /* EXT5_EVENT */
  event_extension,     /* EXT6_EVENT */
  event_extension,     /* EXT7_EVENT */
  event_extension,     /* EXT8_EVENT */
  event_extension,     /* EXT9_EVENT */
  event_extension,     /* EXT10_EVENT */
  event_extension,     /* EXT11_EVENT */
  event_extension,     /* EXT12_EVENT */
  event_extension,     /* EXT13_EVENT */
  event_extension,     /* EXT14_EVENT */
  event_extension,     /* EXT15_EVENT */
};

static uint8_t extension_buffer[EXT_NBR][EXT_EVENT_LEN];
static uint8_t modbus_buffer[MODBUS_EVENT_LEN];

static uint32_t current_time = 0;

static int get_evtypeindex(event_type_t evtype)
{
  getindex(evtype, EXT0_EVENT);
  getindex(evtype, COUNTER1_EVENT);
  getindex(evtype, AO1_EVENT);
  getindex(evtype, AI1_EVENT);
  getindex(evtype, DI_EVENT);

  /* In none of the above  */
  return 0;
}

static void print_event(void *ev, int index)
{
  event_t *event = (event_t *)ev;

  event_dbg_msg("Handler EvType: %d at id: %d, ", event->type, index);
}

static void compare_store_array_event(event_t *event, uint8_t *buffer, uint32_t len)
{ 
  int i, j;

  for (i = 0; i < len; i++)
  {
    if (buffer[i] != event->buffer[i])
    {
      /* Get current UTC time. */
      
      /* open file. */
      
      /* Write TIME:BUFFER */

      /* Close file */
      event_dbg_msg("Event: Store Time: %ds, Data: %d\n",current_time, buffer[0]);

      /* copy buffer locally */
      //memcpy(event->buffer, buffer, len);

      event->number++;
      break;
    }
  }

}

static void compare_store_value_event(event_t *event, uint32_t currval, uint32_t datalen)
{
  /* Compare with last value */
  if (currval != event->lastvalue)
  {
    /* Get current UTC time. */
    
    /* open file. */
    
    /* Write TIME:DATA */

    /* Close file */
    event_dbg_msg("Event: Store Time: %ds, Data: %d\n",current_time, currval);
    event->lastvalue = currval;
    event->number++;
  }
}

static void read_current_value(uint32_t *val, int index)
{
  /* Function for testing only, each module will call its own reading method */
}

static void read_current_buffer(uint8_t *buff, int index)
{
  /* Function for testing only, each module will call its own reading method */
}

static void event_dig_input(void *ev, bool store)
{
  event_t *event = (event_t *)ev;
  int index = get_evtypeindex(event->type);

  uint32_t currentval;

  if (store)
  {
    print_event(ev, index);
    read_current_value(&currentval, index);
    compare_store_value_event(event, currentval, DIGITAL_EVENT_LEN);
  }
  else
  {
    /* Update the last value */
    read_current_value(&event->lastvalue, index);
  }

}

static void event_dig_output(void *ev, bool store)
{
  event_t *event = (event_t *)ev;
  int index = get_evtypeindex(event->type);

  uint32_t currentval;

  if (store)
  {
    print_event(ev, index);
    read_current_value(&currentval, index);
    compare_store_value_event(event, currentval, DIGITAL_EVENT_LEN);
  }
  else
  {
    /* Update the last value */
    read_current_value(&event->lastvalue, index);
  }
}

static void event_ana_input(void *ev, bool store)
{
  event_t *event = (event_t *)ev;
  int index = get_evtypeindex(event->type);

  uint32_t currentval = 6;

  if (store)
  {
    print_event(ev, index);
    read_current_value(&currentval, index);
    compare_store_value_event(event, currentval, ANALOG_EVENT_LEN);
  }
  else
  {
    /* Update the last value */
    read_current_value(&event->lastvalue, index);
  }
}

static void event_ana_output(void *ev, bool store)
{
  event_t *event = (event_t *)ev;
  int index = get_evtypeindex(event->type);

  uint32_t currentval = 5;

  if (store)
  {
    print_event(ev, index);
    read_current_value(&currentval, index);
    compare_store_value_event(event, currentval, ANALOG_EVENT_LEN);
  }
  else
  {
    /* Update the last value */
    read_current_value(&event->lastvalue, index);
  }
}

static void event_counter_input(void *ev, bool store)
{
  event_t *event = (event_t *)ev;
  int index = get_evtypeindex(event->type);

  uint32_t currentval = 1;

  if (store)
  {
    print_event(ev, index);
    read_current_value(&currentval, index);
    compare_store_value_event(event, currentval, COUNTER1_EVENT_LEN);
  }
  else
  {
    /* Update the last value */
    read_current_value(&event->lastvalue, index);
  }
}

static void event_extension(void *ev, bool store)
{
  event_t *event = (event_t *)ev;
  int index = get_evtypeindex(event->type);

  uint8_t *currentbuff;

  if (store)
  {
    print_event(ev, index);
    read_current_buffer(currentbuff, index);
    compare_store_array_event(event, currentbuff, EXT_EVENT_LEN);
  }
  else
  {
    read_current_buffer(event->buffer, index);
    /* update the buffer */
  }

}

static void event_modbus(void *ev, bool store)
{
  event_t *event = (event_t *)ev;
  int index = get_evtypeindex(event->type);


  uint8_t *currentbuff;

  /* Read Current value.*/
  /* It is not a good idea to store the full length 256 */
  /* But only the length requested. */

  if (store)
  {
    print_event(ev, index);
    read_current_buffer(currentbuff, index);
    compare_store_array_event(event, currentbuff, MODBUS_EVENT_LEN);
  }
  else
  {
    read_current_buffer(event->buffer, index);
    /* update the buffer */
  }
}


static bool isevtype_valid(event_type_t evtype)
{
  return ((evtype >= EVENT_TYPE_NBR) || (evtype < 0));
}

static bool restoreconfig(event_t * evlist)
{
  /* Restore the config of the list*/
  /* Restore: Attach, mode, period */
  return false;
}

void event_init()
{
  int i = 0;
  event_t *evlist = gevent_list;
  event_handler_ft *evhandler = gevhandler_list;
  event_handler_ft handler;

  /* Read config from the Flash. */
  /* If it exsist if not, initialize it. */
  if (restoreconfig(evlist))
  {
    event_dbg_msg("Event: Restore config found.\n");

    /* Get last value from SRAM */
    for (i = 0; i < EVENT_TYPE_NBR; i++)
    {
      evlist[i].type = i;
      evlist[i].handler = evhandler[i];

      /* Update the last value and number without store. */
      handler = evlist[i].handler;
      handler(&evlist[i], false);

    }

    for (i = EXT0_EVENT; i <= EXT15_EVENT; i++)
    {
      evlist[i].buffer = extension_buffer[i - EXT0_EVENT];

      /* this should be a memcpy from the SRAM last value. */
      memset(evlist[i].buffer, '\0', EXT_EVENT_LEN);
    }

    evlist[MODBUS_EVENT].buffer = modbus_buffer;
    /* this should be a memcpy from the SRAM last value. */
    memset(evlist[MODBUS_EVENT].buffer, '\0', MODBUS_EVENT_LEN);
  }
  else
  {
    event_dbg_msg("Event: No stored config found.\n");

    for (i = 0; i < EVENT_TYPE_NBR; i++)
    {
      evlist[i].type = i;
      evlist[i].handler = evhandler[i];
      evlist[i].mode = ON_TIME;
      evlist[i].period = 0x01;
      evlist[i].number = 0;
      evlist[i].attached = false;

      /* Update the last value with the current value without store. */
      handler = evlist[i].handler;
      handler(&evlist[i], false);
    }

    /* Initialize extensions buffers. */

    for (i = EXT0_EVENT; i <= EXT15_EVENT; i++)
    {
      evlist[i].buffer = extension_buffer[i - EXT0_EVENT];
    }

    /* Initialize modbus buffer. */

    evlist[MODBUS_EVENT].buffer = modbus_buffer;
  }

  event_dbg_msg("Event Init.\n");
}

int event_attach (event_type_t evtype, event_mode_t evmode, uint32_t evperiod)
{
  event_t *evlist = gevent_list;

  if (isevtype_valid(evtype))
  {
    return ERR_EVENT_TYPE_INVALID;
  }

  evlist[evtype].attached = true;
  evlist[evtype].mode = evmode;
  evlist[evtype].period = evperiod;

  event_dbg_msg("Event Attach: type %d\n", evlist[evtype].type);

  return EVENT_OK;
}

int event_remove(event_type_t evtype)
{
  event_t *evlist = gevent_list;

  if (isevtype_valid(evtype))
  {
    return ERR_EVENT_TYPE_INVALID;
  }

  evlist[evtype].attached = false;
  evlist[evtype].number = 0;
  evlist[evtype].lastvalue = 0;
  event_dbg_msg("Event Removed %d\n", evlist[evtype].type);

  return EVENT_OK;
}

/* Called Exactly every one second. */

void event_loop(void)
{
  int i;
  event_t *evlist = gevent_list;
  event_handler_ft  handler;

  /* Current time is one second based increment. */

  current_time++;

  for (i = 0; i < EVENT_TYPE_NBR; i++)
  {
    if (evlist[i].attached)
    {
      if ((current_time % evlist[i].period) == 0U)
      {
        /* Call event handler and store if any change.*/
        handler = evlist[i].handler;
        handler(&evlist[i], true);
      }
    }
  }
}

int event_geteventnbr(event_type_t evtype)
{
  event_t *evlist = gevent_list;

  if (isevtype_valid(evtype))
  {
    return ERR_EVENT_TYPE_INVALID;
  }

  return evlist[evtype].number;
}

/* Getters */

/**
 * @brief get the attach status of an eventtype
 *
 * @param evtype, the event type
 *
 * @return true if attached false if not.
 *         a negative error code if fails.
 */
int event_getstatus(event_type_t evtype)
{
  event_t *evlist = gevent_list;

  if (isevtype_valid(evtype))
  {
    return ERR_EVENT_TYPE_INVALID;
  }

  return evlist[evtype].attached;
}

int event_getmode(event_type_t evtype)
{
  event_t *evlist = gevent_list;

  if (isevtype_valid(evtype))
  {
    return ERR_EVENT_TYPE_INVALID;
  }

  return evlist[evtype].mode;
}

