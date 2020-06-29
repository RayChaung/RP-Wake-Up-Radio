import serial
import pynmea2
import subprocess
import os
import json
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
    
db  = MySQLdb.connect("140.113.216.91", "cloud", "cloud2016", "ray", cursorclass=MySQLdb.cursors.DictCursor)
#db  = MySQLdb.connect("140.113.216.91", "cloud", "cloud2016", "ray")
sql = """SELECT * FROM `info` WHERE `locationENG` LIKE 'receiver%'"""
cursor = db.cursor()
cursor.execute(sql)
data = cursor.fetchall()
data = list(data)

while data:
    print(json.dumps(data, indent=4))
    for receiver in data:
        
        if distance(readGPS(), receiver) <= 1000:
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
                    print("ACK received from %s." %(rfID))
                    data = [i for i in data if i['rfID'] != rfID]
                    
                #print(line.decode().strip())

