# Basic Stepper Motor Library (ATmega328P + TB6600)

Simpele, low-level embedded C library om een stappenmotor aan te sturen via
een TB6600 driver, geschreven met directe AVR-register-toegang (geen
Arduino-libraries).

## Bestanden
- `stepper.h` — publieke API, pin-definities en motorparameters
- `stepper.c` — implementatie
- `main.c` — voorbeeldgebruik

## Bedrading (default pinnen)
| TB6600 pin | ATmega328P pin | Arduino pin |
|-----------|-----------------|-------------|
| PUL (STEP)| PD7             | D7          |
| DIR       | PD6             | D6          |
| ENA (EN)  | PD5             | D5          |

> **Let op:** in deze library is de enable-ingang **active-high**
> geïmplementeerd: `EN = HIGH` betekent dat de driver is ingeschakeld,
> `EN = LOW` betekent uitgeschakeld. Controleer dit met jouw eigen
> bedrading/module, want dit verschilt per TB6600-variant.

## API

```c
void stepper_init(void);
void stepper_enable(bool enable);
void stepper_set_direction(stepper_direction_t direction);
void stepper_step(void);
void stepper_move_steps(uint16_t steps, uint16_t delay_us);
void stepper_move_revolutions(uint8_t revolutions, uint16_t rpm);
```

## Bouwen (voorbeeld met avr-gcc)

```bash
avr-gcc -mmcu=atmega328p -DF_CPU=16000000UL -Os -o main.elf main.c stepper.c
avr-objcopy -O ihex main.elf main.hex
avrdude -c arduino -p atmega328p -P <poort> -b 115200 -U flash:w:main.hex
```

## Instelbaar
- `MICROSTEPS` in `stepper.h` aanpassen aan de DIP-switch instelling op de
  TB6600 driver.
- `FULL_STEPS_PER_REV` aanpassen aan jouw motor (meestal 200 voor een
  1.8°-stappenmotor).

## Voorbeeld (main.c)
De meegeleverde `main.c` demonstreert:
1. Eén omwenteling met de klok mee op 60 RPM
2. Eén omwenteling tegen de klok in op 120 RPM
3. Een halve omwenteling (1600 stappen) met custom timing tussen stappen
4. Een snelle burst van losse stappen via `stepper_step()`
5. De driver tijdelijk uitschakelen om stroom te besparen
