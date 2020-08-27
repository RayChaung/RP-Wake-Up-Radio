import subprocess
import os
import time
from datetime import datetime


def writelog(str):
    LogDIR = "/home/pi/Desktop/log/"
    date = datetime.today().strftime('%Y-%m-%d')
    flog = open(LogDIR + date +".log", "a+")
    flog.write(str)
    flog.write("\n")
    flog.close()

def readStartTime():
    LogDIR = "/home/pi/Desktop/log/"
    date = datetime.today().strftime('%Y-%m-%d')
    try:
        result = 0
        with open(LogDIR + date +".log", "r") as flog:
            for last_line in flog:
                if last_line.split(":")[0] == "start":
                    result = last_line.strip().split(":")[1]
                if last_line == "------------------------------------------------------------------------":
                    result = 0
                pass
        #print(last_line.split(":"))
        
        return int(result)
        
    except:
        return 0


delay = readStartTime()
#time.sleep(2)
milli_sec1 = int(round(time.time() * 1000))

flag = 0
# set timeout 120 sec
timeout = time.time() + 120
while True:
    proc = subprocess.Popen(["/home/pi/Desktop/IoT_Project/IoTClient", "/home/pi/Desktop/IoT_Project/1kb"], stdout = subprocess.PIPE)
    while True:
        ok = "Ok"
        line = proc.stdout.readline()
        #writelog(str(line.decode().strip()))
        if not line:
            print("Terminate process")
            break
        #print(line.decode().strip().split(" "))
        if ok in line.decode().strip().split(" "):
            writelog("ok")
            flag = 1
            break
    if flag == 1 or time.time() > timeout:
        break
    #time.sleep(1)


milli_sec2 = int(round(time.time() * 1000))
finish1_log = "finished1:" + str(milli_sec1)
writelog(finish1_log)
finish2_log = "finished2:" + str(milli_sec2)
writelog(finish2_log)
if delay != 0:
    temp = float((milli_sec1 - delay)*0.001)
    delay_log = "delay1:"+ str(temp)
    writelog(delay_log)
    temp = float((milli_sec2 - delay)*0.001)
    delay_log = "delay2:"+ str(temp)
    writelog(delay_log)

# set timeout 120 sec
'''timeout = time.time() + 120
while True:
    LogDIR = "/home/pi/Desktop/log/"
    date = datetime.today().strftime('%Y-%m-%d')
    result = 0
    with open(LogDIR + date +".log", "r") as flog:
        for last_line in flog:
            if last_line.split(":")[0] == "finished2":
                result1 = last_line.strip().split(":")[1]
                result1 = result1.split(".")[0]+result1.split(".")[1][0:3]
                if result != 0:
                    result = min(int(result1), result)
                else:
                    result = int(result1)
                if last_line == "------------------------------------------------------------------------":
                    result = 0
            pass
    #print(last_line.split(":"))
    # not find finished2 in this run
    if result > delay:     
        temp = float((result - delay)*0.001)
        delay_log = "delay2:"+ str(temp)
        writelog(delay_log)
        break
    if time.time() > timeout:
        break
    time.sleep(1)
'''
time.sleep(3)
writelog("------------------------------------------------------------------------")

