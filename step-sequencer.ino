//accelstepper
#include <AccelStepper.h>
AccelStepper stepper; // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
//AccelStepper stepper(AccelStepper::HALF4WIRE);

void setup()
{
  stepper.setMaxSpeed(500);
  stepper.setSpeed(200);
}

void loop()
{
  stepper.runSpeed();
}
