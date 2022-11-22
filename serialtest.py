import serial
import time
import signal
import threading
import matplotlib.pyplot as plt
import numpy as np
from tempfile import TemporaryFile


line = []

port = 'COM11'
baud = 57600

exitThread = False

def handler(signum, frame):
     exitThread = True

def readThread(ser):
    global line
    global exitThread
    outfile = TemporaryFile()

    while not exitThread:
        for c in ser.read():
            line.append(int(c))

            if line[-1] == 0xcc and line[-2] == 0xbb:
                img = np.array(line)
                if len(line) == 76803:
                    img = img[1:-2]
                else:
                    img = img[0:-2]
                np.save('./img.npy', img.reshape((240,320)))
                exitThread=True               


if __name__ == '__main__':
    signal.signal(signal.SIGINT, handler)

    ser = serial.Serial(port, baud, timeout=0)

    thread = threading.Thread(target=readThread, args=(ser,))

    thread.start()