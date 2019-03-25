//button
static const int button_pin = 10;

//stepper
#include <AccelStepper.h>
//28BYJ-48 Stepper Motor
#define STEPS_PER_REV           (2048.0)
// speed (rpm) * steps-per-revolution == speed (steps per minute)
//  --> speed (steps per minute) / 60 == speed (steps per second)
//  --> speed (steps per second) * 60 / steps-per-revolution == speed (rpm)
#define STEPS_PER_SEC_TO_RPM    (60.0 / STEPS_PER_REV)
#define RPM_TO_STEPS_PER_SEC    (STEPS_PER_REV / 60.0)

// parameter (torque-speed trade-off)
#define STEPS_PER_SEC_MAX       (500)
#define RPM_MAX                 (STEPS_PER_SEC_MAX * STEPS_PER_SEC_TO_RPM)

//
AccelStepper stepper(AccelStepper::FULL4WIRE, 2, 3, 4, 5);

//action list
static int action_now = 0;
#define ACTION_TAP_MAX 8
#define ACTION_COUNT 2
//actions
static int tap_idx = 0;
static int taps[ACTION_COUNT][ACTION_TAP_MAX][2] = { //unit: (steps, millisec)

  //action #1
  {
    {   200,  1000},
    {   400,  1000},
    {   600,  1000},
    {   800,  1000},
    {   900,  1000},
    {   650,  1000},
    {   800,  1000},
    {  1200,  1000}
  },

  // action #2
  {
    {  1000,  2000},
    {  3000,  2000},
    {  1000,  3000},
    { 10000,   100},
    {  3000,  2000},
    { 10000,  2500},
    {  1000,  1000},
    {   100,   400}
  }
};

//task
#include <TaskScheduler.h>
Scheduler runner;
extern Task stepping_task;
extern Task stop_task;
extern Task step_monitor_task; // DEBUG
extern Task button_task;
bool is_action_time = false;
//
void stepping() {
  //
  if (stepper.distanceToGo() == 0 && is_action_time == true) {

    //the last tap of the sequence is over! --> declare 'done'
    if (tap_idx >= ACTION_TAP_MAX) {
      // rewind the reel.
      tap_idx = 0;
      // cool. i'm done. give me next command.
      is_action_time = false;
      //
      return;
    }

    //
    float cur_step = stepper.currentPosition();
    float target_step = taps[action_now][tap_idx][0];
    float dur = taps[action_now][tap_idx][1];
    float steps = target_step - cur_step;
    float velocity = steps / dur * 1000; // unit conv.: (steps/msec) --> (steps/sec)
    float speed = fabs(velocity);
    //
    if (speed > STEPS_PER_SEC_MAX) {
      Serial.println("oh.. isn't it TOO FAST??");
    } else {
      Serial.println("okay. i go now.");
    }

    //DEBUG
    Serial.print(" --> tap_idx : "); Serial.println(tap_idx);
    Serial.print(" --> speed(steps/sec) : "); Serial.println(speed);
    Serial.print(" --> STEPS_PER_SEC_MAX(steps/sec) : "); Serial.println(STEPS_PER_SEC_MAX);

    //
    stepper.moveTo(target_step);
    stepper.setSpeed(velocity);
    //NOTE: 'setSpeed' should come LATER than 'moveTo'!
    //  --> 'moveTo' re-calculate the velocity.
    //  --> so we need to re-override it.
    //

    // reschedule myself..
    stepping_task.restartDelayed(dur);

    //next tap!
    tap_idx++;

  } else {
    //if the stepper was still busy.. well, relax for 50ms more.

    // reschedule myself..
    stepping_task.restartDelayed(50);
  }
}
Task stepping_task(0, TASK_ONCE, stepping);

// //
// void step_monitor() {
//   Serial.print("currentPosition: ");
//   Serial.println(stepper.currentPosition());
// }
// Task step_monitor_task(100, TASK_FOREVER, step_monitor);

// button
void button() {
  static int button_prev = HIGH;
  //
  int button = digitalRead(button_pin);
  if (button != button_prev && button == LOW) {
    Serial.println("oh! the button triggers!");
    //if (is_action_time == false && stepper.currentPosition() == 0) { // block re-trigger.
    if (is_action_time == false) { // block re-trigger.
      is_action_time = true;
      stepping_task.restart();
      Serial.println("stepper started!");
    } else {
      Serial.println("Er.... something is wrong!");
    }
  }
  //
  button_prev = button;
}
//the task
Task button_task(50, TASK_FOREVER, &button);

//
void setup() {
  //
  Serial.begin(9600);

  //button
  pinMode(button_pin, INPUT_PULLUP);

  //stepper
  /// "
  /// The fastest motor speed that can be reliably supported is about 4000 steps per
  /// second at a clock frequency of 16 MHz on Arduino such as Uno etc.
  /// " @ AccelStepper.h
  stepper.setMaxSpeed(STEPS_PER_SEC_MAX); //steps per second (trade-off between speed vs. torque)

  //tasks
  runner.init();
  runner.addTask(button_task);
  button_task.enable();
  runner.addTask(stepping_task);

  //DEBUG
  // runner.addTask(step_monitor_task);
  // step_monitor_task.enable();
}

void loop() {
  //task
  runner.execute();

  //stepper
  if (stepper.distanceToGo() != 0) {
    stepper.runSpeed();
  }
}
