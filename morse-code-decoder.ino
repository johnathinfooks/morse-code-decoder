#include <string.h>
#include <LiquidCrystal.h>

// declare pin numbers
#define BUZZ_PIN 13 // use active buzzer
#define BUTTON_PIN 4
#define CHAR_INTERRUPT_PIN 2
#define SPACE_INTERRUPT_PIN 3

// declare pin numbers to use with lcd screen
#define RS 12
#define ENABLE 11
#define D4 5
#define D5 6
#define D6 7
#define D7 8
LiquidCrystal lcd(RS, ENABLE, D4, D5, D6, D7);

// declare variables
int td1, td2, dur, t_last_space, now;
int b_state; // LOW = pressed ; HIGH = not pressed
char mc; // morse character ; '.' or '_'

// morse buffer
char morse_buffer[8];
byte morse_index = 0; // 'byte' saves space :)

// interrupt flags
volatile bool char_interrupt_flag = false;
volatile bool space_interrupt_flag = false;

void setup()
{
  // begin serial monitor
  Serial.begin(9600);

  // initialize pin modes
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHAR_INTERRUPT_PIN, INPUT_PULLUP);
  pinMode(SPACE_INTERRUPT_PIN, INPUT_PULLUP);

  // attach interrupts
  attachInterrupt(digitalPinToInterrupt(CHAR_INTERRUPT_PIN), do_char, LOW);
  attachInterrupt(digitalPinToInterrupt(SPACE_INTERRUPT_PIN), do_space, FALLING);

  // begin lcd
  lcd.begin(16, 2);
  lcd.clear();
}

void loop()
{
  /*
   *    handlers that manage functionality;
   *
   *    brief: listens for input, turns that input into '.' or '_' and appends a
   *    string to that input, then associates that string to an ASCII character
   *    and prints it to the serial monitor. if either of the interrupt flags are
   *    true (the button is pressed) then there is an interrupt associated to the flag.
   *    the char interrupt ends the listening and starts translation. the space interrupt
   *    does the same but adds a space character
   */
  key_press_handler();
  char_interrupt_handler();
  space_interrupt_handler();
}

// listens for press and then calculates duration
int get_dur()
{
  if (digitalRead(BUTTON_PIN) == LOW) {
    td1 = millis();
    while (digitalRead(BUTTON_PIN) == LOW)
      td2 = millis();
  }
  dur = td2 - td1;
  return dur;
}

// translating input press durations to '.' or '_'
char dur_to_morse(int dur)
{
  if (dur > 190)
    mc = '_';
  else if (dur <= 190 && dur > 10)
    mc = '.';
  else
    mc = '?';

  return mc;
}

// read input turn to morse '.' or '_'
void key_press_handler()
{
  /* while much of this snippet seems redundant, it is needed to remove noise;
   * lots of skipping or duplicates of signals without mentioned redundancy */
  b_state = digitalRead(BUTTON_PIN);
  if (b_state == LOW) {
    digitalWrite(BUZZ_PIN, HIGH);
    int d = get_dur();
    char c = dur_to_morse(d);
    if ((c == '.' || c == '_') && morse_index < sizeof(morse_buffer) - 1) {
      morse_buffer[morse_index++] = c;
      // comment out line below to not see the morse characters
      Serial.println(c);
    }
    digitalWrite(BUZZ_PIN, LOW);
  }
}

// end of character interrupt
void char_interrupt_handler()
{
  if (!char_interrupt_flag)
    return;

  noInterrupts();
  char_interrupt_flag = false;
  interrupts();

  if (morse_index > 0) {
    morse_buffer[morse_index] = '\0';
    char letter = morse_to_char(morse_buffer);
    output_char(letter);
    morse_index = 0;
  }
}

// space interrupt (end of character add space)
void space_interrupt_handler()
{
  if (!space_interrupt_flag)
    return;

  noInterrupts();
  space_interrupt_flag = false;
  interrupts();

  // remove skipping and multiple signals
  t_last_space = 0;
  now = millis();

  if (morse_index > 0) {
    morse_buffer[morse_index] = '\0';
    char letter = morse_to_char(morse_buffer);
    output_char(letter);
    morse_index = 0;
  }
  output_space();
}

// map morse code to ASCII character
char morse_to_char(const char *code) {
  struct MorseMap {
    const char *code;
    char letter;
  };

  // table associating morse to ASCII characters
  static const MorseMap table[] = {
    {"._",   'A'}, {"_...", 'B'}, {"_._.", 'C'}, {"_..",  'D'},
    {".",    'E'}, {".._.", 'F'}, {"__.",  'G'}, {"....", 'H'},
    {"..",   'I'}, {".___", 'J'}, {"_._",  'K'}, {"._..", 'L'},
    {"__",   'M'}, {"_.",   'N'}, {"___",  'O'}, {".__.", 'P'},
    {"__._", 'Q'}, {"._.",  'R'}, {"...",  'S'}, {"_",    'T'},
    {".._",  'U'}, {"..._", 'V'}, {".__",  'W'}, {"_.._", 'X'},
    {"_.__", 'Y'}, {"__..", 'Z'}
  };

  /* compare the morse input to the codes in the table and
   * return the letter associated with that code             */
  for (byte i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
    if (strcmp(code, table[i].code) == 0) {
      return table[i].letter;
    }
  }

  // if theres no valid code
  return '?';
}

// outputs to both the serial monitor and lcd screen
void output_char(char ch) {
  Serial.println(ch);
  lcd.print(ch);
}

void output_space() {
  Serial.println(' ');
  lcd.print(' ');
}

/* ISRs that set flags; initializing the interrupt routine needed */
void do_char() {
  char_interrupt_flag = true;
}

void do_space() {
  space_interrupt_flag = true;
}
