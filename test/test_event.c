#include "unity.h"

#include "event.h"

void setUp(void)
{
  event_init();
}

void tearDown(void)
{
}

#if 0
void test_event_AttachRemoveEvent(void)
{


  if (event_attach(DI_EVENT, ON_TIME, 4) != 0)
  {
    TEST_ASSERT_EQUAL(0, event_getstatus(DI_EVENT));
  }
  else
  {
    TEST_ASSERT_EQUAL(1, event_getstatus(DI_EVENT));
    TEST_ASSERT_EQUAL(ON_TIME, event_getmode(DI_EVENT));
  }



  if (event_remove(DI_EVENT) != 0)
  {
    TEST_ASSERT_EQUAL(1, event_getstatus(DI_EVENT));
  }
  else
  {
    TEST_ASSERT_EQUAL(0, event_getstatus(DI_EVENT));
  }
}
#endif
void test_event_RegisterDigitalinputEvent(void)
{

  int evnbr = 0;

  if (event_attach(AI2_EVENT, ON_TIME, 2) != 0)
  {
    TEST_ASSERT_EQUAL(0, event_getstatus(DI_EVENT));
  }

  if (event_attach(COUNTER8_EVENT, ON_TIME, 3) != 0)
  {
    TEST_ASSERT_EQUAL(0, event_getstatus(DI_EVENT));
  }

  /* Loop through no event happened */
  /* First second */
  event_loop();

  /* no event yet */
  evnbr = event_geteventnbr(AI2_EVENT);
  TEST_ASSERT_EQUAL(0, evnbr);

  /* Second second */
  event_loop();
  
  /* An event should happen */
  evnbr = event_geteventnbr(AI2_EVENT);
  TEST_ASSERT_EQUAL(1, evnbr);

  /* Third second */
  event_loop();

  /* An event should happen */
  evnbr = event_geteventnbr(COUNTER8_EVENT);
  TEST_ASSERT_EQUAL(1, evnbr);
}
