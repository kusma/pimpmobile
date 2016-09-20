# Pimpmobile

[![Build Status](https://travis-ci.org/kusma/pimpmobile.svg?branch=master)](https://travis-ci.org/kusma/pimpmobile)
[![License: Zlib](https://img.shields.io/badge/License-Zlib-blue.svg)](https://opensource.org/licenses/Zlib)

Pimpmobile is a high performance sample-based music playback engine
targeting the Nintendo Game Boy Advance. It is released under the
Zlib license - a permissive open source license allowing commercial
use. Please note that even though you are allowed to keep
modifications to the source code, we strongly recommend giving
notable improvement to the source code back to us.

## Features

### Currently implemented
* File formats: Protracker MOD, FastTracker II XM
* Most MOD effects, some XM effects
* Good sound-quality - no "cheats"
* Un-cached ARM7 optimized inner-loop
* Relatively low IWRAM-usage (depends on config, around 4k in the default setup)
* Separate sample-banks for sample-sharing across modules
* Module sync-callbacks (E8x and Wxx)
* Binary-only datafiles, generated by offline converter
* All limitations are compile-time defines. No hard limits on channels etc.
* Rather clean C code, with some ARM assembly optimizations.
* Builds with (and depends on) devkitARM

### Planned

* File formats: ScreamTracker S3M, ImpulseTracker IT
* Documentation ;-)
* Game-type sound effects, by reserving sample-channels
* On-target file-loader (more convenient for libfat etc)
* ARM RVDS builds
* Extensive test-suite
