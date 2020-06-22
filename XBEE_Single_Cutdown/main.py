import xbee
import time
# from sys import stdin, stdout
import machine
from machine import Pin
# from machine import UART
# from pyb import UART


# ~~~~~~~~~~~~~~~~~~~~ FUNCTION DEFINITIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Flush the buffer because we have just handled code
def flush_rx_buffer():
    for i in range(100):
        xbee.receive()

def pri_cutdown():
    global priCutdownFlag
    print("PRI CUTDOWN RECEIVED")
    if(priCutdownFlag):
        priCutdownFlag = False
        print("PRI CUTDOWN INITIATED")
        PRI_MOSFET.value(1)
        ASSOC_LED.value(1)
        flush_rx_buffer()
        time.sleep(20)
        PRI_MOSFET.value(0)

def idle_command():
    global priCutdownFlag
    global secCutdownFlag
    priCutdownFlag = True
    priCutdownFlag = True
    ASSOC_LED.value(0)

def check_for_command(payload):
    print("Checking for command")
    # Parse bytes as a string
    payload = payload.decode("utf-8")
    # Take out first 3 characters
    if len(payload) >= 3:
        command = payload[0] + payload[1] + payload[2]
        print("Interpretted command: " + command)
        if "JKL" == command:
            pri_cutdown()
        elif "ABC" == command:
            idle_command()
            print("IDLE")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

print('ID: ' + str(xbee.atcmd('MY')))
# PIN DEFINITIONS
USER_LED = Pin(machine.Pin.board.D4, Pin.OUT, value=0)
PRI_MOSFET = Pin(machine.Pin.board.D12, Pin.OUT, value=0)
ASSOC_LED = Pin(machine.Pin.board.D5, Pin.OUT, value=0)

# FLAG DEFINITONS
priCutdownFlag = True
secCutdownFlag = True

while True:
    packet = xbee.receive()
    #print(packet)
    if packet != None:
        print(packet.get('payload'))
        check_for_command(packet.get('payload'))
    USER_LED.value(1)
    time.sleep(0.2)
    USER_LED.value(0)
    time.sleep(0.2)
