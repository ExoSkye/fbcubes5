import sys

if len(sys.argv) != 2:
    print("Incorrect number of arguments, exiting...")
    sys.exit(1)

i = 0
for c in sys.argv[1].split("-")[2][0:4]:
    print(f"-D{c}{i}", end=" ")
    i += 1

