import serial
import serial.tools.list_ports
import time

# Print all available serial ports and connect to one, y'all know there's just one open, the serial :D
print("Available serial ports:")
port = None
while port is None:
    ports = serial.tools.list_ports.comports()
    for port in ports:
        print(port.device, port.description)

ser = serial.Serial(
    port=port.device, 
    baudrate=115200,
    #timeout=5,  # Set a timeout for read operations, if triggered it just continues with the program   
)

for i in range(10):
    print(ser.read_until(b'Enter an operation:\r\n').decode('utf-8')) #read from serial and print it
    #time.sleep(1) # ESSENTIAL TIMEOUT, OTHERWISE MAY BLOCK. IDK WHY     D: D: D:
    ser.write(b"10 + 3\n") # write to serial, '\n' is the end of line character to use, not '\r'
    print(f"SENT_{i}")
print(ser.read_until(b'Enter an operation:\r\n').decode('utf-8')) # read the final response

ser.close()
