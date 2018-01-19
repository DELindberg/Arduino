#include <MsTimer2.h>
#include <EEPROM.h>

static byte keyCount = 0,          //How long the key is held
            arrayCount = 0;        //resultArray iterator variable

boolean shouldReset = true,   //Indicates whether the timer should be reset
        lightOn = false,      //If the light should be on
        waitForInput = false, //If the system should wait for input
        delaying = false;     //Indicating if we're pausing considering inputs for debounce

static int randomTimer = 0,    //Random timer between 500 and 5000ms
           startTime = 0,  //Time passed between light ON and user reaction
           timePassed = 0,     //Variable used to subtract ms in realtime within the loop
           timeToStore = 0,    //Variable to store the user's reaction time
           storeDelay = 0,     //Safety-variable to prevent button bouncing
           storeDelayLimit = 400,  //How many ms must pass before we consider more input
           arrayTotal = 0;     //Used to store the total sum of all recorded results  

static int resultArray[30] = {};//This is where we store our results

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);         //Establish connection to Serial Monitor
  pinMode(8, OUTPUT);       //Set PIN8 to output
  pinMode(2, INPUT_PULLUP); //Set PIN2 to listen to pull-up (0 = true /1 = false)
  //Call the KeyRead function when the power level on pin2 drops
  attachInterrupt(digitalPinToInterrupt(2), keyRead, FALLING);

  //Initialize random seed using cosmic radiation and wi-fi signals
  randomSeed(analogRead(0));

  //Initialize timer
  MsTimer2::set(1, countDown); // execute function keyRead by given interval in ms
  MsTimer2::start();

  float toPrint;
  //Print the previous result from the EEPROM storage
  Serial.print("Previous result: ");
  Serial.println(EEPROM.get(1000, toPrint));
  
}

//Interrupt function called each time the button is pressed
void keyRead()
{
  //Turn off the light (the light should never not turn off when the button is pressed)
  digitalWrite(8, LOW);

  //If we're not waiting for input and we're not debouncing
  if (!waitForInput && !delaying)
  {
    //TODO: Trash all current results, start over
    Serial.println("CHEATER!");
  }
  else if (!delaying)//If we're waiting for input and we're not debouncing
  {
    //Record how many milliseconds have passed since we started the startTime
    timeToStore = (millis() - startTime);
    //Reset the value of the startTime
    startTime = millis();
    
    //Indicate that we need a new random time to wait
    shouldReset = true;
    //Indicate that we no longer want the player to press the button
    waitForInput = false;

    //Store the result in the element at the given index
    resultArray[arrayCount] = timeToStore;
    arrayCount++; //Increment index-counting variable

    //Print results for feedback on the button-press
    Serial.print("Result stored: ");
    Serial.println(timeToStore);
    Serial.print("arrayCount at: ");
    Serial.println(arrayCount);

    //Indicate that the light should now be turned off
    lightOn = false;    
  }

  //Find result average and save if we've reached our limit
  if(arrayCount == 30)
  {
    static float resultAverage;
    //Summarize total of stored results
    for(int i = 0; i < sizeof(arrayCount); i++)
    {
      arrayTotal += resultArray[i];
    }
    resultAverage = (arrayTotal/sizeof(arrayCount));//Calculate average
    Serial.print("Result average: ");
    Serial.println(resultAverage);

    //Update calculated result to EEPROM if it differs from the previous result
    EEPROM.put(1000, resultAverage);
  }

  //Set delaying to be true, for debounce
  delaying = true;
  storeDelay = 0;//Reset delaying-time, will increment up to storeDelayLimit
}

void loop() {

  //Stores random number between 500 and 5000 in the randomTimer variable
  if (shouldReset)
  {
    randomTimer = (int)random(500, 5000);//Should be considered as milliseconds
    shouldReset = false;
  }

  //If the light should be turned on, but the pin is on LOW
  if (lightOn && !delaying && !digitalRead(8))
  //if (digitalRead(8) == 0)
  {    
    //Light up the diode
   digitalWrite(8, HIGH);
   
   //Debug-print
   //Serial.println("Y");
   //Serial.print(digitalRead(8));
   
   //Reset the value of the startTime
   startTime = millis();
   
    //If we're not waiting for input, we know the light only JUST turned on
    if (!waitForInput)
    {      
      //Wait for input
      waitForInput = true;
      //Serial.println("startTime updated!");
    }
  }
}

//Interrupt function, called every millisecond by MsTimer2
void countDown()
{
  //If the random value for the timer is still above zero
  if (randomTimer > 0)
  {
    //Subtract one from random
    randomTimer -= 1;
    //Serial.println(randomTimer);
  }
  else //If time is out, indicate the light must be turned on
  {
    lightOn = true;
  }

  //Count to prevent debouncing
  if (delaying && (storeDelay < storeDelayLimit))
  {
    //Increment until the specified limit
    storeDelay++;
    //Serial.println(storeDelay);
  }
  else
  {
    //Indicate to the keyRead interrupt that we're no longer compensating for debounce
    delaying = false;
  }

}


