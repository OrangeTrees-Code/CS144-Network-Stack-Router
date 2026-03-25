import subprocess

out1 = subprocess.check_output("sha256sum /tmp/big.txt",shell = True)
out2 = subprocess.check_output("sha256sum /tmp/big-received.txt",shell = True)

out1 = out1.decode('utf-8').strip()
out2 = out2.decode('utf-8').strip()

out1 = out1.split(' ')[0]
out1 = out2.split(' ')[0]

if out1 == out1:
    print("Same!")
else:
    print("Different!")
