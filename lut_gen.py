#!/usr/bin/python
# -*- coding: latin-1 -*-
# lut_gen.cpp -- Look-up table generator for Pimpmobile
# Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
# For conditions of distribution and use, see copyright notice in LICENSE.TXT

AMIGA_DELTA_LUT_LOG2_SIZE = 7
AMIGA_DELTA_LUT_SIZE = 1 << AMIGA_DELTA_LUT_LOG2_SIZE
AMIGA_DELTA_LUT_FRAC_BITS = 15 - AMIGA_DELTA_LUT_LOG2_SIZE

def print_lut(lut):
	print 'const u16 linear_delta_lut[768] =\n{'
	for e in lut:
		print '%d,' % (e),
	print '\n};\n'

def print_lut_to_file(file, lutname, lut):
	max_elem = reduce(max, lut)
	min_elem = reduce(min, lut)
	assert(min_elem >= 0)
	assert(max_elem < (1 << 16))
	file.write('const u16 %s[%d] =\n{\n\t' % (lutname, len(lut)))
	line_start = file.tell()
	for e in lut:
		file.write('%d, ' % e)
		if file.tell() > (line_start + 80):
			file.write('\n\t')
			line_start = file.tell()
	file.write('\n};\n\n')

def gen_linear_delta_lut():
	return [(int(((2.0 ** (i / 768.0)) * 8363.0 / (1 << 8)) * float(1 << 9) + 0.5)) for i in range(0, 12 * 64)]

def gen_amiga_delta_lut():
	return [(int(((8363 * 1712) / float(((i + (AMIGA_DELTA_LUT_SIZE / 2)) * 32768) / AMIGA_DELTA_LUT_SIZE)) * (1 << 6) + 0.5)) for i in range(0, (AMIGA_DELTA_LUT_SIZE / 2) + 1)]

def dump_linear_lut(filename):
	linear_delta_lut = gen_linear_delta_lut()
	f = open(filename, 'w')
	print_lut_to_file(f, 'linear_delta_lut', linear_delta_lut)
	f.close()

def dump_amiga_lut(filename):
	amiga_delta_lut = gen_amiga_delta_lut()
	f = open(filename, 'w')
	f.write('#define AMIGA_DELTA_LUT_LOG2_SIZE %d\n' % (AMIGA_DELTA_LUT_LOG2_SIZE))
	print_lut_to_file(f, 'amiga_delta_lut', amiga_delta_lut)
	f.close()

dump_linear_lut('src/linear_delta_lut.h')
dump_amiga_lut('src/amiga_delta_lut.h')
