#include <Arduino.h>
#include <avr/sleep.h>
#include <EEPROM.h>
#include <WS2812.h>
#include "Color.hpp"

constexpr uint8_t pin_led_cmd = 3;
constexpr uint8_t pin_switch_dimmer = 1;
constexpr uint8_t pin_switch_color = 0;
constexpr uint8_t k_magicNumberForEEPROM = 42;
constexpr uint8_t k_memoryPosition_magicNumber = 0;
constexpr uint8_t k_memoryPosition_selectedColor = 0;
constexpr uint8_t k_memoryPosition_brightness = 2;
constexpr uint8_t k_debouncing_ms_switch_color = 50;
constexpr uint8_t k_debouncing_ms_switch_brightness = 2;
constexpr float k_minimumBrightness = 0.02f;
constexpr float k_maximumBrightness = 1.0f;
constexpr float k_dimmStepSize = 0.05f;
constexpr float k_roundPrecision = 0.5f;
static float m_brightness = 1.0f;

static Color red(255, 0, 0);
static Color green(0, 255, 0);
static Color blue(0, 0, 255);
static Color pink(255, 0, 255);
static Color yellow(0, 255, 255);
static Color teal(255, 255, 0);
static Color white(255, 255, 255);
Color colors[7] = {blue, pink, yellow, teal, white, red, green};

static cRGB m_led_value;
static WS2812 LED(1);

static bool m_brightnessUp = false;
static uint8_t m_magicNumberReadFromEEPROM = 0;

static uint8_t selectedColor = 0;
static uint8_t maxColors = sizeof(colors) / sizeof(Color);

void setRGB(uint8_t r, uint8_t g, uint8_t b)
{
    m_led_value.r = r;
    m_led_value.g = g;
    m_led_value.b = b;
    LED.set_crgb_at(0, m_led_value);
    LED.sync();
}

void writeConfigToEEProm()
{
    EEPROM.write(k_memoryPosition_magicNumber, k_magicNumberForEEPROM);
    EEPROM.put(k_memoryPosition_selectedColor, selectedColor);
    EEPROM.put(k_memoryPosition_brightness, m_brightness);
}

void readConfigFromEEProm()
{
    m_magicNumberReadFromEEPROM = EEPROM.read(k_memoryPosition_magicNumber);

    if (m_magicNumberReadFromEEPROM == k_magicNumberForEEPROM)
    {
        EEPROM.get(k_memoryPosition_selectedColor, selectedColor);
        EEPROM.get(k_memoryPosition_brightness, m_brightness);
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
    setRGB
    (
        floor(colors[selectedColor].r() * m_brightness + k_roundPrecision),
        floor(colors[selectedColor].g() * m_brightness + k_roundPrecision),
        floor(colors[selectedColor].b() * m_brightness + k_roundPrecision)
    );
    //writeConfigToEEProm();
}

void changeBrightness()
{
    if (m_brightness < k_minimumBrightness)
    {
        m_brightnessUp = true;
    }

    if (m_brightness == k_maximumBrightness)
    {
        m_brightnessUp = false;
    }

    if (m_brightnessUp == true)
    {
        m_brightness += k_dimmStepSize;
    }

    if (m_brightnessUp == false)
    {
        m_brightness -= k_dimmStepSize;
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
    //readConfigFromEEProm();

    // Power saving configs
    ADCSRA &= ~(1 << ADEN);          // Turn off ADC
    ACSR |= _BV(ACD);                // Disable the analog comparator
    MCUCR |= _BV(BODS) | _BV(BODSE); // Turn off the brown-out detector

    // Enable interrupt functions for button pins
    GIFR = (0 << PCIF1);     // Reset interrupt flag
    PCMSK1 |= (1 << PCINT9); // Pin mask for PCINT9 interrupt
    PCMSK1 |= (1 << PCINT8); // Pin mask for PCINT8 interrupt
    GIMSK |= (1 << PCIE1);   // enable PCINT interrupts
    sei();
    
    // setup LED pin
    LED.setOutput(pin_led_cmd);

    // Set to color from EEPROM if it has been set
    changeColor(true);

    // Setup digital I/O-pins
    pinMode(pin_switch_color, INPUT);
    pinMode(pin_switch_dimmer, INPUT);
   

    // Send controller to sleep
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); // do a complete power-down
    sleep_enable();                      // enable sleep mode
}

// Interrupt Service routine for PCINT1 interrupts
ISR(PCINT1_vect)
{
    if (digitalRead(pin_switch_color) == LOW)
    {
        if (digitalRead(pin_switch_dimmer) == LOW)
        {
            if (m_brightness == 0.0f)
            {
                m_brightness = 1.0f;
            }
            else 
            {
                m_brightness = 0.0f;
            }
            return;
        }
        
    }
    if (digitalRead(pin_switch_color) == LOW)
    {
        changeColor(false);
    }
    if (digitalRead(pin_switch_dimmer) == LOW)
    {
        changeBrightness();
    }
    GIFR = (0 << PCIF1);     // Reset interrupt flag

}

// Not used at all
void loop()
{
}
