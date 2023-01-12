import argparse, itertools

parser = argparse.ArgumentParser("A small tool for generating formats for use in fbcubes5")

parser.add_argument("-a", "--all", help="Generate all possible formats", action='store_true')
parser.add_argument("-p", "--popular", help="Generate all the popular formats", action='store_true')
parser.add_argument("-o", "--only", help="Only generate one format", type=str)

args = parser.parse_args()

possible = ["A", "R", "G", "B"]

if args.all:
    for comb in itertools.permutations('ARGB'):
        print("".join(comb), end=" ")

if args.popular:
    print("BGRA ARGB RGBA ABGR")

if args.only:
    print(args.only)