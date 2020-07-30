
import time
import serial

# set the appropriate com ports
serialPortRangefinder = 'COM10'
serialPortCNCArm = 'COM24'
# Use this to adjust range and CNC direction
GcodeString = 'G91 Y-.127\n' # move relative (so the command is always the same), by another 5 thousands of an inch, in mm.

try:
	serialRangefinder = serial.Serial(serialPortRangefinder, baudrate=1000000, timeout=1)
except:
	print("WARNING: failed to open connection to rangefinder. Program will now exit.")
	exit()
try:
	serialCNCArm = serial.Serial(serialPortCNCArm, baudrate=115200, timeout=.01)
	CNCAvailable = True
except:
	print("WARNING: failed to open connection to CNC motion arm. Will still run calibration routine for debugging purposes, but cal data will be useless.")
	CNCAvailable = False

for i in range(128,228): # unit is hardcoded with 100 calibration points, starts indexing at 128 to reserve ASCII inputs.
	print("LOOP PROGRESS: " + str(i-128) + '/' + str(100))
	if CNCAvailable: serialCNCArm.write(GcodeString.encode())	
	time.sleep(.6) # needs enough time for the arm to move, settle, and the position sensor to grab 1000 points (speed for
		# this depends on how fast the sensor is running)
	serialRangefinder.write((i).to_bytes(1, byteorder="little"))
	time.sleep(.1)
	dataReturn = serialRangefinder.readline().decode('ascii')
	print("RANGEFINDER RETURN: " + str(dataReturn).strip("\n"))
serialRangefinder.write(228).to_bytes(1, byteorder="little") # command to save cal data
time.sleep(.1)
dataReturn = serialRangefinder.readline().decode('ascii')
print("RANGEFINDER RETURN: " + str(dataReturn))