#!/usr/bin/env python
import sys
import os
import re

def search_inc(file, file_includes_map):
    result = []
    for i in [incs for f, incs in file_includes_map if f == file][0]:
        new_incs = search_inc(i, file_includes_map)
        result += new_incs
        result += [i]

    # remove duplicates and save order
    return sorted(set(result), key=lambda x: result.index(x))

# generate variable name based on file and its type
def filevarname(file, typest):
    return 'g_'+ file.replace(ext, "") + "_" + typest

def printfile(filename, dir):

    fh = open(dir + "/" + filename)
    # include only new files
    inc = []
    for line in fh.readlines():
        a = line.strip('\r\n')
        inl = re.search("#include\s*<.*/(.+)>", a)
        if inl and inl not in inc:
            inc.append(inl.group(1))
            print("\"#include <" + inl.group(1) + "> \\n\"\\")
        else:
        	print( '"' + a.replace("\\","\\\\").replace("\"", "\\\"") + ' \\n"\\' )
    return inc

def stringify(filename, dir, typest):
    print( 'static const char ' + filevarname(filename, typest) +'[] = \\' )
    inc = printfile(filename, dir)
    print(";\n")
    return inc

argvs = sys.argv

if len(argvs) == 4:
    dir = argvs[1]
    ext = argvs[2]
    typest = argvs[3]
else:
	sys.error("Wrong argument count!")

files = os.listdir(dir)

print("/* This is an auto-generated file. Do not edit manually! */\n")

print("#pragma once\n")
print("#include <map>\n")

# this will contain tuple(filename, include files)
file_includes_map = []
for file in files:
    if file.find(ext) == -1:
        continue

    inc = stringify(file, dir, typest)
    file_includes_map.append((file, inc))

for file, includes in file_includes_map:
    if not includes:
        continue

    print("static const std::map<char const*, char const*> " + filevarname(file, typest) + '_inc = {')

    for i in search_inc(file, file_includes_map):
        print ("    {\"" + i + "\", " +filevarname(i, typest) + "},")

    print("};\n")
