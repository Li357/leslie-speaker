import serial

s = serial.Serial('/dev/tty.usbmodem103', 115200)

s.read_until(b'\x55\xBB\x55\xBB')
while True:
    #bites = s.readline()
    #print(bites);
    bites = s.read(4)
    print('{:4d}'.format(int.from_bytes(bites, 'little')), end='\r')
