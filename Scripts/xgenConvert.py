
import os, math, argparse


if __name__ == '__main__':

	parser = argparse.ArgumentParser(description="XGen curve extractor.")
	parser.add_argument('mel', help="MEL output file from XGen.")
	args = vars(parser.parse_args())

	melFile = args['mel']
	if not os.path.exists(melFile):
		print "MEL file %s doesn't exist." % melFile
		quit()

	fp = open(melFile, 'r')
	lines = fp.readlines()

	curve = 0
	for line in lines:

		t = line.split('-p')
		if len(t)<2: continue
		print '\ncurve %d' % curve
		curve += 1
		numCVs = len(t)-1
		
		for cv in range(0, numCVs):
			coords = t[cv+1].strip('`;\n').split()
			print '%s %s %s' % (float(coords[0]), float(coords[1]), float(coords[2]))


