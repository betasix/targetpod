#include <Arduino.h>
#include <avr/sleep.h>
#include <EEPROM.h>
#include "color.hpp"

constexpr uint8_t pin_analog_red = 7;
constexpr uint8_t pin_analog_green = 6;
constexpr uint8_t pin_analog_blue = 8;
constexpr uint8_t pin_switch_dimmer = 9;
constexpr uint8_t pin_switch_color = 10;
constexpr float m_dimmStepSize = 0.05f;
constexpr float m_roundPrecision = 0.5f;
constexpr uint8_t m_debouncing_ms_switch_color = 50;
constexpr uint8_t m_debouncing_ms_switch_brightness = 2;
constexpr float m_minimumBrightness = 0.02f;
constexpr float m_maximumBrightness = 1.0f;
constexpr uint8_t m_magicNumberForEEPROM = 42;
constexpr uint8_t m_memoryPosition_magicNumber = 0;
constexpr uint8_t m_memoryPosition_selectedColor = 1;
constexpr uint8_t m_memoryPosition_brightness = 2;

static Color red(255, 0, 0);
static Color green(0, 255, 0);
static Color blue(0, 0, 255);
static Color pink(255, 0, 255);
static Color yellow(0, 255, 255);
static Color teal(255, 255, 0);
static Color white(255, 255, 255);

Color colors[7] = {red, green, blue, pink, yellow, teal, white};

static float m_brightness = 1.0f;
static bool m_brightnessUp = false;
static uint8_t m_magicNumberReadFromEEPROM = 0;

static uint8_t selectedColor = 0;
static uint8_t maxColors = sizeof(colors) / sizeof(Color);

void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    analogWrite(pin_analog_red, r);
    analogWrite(pin_analog_green, g);
    analogWrite(pin_analog_blue, b);
}

void writeConfigToEEProm()
{
    EEPROM.write(m_memoryPosition_magicNumber, m_magicNumberForEEPROM);
    EEPROM.put(m_memoryPosition_selectedColor, selectedColor);
    EEPROM.put(m_memoryPosition_brightness, m_brightness);
}

void readConfigFromEEProm()
{
    m_magicNumberReadFromEEPROM = EEPROM.read(m_memoryPosition_magicNumber);

    if (m_magicNumberReadFromEEPROM == 42)
    {
        EEPROM.get(m_memoryPosition_selectedColor, selectedColor);
        EEPROM.get(m_memoryPosition_brightness, m_brightness);
    }
}

void changeColor(bool onlyBrightness)
{
    if (onlyBrightness == false)
    {
        if (selectedColor < maxColors)
            ++selectedColor;
    }
    if (selectedColor == maxColors)
    {
        selectedColor = 0;
    }
    setRGB(floor(colors[selectedColor].r() * m_brightness + m_roundPrecision),
           floor(colors[selectedColor].g() * m_brightness + m_roundPrecision),
           floor(colors[selectedColor].b() * m_brightness + m_roundPrecision));
    writeConfigToEEProm();
}

void changeBrightness()
{
    if (m_brightness < m_minimumBrightness)
    {
        m_brightnessUp = true;
    }

    if (m_brightness == m_maximumBrightness)
    {
        m_brightnessUp = false;
    }

    if (m_brightnessUp == true)
    {
        m_brightness += m_dimmStepSize;
    }

    if (m_brightnessUp == false)
    {
        m_brightness -= m_dimmStepSize;
    }

    changeColor(true);
}

void interrupt_button_change_color(void)
{
    // Switch Led state
    changeColor(false);
}

void interrupt_button_change_brightness(void)
{
    changeBrightness();
}

void setup()
{
    // Read config from EEPROM
    readConfigFromEEProm();

    // Power saving configs
    ADCSRA &= ~(1 << ADEN);          // Turn off ADC
    ACSR |= _BV(ACD);                // Disable the analog comparator
    MCUCR |= _BV(BODS) | _BV(BODSE); // Turn off the brown-out detector

    // Changing green timer (TIMER1) to be in sync with the other colors
    // and remove flickering
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1A |= (1 << WGM10);
    TCCR1B |= (1 << WGM12);
    TCCR1B |= (1 << CS10);

    // Enable interrupt functions for button pins
    GIFR = (0 << PCIF1);     // Reset interrupt flag
    PCMSK1 |= (1 << PCINT9); // Pin mask for PCINT9 interrupt
    PCMSK1 |= (1 << PCINT8); // Pin mask for PCINT8 interrupt
    GIMSK |= (1 << PCIE1);   // enable PCINT interrupts
    sei();

    // Set initial color to red on start
    setRGB(255, 0, 0);

    // Set to color from EEPROM if it has been set
    changeColor(true);

    // Setup digital I/O-pins
    pinMode(pin_switch_color, INPUT);
    pinMode(pin_switch_dimmer, INPUT);

    // setup anlog pins
    pinMode(pin_analog_red, OUTPUT);
    pinMode(pin_analog_green, OUTPUT);
    pinMode(pin_analog_blue, OUTPUT);

    // Send controller to sleep
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // do a complete power-down
    sleep_enable();                      // enable sleep mode
}

// Interrupt Service routing for PCINT1 interrupts
ISR(PCINT1_vect)
{
    if (digitalRead(pin_switch_color) == LOW)
    {
        changeColor(false);
        delay(m_debouncing_ms_switch_color);
    }
    if (digitalRead(pin_switch_dimmer) == LOW)
    {
        changeBrightness();
        delay(m_debouncing_ms_switch_brightness);
    }
}

// Not used at all
void loop()
{
}