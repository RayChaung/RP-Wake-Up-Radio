import os, sys
import time

# This program must be run as root
if not os.geteuid()==0:
    sys.exit("Hint: call me as root")

time.sleep(10) # Wait for "rfresponse" send ACK

cmd = "rfwait"
os.system(cmd)

time.sleep(45) # Wait for ~~~

print("Bye!\n")

cmd = "shutdown now"
os.system(cmd)

