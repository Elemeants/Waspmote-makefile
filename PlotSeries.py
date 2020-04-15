"""
Display analog data from Arduino using Python (matplotlib)

pip install pySerial
pip install numpy
pip install matplotlib
pip install argparse
"""

import sys, serial, argparse
import numpy as np
from time import sleep
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import csv

BAUDRATE = 115200
LENGHT_DATA = 3

# plot class
class AnalogPlot:
    # constr
    def __init__(self, strPort, maxLen):
        # open serial port
        self.ser = serial.Serial(strPort, BAUDRATE)

        self.measure_file = open('measures.csv', mode='w')
        header = ' '.join(map(str, ['Hora', 'Calcio', 'Nitratos', 'Potasio'])).replace(' ', ', ')
        print(header)
        self.measure_file.write(header)
        self.measure_file.write('\n')

        self.ax = deque([0.0] * maxLen)
        self.ay = deque([0.0] * maxLen)
        self.az = deque([0.0] * maxLen)
        self.maxLen = maxLen

    # add to buffer
    def addToBuf(self, buf, val):
        if len(buf) < self.maxLen:
            buf.append(val)
        else:
            buf.pop()
            buf.appendleft(val)

    # add data
    def add(self, data):
        assert (len(data) == LENGHT_DATA)
        self.addToBuf(self.ax, data[0])
        self.addToBuf(self.ay, data[1])
        self.addToBuf(self.az, data[2])

    # update plot
    def update(self, frameNum, a0, a1, a2, a3):
        try:
            line = self.ser.readline()
            vals = line.split()
            time = vals.pop(0).decode('utf-8') if vals.__len__() > 0 else ''
            data = [float(val) for val in vals]
            data_csv_row = time + ', ' + data.__str__().replace('[', '').replace(']', '')
            print(data_csv_row)
            # print data
            if len(data) == LENGHT_DATA:
                self.add(data)
                a0.set_data(range(self.maxLen), self.ax)
                a1.set_data(range(self.maxLen), self.ay)
                a2.set_data(range(self.maxLen), self.ay)
                plt.legend(['Calcio', 'Nitratos', 'Potasio'])
                self.measure_file.write(data_csv_row)
                self.measure_file.write('\n')

        except KeyboardInterrupt:
            self.measure_file.close()
            print('exiting')

        return a0,

        # clean up

    def close(self):
        # close serial
        self.ser.flush()
        self.ser.close()
        self.measure_file.close()

    # main() function


def main():
    # create parser
    parser = argparse.ArgumentParser(description="LDR serial")
    # add expected arguments
    parser.add_argument('--port', dest='port', required=True)

    # parse args
    args = parser.parse_args()

    # strPort = '/dev/tty.usbserial-A7006Yqh'
    strPort = args.port

    print('reading from serial port %s...' % strPort)

    # plot parameters
    analogPlot = AnalogPlot(strPort, 100)

    print('plotting data...')

    # set up animation
    fig = plt.figure()
    ax = plt.axes(xlim=(0, 100), ylim=(0, 5))
    a0, = ax.plot([], [])
    a1, = ax.plot([], [])
    a2, = ax.plot([], [])
    a3, = ax.plot([], [])
    anim = animation.FuncAnimation(fig, analogPlot.update,
                                   fargs=(a0, a1, a2, a3),
                                   interval=1000)

    # show plot
    plt.title('Mediciones IONES')
    plt.ylabel('Volts')
    plt.show()

    # clean up
    analogPlot.close()

    print('exiting.')


# call main
if __name__ == '__main__':
    main()