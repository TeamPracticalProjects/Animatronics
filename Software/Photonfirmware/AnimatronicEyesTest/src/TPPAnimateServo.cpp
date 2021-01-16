/*
 * TPPAnimateServo.cpp
 * 
 * Team Practical Project animatronic library
 * 
 * Servos move on command. For animatronics we need to command a servo to move to 
 * some position with some amount of speed. This library wraps the AdaFruit_PWMServoDriver
 * to provide this functionality.
 * 
 * Instantiate this class, and it will create an instance of the AdaFruit PWM Servo Driver.
 * Key methods
 *      begin:  pass in the servo number on the AdaFruit servo driver board
 *      moveTo: pass in a target PWM duration and increment 
 *      process: called over and over to cause the servo to move from its current
 *              position to the new target position
 * 
 * For full documentation see https://github/TeamPracticalProjects/XXXX
 * 
 * (cc) Non-Commercial Share-Alike Attribution 2021 Bob Glicksman, Jim Schrempp
 * 
 */

#include <TPPAnimateServo.h>
#define TPPServo_DEBUG   // note that the debug prints in process() are not recommended because they
                            // slow the timer work

#define MS_BETWEEN_MOVES 10  // we will not move any particular servo more ofen than this
#define MAX_SERVOS 6

Logger logAniservo("app.aniservo");


/*------ begin -----
 * servoNum: based on the AdaFruit servo driver board
 * position: where to set the servo on initialization
 */
void TPP_AnimateServo::begin(int servoNumIn, int positionIn) {

    pwm_.begin(); // XXX should we do this on each object, or just once overall?
    pwm_.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

    // store values in class variables
    servoNum_ = servoNumIn;
    destination_ = positionIn; 
    position_ = positionIn;

    // move servo to new position
    int setPos = floor(position_);
    pwm_.setPWM(servoNum_, 0, setPos); 

    logAniservo.info("Begin Servo: %d at Pos: %.1f", servoNum_, position_);

}


/*------- moveTo -------
 *  newPos: new position for the servo
 *  speed: the increment to use in each call to get to the new position 
 *     1 is slow, 10 is fast; 
 *     speed defines: MOVE_SPEED_SLOW 1, MOVE_SPEED_FAST 10, MOVE_SPEED_IMMEDIATE 100
 *  Returns the estimated milliseconds needed to get from the current position 
 *     to the new position.
 */
int TPP_AnimateServo::moveTo (int newPos, int speed) {

    int estimatedMSToFinish = 0;

    // Set new destination and start time
    destination_ = newPos;
    timeStart_ = millis();
    lastDebugNeedsPrinting_ = true;

    // Will we count up or down?
    if (destination_ < position_) {
        increment_ = -1 * speed; // count down
    } else {
        increment_ = speed; // count up
    }

    // How long do we anticipate the move will take?
    if (speed != 0) {
        estimatedMSToFinish = (abs((destination_-position_)/speed) * MS_BETWEEN_MOVES * 1.4);
    }
    
    if (estimatedMSToFinish < 200) {
        estimatedMSToFinish = 200;
    }

    timeStart_ = millis();
    logAniservo.trace("MoveTo, ServoNum: %i, pos: %.1f, dest: %i, inc: %.1f, estDur: %d", 
              servoNum_, position_, destination_, increment_, estimatedMSToFinish);

    return estimatedMSToFinish;

};


/* ----- process -----
 * Called often to give the animation a chance to step forward
 * Will position the servos every MS_BETWEEN_MOVES 
 */
void TPP_AnimateServo::process() {

    bool atDestination = false;

    //are we at the destination now?
    int posInt =  floor(position_);
    int distanceToGo = abs(posInt - destination_);
    if ( distanceToGo < 2 )  {  
        // we are at the destination
        atDestination = true;
        position_ = destination_; // set to prevent servo chatter
    }

    // Not at the destination yet, find new position for this cycle
    if (!atDestination) {

        //have we waited long enough to make a new position change?
        if (millis() - lastMoveMade_ > MS_BETWEEN_MOVES) {
        
            // calculate new position
            position_ += increment_;

            // don't overshoot the destination
            if (increment_ < 0) {
                // we are counting down
                if (position_ < destination_) {
                    position_ = destination_;  
                }
            } else {
                // we are counting up
                if (position_ > destination_) {
                    position_ = destination_; 
                }
            }
            
            // Command the servo
            int newPosition = floor(position_);
            pwm_.setPWM (servoNum_, 0, newPosition);
            lastMoveMade_ = millis();

        }

    }

    // we have arrived
    if (atDestination) {
        // we've arrived at the destination, so print some final info but only once
        if (lastDebugNeedsPrinting_) {

            lastDebugNeedsPrinting_ = false;
 
            int timeEnd = millis();
            float MSPerMoveUnit;
            if (startPosition_ == position_) {
                MSPerMoveUnit = 0;
            } else {
                MSPerMoveUnit = ((timeEnd - timeStart_)/abs(startPosition_ - position_));
            }
            int actualDuration = timeEnd - timeStart_;

        // XXX If this is called from a Timer object and I uncomment these then
        //    the process crashes. The documentation explicitly says to use
        //    the log object instead of Serial.print in a Timer call back.
        //    why doesn't this work?

            logAniservo.trace("Arrived, ServoNum: %i, pos: %.2f, inc: %.2f, actDur: %d", 
              servoNum_, position_, increment_, actualDuration );
            logAniservo.trace("Summary, inc: %.2f , MS Per Move Unit: %.2f", 
                increment_, MSPerMoveUnit);

        }
    }

}
