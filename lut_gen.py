#!/usr/bin/python
# -*- coding: latin-1 -*-
# lut_gen.cpp -- Look-up table generator for Pimpmobile
# Copyright (C) 2005-2006 Jørn Nystad and Erik Faye-Lund
# For conditions of distribution and use, see copyright notice in LICENSE.TXT

import sys

AMIGA_DELTA_LUT_LOG_SIZE = 7
AMIGA_DELTA_LUT_SIZE = 1 << AMIGA_DELTA_LUT_LOG_SIZE
AMIGA_DELTA_LUT_FRAC_BITS = 15 - AMIGA_DELTA_LUT_LOG_SIZE

def print_lut(lut):
    print 'const u16 linear_delta_lut[768] =\n{'
    for e in lut:
        print '%d,' % (e),
    print '\n};\n'

def print_lut_to_file(file, lutname, lut):
    file.write('const u16 %s[%d] =\n{\n\t' % (lutname, len(lut)))
    line_start = file.tell()
    for e in lut:
        file.write('%d, ' % e)
        if file.tell() > (line_start + 80):
            file.write('\n\t')
            line_start = file.tell()    
    file.write('\n};\n\n')

def gen_linear_delta_lut():
    lut = []
    for i in range(0, 12 * 64):
        val = int(((2.0 ** (i / 768.0)) * 8363.0 / (1 << 8)) * float(1 << 9) + 0.5)
        lut.append(val)
    return lut

def gen_amiga_delta_lut():
    lut = []
    for i in range(0, (AMIGA_DELTA_LUT_SIZE / 2) + 1):
        p = i + (AMIGA_DELTA_LUT_SIZE / 2);
        val = int(((8363 * 1712) / float((p * 32768) / AMIGA_DELTA_LUT_SIZE)) * (1 << 6) + 0.5);
        lut.append(val)
    return lut

linear_delta_lut = gen_linear_delta_lut()
f = open('src/linear_delta_lut.h', 'w')
print_lut_to_file(f, 'linear_delta_lut', linear_delta_lut)
f.close()

amiga_delta_lut = gen_amiga_delta_lut()
f = open('src/amiga_delta_lut.h', 'w')
f.write('#define AMIGA_DELTA_LUT_LOG_SIZE %d\n' % (AMIGA_DELTA_LUT_LOG_SIZE))
print_lut_to_file(f, 'amiga_delta_lut', amiga_delta_lut)
f.close()
