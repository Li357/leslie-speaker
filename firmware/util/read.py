import serial

s = serial.Serial('/dev/tty.usbmodem11203', 115200)

def to_int(bites):
    return int.from_bytes(bites, 'little')

s.read_until(b'\x55\xBB\x55\xBB')
while True:
    #bites = s.readline()
    #print(bites);
    speed = s.read(2);
    coil1 = s.read(2);
    coil2 = s.read(2);
    print(f'{to_int(speed):4d} {to_int(coil1):4d} {to_int(coil2):4d}', end='\r')
