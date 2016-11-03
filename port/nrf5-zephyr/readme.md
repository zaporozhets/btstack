# Experimental port for Nordic nRF5 Series on top of the Zephyr

## Overview

This port targets the bare Nordic nRF5-Series chipsets with the BLE Link Layer provided by the Zephyr project.

## Status

Only tested on the pca10040 dev board.

## Getting Started

To integrate BTstack into Zephyr, please move the BTstack project into the Zephyr root folder 'zephyr'.

Then create projects for the BTstack examples in zephyr/samples/btstack by running:

	./create_examples.py

Now, the BTstack examples can be build from the zephyr examples folder in the same way as other examples, e.g.:

	cd samples/btstack/gap_le_advertisements
	make

to build the gap_le_advertisements example for the pca10040 dev kit using the ARM GCC compiler.

See nRF5 SDK documentation about how to install it.

