# test_pn532.py
import time
import board
import busio
from adafruit_pn532.i2c import PN532_I2C

i2c = busio.I2C(board.SCL, board.SDA)
pn532 = PN532_I2C(i2c, debug=False)
pn532.SAM_configuration()

print("Venter på kort...")

while True:
    uid = pn532.read_passive_target(timeout=1)
    if uid:
        print("Kort læst! UID:", [hex(i) for i in uid])
        break
