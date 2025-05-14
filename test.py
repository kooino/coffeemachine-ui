import board
import busio
from digitalio import DigitalInOut
from adafruit_pn532.i2c import PN532_I2C

# I2C setup
i2c = busio.I2C(board.SCL, board.SDA)
pn532 = PN532_I2C(i2c, debug=False)

pn532.SAM_configuration()  # Initialiser chippen

print("Klar. Hold kortet hen...")

while True:
    uid = pn532.read_passive_target(timeout=0.5)
    if uid is not None:
        if len(uid) == 4:
            uid_decimal = 0
            for b in uid:
                uid_decimal = (uid_decimal << 8) | b
            print(uid_decimal)
        else:
            print("UID ikke 4 bytes – ignorerer.")
        break
