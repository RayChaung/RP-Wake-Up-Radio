import serial
import pynmea2
import subprocess
import os
import json
import time
import MySQLdb
import MySQLdb.cursors
from math import sin, cos, sqrt, atan2, radians


def distance(loc1, loc2):
    R = 6373.0
    lat1 = radians(float(loc1['lat']))
    lon1 = radians(float(loc1['lng']))
    lat2 = radians(float(loc2['lat']))
    lon2 = radians(float(loc2['lng']))
    dlon = lon2 - lon1
    dlat = lat2 - lat1

    a = sin(dlat / 2) ** 2 + cos(lat1) * cos(lat2) * sin(dlon / 2) **2
    c = 2 * atan2(sqrt(a), sqrt(1-a))
    distance = R * c
    return distance * 1000

def readGPS():
    '''
    port    = "/dev/ttyAMA0"
    ser     = serial.Serial(port, baudrate=9600, timeout=0.5)
    dataout = pynmea2.NMEAStreamReader()

    newdata = ser.readline()
    if newdata[0:6] == "$GPRMC":
        newmsg=pynmea2.parse(newdata)
        lat = newmsg.latitude
        lng = newmsg.longitude
        payload = {'lat' : lat, 'lng' : lng}
        return payload
    '''
    f = open("/home/pi/Desktop/log/gps.log")
    loc = f.read().split()
    lat = loc[0]
    lng = loc[1]
    payload = {'lat': lat, 'lng': lng}
    return payload

def writelog(rfID, str):
    LogDIR = "/home/pi/Desktop/log/"
    flog = open(LogDIR + rfID + ".log", "a")
    flog.write(str)
    flog.write("\n")
    flog.close()

data = {'locationENG': 'receiver4', 'rfID': '11:11:11:11:11:11:11:14', 'ipAddr': '192.168.10.4', 'lat': 24.786715166666667, 'lng': 120.99349283333333, 'locationCH': 'receiver4'}
#data = json.dumps(data)
print(data)
dict_time = {}

writelog(data['rfID'], "----------------------------------------------------------------------------------------------------------")
receiver = data

while data:
    #print(json.dumps(data, indent=4))
    dist = distance(readGPS(), receiver) 
    
    # when IoT devices in wakeup range (not available now)
    if dist <= 500:
            # first time wakeup and set timestamp in dictionary
        if dict_time.get(receiver['rfID']) == None:
            dict_time[receiver['rfID']] = time.time()
        #print("Wakeup %s" % receiver['rfID'])
        cmd = "/usr/local/bin/rfwake " + receiver['rfID']
        print(cmd)
        proc = subprocess.Popen(['/usr/local/bin/rfwake', receiver['rfID']], stdout=subprocess.PIPE)

        expect = "ACK received from called Station RF ID" 
        #+ receiver['rfID']
        while True:
            line = proc.stdout.readline()
            if not line:
                print("Terminate process")
                break
            if expect in line.decode().strip():  
                rfID = line.decode().strip().split(" ")[-1]
                startTime = time.time()
                if dict_time[rfID] != None:
                    startTime = dict_time[rfID]
                endTime = time.time()
                escape  = endTime - startTime
                writelog(rfID, "escape: " + str(escape) + " sec")

                print("ACK received from %s" %(rfID))

                    # remove rfID which received ACK
                    #data = [i for i in data if i['rfID'] != rfID]

                #print(line.decode().strip())
    else:
        print("%fm out of range" % dist)

